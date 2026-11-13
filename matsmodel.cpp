#include <QDebug>

#include "matsmodel.h"

MatsModel::MatsModel(QObject *parent)
    : QObject{parent}
{
    _solutionGenerator = nullptr;
    init();
}

void MatsModel::init()
{
    delete _solutionGenerator;
    _solutionGenerator = nullptr;
    _hasGoalBeenSet = false;
    for (DiceList *diceList : {&_solutionDice, &_goalDice, &_availableDice, &_requiredDice, &_permittedDice, &_forbiddenDice})
        diceList->clear();
}

void MatsModel::reset()
{
    init();
}

QString MatsModel::diceListToString(const DiceList &diceList) const
{
    QString str;
    for (const Die &die : diceList)
        str.append(die.face);
    return str;
}

int MatsModel::diceListToInt(const DiceList &diceList) const
{
    if (diceList.isEmpty())
        return -1;
    int result = 0;
    for (int i = 0; i < diceList.length(); i++)
    {
        int digit = diceList.at(i).digitValue();
        if (digit < 0)
            return -1;
        if (digit == 0 && i == 0 && diceList.length() > 1)
            return -1;
        result = result * 10 + digit;
    }
    return result;
}

QSet<int> MatsModel::makeAllNumbersFromAvailable() const
{
    const DiceList &dice(_availableDice);
    QList<int> digits;  // One entry per die showing a digit
    QSet<int> numbers;  // Unique set of generated (1- or 2-digit) numbers

    // First, collect the digits showing on the dice.
    for (const Die &die : dice)
    {
        int digit = die.digitValue();
        if (digit >= 0)
            digits.append(digit);
    }

    // Generate all one-digit numbers.
    for (int digit : digits)
        numbers.insert(digit);

    // Generate all two-digit numbers using different dice.
    for (qsizetype i = 0; i < digits.size(); ++i)
    {
        if (digits.at(i) == 0)
            continue;
        for (qsizetype j = 0; j < digits.size(); ++j)
        {
            if (i == j)
                continue;
            int number = digits.at(i) * 10 + digits.at(j);
            numbers.insert(number);
        }
    }

    return numbers;
}

DiceList MatsModel::possibleDice() const
{
    return _availableDice + _requiredDice + _permittedDice;
}

DiceList MatsModel::playedDice() const
{
    return _requiredDice + _permittedDice + _forbiddenDice;
}

bool MatsModel::setGoalFromGoalDice()
{
    _goal = -1;
    if (_goalDice.isEmpty() || _goalDice.length() > 2)
        return false;
    int goal = diceListToInt(_goalDice);
    if (goal < 0)
        return false;
    qDebug() << "Goal set to:" << goal;
    _goal = goal;
    return true;
}

bool MatsModel::makeGoalFromPossible()
{
    QMap<int, DiceCount> operandCounts;
    QHash<char, DiceCount> operatorCounts;
    for (const Die &die : _requiredDice)
    {
        if (die.isDigit())
            operandCounts[die.digitValue()].required++;
        else if (die.isSymbol())
            operatorCounts[die.face].required++;
    }
    for (const Die &die : _availableDice + _permittedDice)
    {
        if (die.isDigit())
            operandCounts[die.digitValue()].allowed++;
        else if (die.isSymbol())
            operatorCounts[die.face].allowed++;
    }

    delete _solutionGenerator;
    _solutionGenerator = nullptr;
    _solutionGenerator = new SolutionGenerator(_goal, operandCounts, operatorCounts, this);
    connect(_solutionGenerator, &SolutionGenerator::statusMessage, this, &MatsModel::statusMessage);

    _solutionGenerator->generate();

    bool canBeMade = _solutionGenerator->hasSolution();

    qDebug() << "Goal" << (canBeMade ? "can" : "cannot") << "be made";
    return canBeMade;
}

qint64 MatsModel::time() const
{
    return _solutionGenerator->time();
}

qint64 MatsModel::evaluations() const
{
    return _solutionGenerator->evaluations();
}

const QStringList &MatsModel::solutions() const
{
    return _solutionGenerator->solutions();
}

const QList<UniqueDiceSolution> MatsModel::uniqueDiceSolutions() const
{
    return _solutionGenerator->uniqueDiceSolutions();
}
