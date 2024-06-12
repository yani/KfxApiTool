#include "subscribe_event_dialog.h"
#include "ui_subscribe_event_dialog.h"

SubscribeEventDialog::SubscribeEventDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SubscribeEventDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SubscribeEventDialog::onAccepted);
}

SubscribeEventDialog::~SubscribeEventDialog()
{
    delete ui;
}

void SubscribeEventDialog::onAccepted()
{
    QString event = ui->comboBox->currentText();
    emit subEventSubmitted(event);
    accept();
}

