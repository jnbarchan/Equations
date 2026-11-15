#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include "dice.h"
#include "diewidget.h"
#include "flowlayout.h"
#include "matsmodel.h"
#include "settingsdialog.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    replaceWithFlowLayouts();

    for (MatWidget *mat : allMats())
    {
        mat->dropAllowed = [this](const MatWidget *source, const MatWidget *destination)
        {
            return isDieTransferAllowed(source, destination);
        };

        connect(mat, &MatWidget::diceDropped, this, &MainWindow::onMatDiceDropped);
    }

    ui->actionAdmin_Mode->setChecked(g_settings->adminMode());

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onActionSettings);
    connect(ui->actionAdmin_Mode, &QAction::triggered, this, &MainWindow::onActionAdminMode);
    connect(ui->actionReSeed, &QAction::triggered, this, &MainWindow::onActionReSeed);
    connect(ui->actionReRoll, &QAction::triggered, this, &MainWindow::onActionReRoll);
    connect(ui->actionSet_Goal, &QAction::triggered, this, &MainWindow::onActionSetGoal);
    connect(ui->actionAI_Turn, &QAction::triggered, this, &MainWindow::onActionAITurn);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onActionExit);

    matsModel = new MatsModel(this);
    connect(matsModel, &MatsModel::statusMessage, ui->statusbar, [this](const QString &message) { ui->statusbar->showMessage(message); });

    for (MatWidget *mat : allMats())
        mat->setModel(matsModel);
    ui->matSolution->setDiceList(matsModel->solutionDice());
    ui->matGoal->setDiceList(matsModel->goalDice());
    ui->matAvailable->setDiceList(matsModel->availableDice());
    ui->matRequired->setDiceList(matsModel->requiredDice());
    ui->matPermitted->setDiceList(matsModel->permittedDice());
    ui->matForbidden->setDiceList(matsModel->forbiddenDice());

    reRoll();
}

MainWindow::~MainWindow()
{
    delete ui;
}


QList<MatWidget *> MainWindow::allMats() const
{
    return { ui->matSolution, ui->matGoal, ui->matAvailable, ui->matRequired, ui->matPermitted, ui->matForbidden };
}

void MainWindow::replaceWithFlowLayouts()
{
    QList<MatWidget *> widgets{ allMats() };
    for (QWidget *widget : widgets)
    {
        QLayout *oldLayout = widget->layout();
        QLayout *newLayout = new FlowLayout;
        while (QLayoutItem *item = oldLayout->takeAt(0))
        {
            if (QWidget *widget = item->widget())
                newLayout->addWidget(widget);
            delete item;
        }

        delete oldLayout;
        widget->setLayout(newLayout);
    }

    ui->splitterVertical->setSizes({50, 100, 200});
}

void MainWindow::reRoll()
{
    matsModel->reset();
    ui->actionSet_Goal->setEnabled(true);
    ui->actionAI_Turn->setEnabled(false);

    for (QWidget *mat : allMats())
    {
        QLayout *layout = mat->layout();
        while (QLayoutItem *item = layout->takeAt(0))
        {
            delete item->widget();
            delete item;
        }
    }

    QLayout *layout = ui->matAvailable->layout();
    for (int row = 0; row < 4; row++)
        if (g_settings->useDiceColour(row))
            for (int col = 0; col < 6; col++)
            {
                Die die(row);
                die.roll();
                matsModel->availableDice()->append(die);
                DieWidget *dw = new DieWidget(die);
                layout->addWidget(dw);
            }

    for (MatWidget *mat : allMats())
        mat->setEnabled(false);
    ui->matAvailable->setEnabled(true);
    ui->matGoal->setEnabled(true);
}

bool MainWindow::inAdminMode() const
{
    return g_settings->adminMode();
}

bool MainWindow::goalHasBeenSet() const
{
    return matsModel->hasGoalBeenSet();
}

bool MainWindow::isDieTransferAllowed(const MatWidget *source, const MatWidget *destination) const
{
    if (destination == ui->matSolution)
    {
        return false;
    }

    if (inAdminMode())
        return true;

    if (!goalHasBeenSet())
    {
        if (source == ui->matAvailable && destination == ui->matGoal)
            return true;
        return false;
    }

    if (destination == ui->matGoal)
    {
        return false;
    }

    if (source == ui->matAvailable)
    {
        return true;
    }

    return false;
}

bool MainWindow::chooseDieToMove(DieWidget *&dieWidget, MatWidget *&matWidget)
{
    dieWidget = nullptr;
    matWidget = nullptr;

    QList<UniqueDiceSolution> uniqueDiceSolutions(matsModel->uniqueDiceSolutions());
    qDebug().noquote() << __FUNCTION__
             << "Time:" << QLocale().toString(matsModel->time())
             << "Evaluations:" << QLocale().toString(matsModel->evaluations())
             << "Solutions:" << QLocale().toString(matsModel->solutions().length())
             << "Uniques:" << QLocale().toString(uniqueDiceSolutions.length());
    if (uniqueDiceSolutions.isEmpty())
    {
        QMessageBox::information(this, "No Move", "No solution found!");
        return false;
    }

    QString availableDice = matsModel->diceListToString(*matsModel->availableDice());
    QString requiredDice = matsModel->diceListToString(*matsModel->requiredDice());
    QString permittedDice = matsModel->diceListToString(*matsModel->permittedDice());

    QMap<int, QList<UniqueDiceSolution> > uniqueSolutionsMissingDice;
    for (const UniqueDiceSolution &uniqueDiceSolution : uniqueDiceSolutions)
    {
        QString missing(uniqueDiceSolution.faces);
        int index;
        for (char ch : requiredDice.toLatin1())
            missing.removeAt(missing.indexOf(ch));
        for (char ch : permittedDice.toLatin1())
            if ((index = missing.indexOf(ch)) >= 0)
                missing.removeAt(index);
        int length = missing.length();
        bool found = false;
        for (const UniqueDiceSolution &uniqueSolutionMissingDice : uniqueSolutionsMissingDice[length])
            if (uniqueSolutionMissingDice.faces.contains(missing))
                found = true;
        if (!found)
            uniqueSolutionsMissingDice[length].append({missing, uniqueDiceSolution.sampleSolution});
    }

    int fewestMissing = uniqueSolutionsMissingDice.firstKey();
    if (fewestMissing == 0)
    {
        QMessageBox::information(this, "No Move", "Solution needs no more dice!");
        return false;
    }
    if (availableDice.isEmpty())
    {
        QMessageBox::information(this, "No Move", "No dice to move!");
        return false;
    }
    if (fewestMissing == 1)
        QMessageBox::information(this, "No Move", "Solution needs just 1 more die!");

    bool toRequired, toPermitted, toForbidden;
    toRequired = toPermitted = toForbidden = false;
    QString twoMissingDiceSolutions;
    UniqueDiceSolution chosenMissingDiceSolution;
    char chosenFace = '\0';
    if (fewestMissing == 1)
    {
        int index = randomInRange(uniqueSolutionsMissingDice[1].length());
        chosenMissingDiceSolution = uniqueSolutionsMissingDice[1].at(index);
        chosenFace = chosenMissingDiceSolution.faces.toLatin1()[0];
        toRequired = true;
    }
    else if (fewestMissing == 2)
    {
        for (const UniqueDiceSolution &uniqueDiceSolution : uniqueSolutionsMissingDice[2])
            for (char face : uniqueDiceSolution.faces.toLatin1())
                if (!twoMissingDiceSolutions.contains(face))
                    twoMissingDiceSolutions.append(face);
    }

    if (fewestMissing >= 2)
    {
        QList<UniqueDiceSolution> safeSolutionsMissingDice;
        for (auto i = uniqueSolutionsMissingDice.cbegin(), end = uniqueSolutionsMissingDice.cend(); i != end; ++i)
            safeSolutionsMissingDice.append(i.value());

        while (chosenFace == '\0')
        {
            toRequired = toPermitted = toForbidden = false;
            if (safeSolutionsMissingDice.isEmpty())
            {
                QMessageBox::information(this, "No Move", "No remaining dice to move safely!");
                return false;
            }

            int index = randomInRange(safeSolutionsMissingDice.length());
            chosenMissingDiceSolution = safeSolutionsMissingDice.at(index);
            safeSolutionsMissingDice.removeAt(index);

            QString remainingAvailableDice(availableDice);
            while (!remainingAvailableDice.isEmpty())
            {
                index = randomInRange(remainingAvailableDice.length());
                char face = remainingAvailableDice.at(index).toLatin1();
                remainingAvailableDice.removeAt(index);

                int countMissing = chosenMissingDiceSolution.faces.count(face);
                toRequired = (countMissing > 0);
                toPermitted = true;
                toForbidden = (availableDice.count(face) > countMissing);

                if (twoMissingDiceSolutions.contains(face))
                    toRequired = toPermitted = false;

                if (toRequired || toPermitted || toForbidden)
                {
                    chosenFace = face;
                    break;
                }
            }
        }
    }

    qDebug() << __FUNCTION__ << "Chosen missing dice solution:" << chosenMissingDiceSolution.faces
             << "sample solution:" << chosenMissingDiceSolution.sampleSolution;
    dieWidget = ui->matAvailable->findDieWidgetForFace(chosenFace);
    qDebug() << __FUNCTION__ << "Chosen die to move:" << dieWidget->die().colour << dieWidget->die().face;

    QList<MatWidget *> matWidgets;
    if (toRequired)
    {
        matWidgets.append(ui->matRequired);
        matWidgets.append(ui->matRequired);
    }
    if (toPermitted)
        matWidgets.append(ui->matPermitted);
    if (toForbidden)
    {
        matWidgets.append(ui->matForbidden);
        matWidgets.append(ui->matForbidden);
    }
    if (matWidgets.isEmpty())
    {
        QMessageBox::information(this, "No Move", "No mat to move to!");
        return false;
    }

    int index = randomInRange(matWidgets.length());
    matWidget = matWidgets.at(index);
    qDebug() << __FUNCTION__ << "Choosing to move to mat:" << matWidget->objectName();

    return true;
}

void MainWindow::moveDieToMat(DieWidget *dw, MatWidget *matTo)
{
    matTo->emulateDragDrop(dw, centralWidget());
}

void MainWindow::makeGoalFromPossible()
{
    setCursor(Qt::WaitCursor);
    matsModel->makeGoalFromPossible();
    unsetCursor();
}

void MainWindow::onMatDiceDropped()
{
    if (!goalHasBeenSet())
        return;

    QTimer::singleShot(1, this, &MainWindow::makeGoalFromPossible);
}

void MainWindow::onActionSettings()
{
    SettingsDialog dlg(this);
    dlg.exec();

    ui->actionAdmin_Mode->setChecked(g_settings->adminMode());
}

void MainWindow::onActionAdminMode()
{
    g_settings->setAdminMode(ui->actionAdmin_Mode->isChecked());
}

void MainWindow::onActionReSeed()
{
    Die::reseedRandomGenerator();
}

void MainWindow::onActionReRoll()
{
    reRoll();
}

void MainWindow::onActionSetGoal()
{
    if (!matsModel->setGoalFromGoalDice())
        return;
    matsModel->setHasGoalBeenSet(true);
    ui->actionSet_Goal->setEnabled(false);
    ui->actionAI_Turn->setEnabled(true);
    for (MatWidget *mat : allMats())
        mat->setEnabled(true);
    ui->matSolution->setEnabled(false);
    ui->matGoal->setEnabled(false);

    QTimer::singleShot(1, this, &MainWindow::makeGoalFromPossible);
}

void MainWindow::onActionAITurn()
{
    if (!matsModel->hasGoalBeenSet())
        return;

    makeGoalFromPossible();

    DieWidget *dieWidget;
    MatWidget *matWidget;
    if (!chooseDieToMove(dieWidget, matWidget))
        return;
    moveDieToMat(dieWidget, matWidget);
}

void MainWindow::onActionExit()
{
    qApp->quit();
}
