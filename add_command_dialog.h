#ifndef ADD_COMMAND_DIALOG_H
#define ADD_COMMAND_DIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class AddCommandDialog;
}

class AddCommandDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCommandDialog(QWidget *parent = nullptr);
    ~AddCommandDialog();

    int type;

    struct CommandPreset {
        QString name;
        int type;
        QString command;
    };

    QMap<int, CommandPreset> commandPresetMap;

signals:
    void commandSubmitted(const QString &name, int type, const QString &command);

private slots:
    void onAccepted();

private:
    Ui::AddCommandDialog *ui;
    void loadPreset(int index);
};

#endif // ADD_COMMAND_DIALOG_H
