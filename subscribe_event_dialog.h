#ifndef SUBSCRIBE_EVENT_DIALOG_H
#define SUBSCRIBE_EVENT_DIALOG_H

#include <QDialog>

namespace Ui {
class SubscribeEventDialog;
}

class SubscribeEventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubscribeEventDialog(QWidget *parent = nullptr);
    ~SubscribeEventDialog();

signals:
    void subEventSubmitted(const QString &event);

private slots:
    void onAccepted();

private:
    Ui::SubscribeEventDialog *ui;
};

#endif // SUBSCRIBE_EVENT_DIALOG_H
