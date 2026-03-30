#include "client.h"
#include "ui_client.h"
#include <QUrl>
#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>

client::client(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::client),
    m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
    m_connectingDialog(nullptr),
    m_reconnectTimer(new QTimer(this)),
    m_reconnectCount(0),
    m_shouldReconnect(false)
{
    ui->setupUi(this);
    ui->connectButton->setEnabled(true);  // 初始连接按钮可用
    ui->serverUrlEdit->clear();           // 清空地址输入框
    ui->statusLabel->hide();              // 隐藏状态标签（状态在主窗口显示）

    connect(ui->connectButton, &QPushButton::clicked, this, &client::onConnectButtonClicked);
    connect(m_webSocket, &QWebSocket::connected, this, &client::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &client::onDisconnected);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &client::onError);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &client::onBinaryMessageReceived);
    connect(m_reconnectTimer, &QTimer::timeout, this, &client::onReconnectTimer);
}

client::~client()
{
    m_reconnectTimer->stop();
    delete m_webSocket;
    delete ui;
}

bool client::isConnected() const
{
    return (m_webSocket->state() == QAbstractSocket::ConnectedState);
}

void client::clearAddress()
{
    ui->serverUrlEdit->clear();
}

void client::onConnectButtonClicked()
{
    if (isConnected() || m_connectingDialog) {
        return;
    }

    QString url = ui->serverUrlEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入 WebSocket 服务器地址");
        return;
    }

    if (!url.startsWith("ws://") && !url.startsWith("wss://")) {
        url = "ws://" + url;
    }

    QUrl serverUrl(url);
    if (!serverUrl.isValid()) {
        QMessageBox::warning(this, "错误", "无效的 URL 格式\n正确格式: ws://IP:端口");
        return;
    }

    m_serverUrl = url;
    m_shouldReconnect = true;  // 用户主动连接，启用自动重连
    m_reconnectCount = 0;      // 重置重连计数

    m_connectingDialog = new QDialog(this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    m_connectingDialog->setWindowTitle("连接中");
    m_connectingDialog->setModal(true);
    QVBoxLayout *layout = new QVBoxLayout(m_connectingDialog);
    layout->addWidget(new QLabel("正在连接 WebSocket 服务器，请稍候..."));
    m_connectingDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_connectingDialog->show();

    ui->connectButton->setEnabled(false);
    ui->serverUrlEdit->setEnabled(false);

    m_webSocket->open(QUrl(m_serverUrl));
}

void client::disconnectFromServer()
{
    m_shouldReconnect = false;  // 用户主动断开，停止自动重连
    m_reconnectTimer->stop();
    m_reconnectCount = 0;
    
    if (isConnected()) {
        m_webSocket->close();
    }
}

void client::onConnected()
{
    qDebug() << "WebSocket 连接成功";
    
    m_reconnectTimer->stop();  // 停止重连定时器
    m_reconnectCount = 0;      // 重置重连计数

    if (m_connectingDialog) {
        m_connectingDialog->accept();
        m_connectingDialog = nullptr;
    }

    ui->connectButton->setEnabled(false);
    ui->serverUrlEdit->setEnabled(false);

    emit connected();

    this->close();
}

void client::onDisconnected()
{
    qDebug() << "WebSocket 已断开";

    if (m_connectingDialog) {
        m_connectingDialog->reject();
        m_connectingDialog = nullptr;
    }

    if (m_shouldReconnect && m_reconnectCount < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectCount++;
        qDebug() << "尝试重连 (" << m_reconnectCount << "/" << MAX_RECONNECT_ATTEMPTS << ")";
        
        emit reconnecting(m_reconnectCount, MAX_RECONNECT_ATTEMPTS);
        emit disconnected();  // 先发送重连信号，再发送断开信号
        m_reconnectTimer->start(3000);  // 3秒后重连
    } else {
        ui->connectButton->setEnabled(true);
        ui->serverUrlEdit->setEnabled(true);
        
        if (m_reconnectCount >= MAX_RECONNECT_ATTEMPTS) {
            QMessageBox::warning(this, "重连失败", QString("已尝试重连 %1 次均失败，请检查服务器状态").arg(MAX_RECONNECT_ATTEMPTS));
        }
        
        m_shouldReconnect = false;
        m_reconnectCount = 0;
        emit disconnected();
    }
}

void client::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errorMsg = m_webSocket->errorString();
    qDebug() << "WebSocket 错误:" << errorMsg;

    if (m_connectingDialog) {
        m_connectingDialog->reject();
        m_connectingDialog = nullptr;
    }

    QMessageBox::warning(this, "连接失败", "WebSocket 连接失败:\n" + errorMsg);

    ui->connectButton->setEnabled(true);
    ui->serverUrlEdit->setEnabled(true);
}

void client::sendTextMessage(const QString& message)
{
    if (m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->sendTextMessage(message);
        qDebug() << "发送文本消息:" << message;
    } else {
        qDebug() << "WebSocket 未连接，无法发送消息";
    }
}

void client::sendBinaryMessage(const QByteArray& message)
{
    if (m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->sendBinaryMessage(message);
        qDebug() << "发送二进制消息，大小:" << message.size();
    } else {
        qDebug() << "WebSocket 未连接，无法发送消息";
    }
}

void client::onBinaryMessageReceived(const QByteArray& message)
{
    qDebug() << "接收到二进制消息，大小:" << message.size();
    emit binaryMessageReceived(message);
}

void client::onReconnectTimer()
{
    qDebug() << "执行自动重连...";
    m_reconnectTimer->stop();
    
    if (!m_serverUrl.isEmpty() && m_shouldReconnect) {
        m_webSocket->open(QUrl(m_serverUrl));
    }
}
