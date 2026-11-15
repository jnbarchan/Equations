#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    for (int i = 0; i < 4; i++)
        ui->btngrpUseDice->buttons().at(i)->setChecked(g_settings->useDiceColour(i));
    ui->chkAdminMode->setChecked(g_settings->adminMode());
    ui->spnMaxOperands->setValue(g_settings->maxOperands());
    ui->spnMaxEvaluations->setValue(g_settings->maxEvaluations());
    ui->spnMaxTime->setValue(g_settings->maxTime());
    ui->chkUseThreads->setChecked(g_settings->useThreads());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept() /*override*/
{
    for (int i = 0; i < 4; i++)
        g_settings->setUseDiceColour(i, ui->btngrpUseDice->buttons().at(i)->isChecked());
    g_settings->setAdminMode(ui->chkAdminMode->isChecked());
    g_settings->setMaxOperands(ui->spnMaxOperands->value());
    g_settings->setMaxEvaluations(ui->spnMaxEvaluations->value());
    g_settings->setMaxTime(ui->spnMaxTime->value());
    g_settings->setUseThreads(ui->chkUseThreads->isChecked());

    QDialog::accept();
}
