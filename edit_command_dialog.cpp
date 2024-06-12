#include "edit_command_dialog.h"
#include "ui_edit_command_dialog.h"

EditCommandDialog::EditCommandDialog(QWidget *parent, CommandWidget *widget, const QString &name, int type, const QString &command)
    : QDialog(parent)
    , ui(new Ui::EditCommandDialog)
    , widget(widget)
    , name(name)
    , type(type)
    , command(command)
{
    ui->setupUi(this);

    ui->nameText->setText(name);
    ui->commandTypeBox->setCurrentIndex(type);
    ui->commandText->setText(command);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditCommandDialog::onAccepted);
}

EditCommandDialog::~EditCommandDialog()
{
    delete ui;
}

void EditCommandDialog::onAccepted()
{
    // Get values
    int type = ui->commandTypeBox->currentIndex();
    QString name= ui->nameText->text();
    QString command = ui->commandText->text();

    // Make sure name and command are set
    if(name.length() == 0 || command.length() == 0){
        QMessageBox::warning(this, "KfxApiTool", "Name and command must be set");
        return;
    }

    // Emit the submit signal and close the dialog
    emit commandUpdated(this->widget, name, type, command);
    accept();
}
