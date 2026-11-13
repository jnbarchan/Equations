#include <random>

unsigned int initialSeed = 123456789;//std::random_device{}();
std::mt19937 generator(initialSeed);
std::uniform_int_distribution<int> dieRoll(1, 6);

#include "dice.h"

int randomInRange(int upper)
{
    std::uniform_int_distribution<int> distribution(0, upper - 1);
    int num = distribution(generator);
    return num;
}

void randomiseString(std::string &str)
{
    std::shuffle(str.begin(), str.end(), generator);
}

const QList<DieType> DiceTypes {
    { Qt::red, { '0', '1', '2', '3', '+', '-' } },
    { Qt::blue, { '0', '1', '2', '3', '*', '+' } },
    { Qt::darkGreen, { '4', '5', '6', '-', '*', '/' } },
    { Qt::black, { '7', '8', '9', '+', '^', '~' } },
};

Die::Die(const DieType *dieType)
{
    this->dieType = dieType;
    this->colour = dieType->colour;
}

Die::Die(int row)
{
    const DieType *dieType = &DiceTypes.at(row);
    this->dieType = dieType;
    this->colour = dieType->colour;
}

bool Die::operator==(const Die &other) const
{
    return dieType == other.dieType
           && colour == other.colour
           && face == other.face;
}

void Die::reseedRandomGenerator()
{
    generator.seed(initialSeed);
}

void Die::roll()
{
    int num = dieRoll(generator);
    this->face = this->dieType->faces[num - 1];
}
