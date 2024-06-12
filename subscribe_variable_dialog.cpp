#include "subscribe_variable_dialog.h"
#include "ui_subscribe_variable_dialog.h"
#include "q_kfx_variable_completer.h"

SubscribeVariableDialog::SubscribeVariableDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SubscribeVariableDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SubscribeVariableDialog::onAccepted);

    // Create and set the custom completer
    QKfxVariableCompleter *completer = new QKfxVariableCompleter(ui->lineEdit);
    ui->lineEdit->setCompleter(completer);

    ui->lineEdit->setFocus();
}

SubscribeVariableDialog::~SubscribeVariableDialog()
{
    delete ui;
}

void SubscribeVariableDialog::onAccepted()
{
    QString variable = ui->lineEdit->text();
    QString player = ui->comboBox->currentText();
    emit subVarSubmitted(player, variable.toUpper());
    accept();
}
