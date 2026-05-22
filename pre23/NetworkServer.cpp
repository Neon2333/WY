/*启动和停止服务器: 监听指定端口，接受连接请求；或者关闭监听。
*处理新连接: 当有新的客户端尝试连接时，QTcpServer 会发出信号，NetworkServer 负责接收这个信号。
*委派连接处理: 对于每一个新的客户端连接，它会创建一个独立的 ConnectionHandler 对象来专门处理与该特定客户端的所有后续通信和业务逻辑。这种设计使得主服务器线程不会被单个客户端的长时间操作阻塞。
*事件通知: 通过 Qt 的信号机制，向外（可能是主程序或其他组件）报告服务器状态的变化（如启动、停止）以及客户端的连接和断开事件。
*/
//NetworkServer.cpp
#include "NetworkServer.h"
#include "ConnectionHandler.h"
#include <QHostAddress>
#include <QDebug>

NetworkServer::NetworkServer(QObject *parent)
    : QObject(parent),
    m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer()
{
    stopServer();
}

bool NetworkServer::startServer(quint16 port)
{
    if (m_server->isListening())
        m_server->close();

    bool ok = m_server->listen(QHostAddress::AnyIPv4, port);
    if (ok) {
        emit serverStarted(port);
        qDebug() << "[NetworkServer] Listening on port" << port;
    } else {
        qWarning() << "[NetworkServer] Failed to start:" << m_server->errorString();
    }
    return ok;
}

void NetworkServer::stopServer()
{
    if (m_server->isListening()) {
        m_server->close();
        emit serverStopped();
        qDebug() << "[NetworkServer] Server stopped";
    }
}

void NetworkServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *clientSocket = m_server->nextPendingConnection();
        QString ip = clientSocket->peerAddress().toString();
        quint16 port = clientSocket->peerPort();

        ConnectionHandler *handler = new ConnectionHandler(clientSocket, this);

        connect(handler, &ConnectionHandler::clientDisconnected, this,
                [this, ip, port]() {
                    emit clientDisconnected(ip, port);
                    qDebug() << "[NetworkServer] Client disconnected:" << ip << ":" << port;
                });

        handler->start();  // 直接调用
        emit clientConnected(ip, port);
        qDebug() << "[NetworkServer] New client:" << ip << ":" << port;
    }
}

