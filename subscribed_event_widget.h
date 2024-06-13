#ifndef SUBSCRIBED_EVENT_WIDGET_H
#define SUBSCRIBED_EVENT_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class SubscribedEventWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SubscribedEventWidget(QWidget *parent = nullptr, const QString &event = nullptr);
    ~SubscribedEventWidget();

    QString event;

signals:
    void removeSubbedEvent(const QString &event);
};

#endif // SUBSCRIBED_EVENT_WIDGET_H
