#ifndef KFX_API_TOOL_H
#define KFX_API_TOOL_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QLabel>
#include <QList>
#include <QVBoxLayout>
#include <QThread>
#include <QMap>
#include <QOperatingSystemVersion>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>

#include "subscribed_variable_widget.h"
#include "subscribed_event_widget.h"
#include "command_widget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class KfxApiTool;
}
QT_END_NAMESPACE

class KfxApiTool : public QMainWindow
{
    Q_OBJECT

public:
    KfxApiTool(QWidget *parent = nullptr);
    ~KfxApiTool();

private slots:
    void openAboutDialog();

    void openConnectDialog();
    void handleServerApiSubmitted(const QString &text);

    void openSubscribeVariableDialog();
    void handleSubscribeVariableSubmitted(const QString &player, const QString &text);
    void removeSubscribedVariable(const QString &player, const QString &variable);
    void editSubscribedVariable(const QString &player, const QString &variable, int value);

    void openSubscribeEventDialog();
    void handleSubscribeEventSubmitted(const QString &event);
    void removeSubscribedEvent(const QString &event);

    void toggleStayOnTop(bool checked);
    void toggleReconnect(bool checked);

    void openAddCommandDialog();
    void handleCommandSubmitted(const QString &name, int type, const QString &command);
    void handleCommandExecuted(const QString &name, int type, const QString &command);

    void openEditCommandDialog(CommandWidget *widget, const QString &name, int type, const QString &command);
    void handleCommandUpdated(CommandWidget *widget, const QString &name, int type, const QString &command);

    void openSetVariableDialog();
    void handleSetVarSubmitted(const QString &player, const QString &command, int value);

    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onConnectionTimeout();
    void disconnect();
    void onReconnectTimer();

    void savePreset();
    void loadPreset();

private:
    int currentAckId;
    Ui::KfxApiTool *ui;
    QTcpSocket *tcpSocket;
    QTimer *connectionTimeoutTimer;
    QTimer *connectionReconnectTimer;
    QLabel *statusLabel;
    QVBoxLayout *areaVarSubsScrollLayout;
    QVBoxLayout *areaCommandsScrollLayout;
    bool shouldReconnect;

    QString currentIP;
    int currentPort;

    void setSplitterSizes();

    QString playerToHtml(QString playerName);

    void subscribeToVariable(const QString &player, const QString &variable);
    QList<SubscribedVariableWidget*> subbedVariableWidgetList;
    SubscribedVariableWidget* findSubscribedVariableWidget(const QString &player, const QString &variable);
    void removeSubscribedVariableWidget(SubscribedVariableWidget *widget);

    void subscribeToEvent(const QString &event);
    QList<SubscribedEventWidget*> subbedEventWidgetList;
    SubscribedEventWidget* findSubscribedEventWidget(const QString &event);
    void removeSubscribedEventWidget(SubscribedEventWidget *widget);

    void handleCommandExecutedReturn(const QJsonObject &request, const QJsonObject &response);

    void appendLog(const QString string);
    void updateStatusLabel(const QString string);

    void setConnectedGuiStatus();
    void setDisconnectedGuiStatus();

    void sendData(const QByteArray &data);
    void sendJSON(const QJsonObject &jsonObject);

    std::optional<QJsonObject> getJsonFromPacket(QByteArray data);
    void handleReceivedJson(const QJsonObject &jsonObject);

    using ActionCallbackFunction = std::function<void(const QJsonObject&, const QJsonObject&)>;
    struct ActionCallbackInfo {
        ActionCallbackFunction callback;
        QJsonObject request;
    };
    void addActionCallback(int id, const ActionCallbackFunction& callback, const QJsonObject &request);
    void callActionCallback(int id, const QJsonObject &response);
    void removeActionCallback(int id);
    QMap<int, ActionCallbackInfo> actionCallbackMap;

    void addSubscribedVariableWidget(const QString &player, const QString &variable, int value);
    void addSubscribedEventWidget(const QString &event);
    void addCommandWidget(const QString &name, int type, const QString &command);

    void handleSubscribeToVariableReturn(const QJsonObject &request, const QJsonObject &response);
    void handleSubscribeToVariableReadReturn(const QJsonObject &request, const QJsonObject &response);
    void handleSubscribeToEventReturn(const QJsonObject &request, const QJsonObject &response);
    void handleSetVariableReturn(const QJsonObject &request, const QJsonObject &response);
    void handleUnsubscribeVariableReturn(const QJsonObject &request, const QJsonObject &response);
    void handleUnsubscribeEventReturn(const QJsonObject &request, const QJsonObject &response);

    void savePresetToFile(const QString &filePath);
    void loadPresetFromFile(const QString &filePath);
};
#endif // KFX_API_TOOL_H
