#ifndef COMMAND_WIDGET_H
#define COMMAND_WIDGET_H

#include <QWidget>
#include <QPushButton>

class CommandWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CommandWidget(QWidget *parent = nullptr, const QString &name = nullptr, int type = 0, const QString &command = nullptr);

    QString name;
    QString command;
    int type;

    void updateWidget(const QString &name = nullptr, int type = 0, const QString &command = nullptr);

private:
    QPushButton *executeButton;

signals:
    void executeCommand(const QString &name, int type, const QString &command);
    void editCommand(CommandWidget *widget, const QString &name, int type, const QString &command);
};

#endif // COMMAND_WIDGET_H
