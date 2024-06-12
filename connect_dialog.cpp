#include "connect_dialog.h"
#include "ui_connect_dialog.h"

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectDialog::onAccepted);
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

void ConnectDialog::onAccepted()
{
    QString text = ui->lineEdit->text();

    static QRegularExpression ipPortPattern("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+\\:[0-9]+$");
    QRegularExpressionMatch match = ipPortPattern.match(text);

    if(match.hasMatch()){
        emit textSubmitted(text);
        accept();  // Close the dialog
    } else {
        QMessageBox::warning(this, "Connect to server", "Invalid IP address and port combination");
    }
}
