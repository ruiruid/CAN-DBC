#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QtWebSockets/QWebSocket>
#include <QDialog>
#include <QPointer>
#include <QTimer>

namespace Ui {
class client;
}

class client : public QWidget
{
    Q_OBJECT

public:
    explicit client(QWidget *parent = nullptr);
    ~client();
    bool isConnected() const;
    void sendTextMessage(const QString& message);
    void sendBinaryMessage(const QByteArray& message);
    void clearAddress();  // 清空地址输入框

signals:
    void connected();
    void disconnected();
    void reconnecting(int current, int max);  // 重连状态信号
    void binaryMessageReceived(const QByteArray& message);
    void logMessage(const QString& message);

public slots:
    void disconnectFromServer();   // 供主窗口调用的断开函数

private slots:
    void onConnectButtonClicked();   // 连接按钮槽函数
    void onConnected();              // WebSocket 连接成功
    void onDisconnected();           // 断开连接
    void onError(QAbstractSocket::SocketError error); // 连接错误
    void onBinaryMessageReceived(const QByteArray& message);
    void onReconnectTimer();         // 重连定时器槽函数


private:
    Ui::client *ui;
    QWebSocket *m_webSocket;         // WebSocket 对象
    QString m_serverUrl;             // 服务器地址（例如 "ws://127.0.0.1:8080"）
    QPointer<QDialog> m_connectingDialog;
    QTimer *m_reconnectTimer;        // 重连定时器
    int m_reconnectCount;            // 重连次数
    bool m_shouldReconnect;          // 是否应该自动重连
    static const int MAX_RECONNECT_ATTEMPTS = 5;  // 最大重连次数
};

#endif // CLIENT_H
