#include <QCoreApplication>
#include <QDebug>
#include <QLocale>
#include <QtConcurrent>

#include "settings.h"
#include "solutiongenerator.h"

SolutionGenerator::SolutionGenerator(int goal, const QMap<int, DiceCount> &operandCounts, const QHash<char, DiceCount> &operatorCounts, QObject *parent)
    : QObject{parent}
{
    _debugOutput = false;
    _goal = goal;
    _operandCounts = operandCounts;
    _operatorCounts = operatorCounts;

    _maxOperands = 0;
    for (auto it = operandCounts.cbegin(), end = operandCounts.cend(); it != end; ++it)
        _maxOperands += it->required + it->allowed;
    _maxOperators = 0;
    for (auto it = operatorCounts.cbegin(), end = operatorCounts.cend(); it != end; ++it)
        _maxOperators += it->required + it->allowed;

    int threadCount = QThread::idealThreadCount();
    _threadPool.setMaxThreadCount(threadCount);
}

QString SolutionGenerator::expressionToString(const QList<int> &operands, const QList<char> &operators)
{
    QString str;
    if (operands.isEmpty())
        return str;
    str = QString::number(operands.at(0));
    for (int i = 1; i < operands.length(); i++)
    {
        str.append(operators.at(i - 1));
        str.append(QString::number(operands.at(i)));
    }
    return str;
}

void SolutionGenerator::generate()
{
    _useThreads = g_settings->useThreads();
    _futures.clear();

    _solutions.clear();
    _uniqueDiceSolutions.clear();
    _evaluations = 0L;
    _time = 0L;
    int maxOperands = (_maxOperators < _maxOperands) ? _maxOperators + 1 : _maxOperands;
    int settingsMaxOperands = g_settings->maxOperands();
    if (settingsMaxOperands > 0 && maxOperands > settingsMaxOperands)
        maxOperands = settingsMaxOperands;
    _maxEvaluations = g_settings->maxEvaluations();
    _maxTime = g_settings->maxTime() * 1000L;

    _elapsed.start();

    int maxOperandsDepth = 0;
    for (int i = 1; i <= maxOperands; i++)
    {
        if (_maxEvaluations > 0 && _evaluations >= _maxEvaluations)
            break;
        if (_maxTime > 0 && _time >= _maxTime)
            break;

        maxOperandsDepth = i;

        _remainingRequired = 0;
        for (auto it = _operandCounts.begin(), end = _operandCounts.end(); it != end; ++it)
        {
            it->used = 0;
            _remainingRequired += it->required;
        }
        for (auto it = _operatorCounts.begin(), end = _operatorCounts.end(); it != end; ++it)
        {
            it->used = 0;
            _remainingRequired += it->required;
        }

        int prevSolutions = _solutions.length();
        QList<int> operands;
        QList<char> operators;

        generate(i, operands, operators);

        if (_useThreads)
        {
            _threadPool.waitForDone();
            collectFinishedJobs();
        }

        int newSolutions =  _solutions.length() - prevSolutions;
        if (_debugOutput)
            qDebug() << i << "operands:" << newSolutions << "solutions found";
    }

    _uniqueDiceSolutions = makeUniqueDiceSolutions();
    int uniqueSolutions = _uniqueDiceSolutions.count();
    if (_debugOutput)
    {
        qDebug().noquote() << QLocale().toString(_solutions.length()) << "total solutions found";
        qDebug().noquote() << QLocale().toString(uniqueSolutions) << "unique dice solutions found";
        qDebug().noquote() << QLocale().toString(_evaluations) << "total evaluations," << QLocale().toString(_elapsed.elapsed()) << "milliseconds";
    }

    QString message = QString("%1 / %2 / %3 / %4")
                          .arg(QLocale().toString(_evaluations))
                          .arg(QLocale().toString(_solutions.length()))
                          .arg(QLocale().toString(uniqueSolutions))
                          .arg(QLocale().toString(maxOperandsDepth));
    emit statusMessage(message);
}

void SolutionGenerator::generate(int numOperands, QList<int> &operands, QList<char> &operators)
{
    if (_maxEvaluations > 0 && _evaluations >= _maxEvaluations)
        return;
    if (_maxTime > 0 && (_time = _elapsed.elapsed()) >= _maxTime)
        return;
    //VERYTEMPORARY
    QCoreApplication::processEvents();

    bool wantOperand = (operands.length() == operators.length());
    if (numOperands == 0)
        return;

    if (wantOperand)
    {
        for (auto it = _operandCounts.begin(), end = _operandCounts.end(); it != end; ++it)
        {
            if (it->used >= it->required + it->allowed)
                continue;
            bool takeFromRequired = (it->used < it->required);
            if (takeFromRequired)
                _remainingRequired--;
            it->used++;

            int val = it.key();
            operands.append(val);

            if (numOperands == 1)
                if (_remainingRequired == 0)
                {
                    if (_useThreads)
                    {
                        _futures.append(QtConcurrent::run(&_threadPool,
                                                          &SolutionGenerator::evaluateJob,
                                                          this, operands, operators));
                        if (_futures.size() >= 100)
                            collectFinishedJobs();
                    }
                    else
                    {
                        JobResults jobResults =  evaluateJob(operands, operators);
                        _solutions.append(jobResults.solutions);
                        _evaluations += jobResults.evaluations;                    }
                }

            generate(numOperands - 1, operands, operators);

            operands.removeLast();
            it->used--;
            if (takeFromRequired)
                _remainingRequired++;
        }
    }
    else
    {
        for (auto it = _operatorCounts.begin(), end = _operatorCounts.end(); it != end; ++it)
        {
            if (it->used >= it->required + it->allowed)
                continue;
            bool takeFromRequired = (it->used < it->required);
            if (takeFromRequired)
                _remainingRequired--;
            it->used++;

            char op = it.key();
            operators.append(op);

            generate(numOperands, operands, operators);

            operators.removeLast();
            it->used--;
            if (takeFromRequired)
                _remainingRequired++;
        }
    }
}

QList<UniqueDiceSolution> SolutionGenerator::makeUniqueDiceSolutions() const
{
    QHash<QString, UniqueDiceSolution> uniques;

    for (const QString &solution : _solutions)
    {
        QString sorted = solution;
        std::sort(sorted.begin(), sorted.end());
        sorted.remove('(').remove(')');
        uniques[sorted] = {sorted, solution};
    }

    return uniques.values();
}


SolutionGenerator::JobResults SolutionGenerator::evaluateJob(const QList<int> &operands, const QList<char> &operators)
{
    std::vector<int> vOperands(operands.constBegin(), operands.constEnd());
    std::vector<char> vOperators(operators.constBegin(), operators.constEnd());

    MathExprEval mee;
    MathExprEval::EvalResults allResults = mee.evaluateAllParenthesisations(vOperands, vOperators);

    JobResults jobResults;
    for (const MathExprEval::EvalResult &result : allResults)
        if (result.value == _goal)
            jobResults.solutions.append(QString::fromLatin1(result.expression));
    jobResults.evaluations = allResults.size();

    return jobResults;
}

void SolutionGenerator::collectFinishedJobs()
{
    for (int i = _futures.size() - 1; i >= 0; i--)
    {
        QFuture<JobResults> &future = _futures[i];
        if (future.isFinished())
        {
            JobResults results = future.result();
            _solutions.append(results.solutions);
            _evaluations += results.evaluations;
            _futures.removeAt(i);
        }
    }
}
