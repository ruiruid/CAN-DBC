#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include <QList>
#include <QByteArray>
#include "can_message.pb.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(quint16 port, QObject *parent = nullptr);
    ~Server();

signals:
    void messageReceived(const QString &message);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onSocketDisconnected();

private:
    bool deserializeProtobuf(const QByteArray &data);

    QWebSocketServer *m_webSocketServer;
    QList<QWebSocket *> m_clients;
};

#endif // SERVER_H
