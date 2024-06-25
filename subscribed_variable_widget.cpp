#include "subscribed_variable_widget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QContextMenuEvent>


SubscribedVariableWidget::SubscribedVariableWidget(QWidget *parent, const QString &player, const QString &variable, int value)
    : QWidget{parent},
    player(player),
    variable(variable),
    value(value)
{
    // Create identifier label
    label = new QLabel(this);
    label->setText(this->player + " : " + this->variable);
    label->show();

    // Create value box
    valueInput = new QLineEdit(this);
    valueInput->setReadOnly(true);
    valueInput->setText(QString::number(this->value));
    valueInput->show();

    // Create remove button
    QPushButton *removeButton = new QPushButton("X", this);
    removeButton->setFixedSize(20, 20);

    // Connect the remove button's clicked signal to emit removeSubbedVariable signal
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeSubbedVariable(this->player, this->variable);
    });

    // Both elements (except the button) try to be 50%
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    valueInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Create a layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(valueInput);
    layout->addWidget(removeButton);

    // Set margins for the layout
    layout->setContentsMargins(3, 1, 3, 1); // Left, Top, Right, Bottom

    // Set the layout of this widget
    setLayout(layout);

    // Ensure this widget expands horizontally
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Set the context menu policies
    setContextMenuPolicy(Qt::CustomContextMenu);
    valueInput->setContextMenuPolicy(Qt::CustomContextMenu);

    // Create a context menu for right click
    QMenu *contextMenu = new QMenu(this);

    // Create context menu option for the edit function
    QAction *editAction = contextMenu->addAction("Set variable");
    connect(editAction, &QAction::triggered, this, [this]() {
        emit editSubbedVariable(this->player, this->variable, this->value);
    });

    // Create context menu option for the unsub function
    QAction *unsubAction = contextMenu->addAction("Unsubscribe");
    connect(unsubAction, &QAction::triggered, this, [this]() {
        emit removeSubbedVariable(this->player, this->variable);
    });

    // Connect the context menu events
    // We connect both the widget and the value input as they are different
    connect(this, &QWidget::customContextMenuRequested, this, [this, contextMenu](const QPoint &pos) {
        contextMenu->exec(mapToGlobal(pos));
    });
    connect(valueInput, &QWidget::customContextMenuRequested, this, [this, contextMenu](const QPoint &pos) {
        contextMenu->exec(mapToGlobal(pos));
    });

    // Show the widget
    this->show();
}

void SubscribedVariableWidget::update(const int value)
{
    this->value = value;
    valueInput->setText(QString::number(value));
}

SubscribedVariableWidget::~SubscribedVariableWidget()
{
    delete layout();
}

void SubscribedVariableWidget::setValidStatus(bool isValid)
{
    if(isValid) {
        // Normal text if valid
        label->setText(this->player + " : " + this->variable);
    } else {
        // Set striketrough text
        label->setText("<span style=\"text-decoration:line-through\">" + this->player + " : " + this->variable + "</span>");
    }
}
