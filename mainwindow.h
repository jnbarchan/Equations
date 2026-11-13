#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DieWidget;
class MatWidget;
class MatsModel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    MatsModel *matsModel;

    QList<MatWidget *> allMats() const;
    void replaceWithFlowLayouts();
    void reRoll();
    bool inAdminMode() const;
    bool goalHasBeenSet() const;
    bool isDieTransferAllowed(const MatWidget *source, const MatWidget *destination) const;
    bool chooseDieToMove(DieWidget *&dieWidget, MatWidget *&matWidget);
    void moveDieToMat(DieWidget *dw, MatWidget *matTo);

private slots:
    void makeGoalFromPossible();
    void onMatDiceDropped();
    void onActionSettings();
    void onActionAdminMode();
    void onActionReSeed();
    void onActionReRoll();
    void onActionSetGoal();
    void onActionAITurn();
    void onActionExit();
};
#endif // MAINWINDOW_H
