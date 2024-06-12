#ifndef SUBSCRIBE_VARIABLE_DIALOG_H
#define SUBSCRIBE_VARIABLE_DIALOG_H

#include <QDialog>
#include <QCompleter>

namespace Ui {
class SubscribeVariableDialog;
}

class SubscribeVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubscribeVariableDialog(QWidget *parent = nullptr);
    ~SubscribeVariableDialog();

signals:
    void subVarSubmitted(const QString &player, const QString &variable);

private slots:
    void onAccepted();

private:
    Ui::SubscribeVariableDialog *ui;
};

#endif // SUBSCRIBE_VARIABLE_DIALOG_H
