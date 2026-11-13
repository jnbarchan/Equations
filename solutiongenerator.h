#ifndef SOLUTIONGENERATOR_H
#define SOLUTIONGENERATOR_H

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QMap>

#include "mathexpreval.h"

struct DiceCount
{
    int required;
    int allowed;
    int used;
};

struct UniqueDiceSolution
{
    QString faces;
    QString sampleSolution;
};

class SolutionGenerator : public QObject
{
    Q_OBJECT

public:
    SolutionGenerator(int goal, const QMap<int, DiceCount> &operandCounts, const QHash<char, DiceCount> &operatorCounts, QObject *parent = nullptr);

    void generate();
    bool hasSolution() const { return !_solutions.isEmpty(); }
    qint64 evaluations() const { return _evaluations; }
    const QStringList &solutions() const { return _solutions; };
    const QList<UniqueDiceSolution> &uniqueDiceSolutions() const { return _uniqueDiceSolutions; };

private:
    bool _debugOutput;
    int _goal;
    int _maxOperands, _maxOperators;
    qint64 _maxEvaluations;
    qint64 _maxTime;
    QMap<int, DiceCount> _operandCounts;     // Digit(s) -> count
    QHash<char, DiceCount> _operatorCounts;   // Operator -> count
    int _remainingRequired;
    QStringList _solutions;
    QList<UniqueDiceSolution> _uniqueDiceSolutions;
    qint64 _evaluations;
    qint64 _time;
    QElapsedTimer _elapsed;

    QString expressionToString(const QList<int> &operands, const QList<char> &operators);
    void generate(int numOperands, QList<int> &operands, QList<char> &operators);
    MathExprEval::EvalResults evaluate(const QList<int> &operands, const QList<char> &operators);
    QList<UniqueDiceSolution> makeUniqueDiceSolutions() const;

signals:
    void statusMessage(const QString &message);
};

#endif // SOLUTIONGENERATOR_H
