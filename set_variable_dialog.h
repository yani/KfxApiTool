#ifndef SET_VARIABLE_DIALOG_H
#define SET_VARIABLE_DIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QString>

namespace Ui {
class SetVariableDialog;
}

class SetVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetVariableDialog(QWidget *parent = nullptr, const QString &player = nullptr, const QString &variable = nullptr, int value = 0);
    ~SetVariableDialog();

signals:
    void setVarSubmitted(const QString &player, const QString &variable, int value);

private slots:
    void onAccepted();

private:
    Ui::SetVariableDialog *ui;
};

#endif // SET_VARIABLE_DIALOG_H
