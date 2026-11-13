#ifndef MATSMODEL_H
#define MATSMODEL_H

#include <QObject>

#include "dice.h"
#include "solutiongenerator.h"

class MatsModel : public QObject
{
    Q_OBJECT

public:
    explicit MatsModel(QObject *parent = nullptr);

    bool hasGoalBeenSet() const { return _hasGoalBeenSet; }
    void setHasGoalBeenSet(bool newHasGoalBeenSet) { _hasGoalBeenSet = newHasGoalBeenSet; }
    int goal() const { return _goal; }
    void setGoal(int newGoal) { _goal = newGoal; }

    DiceList *solutionDice() { return &_solutionDice; };
    DiceList *goalDice() { return &_goalDice; };
    DiceList *availableDice() { return &_availableDice; };
    DiceList *requiredDice() { return &_requiredDice; };
    DiceList *permittedDice() { return &_permittedDice; };
    DiceList *forbiddenDice() { return &_forbiddenDice; };

    DiceList possibleDice() const;
    DiceList playedDice() const;

    QString diceListToString(const DiceList &diceList) const;

    bool makeGoalFromPossible();
    qint64 evaluations() const;
    const QStringList &solutions() const;
    const QList<UniqueDiceSolution> uniqueDiceSolutions() const;

public slots:
    void reset();
    bool setGoalFromGoalDice();

private:
    bool _hasGoalBeenSet;
    int _goal;
    DiceList _solutionDice, _goalDice, _availableDice, _requiredDice, _permittedDice, _forbiddenDice;
    SolutionGenerator *_solutionGenerator;

    void init();
    int diceListToInt(const DiceList &diceList) const;
    QSet<int> makeAllNumbersFromAvailable() const;

signals:
    void statusMessage(const QString &message);
};

#endif // MATSMODEL_H
