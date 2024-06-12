#include "command_widget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

CommandWidget::CommandWidget(QWidget *parent, const QString &name, int type, const QString &command)
    : QWidget{parent},
    name(name),
    type(type),
    command(command)
{
    // Create execute button
    executeButton = new QPushButton(this->name, this);
    connect(executeButton, &QPushButton::clicked, this, [this]() {
        emit executeCommand(this->name, this->type, this->command);
    });

    // Create edit button
    QPushButton *editButton = new QPushButton("E", this);
    editButton->setFixedSize(20, 20);
    connect(editButton, &QPushButton::clicked, this, [this]() {
        emit editCommand(this, this->name, this->type, this->command);
    });
    editButton->setToolTip("Edit");

    // Create remove button
    QPushButton *removeButton = new QPushButton("X", this);
    removeButton->setFixedSize(20, 20);
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        // We can simply delete the widget because it is not linked to anything in the game
        delete this;
    });

    // Both elements (except the button) try to be 50%
    executeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Create a layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(executeButton);
    layout->addWidget(editButton);
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

void CommandWidget::updateWidget(const QString &name, int type, const QString &command)
{
    // Update the data for this command
    this->name = name;
    this->type = type;
    this->command = command;

    // Update the button text
    this->executeButton->setText(name);
}
