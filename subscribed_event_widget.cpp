#include "subscribed_event_widget.h"

SubscribedEventWidget::SubscribedEventWidget(QWidget *parent, const QString &event)
    : QWidget{parent},
    event(event)
{
    // Create label with name of event
    QLabel *label = new QLabel(this);
    label->setText("Event : " + this->event);
    label->show();

    // Create remove button
    QPushButton *removeButton = new QPushButton("X", this);
    removeButton->setFixedSize(20, 20);

    // Connect the remove button's clicked signal to emit removeSubbedEvent signal
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeSubbedEvent(this->event);
    });

    // Both elements (except the button) try to be 50%
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Create a layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(removeButton);

    // Set margins for the layout
    layout->setContentsMargins(3, 3, 3, 3); // Left, Top, Right, Bottom

    // Set the layout of this widget
    setLayout(layout);

    // Ensure this widget expands horizontally
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Show the widget
    this->show();
}

SubscribedEventWidget::~SubscribedEventWidget()
{
    delete layout();
}
