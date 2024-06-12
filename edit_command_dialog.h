#ifndef EDIT_COMMAND_DIALOG_H
#define EDIT_COMMAND_DIALOG_H

#include <QDialog>
#include <QString>
#include <QMessageBox>

#include "command_widget.h"

namespace Ui {
class EditCommandDialog;
}

class EditCommandDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCommandDialog(QWidget *parent = nullptr, CommandWidget *widget = nullptr, const QString &name = nullptr, int type = 0, const QString &command = nullptr);
    ~EditCommandDialog();

    CommandWidget *widget;
    int type;
    QString name;
    QString command;

    struct CommandPreset {
        QString name;
        int type;
        QString command;
    };

signals:
    void commandUpdated(CommandWidget *widget, const QString &name, int type, const QString &command);

private slots:
    void onAccepted();

private:
    Ui::EditCommandDialog *ui;
};

#endif // EDIT_COMMAND_DIALOG_H
