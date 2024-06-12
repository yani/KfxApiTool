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

    // Connect Actions
    connect(ui->actionAbout, &QAction::triggered, this, &KfxApiTool::openAboutDialog);
    connect(ui->actionConnect, &QAction::triggered, this, &KfxApiTool::openConnectDialog);
    connect(ui->actionSubscribeVariable, &QAction::triggered, this, &KfxApiTool::openSubscribeVariableDialog);
    connect(ui->actionSubscribeEvent, &QAction::triggered, this, &KfxApiTool::openSubscribeEventDialog);
    connect(ui->actionAddCommand, &QAction::triggered, this, &KfxApiTool::openAddCommandDialog);
    connect(ui->actionSetVar, &QAction::triggered, this, &KfxApiTool::openSetVariableDialog);
    connect(ui->actionDisconnect, &QAction::triggered, this, &KfxApiTool::disconnect);
    connect(ui->actionStayOnTop, &QAction::toggled, this, &KfxApiTool::toggleStayOnTop);
    connect(ui->actionAutoReconnect, &QAction::toggled, this, &KfxApiTool::toggleReconnect);
    connect(ui->actionConnectDefault, &QAction::triggered, this, [this]() {
        handleServerApiSubmitted("127.0.0.1:5599");
    });
    connect(ui->actionKeeperFxWebsite, &QAction::triggered, this, [this]() {
        QDesktopServices::openUrl(QUrl("https://keeperfx.net"));
    });

    // Connect QTcpSocket signals to slots
    connect(tcpSocket, &QTcpSocket::connected, this, &KfxApiTool::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &KfxApiTool::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &KfxApiTool::onReadyRead);

    // Connect QTimer timeout signal to onConnectionTimeout slot
    connect(connectionTimeoutTimer, &QTimer::timeout, this, &KfxApiTool::onConnectionTimeout);

    // Reconnect Timer
    connect(connectionReconnectTimer, &QTimer::timeout, this, &KfxApiTool::onReconnectTimer);

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

// Set the sizes of the top and bottom elements in the window to 2/3 and 1/3
void KfxApiTool::setSplitterSizes() {
    int totalHeight = ui->splitter->height();
    int scrollAreaHeight = 2 * (totalHeight / 3);
    int plainTextEditHeight = totalHeight / 3;
    ui->splitter->setSizes({scrollAreaHeight, plainTextEditHeight});

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

void KfxApiTool::handleSubscribeToVariableReturn(const QJsonObject &request, const QJsonObject &response)
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

    // Get current value
    QJsonObject jsonValueRequestObject;
    jsonValueRequestObject["ack"] = currentAckId;
    jsonValueRequestObject["action"] = "read_var";
    jsonValueRequestObject["var"] = request["var"];
    jsonValueRequestObject["player"] = request["player"];

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleSubscribeToVariableReadReturn(request, response); }, jsonValueRequestObject);

    sendJSON(jsonValueRequestObject);

    // Increment current ack
    currentAckId++;

    appendLog("Subscribed to " + playerToHtml(request["player"].toString()) + " : " + request["var"].toString());
}

void KfxApiTool::handleSubscribeToEventReturn(const QJsonObject &request, const QJsonObject &response)
{
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

    SubscribedEventWidget *widget = findSubscribedEventWidget(request["event"].toString());
    if(widget == nullptr)
    {
        widget = new SubscribedEventWidget(nullptr, request["event"].toString());
        connect(widget, &SubscribedEventWidget::removeSubbedEvent, this, &KfxApiTool::removeSubscribedEvent);
        areaVarSubsScrollLayout->addWidget(widget);
        subbedEventWidgetList.append(widget);
    }

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

    QString player = request["player"].toString();
    QString variable = request["var"].toString();
    int value = response["data"].toInt();

    SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
    if(widget == nullptr)
    {
        widget = new SubscribedVariableWidget(nullptr, player, variable, value);
        connect(widget, &SubscribedVariableWidget::removeSubbedVariable, this, &KfxApiTool::removeSubscribedVariable);
        connect(widget, &SubscribedVariableWidget::editSubbedVariable, this, &KfxApiTool::editSubscribedVariable);
        areaVarSubsScrollLayout->addWidget(widget);
        subbedVariableWidgetList.append(widget);
    } else {

        // Update already existing value
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

    // Remove from UI
    areaVarSubsScrollLayout->removeWidget(widget);

    // Remove widget itself
    delete widget;

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

    if(response.contains("success") && response["success"].toBool() != true)
    {
        QMessageBox::warning(this, "KfxApiTool", "Failed to unsubscribe");
        return;
    }

    SubscribedEventWidget *widget = findSubscribedEventWidget(request["event"].toString());
    if(widget == nullptr){
        QMessageBox::warning(this, "KfxApiTool", "Something went wrong");
        return;
    }

    // Remove from UI
    areaVarSubsScrollLayout->removeWidget(widget);

    // Remove widget itself
    delete widget;

    // Show message
    appendLog("Unsubscribed from " + request["event"].toString());
}

void KfxApiTool::handleSetVariableReturn(const QJsonObject &request, const QJsonObject &response){

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
    QJsonParseError parseError;

    // Create JSON object to send to API
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "subscribe_var";
    jsonSubObject["var"] = variable;
    jsonSubObject["player"] = player;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleSubscribeToVariableReturn(request, response); }, jsonSubObject);

    sendJSON(jsonSubObject);

    currentAckId++;
}

void KfxApiTool::subscribeToEvent(const QString &event)
{
    QJsonParseError parseError;

    // Create JSON object to send to API
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "subscribe_event";
    jsonSubObject["event"] = event;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleSubscribeToEventReturn(request, response); }, jsonSubObject);

    sendJSON(jsonSubObject);

    currentAckId++;
}

void KfxApiTool::handleSubscribeVariableSubmitted(const QString &player, const QString &variable)
{
    // Make sure we are connected
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        return;
    }

    // Make sure we are not subbed to this variable yet
    if(findSubscribedVariableWidget(player, variable) != nullptr){
        QMessageBox::warning(this, "KfxApiTool", "Already subscribed to this variable");
        return;
    }

    subscribeToVariable(player, variable);
}

void KfxApiTool::handleCommandSubmitted(const QString &name, int type, const QString &command)
{
    //CommandWidget *widget = findCommandWidget(name, type, command);
    CommandWidget *widget = nullptr;
    if(widget == nullptr)
    {
        widget = new CommandWidget(nullptr, name, type, command);
        connect(widget, &CommandWidget::executeCommand, this, &KfxApiTool::handleCommandExecuted);
        connect(widget, &CommandWidget::editCommand, this, &KfxApiTool::openEditCommandDialog);
        areaCommandsScrollLayout->addWidget(widget);
        commandWidgetList.append(widget);

        appendLog("Command added: " + name);
    } else {

        // Update already existing value
        // ERROR already exists
    }
}

void KfxApiTool::handleCommandUpdated(CommandWidget *widget, const QString &name, int type, const QString &command)
{
    widget->updateWidget(name, type, command);

    appendLog("Command updated: " + name);
}

void KfxApiTool::handleSetVarSubmitted(const QString &player, const QString &variable, int value)
{
    QJsonObject jsonSubObject;
    jsonSubObject["ack"] = currentAckId;
    jsonSubObject["action"] = "set_var";
    jsonSubObject["player"] = player;
    jsonSubObject["var"] = variable;
    jsonSubObject["value"] = value;

    // Create a callback to handle the response
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleSetVariableReturn(request, response); }, jsonSubObject);

    sendJSON(jsonSubObject);

    currentAckId++;
}

void KfxApiTool::handleCommandExecuted(const QString &name, int type, const QString &command)
{
    QJsonParseError parseError;

    // Decide the action
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
    addActionCallback(currentAckId, [this](const QJsonObject& request, const QJsonObject& response) { handleCommandExecutedReturn(request, response); }, jsonSubObject);

    sendJSON(jsonSubObject);

    currentAckId++;
}

void KfxApiTool::handleSubscribeEventSubmitted(const QString &event)
{
    // Make sure we are connected
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        return;
    }

    // Make sure we are not subbed to this variable yet
    if(findSubscribedEventWidget(event) != nullptr){
        QMessageBox::warning(this, "KfxApiTool", "Already subscribed to this event");
        return;
    }

    subscribeToEvent(event);
}

void KfxApiTool::removeSubscribedVariable(const QString &player, const QString &variable){

    SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
    if(widget == nullptr){
        return;
    }

    if (tcpSocket->state() == QTcpSocket::UnconnectedState) {
        areaVarSubsScrollLayout->removeWidget(widget);
        delete widget;
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

void KfxApiTool::editSubscribedVariable(const QString &player, const QString &variable, int value){

    SetVariableDialog *dialog = new SetVariableDialog(this, player, variable, value);
    connect(dialog, &SetVariableDialog::setVarSubmitted, this, &KfxApiTool::handleSetVarSubmitted);
    dialog->exec();
}

void KfxApiTool::removeSubscribedEvent(const QString &event)
{
    SubscribedEventWidget *widget = findSubscribedEventWidget(event);
    if(widget == nullptr){
        return;
    }

    if (tcpSocket->state() == QTcpSocket::UnconnectedState) {
        areaVarSubsScrollLayout->removeWidget(widget);
        delete widget;
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
    appendLog("Connecting to " + text + "...");
    statusLabel->setText("Connecting...");

    // Split the input text to get IP and port
    QStringList parts = text.split(':');
    if (parts.size() != 2) {
        appendLog("Invalid format. Use IP:Port");
        statusLabel->setText("Not connected");
        return;
    }

    currentIP = parts[0];
    currentPort = parts[1].toInt();

    // Start the connection timer with a 5-second timeout
    connectionTimeoutTimer->start(5000);

    ui->actionConnect->setDisabled(true);
    ui->actionConnectDefault->setDisabled(true);

    tcpSocket->connectToHost(currentIP, currentPort);
}

void KfxApiTool::setConnectedGuiStatus()
{
    ui->actionConnect->setDisabled(true);
    ui->actionConnectDefault->setDisabled(true);
    ui->actionDisconnect->setDisabled(false);
    ui->actionSubscribeVariable->setDisabled(false);
    ui->actionSubscribeEvent->setDisabled(false);
    ui->actionSetVar->setDisabled(false);
    ui->actionAddCommand->setDisabled(false);
}

void KfxApiTool::setDisconnectedGuiStatus()
{
    ui->actionConnect->setDisabled(false);
    ui->actionConnectDefault->setDisabled(false);
    ui->actionDisconnect->setDisabled(true);
    ui->actionSubscribeVariable->setDisabled(true);
    ui->actionSubscribeEvent->setDisabled(true);
    ui->actionSetVar->setDisabled(true);
    ui->actionAddCommand->setDisabled(true);
}

void KfxApiTool::onConnected()
{

    appendLog("Connected!");
    statusLabel->setText("Connected to " + currentIP + ":" + QString::number(currentPort));
    setConnectedGuiStatus();
    connectionTimeoutTimer->stop();
    shouldReconnect = true;

    // Subscribe to any subscribtions in the UI
    for(SubscribedVariableWidget *widget: subbedVariableWidgetList)
    {
        subscribeToVariable(widget->player, widget->variable);
        QThread::msleep(50);
    }
}

void KfxApiTool::onDisconnected()
{
    appendLog("Disconnected");
    statusLabel->setText("Not connected");
    setDisconnectedGuiStatus();
    connectionTimeoutTimer->stop();

    if(shouldReconnect && connectionReconnectTimer->isActive()){
        appendLog("Trying to reconnect...");
        statusLabel->setText("Trying to reconnect...");
    }
}

void KfxApiTool::disconnect()
{
    shouldReconnect = false;
    if (tcpSocket->state() == QTcpSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
    }
}

void KfxApiTool::onConnectionTimeout()
{
    if (tcpSocket->state() != QTcpSocket::ConnectedState) {
        tcpSocket->abort(); // Abort the connection attempt
        appendLog("Connection timed out");
        statusLabel->setText("Not connected");
        setDisconnectedGuiStatus();
        connectionTimeoutTimer->stop();
    }
}

std::optional<QJsonObject> KfxApiTool::getJsonFromPacket(QByteArray data)
{
    // Attempt to parse received data as JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        if (jsonDoc.isObject()) {
            return jsonDoc.object();
        } else {
            qDebug() << "Received data is not in JSON format";
            appendLog("Invalid data in packet");
        }
    } else {
        qDebug() << "JSON parse error:" << parseError.errorString();
        appendLog("JSON parse error");

        qDebug() << "***" << data << "***";
    }

    return std::nullopt;
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
    if (tcpSocket->state() == QTcpSocket::ConnectedState) {
        qDebug() << data;

         // Send data and try to flush
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

        // Check if there is a widget for this var
        SubscribedVariableWidget *widget = findSubscribedVariableWidget(player, variable);
        if(widget != nullptr){

            // Update the widget
            widget->update(value);

            // Log this if it wasn't a game turn update
            if(variable != "GAME_TURN"){
                appendLog("Var update: " + playerToHtml(player) + " : " + variable + " -> " + QString::number(value));
            }

        } else {

            // Create a new widget
            // This can happen when we connect to a game that already has subscribed variables
            SubscribedVariableWidget *widget = new SubscribedVariableWidget(nullptr, player, variable, value);
            connect(widget, &SubscribedVariableWidget::removeSubbedVariable, this, &KfxApiTool::removeSubscribedVariable);
            connect(widget, &SubscribedVariableWidget::editSubbedVariable, this, &KfxApiTool::editSubscribedVariable);
            areaVarSubsScrollLayout->addWidget(widget);
            subbedVariableWidgetList.append(widget);
            appendLog("Pre-subscribed variable found: " + player + " : " + variable + " -> " + QString::number(value));
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
