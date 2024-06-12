#include "set_variable_dialog.h"
#include "ui_set_variable_dialog.h"

#include "q_kfx_set_variable_completer.h"

SetVariableDialog::SetVariableDialog(QWidget *parent, const QString &player, const QString &variable, int value)
    : QDialog(parent)
    , ui(new Ui::SetVariableDialog)
{
    ui->setupUi(this);

    // Accepted event
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SetVariableDialog::onAccepted);

    if(player != nullptr)
    {
        ui->playerBox->setCurrentText(player);
    }

    if(variable != nullptr)
    {
        ui->variableInput->setText(variable);
    }

    ui->valueInput->setText(QString::number(value));

    // Create and set the custom completer
    QKfxSetVariableCompleter *completer = new QKfxSetVariableCompleter(ui->variableInput);
    ui->variableInput->setCompleter(completer);

    // Focus the first input (variable)
    ui->variableInput->setFocus();
}

SetVariableDialog::~SetVariableDialog()
{
    delete ui;
}

void SetVariableDialog::onAccepted()
{
    // Get values
    QString player = ui->playerBox->currentText();
    QString variable = ui->variableInput->text();
    int value = ui->valueInput->text().toInt();

    // Make sure variable name is set
    if(variable.length() == 0){
        QMessageBox::warning(this, "KfxApiTool", "Variable name must be set");
        return;
    }

    // Emit the submit signal and close the dialog
    emit setVarSubmitted(player, variable, value);
    accept();
}
