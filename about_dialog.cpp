#include "about_dialog.h"
#include "ui_about_dialog.h"
#include "version.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->label_2->setText("KeeperFX API Tool v" + QString(APP_VERSION));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
