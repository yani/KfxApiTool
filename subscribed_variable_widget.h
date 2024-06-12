#ifndef SUBSCRIBED_VARIABLE_WIDGET_H
#define SUBSCRIBED_VARIABLE_WIDGET_H

#include <QWidget>
#include <QLineEdit>

class SubscribedVariableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SubscribedVariableWidget(QWidget *parent = nullptr, const QString &player = nullptr, const QString &variable = nullptr, int value = -1);
    QLineEdit *valueInput;

    QString player;
    QString variable;
    int value;

    void update(int value);

signals:
    void removeSubbedVariable(const QString &player, const QString &variable);
    void editSubbedVariable(const QString &player, const QString &variable, int value);
};

#endif // SUBSCRIBED_VARIABLE_WIDGET_H
