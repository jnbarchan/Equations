#ifndef MATWIDGET_H
#define MATWIDGET_H

#include <QGroupBox>

#include "dice.h"
#include "matsmodel.h"

class DieWidget;

class MatWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit MatWidget(QWidget *parent = nullptr);

    using DropAllowed = std::function<bool(const MatWidget *source, const MatWidget *destination)>;
    DropAllowed dropAllowed;

    MatsModel *model() const { return _model; }
    void setModel(MatsModel *newModel) { _model = newModel; }
    DiceList *diceList() const { return _diceList; }
    void setDiceList(DiceList *newDiceList) { _diceList = newDiceList; }

    DieWidget *findDieWidgetForFace(char face) const;
    void emulateDragDrop(DieWidget *dieWidget, QWidget *ancestor);

private:
    MatsModel *_model;
    DiceList *_diceList;

    void moveDieHere(DieWidget *dw);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void diceChanged();
    void diceDropped();
};

#endif // MATWIDGET_H
