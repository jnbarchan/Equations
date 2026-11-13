#ifndef DICE_H
#define DICE_H

#include <QObject>

extern int randomInRange(int upper);
extern void randomiseString(std::string &str);

class DieType
{
public:
    Qt::GlobalColor colour;
    char faces[6];
};

extern const QList<DieType> DiceTypes;


class Die
{
public:
    Die(const DieType *dieType);
    Die(int row);

    const DieType *dieType;
    Qt::GlobalColor colour;
    char face;

    bool operator==(const Die &other) const;

    bool isDigit() const {return std::isdigit(face); };
    bool isSymbol() const {return !std::isdigit(face); };
    int digitValue() const{return std::isdigit(face) ? face - '0' : -1; };

    static void reseedRandomGenerator();
    void roll();
};

using DiceList = QList<Die>;

#endif // DICE_H
