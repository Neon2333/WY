#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>

class RemoteUserManager;
class SensorData;
class HistoryData;
class TrailData;
class DatabaseManager;

class ConnectionHandler : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ConnectionHandler();

    void start();

signals:
    void clientDisconnected();
    void finished();

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onPingTimeout();

private:
    // 核心功能
    void processIncomingData();
    bool processPacket(const QByteArray &jsonBytes);
    bool sendJson(const QJsonObject &obj);
    bool authenticate(const QString &user, const QString &pwd);

    // 数据请求处理
    bool handleDataRequest(const QString& cmd, const QJsonObject& obj, const QString& format);

    // 数据转换
    QJsonObject historyDataToJson(const HistoryData& data);
    QByteArray historyDataListToCsv(const QList<HistoryData>& dataList);
    QJsonObject trailDataToJson(const TrailData& data);
    QByteArray trailDataListToCsv(const QList<TrailData>& dataList);

    // 数据发送
    bool sendHistoryDataAsJson(const QList<HistoryData>& dataList, const QString& cmd, const QJsonObject& obj);
    bool sendHistoryDataAsFile(const QList<HistoryData>& dataList, const QString& cmd, const QJsonObject& obj);
    bool sendTrailDataAsJson(const QList<TrailData>& dataList, const QString& cmd, const QJsonObject& obj);
    bool sendTrailDataAsFile(const QList<TrailData>& dataList, const QString& cmd, const QJsonObject& obj);

    // 文件传输
    bool sendFileInfoHeader(qint64 fileSize, const QString& filename, const QString& fileType);
    bool sendFile(const QString& filename, const QByteArray& fileData, const QString& fileType);

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    qint32 m_expectedSize;
    QTimer m_pingTimer;

    // 认证状态
    bool m_isAuthenticated;
    QString m_token;
};

#endif // CONNECTIONHANDLER_H
