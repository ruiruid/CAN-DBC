#include "server.h"
#include <QDebug>
#include <QDateTime>

Server::Server(quint16 port, QObject *parent)
    : QObject(parent),
      m_webSocketServer(new QWebSocketServer(QStringLiteral("WebSocket Server"),
                                             QWebSocketServer::NonSecureMode,
                                             this))
{
    if (m_webSocketServer->listen(QHostAddress::Any, port)) {
        QString msg = QString("WebSocket server listening on port %1").arg(port);
        qDebug() << msg;
        emit messageReceived(msg);
        connect(m_webSocketServer, &QWebSocketServer::newConnection,
                this, &Server::onNewConnection);
    } else {
        QString msg = QString("Failed to start server on port %1").arg(port);
        qDebug() << msg;
        emit messageReceived(msg);
    }
}

Server::~Server()
{
    m_webSocketServer->close();
    qDeleteAll(m_clients);
}

void Server::onNewConnection()
{
    QWebSocket *client = m_webSocketServer->nextPendingConnection();
    if (!client) return;

    m_clients.append(client);

    connect(client, &QWebSocket::textMessageReceived,
            this, &Server::onTextMessageReceived);
    connect(client, &QWebSocket::binaryMessageReceived,
            this, &Server::onBinaryMessageReceived);
    connect(client, &QWebSocket::disconnected,
            this, &Server::onSocketDisconnected);

    QString msg = QString("[%1] New client connected, total clients: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(m_clients.size());
    qDebug() << msg;
    emit messageReceived(msg);
}

void Server::onTextMessageReceived(const QString &message)
{
    QWebSocket *senderSocket = qobject_cast<QWebSocket *>(sender());
    if (!senderSocket) return;

    QString msg = QString("[%1] Received text: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(message);
    qDebug() << msg;
    emit messageReceived(msg);

    senderSocket->sendTextMessage("Server echo: " + message);
}

void Server::onBinaryMessageReceived(const QByteArray &message)
{
    QWebSocket *senderSocket = qobject_cast<QWebSocket *>(sender());
    if (!senderSocket) return;

    QString msg = QString("[%1] Received binary message, size: %2 bytes")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(message.size());
    qDebug() << msg;
    emit messageReceived(msg);

    bool success = deserializeProtobuf(message);

    if (success) {
        senderSocket->sendTextMessage("Protobuf message received and parsed successfully");
    } else {
        senderSocket->sendTextMessage("Failed to parse Protobuf message");
    }
}

bool Server::deserializeProtobuf(const QByteArray &data)
{
    canproto::CanFrame frame;
    
    if (!frame.ParseFromArray(data.constData(), data.size())) {
        QString msg = "Failed to parse Protobuf message!";
        qDebug() << msg;
        emit messageReceived(msg);
        return false;
    }

    QString msg = QString("=== CanFrame Parsed ===\n"
                         "Message ID: 0x%1 (%2)\n"
                         "Message Name: %3\n"
                         "Signals Count: %4\n")
        .arg(frame.message_id(), 0, 16)
        .arg(frame.message_id())
        .arg(QString::fromStdString(frame.message_name()))
        .arg(frame.signal_list_size());

    for (int i = 0; i < frame.signal_list_size(); ++i) {
        const canproto::Signal& sig = frame.signal_list(i);
        msg += QString("\n  Signal %1: %2\n"
                      "    Physical Value: %3 %4\n"
                      "    Raw Value: %5")
            .arg(i + 1)
            .arg(QString::fromStdString(sig.name()))
            .arg(sig.physical_value())
            .arg(QString::fromStdString(sig.unit()))
            .arg(sig.raw_value());
    }
    msg += "\n==================================";

    qDebug() << msg;
    emit messageReceived(msg);
    
    return true;
}

void Server::onSocketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client) return;

    m_clients.removeAll(client);
    client->deleteLater();

    QString msg = QString("[%1] Client disconnected, remaining clients: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(m_clients.size());
    qDebug() << msg;
    emit messageReceived(msg);
}
