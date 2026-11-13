#include <QCoreApplication>
#include <QDebug>
#include <QLocale>

#include "settings.h"
#include "solutiongenerator.h"

SolutionGenerator::SolutionGenerator(int goal, const QMap<int, DiceCount> &operandCounts, const QHash<char, DiceCount> &operatorCounts, QObject *parent)
    : QObject{parent}
{
    _debugOutput = true;
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

        _jobResults.solutions.clear();
        _jobResults.evaluations = 0;

        generate(i, operands, operators);

        _threadPool.waitForDone();

        _solutions.append(_jobResults.solutions);
        _evaluations += _jobResults.evaluations;

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
                    // MathExprEval::EvalResults allResults = evaluate(operands, operators);
                    // for (const MathExprEval::EvalResult &result : allResults)
                    //     if (result.value == _goal)
                    //         _solutions.append(QString::fromLatin1(result.expression));

                    _threadPool.start(new EvaluationJob(this, operands, operators));
                    _evaluations++;
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

MathExprEval::EvalResults SolutionGenerator::evaluate(const QList<int> &operands, const QList<char> &operators)
{
    MathExprEval mee;
    std::vector<int> vOperands(operands.constBegin(), operands.constEnd());
    std::vector<char> vOperators(operators.constBegin(), operators.constEnd());

    MathExprEval::EvalResults allResults = mee.evaluateAllParenthesisations(vOperands, vOperators);
    // _evaluations += allResults.size();

    return allResults;
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


SolutionGenerator::EvaluationJob::EvaluationJob(SolutionGenerator *generator, const QList<int> &operands, const QList<char> &operators)
    : _generator(generator),
    _operands(operands),
    _operators(operators)
{
    setAutoDelete(true);
}

void SolutionGenerator::EvaluationJob::run()
{
    MathExprEval::EvalResults allResults = _generator->evaluate(_operands, _operators);
    QStringList solutions;
    for (const MathExprEval::EvalResult &result : allResults)
        if (result.value == _generator->_goal)
            solutions.append(QString::fromLatin1(result.expression));

    QMutexLocker locker(&_generator->_jobResults.mutex);
    _generator->_jobResults.evaluations += static_cast<qsizetype>(allResults.size());
    _generator->_jobResults.solutions.append(solutions);
}
