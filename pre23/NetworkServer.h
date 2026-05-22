#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

class ConnectionHandler;   // 前向声明（后续你会创建）

class NetworkServer : public QObject
{
    Q_OBJECT
public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    bool startServer(quint16 port);
    void stopServer();

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString &ip, quint16 port);
    void clientDisconnected(const QString &ip, quint16 port);

private slots:
    void onNewConnection();

private:
    QTcpServer *m_server;
};

#endif // NETWORKSERVER_H
