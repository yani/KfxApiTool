#ifndef CONNECT_DIALOG_H
#define CONNECT_DIALOG_H

#include <QDialog>
#include <QHostAddress>
#include <QRegularExpression>
#include <QMessageBox>

namespace Ui {
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);
    ~ConnectDialog();

signals:
    void textSubmitted(const QString &text);

private slots:
    void onAccepted();

private:
    Ui::ConnectDialog *ui;
};

#endif // CONNECT_DIALOG_H
