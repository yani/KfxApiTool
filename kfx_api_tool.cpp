#include "kfx_api_tool.h"
#include "./ui_kfx_api_tool.h"

#include "about_dialog.h"
#include "connect_dialog.h"
#include "subscribe_variable_dialog.h"
#include "subscribe_event_dialog.h"
#include "add_command_dialog.h"
#include "edit_command_dialog.h"
#include "set_variable_dialog.h"


KfxApiTool::KfxApiTool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::KfxApiTool)
    , tcpSocket(new QTcpSocket(this))
    , connectionTimeoutTimer(new QTimer(this))
    , connectionReconnectTimer(new QTimer(this))
    , statusLabel(new QLabel(this->statusBar()))
    , currentAckId(0)
    , shouldReconnect(false)
{
    ui->setupUi(this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Menu: API
    connect(ui->actionConnect, &QAction::triggered, this, &KfxApiTool::openConnectDialog);
    connect(ui->actionConnectDefault, &QAction::triggered, this, [this]() {
        handleServerApiSubmitted("127.0.0.1:5599");
    });
    connect(ui->actionAutoReconnect, &QAction::toggled, this, &KfxApiTool::toggleReconnect);
    connect(ui->actionDisconnect, &QAction::triggered, this, &KfxApiTool::disconnect);

    // Menu: COMMAND
    connect(ui->actionAddCommand, &QAction::triggered, this, &KfxApiTool::openAddCommandDialog);

    // Menu: VARIABLE
    connect(ui->actionSetVar, &QAction::triggered, this, &KfxApiTool::openSetVariableDialog);
    connect(ui->actionSubscribeVariable, &QAction::triggered, this, &KfxApiTool::openSubscribeVariableDialog);

    // Menu: EVENT
    connect(ui->actionSubscribeEvent, &QAction::triggered, this, &KfxApiTool::openSubscribeEventDialog);

    // Menu: PRESET
    connect(ui->actionSavePreset, &QAction::triggered, this, &KfxApiTool::savePreset);
    connect(ui->actionLoadPreset, &QAction::triggered, this, &KfxApiTool::loadPreset);

    // Menu: WINDOW
    // This menu is disabled on non Windows machines
    // On Linux for example it requires heavy x11 bypasses which we do not want to do
    connect(ui->actionStayOnTop, &QAction::toggled, this, &KfxApiTool::toggleStayOnTop);

    // Menu: ABOUT
    connect(ui->actionAbout, &QAction::triggered, this, &KfxApiTool::openAboutDialog);
    connect(ui->actionKeeperFxWebsite, &QAction::triggered, this, [this]() {
        QDesktopServices::openUrl(QUrl("https://keeperfx.net"));
    });

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Connect QTcpSocket signals to slots
    connect(tcpSocket, &QTcpSocket::connected, this, &KfxApiTool::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &KfxApiTool::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &KfxApiTool::onReadyRead);

    // Connect QTimer timeout signal to onConnectionTimeout slot
    connect(connectionTimeoutTimer, &QTimer::timeout, this, &KfxApiTool::onConnectionTimeout);

    // Reconnect Timer
    connect(connectionReconnectTimer, &QTimer::timeout, this, &KfxApiTool::onReconnectTimer);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Handle GUI start
    ui->logDisplay->setReadOnly(true);
    statusLabel->setText("Not Connected");
    ui->statusbar->addPermanentWidget(statusLabel);
    setDisconnectedGuiStatus();

    // Handle Var Subscription scroll layout
    areaVarSubsScrollLayout = new QVBoxLayout(ui->areaVarSubs);
    areaVarSubsScrollLayout->setAlignment(Qt::AlignTop);
    ui->areaVarSubs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    ui->areaVarSubs->setLayout(areaVarSubsScrollLayout);

    // Handle Command area scroll layout
    areaCommandsScrollLayout = new QVBoxLayout(ui->areaCommands);
    areaCommandsScrollLayout->setAlignment(Qt::AlignTop);
    ui->areaCommands->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    ui->areaCommands->setLayout(areaCommandsScrollLayout);

    // Use a timer to set sizes after the main window is shown and layout is updated
    QTimer::singleShot(0, this, &KfxApiTool::setSplitterSizes);

    // Disable Always-on-Top button on non Windows OS's
    QOperatingSystemVersion os = QOperatingSystemVersion::current();
    if (os.type() != QOperatingSystemVersion::Windows) {
        ui->menuWindow->setDisabled(true);
        ui->menuWindow->setToolTip("Only available on Windows");
    }
}

KfxApiTool::~KfxApiTool()
{
    delete ui;
}

QString KfxApiTool::playerToHtml(QString playerName)
{
    if(playerName == "PLAYER0"){
        return "<span style=\"color: red\">PLAYER0</span>";
    }
    else if(playerName == "PLAYER1"){
        return "<span style=\"color: blue\">PLAYER1</span>";
    }
    else if(playerName == "PLAYER2"){
        return "<span style=\"color: green\">PLAYER2</span>";
    }
    else if(playerName == "PLAYER3"){
        return "<span style=\"color: yellow\">PLAYER3</span>";
    }
    else if(playerName == "PLAYER4"){
        return "<span style=\"color: purple\">PLAYER4</span>";
    }
    else if(playerName == "PLAYER5"){
        return "<span style=\"color: black\">PLAYER5</span>";
    }
    else if(playerName == "PLAYER6"){
        return "<span style=\"color: orange\">PLAYER6</span>";
    }
    else if(playerName == "PLAYER_GOOD"){
        return "<span style=\"color: gray\">PLAYER_GOOD</span>";
    }
    else if(playerName == "PLAYER_NEUTRAL"){
        return "<span style=\"color: cyan\">PLAYER_NEUTRAL</span>";
    }

    return playerName;
}


void KfxApiTool::setSplitterSizes() {

    // Set the sizes of the top and bottom elements in the window to 2/3 and 1/3
    int totalHeight = ui->splitter->height();
    int scrollAreaHeight = 2 * (totalHeight / 3);
    int plainTextEditHeight = totalHeight / 3;
    ui->splitter->setSizes({scrollAreaHeight, plainTextEditHeight});

    // Set the sizes of the top 2 elements to 3/5 and 2/5
    int splitter2totalWidth = ui->splitter_2->width();
    int splitter2scrollAreaHeight = 3 * (splitter2totalWidth / 5);
    int splitter2plainTextEditHeight = 2 * (splitter2totalWidth / 5);
    ui->splitter_2->setSizes({splitter2scrollAreaHeight, splitter2plainTextEditHeight});
}

void KfxApiTool::appendLog(const QString string)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString timestampString = currentDateTime.toString("HH:mm:ss");
    ui->logDisplay->appendHtml("[" + timestampString + "] " + string);
}

void KfxApiTool::openAboutDialog()
{
    AboutDialog *dialog = new AboutDialog();
    dialog->exec();
}

void KfxApiTool::openConnectDialog()
{
    ConnectDialog *dialog = new ConnectDialog(this);
    connect(dialog, &ConnectDialog::textSubmitted, this, &KfxApiTool::handleServerApiSubmitted);
    dialog->exec();
}

void KfxApiTool::openSubscribeVariableDialog()
{
    SubscribeVariableDialog *dialog = new SubscribeVariableDialog(this);
    connect(dialog, &SubscribeVariableDialog::subVarSubmitted, this, &KfxApiTool::handleSubscribeVariableSubmitted);
    dialog->exec();
}

void KfxApiTool::openSubscribeEventDialog()
{
    SubscribeEventDialog *dialog = new SubscribeEventDialog(this);
    connect(dialog, &SubscribeEventDialog::subEventSubmitted, this, &KfxApiTool::handleSubscribeEventSubmitted);
    dialog->exec();
}

void KfxApiTool::openAddCommandDialog()
{
    AddCommandDialog *dialog = new AddCommandDialog(this);
    connect(dialog, &AddCommandDialog::commandSubmitted, this, &KfxApiTool::handleCommandSubmitted);
    dialog->exec();
}

void KfxApiTool::openSetVariableDialog()
{
    SetVariableDialog *dialog = new SetVariableDialog(this);
    connect(dialog, &SetVariableDialog::setVarSubmitted, this, &KfxApiTool::handleSetVarSubmitted);
    dialog->exec();
}

void KfxApiTool::openEditCommandDialog(CommandWidget *widget, const QString &name, int type, const QString &command)
{
    EditCommandDialog *dialog = new EditCommandDialog(this, widget, name, type, command);
    connect(dialog, &EditCommandDialog::commandUpdated, this, &KfxApiTool::handleCommandUpdated);
    dialog->exec();
}

void KfxApiTool::toggleStayOnTop(bool checked) {
    if (checked) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    show();
}

void KfxApiTool::toggleReconnect(bool checked) {
    if (checked) {
        if(connectionReconnectTimer->isActive() == false){
            connectionReconnectTimer->start(1000);
            appendLog("Automatic reconnect enabled");
        }
    } else {
        connectionReconnectTimer->stop();
        appendLog("Automatic reconnect disabled");
    }
}

SubscribedVariableWidget* KfxApiTool::findSubscribedVariableWidget(const QString &player, const QString &variable)
{
    for(SubscribedVariableWidget *widget : subbedVariableWidgetList)
    {
        if(widget->player == player && widget->variable == variable){
            return widget;
        }
    }

    return nullptr;
}

void KfxApiTool::removeSubscribedVariableWidget(SubscribedVariableWidget *widget)
{
    int index = subbedVariableWidgetList.indexOf(widget);
    if (index != -1) {
        // Remove the widget from the layout
        layout()->removeWidget(widget);

        // Remove the widget from the list
        subbedVariableWidgetList.removeAt(index);

        // Delete the widget if necessary
        widget->deleteLater();
    }
}

SubscribedEventWidget* KfxApiTool::findSubscribedEventWidget(const QString &event)
{
    for(SubscribedEventWidget *widget : subbedEventWidgetList)
    {
        if(widget->event == event){
            return widget;
        }
    }

    return nullptr;
}

void KfxApiTool::removeSubscribedEventWidget(SubscribedEventWidget *widget)
{
    int index = subbedEventWidgetList.indexOf(widget);
    if (index != -1) {
        // Remove the widget from the layout
        layout()->removeWidget(widget);

        // Remove the widget from the list
        subbedEventWidgetList.removeAt(index);

        // Delete the widget if necessary
        widget->deleteLater();
    }
}

void KfxApiTool::handleSubscribeToVariableReturn(const QJsonObject &request, const QJsonObject &response)
{
    // Get some variables
    QString player = request["player"].toString();
    QString variable = request["var"].toString();

    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_VAR"){

            // Find the widget with this subscription
            SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
            if(widget != nullptr)
            {
                // Mark it as broken
                widget->setValidStatus(false);
            }

            QMessageBox::warning(this, "KfxApiTool", "Unknown variable: " + player + " : " + variable);
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Create JSON object
    QJsonObject jsonValueRequestObject;
    jsonValueRequestObject["ack"] = currentAckId;
    jsonValueRequestObject["action"] = "read_var";
    jsonValueRequestObject["var"] = variable;
    jsonValueRequestObject["player"] = player;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) {
            handleSubscribeToVariableReadReturn(request, response);
    }, jsonValueRequestObject);

    // Send the JSON
    sendJSON(jsonValueRequestObject);

    // Increment current ack
    currentAckId++;

    // Show message to end user
    appendLog("Subscribed to " + playerToHtml(request["player"].toString()) + " : " + request["var"].toString());
}

void KfxApiTool::handleSubscribeToEventReturn(const QJsonObject &request, const QJsonObject &response)
{
    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_EVENT"){
            QMessageBox::warning(this, "KfxApiTool", "Unknown event: " + request["var"].toString());
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Check if a widget already exists
    SubscribedEventWidget *widget = findSubscribedEventWidget(request["event"].toString());
    if(widget != nullptr)
    {

        // This should never happen but we'll log a debug message
        qDebug() << "Tried create event sub widget but it already exists";
        return;
    }

    // Create the widget
    widget = new SubscribedEventWidget(nullptr, request["event"].toString());
    connect(widget, &SubscribedEventWidget::removeSubbedEvent, this, &KfxApiTool::removeSubscribedEvent);
    areaVarSubsScrollLayout->addWidget(widget);
    subbedEventWidgetList.append(widget);

    // Show message to end user
    appendLog("Subscribed to " + request["event"].toString());
}

void KfxApiTool::handleCommandExecutedReturn(const QJsonObject &request, const QJsonObject &response)
{

    // Check for error
    if(response.contains("error")){

        // Handle game is paused
        if(response["error"].toString() == "GAME_IS_PAUSED"){
            QMessageBox::warning(this, "KfxApiTool", "Can't execute command.\nGame is paused.");
            return;
        }

        // Failed to execute
        if(response["error"].toString() == "FAILED_TO_EXECUTE_MAP_COMMAND"){
            QMessageBox::warning(this, "KfxApiTool", "The command failed to execute.\nIt's possible there is an error in the command.");
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Show message to end user
    appendLog("Command executed!");
}

void KfxApiTool::handleSubscribeToVariableReadReturn(const QJsonObject &request, const QJsonObject &response)
{
    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_VAR"){
            QMessageBox::warning(this, "KfxApiTool", "Unknown variable");
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Get variables
    QString player = request["player"].toString();
    QString variable = request["var"].toString();
    int value = response["data"].toInt();

    // Check if we have a widget for this variable
    SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
    if(widget == nullptr)
    {
        // Create the widget
        addSubscribedVariableWidget(player, variable, value);
    } else {

        // Update already existing widget
        widget->update(value);
    }
}

void KfxApiTool::handleUnsubscribeVariableReturn(const QJsonObject &request, const QJsonObject &response)
{
    qDebug() << request << response;

    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_VAR"){
            QMessageBox::warning(this, "KfxApiTool", "Unknown variable");
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    if(response.contains("success") && response["success"].toBool() != true)
    {
        QMessageBox::warning(this, "KfxApiTool", "Failed to unsubscribe");
        return;
    }

    SubscribedVariableWidget *widget = findSubscribedVariableWidget(request["player"].toString(), request["var"].toString());
    if(widget == nullptr){
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong");
        return;
    }

    // Remove the widget
    removeSubscribedVariableWidget(widget);

    // Show message
    appendLog("Unsubscribed from " + playerToHtml(request["player"].toString()) + " : " + request["var"].toString());
}

void KfxApiTool::handleUnsubscribeEventReturn(const QJsonObject &request, const QJsonObject &response)
{
    qDebug() << request << response;

    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_EVENT"){
            QMessageBox::warning(this, "KfxApiTool", "Unknown event");
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Make sure response was a succes
    if(response.contains("success") && response["success"].toBool() != true)
    {
        QMessageBox::warning(this, "KfxApiTool", "Failed to unsubscribe");
        return;
    }

    // Check if there is a widget for this event
    SubscribedEventWidget *widget = findSubscribedEventWidget(request["event"].toString());
    if(widget == nullptr){
        qDebug() << "Tried to delete non existing widget for event: " << request["event"].toString();
        // No need to show a message to the user as this should never happen
        return;
    }

    // Delete the widget
    removeSubscribedEventWidget(widget);

    // Show a success message
    appendLog("Unsubscribed from " + request["event"].toString());
}

void KfxApiTool::handleSetVariableReturn(const QJsonObject &request, const QJsonObject &response)
{
    // Check for error
    if(response.contains("error")){

        // Handle unknown variable
        if(response["error"].toString() == "UNKNOWN_VAR"){
            QMessageBox::warning(this, "KfxApiTool", "Unknown variable");
            return;
        }

        // Handle unable to set variable
        if(response["error"].toString() == "UNABLE_TO_SET_VAR"){
            QMessageBox::warning(this, "KfxApiTool", "Unable to set this variable");
            return;
        }

        // Fallback to simply displaying the returned error
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong\n\nError: " + response["error"].toString());
        return;
    }

    // Check for success
    if(response.contains("success") && response["success"].toBool() != true)
    {
        QMessageBox::warning(this, "KfxApiTool", "Failed to set variable");
        return;
    }

    // Get some values
    QString player = request["player"].toString();
    QString variable = request["var"].toString();
    int value = request["value"].toInt();

    // Show the var updated message if we are not subscribed to the variable
    if(findSubscribedVariableWidget(player, variable) == nullptr){
        appendLog("Var update: " + playerToHtml(player) + " : " + variable + " -> " + QString::number(value));
    }
}

void KfxApiTool::subscribeToVariable(const QString &player, const QString &variable)
{

    // Create JSON object to send to API
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "subscribe_var";
    jsonSubObject["var"] = variable;
    jsonSubObject["player"] = player;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) {
            handleSubscribeToVariableReturn(request, response);
    }, jsonSubObject);

    // Send JSON
    sendJSON(jsonSubObject);

    // Update current ACK
    currentAckId++;
}

void KfxApiTool::subscribeToEvent(const QString &event)
{
    // Create JSON object to send to API
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "subscribe_event";
    jsonSubObject["event"] = event;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) {
            handleSubscribeToEventReturn(request, response);
    }, jsonSubObject);

    // Send JSON
    sendJSON(jsonSubObject);

    // Update current ACK
    currentAckId++;
}

void KfxApiTool::handleSubscribeVariableSubmitted(const QString &player, const QString &variable)
{
    // Make sure we are not subbed to this variable yet
    if(findSubscribedVariableWidget(player, variable) != nullptr){
        QMessageBox::warning(this, "KfxApiTool", "Already subscribed to this variable");
        return;
    }

    // Create the widget
    addSubscribedVariableWidget(player, variable, -1);

    // Check if we are connected to the API
    if (tcpSocket->state() == QTcpSocket::ConnectedState) {

        // Actually subscribe to the variable
        subscribeToVariable(player, variable);
    }
}

void KfxApiTool::handleCommandSubmitted(const QString &name, int type, const QString &command)
{
    // Simply create the widget
    // Commands can be duplicates because it would be a pain to check if it already exists
    addCommandWidget(name, type, command);

    // Show the end user a message
    appendLog("Command added: " + name);
}

void KfxApiTool::handleCommandUpdated(CommandWidget *widget, const QString &name, int type, const QString &command)
{
    // Update the widget which also holds the data of the command
    widget->updateWidget(name, type, command);

    // Show message to end user
    appendLog("Command updated: " + name);
}

void KfxApiTool::handleSetVarSubmitted(const QString &player, const QString &variable, int value)
{
    // Make sure we are connected
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        appendLog("Can't update variable when not connected to the API!");
        QMessageBox::information(this, "KfxApiTool", "Can't update variable when not connected to the API!");
        return;
    }

    // Make the JSON object
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "set_var";
    jsonSubObject["player"] = player;
    jsonSubObject["var"] = variable;
    jsonSubObject["value"] = value;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) {
            handleSetVariableReturn(request, response);
    }, jsonSubObject);

    // Send the JSON to the API
    sendJSON(jsonSubObject);

    // Update current ACK
    currentAckId++;
}

void KfxApiTool::handleCommandExecuted(const QString &name, int type, const QString &command)
{
    // Make sure we are connected
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        appendLog("Can't execute command when not connected!");
        return;
    }

    // Decide the action based on the users choice in the GUI
    QString action;
    if(type == 0){
        action = "map_command";
    } else if (type == 1) {
        action = "console_command";
    }

    // Create JSON object to send to API
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = action;
    jsonSubObject["command"] = command;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) {
            handleCommandExecutedReturn(request, response);
    }, jsonSubObject);

    // Send the JSON to the API
    sendJSON(jsonSubObject);

    // Update current ACK
    currentAckId++;
}

void KfxApiTool::handleSubscribeEventSubmitted(const QString &event)
{

    // Make sure we are not subbed to this variable yet
    if(findSubscribedEventWidget(event) != nullptr){
        QMessageBox::information(this, "KfxApiTool", "Already subscribed to event: " + event);
        return;
    }

    // Check if we are disconneced
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {

        // Simply create the widget
        // We don't need to send a packet to the server here because
        // it will be sent when we connect to the API
        addSubscribedEventWidget(event);
        return;
    }

    // Actually subscribe to this event on the API
    subscribeToEvent(event);
}

void KfxApiTool::removeSubscribedVariable(const QString &player, const QString &variable)
{
    // Make sure a widget for this subscription exists
    SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
    if(widget == nullptr){
        return;
    }

    // Check if we are disconnected
    if (tcpSocket->state() == QTcpSocket::UnconnectedState) {

        // Delete the subscribed variable widget
        removeSubscribedVariableWidget(widget);

        // We don't need to send a packet to the server because
        // the server will already have removed the subscription
        return;
    }

    // Create JSON
    QJsonObject jsonValueRequestObject;
    jsonValueRequestObject["ack"] = currentAckId;
    jsonValueRequestObject["action"] = "unsubscribe_var";
    jsonValueRequestObject["var"] = variable;
    jsonValueRequestObject["player"] = player;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleUnsubscribeVariableReturn(request, response); }, jsonValueRequestObject);

    // Send JSON data
    sendJSON(jsonValueRequestObject);

    // Increment current ack
    currentAckId++;
}

void KfxApiTool::editSubscribedVariable(const QString &player, const QString &variable, int value)
{
    // Make sure we are connected
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        appendLog("Can not set a variable while disconnected");
        QMessageBox::warning(this, "KfxApiTool", "Unable to set a variable when not connected to the API");
        return;
    }

    // Open the set variable dialog
    SetVariableDialog *dialog = new SetVariableDialog(this, player, variable, value);
    connect(dialog, &SetVariableDialog::setVarSubmitted, this, &KfxApiTool::handleSetVarSubmitted);
    dialog->exec();
}

void KfxApiTool::removeSubscribedEvent(const QString &event)
{
    // Make sure there is a widget for this subscription
    SubscribedEventWidget *widget = findSubscribedEventWidget(event);
    if(widget == nullptr){
        return;
    }

    // Check if we are disconnected
    if (tcpSocket->state() == QTcpSocket::UnconnectedState) {

        // Delete the subscribed event widget
        removeSubscribedEventWidget(widget);

        // We don't need to send a packet to the server because
        // the server will already have removed the subscription
        return;
    }

    // Create JSON
    QJsonObject jsonValueRequestObject;
    jsonValueRequestObject["ack"] = currentAckId;
    jsonValueRequestObject["action"] = "unsubscribe_event";
    jsonValueRequestObject["event"] = event;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleUnsubscribeEventReturn(request, response); }, jsonValueRequestObject);

    // Send JSON data
    sendJSON(jsonValueRequestObject);

    // Increment current ack
    currentAckId++;
}

void KfxApiTool::handleServerApiSubmitted(const QString &text)
{
    // Split the input text to get IP and port
    QStringList parts = text.split(':');
    if (parts.size() != 2) {
        appendLog("Invalid format. Use IP:Port");
        return;
    }

    // Get IP parts
    currentIP = parts[0];
    currentPort = parts[1].toInt();

    // Show message that we are connecting
    appendLog("Connecting to " + text + "...");
    statusLabel->setText("Connecting...");

    // Disable Connect GUI buttons
    ui->actionConnect->setDisabled(true);
    ui->actionConnectDefault->setDisabled(true);

    // Start the connection timer with a 5-second timeout
    connectionTimeoutTimer->start(5000);

    // Connect to API
    tcpSocket->connectToHost(currentIP, currentPort);
}

void KfxApiTool::setConnectedGuiStatus()
{
    ui->actionConnect->setDisabled(true);
    ui->actionConnectDefault->setDisabled(true);
    ui->actionDisconnect->setDisabled(false);
    ui->actionSetVar->setDisabled(false);
}

void KfxApiTool::setDisconnectedGuiStatus()
{
    ui->actionConnect->setDisabled(false);
    ui->actionConnectDefault->setDisabled(false);
    ui->actionDisconnect->setDisabled(true);
    ui->actionSetVar->setDisabled(true);
}

void KfxApiTool::onConnected()
{
    // Stop the timeout timer
    connectionTimeoutTimer->stop();

    // Show a message to the end user
    appendLog("Connected!");

    // Update GUI
    statusLabel->setText("Connected to " + currentIP + ":" + QString::number(currentPort));
    setConnectedGuiStatus();

    // If we should reconnect we need to remember this
    shouldReconnect = true;

    // Subscribe to any variable subscriptions already in the UI
    for(SubscribedVariableWidget *widget: subbedVariableWidgetList)
    {
        // Subscribe to the var on the API server
        subscribeToVariable(widget->player, widget->variable);

        // This sleep fixes a condition where the underlying TCP socket
        // tries to combine packets even if we don't want that
        QThread::msleep(50);
    }

    // Subscribe to any event subscriptions already in the UI
    for(SubscribedEventWidget *widget: subbedEventWidgetList)
    {
        // Subscribe to the var on the API server
        subscribeToEvent(widget->event);

        // This sleep fixes a condition where the underlying TCP socket
        // tries to combine packets even if we don't want that
        QThread::msleep(50);
    }
}

void KfxApiTool::onDisconnected()
{
    // Stop a possible timeout timer
    connectionTimeoutTimer->stop();

    // Show a message to the end user that we are disconnected
    appendLog("Disconnected");

    // Update the GUI
    statusLabel->setText("Not connected");
    setDisconnectedGuiStatus();

    // Check if we need to reconnect automatically
    if(shouldReconnect && connectionReconnectTimer->isActive()){

        // Only update the GUI here
        appendLog("Trying to reconnect...");
        statusLabel->setText("Trying to reconnect...");
    }
}

void KfxApiTool::disconnect()
{
    // Disable the auto reconnect when we chose to disconnect
    shouldReconnect = false;

    // Check if we are connected
    if (tcpSocket->state() == QTcpSocket::ConnectedState) {

        // Disconnect from the API
        tcpSocket->disconnectFromHost();
    }
}

void KfxApiTool::onConnectionTimeout()
{
    // Check if we are connected yet
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {

        // Abort the connection attempt
        // This is done in case of a buggy connection keeping the connection attempt going
        tcpSocket->abort();

        // Stop the connection timeout timer
        connectionTimeoutTimer->stop();

        // Show message to end user
        appendLog("Connection timed out");

        // Update the GUI
        statusLabel->setText("Not connected");
        setDisconnectedGuiStatus();
    }
}

std::optional<QJsonObject> KfxApiTool::getJsonFromPacket(QByteArray data)
{
    // Attempt to parse received data as JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    // Check if there was a parse error
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error: " << parseError.errorString();
        qDebug() << "JSON parse data: " << data;
        appendLog("JSON parse error in received API data");
        return std::nullopt;
    }

    // Make sure the received JSON is an object
    if (jsonDoc.isObject() == false) {
        qDebug() << "Received data is not a JSON object";
        appendLog("Received invalid data from the API");
        return std::nullopt;
    }

    // Return the object
    return jsonDoc.object();
}

void KfxApiTool::onReadyRead()
{
    // Read the data from the socket
    while(tcpSocket->bytesAvailable())
    {
        QByteArray data = tcpSocket->readLine();

        // Remove possible newline characters at the end
        while (!data.isEmpty() && (data.endsWith('\n') || data.endsWith('\r'))) {
            data.chop(1);
        }

        // Get JSON
        std::optional<QJsonObject> result = getJsonFromPacket(data);
        if(result){

            QJsonObject jsonObject = result.value();

            // Check for ack
            if(jsonObject.contains("ack")){

                // Call the callback
                callActionCallback(jsonObject["ack"].toInt(), jsonObject);
                removeActionCallback(jsonObject["ack"].toInt());

            } else {

                // Handle JSON
                // These are probably subscriptions
                handleReceivedJson(jsonObject);
            }

        } else {
            appendLog("Received data that was not a valid JSON");
        }
    }
}

// Method to send data to the server
void KfxApiTool::sendData(const QByteArray &data)
{
    // Make sure we are connected
    if (tcpSocket->state() == QTcpSocket::ConnectedState) {
        qDebug() << data;

        // Send data and try to flush
        // For some reason the flush does not clear the buffer,
        // which means that some messages are combined in a packet
        tcpSocket->write(data);
        tcpSocket->flush();

    } else {
        qDebug() << "Not connected to API";
    }
}

void KfxApiTool::sendJSON(const QJsonObject &jsonObject)
{
    QJsonDocument doc(jsonObject);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    sendData(jsonData);
}

void KfxApiTool::handleReceivedJson(const QJsonObject &jsonObject)
{
    qDebug() << "Received JSON:" << QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);

    // Check for errors
    if(jsonObject.contains("error")){
        return;
    }

    // Everything after this point should be events
    if(jsonObject.contains("event") == false){
        return;
    }

    if(jsonObject["event"] == "VAR_UPDATE"){

        // Get the data
        QJsonObject varObject = jsonObject["var"].toObject();
        QString player = varObject["player"].toString();
        QString variable = varObject["name"].toString();
        int value = varObject["value"].toInt();

        // Make sure there is a widget for this var
        SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
        if(widget == nullptr){
            qDebug() << "Received a VAR_UPDATE but widget does not exist for " << player << " : " << variable;
            return;
        }

        // Update the widget
        widget->update(value);

        // Log this if it wasn't a game turn update
        if(variable != "GAME_TURN"){
            appendLog("Var update: " + playerToHtml(player) + " : " + variable + " -> " + QString::number(value));
        }
    }
}

void KfxApiTool::onReconnectTimer()
{
    if (shouldReconnect && tcpSocket->state() == QTcpSocket::UnconnectedState) {
        tcpSocket->connectToHost(currentIP, currentPort);
    }
}

// Function to add callback information with an ID
void KfxApiTool::addActionCallback(int id, const ActionCallbackFunction& callback, const QJsonObject& request) {
    ActionCallbackInfo info{callback, request};
    actionCallbackMap.insert(id, info);
}

// Function to call the callback function associated with an ID
void KfxApiTool::callActionCallback(int id, const QJsonObject& response) {
    if (actionCallbackMap.contains(id)) {
        auto info = actionCallbackMap.value(id);
        info.callback(info.request, response);
    }
}

void KfxApiTool::removeActionCallback(int id)
{
    if (actionCallbackMap.contains(id)) {
        actionCallbackMap.remove(id);
    }
}

void KfxApiTool::savePreset()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Preset"), "", tr("Preset Files (*.api);;All Files (*)"));
    if (!filePath.isEmpty()) {
        savePresetToFile(filePath);
    }
}

void KfxApiTool::loadPreset()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Load Preset"), "", tr("Preset Files (*.api);;All Files (*)"));
    if (!filePath.isEmpty()) {
        loadPresetFromFile(filePath);
    }
}

void KfxApiTool::savePresetToFile(const QString &filePath)
{
    // JSON object variables
    QJsonObject jsonObject;
    QJsonArray subscriptions;
    QJsonArray commands;

    // Loop through each item in the scroll area
    for (int i = 0; i < areaVarSubsScrollLayout->count(); ++i) {

        // Get widget in scroll area
        QWidget *widget = areaVarSubsScrollLayout->itemAt(i)->widget();
        if (!widget) {
            continue;
        }

        // Handle variable subscription
        SubscribedVariableWidget *subVarWidget = qobject_cast<SubscribedVariableWidget*>(widget);
        if (subVarWidget) {
            QJsonObject subVarJsonObject;
            subVarJsonObject["player"] = subVarWidget->player;
            subVarJsonObject["variable"] = subVarWidget->variable;
            subscriptions.append(subVarJsonObject);
            continue;
        }

        // Handle event subscription
        SubscribedEventWidget *subEventWidget = qobject_cast<SubscribedEventWidget*>(widget);
        if (subEventWidget) {
            QJsonObject subEventJsonObject;
            subEventJsonObject["event"] = subEventWidget->event;
            subscriptions.append(subEventJsonObject);
            continue;
        }
    }

    // Loop through each item in the scroll area
    for (int i = 0; i < areaCommandsScrollLayout->count(); ++i) {

        // Get widget in scroll area
        QWidget *widget = areaCommandsScrollLayout->itemAt(i)->widget();
        if (!widget) {
            continue;
        }

        // Handle command
        CommandWidget *commandWidget = qobject_cast<CommandWidget*>(widget);
        if (commandWidget) {
            QJsonObject commandJsonObject;
            commandJsonObject["name"] = commandWidget->name;
            commandJsonObject["type"] = commandWidget->type;
            commandJsonObject["command"] = commandWidget->command;
            commands.append(commandJsonObject);
            continue;
        }
    }

    // Add stuff to main JSON object
    jsonObject.insert("subscriptions", subscriptions);
    jsonObject.insert("commands", commands);

    // Convert JSON to string
    QJsonDocument doc(jsonObject);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // Open the file for saving
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text) == false) {
        appendLog("Failed to create and open file for saving");
        QMessageBox::warning(this, "KfxApiTool", "Failed to create and open file for saving");

        return;
    }

    // Write JSON to file
    file.write(jsonData);

    // Close the file
    file.close();

    // Show success to end user
    appendLog("Preset saved! -> " + filePath);
    QMessageBox::information(this, "KfxApiTool", "Preset saved!");
}

void KfxApiTool::loadPresetFromFile(const QString &filePath)
{
    // Open the file for reading
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) == false) {

        // Show error message to user
        appendLog("Failed to open file for reading");
        QMessageBox::warning(this, "KfxApiTool", "Failed to open file for reading");

        return;
    }

    // Read the file
    QByteArray data = file.readAll();

    // Attempt to parse data as JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    // Close the file
    file.close();

    // Check if there was a parse error
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error: " << parseError.errorString();
        qDebug() << "JSON parse data: " << data;
        appendLog("JSON parse error in loaded preset");
        QMessageBox::warning(this, "KfxApiTool", "Invalid preset file.");
        return;
    }

    // Make sure the received JSON is an object
    if (jsonDoc.isObject() == false) {
        qDebug() << "Data in preset file is not a JSON object";
        appendLog("Data in preset file is not a JSON object");
        QMessageBox::warning(this, "KfxApiTool", "Invalid preset file.");
        return;
    }

    // Clear the UI
    // Loop through each item in the scroll area and delete them
    for (int i = 0; i < areaCommandsScrollLayout->count(); i++) {
        QWidget *widget = areaCommandsScrollLayout->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->deleteLater();
        }
    }
    for (int i = 0; i < areaVarSubsScrollLayout->count(); i++) {
        QWidget *widget = areaVarSubsScrollLayout->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->deleteLater();
        }
    }

    // Clear the subscription lists
    subbedEventWidgetList.clear();
    subbedVariableWidgetList.clear();

    // Get the JSON object
    QJsonObject jsonObject = jsonDoc.object();

    // Load the subscriptions
    if(jsonObject.contains("subscriptions") && jsonObject["subscriptions"].isArray())
    {
        for (const QJsonValue &value : jsonObject["subscriptions"].toArray()) {
            if (value.isObject() == false) {
                continue;
            }

            QJsonObject subObject = value.toObject();

            if(subObject.contains("event") && subObject["event"].isString())
            {
                addSubscribedEventWidget(subObject["event"].toString());
                continue;
            }

            if(subObject.contains("player") && subObject["player"].isString()
                && subObject.contains("variable") && subObject["variable"].isString())
            {
                addSubscribedVariableWidget(subObject["player"].toString(), subObject["variable"].toString(), -1);
                continue;
            }
        }
    }

    // Load the commands
    if(jsonObject.contains("commands") && jsonObject["commands"].isArray())
    {
        for (const QJsonValue &value : jsonObject["commands"].toArray()) {
            if (value.isObject() == false) {
                continue;
            }

            QJsonObject subObject = value.toObject();

            if(subObject.contains("name") && subObject["name"].isString()
                && subObject.contains("type") && subObject["type"].isDouble()
                && subObject.contains("command") && subObject["command"].isString())
            {
                addCommandWidget(subObject["name"].toString(), subObject["type"].toDouble(), subObject["command"].toString());
                continue;
            }
        }
    }

    // Show success to end user
    // No need for a messagebox here as they'll see the GUI update
    appendLog("Preset loaded! -> " + filePath);
    QMessageBox::information(this, "KfxApiTool", "Preset loaded!");
}



void KfxApiTool::addSubscribedVariableWidget(const QString &player, const QString &variable, int value){
    SubscribedVariableWidget *widget = new SubscribedVariableWidget(nullptr, player, variable, value);
    connect(widget, &SubscribedVariableWidget::removeSubbedVariable, this, &KfxApiTool::removeSubscribedVariable);
    connect(widget, &SubscribedVariableWidget::editSubbedVariable, this, &KfxApiTool::editSubscribedVariable);
    areaVarSubsScrollLayout->addWidget(widget);
    subbedVariableWidgetList.append(widget);
}

void KfxApiTool::addSubscribedEventWidget(const QString &event){
    SubscribedEventWidget *widget = new SubscribedEventWidget(nullptr, event);
    connect(widget, &SubscribedEventWidget::removeSubbedEvent, this, &KfxApiTool::removeSubscribedEvent);
    areaVarSubsScrollLayout->addWidget(widget);
    subbedEventWidgetList.append(widget);
}

void KfxApiTool::addCommandWidget(const QString &name, int type, const QString &command){
    CommandWidget *widget = new CommandWidget(nullptr, name, type, command);
    connect(widget, &CommandWidget::executeCommand, this, &KfxApiTool::handleCommandExecuted);
    connect(widget, &CommandWidget::editCommand, this, &KfxApiTool::openEditCommandDialog);
    areaCommandsScrollLayout->addWidget(widget);
}
