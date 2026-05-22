/*
*接收和解析数据: 从关联的 QTcpSocket 中读取客户端发送的数据流。
*处理粘包/拆包: 网络传输中，数据可能被合并发送（粘包）或分割发送（拆包）。该类通过自定义协议（消息长度前缀）解决了这个问题，确保能准确地提取出一个个完整的 JSON 消息包。
*JSON 消息处理: 解析接收到的 JSON 数据包，并根据其中的 "cmd" 字段执行不同的操作。
*用户身份验证: 处理 "AUTH" 命令，调用 RemoteUserManager 进行登录验证，并管理该连接的认证状态 (m_isAuthenticated) 和 Token (m_token)。
*响应发送: 将服务器的响应（也是 JSON 格式）按照相同的带长度前缀的格式打包并通过 QTcpSocket 发送给客户端。
*连接状态监控: 监听 QTcpSocket 的 disconnected 和 errorOccurred 信号，以便在网络连接异常断开时做出反应。
*心跳/保活机制: 使用 QTimer (m_pingTimer) 实现简单的超时检测。如果在设定时间内（如 30 秒）没有收到来自客户端的任何数据，则认为连接已失效并主动断开。
*生命周期管理: 当连接断开或发生错误时，通过发射 finished() 信号通知其管理者（通常是 NetworkServer）它可以被安全地销毁。
*/

/*文件传输的消息格式：
请求: {"cmd": "GET_HISDATA_TIME", "start": "t1", "end": "t2", "format": "file"}
响应: 先发送文件信息头，然后发送文件数据
*/

//ConnectionHandler.cpp
#include "ConnectionHandler.h"
#include "RemoteUserManager.h"
#include "SensorData.h"
#include "SensorDataProvider.h"
#include "DatabaseManager.h"
#include "HistoryData.h"
#include "TrailData.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QDebug>
#include <QHostAddress>
#include <QBuffer>
#include <QDateTime>

ConnectionHandler::ConnectionHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent),
    m_socket(socket),
    m_expectedSize(-1),
    m_isAuthenticated(false)
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &ConnectionHandler::onReadyRead);

    connect(m_socket, &QTcpSocket::disconnected,
            this, &ConnectionHandler::onDisconnected);

    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this,
            &ConnectionHandler::onSocketError);

    // ping 超时（30 秒无数据表示掉线）
    m_pingTimer.setInterval(30000);
    m_pingTimer.setSingleShot(true);
    connect(&m_pingTimer, &QTimer::timeout,
            this, &ConnectionHandler::onPingTimeout);
}

ConnectionHandler::~ConnectionHandler()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
}

void ConnectionHandler::start()
{
    qDebug() << "[ConnectionHandler] Started for client:"
             << m_socket->peerAddress().toString();

    m_pingTimer.start();
}


// 1. 读取数据（解决黏包/拆包）
void ConnectionHandler::onReadyRead()
{
    m_pingTimer.start();  // 收到数据，刷新保活计时

    m_buffer.append(m_socket->readAll());
    processIncomingData();
}

void ConnectionHandler::processIncomingData()
{
    while (true) {
        // 1. 未读到长度
        if (m_expectedSize < 0) {
            if (m_buffer.size() < 4)
                return;
            // --- 安全方式读取长度 ---
            QBuffer buf(&m_buffer);
            buf.open(QIODevice::ReadOnly);

            QDataStream stream(&buf);
            stream.setByteOrder(QDataStream::BigEndian);

            quint32 size;
            stream >> size;
            m_expectedSize = size;

            // 删除头部
            m_buffer.remove(0, 4);
        }
        // 2. 数据长度不够
        if (m_buffer.size() < m_expectedSize)
            return;
        // 3. 拆包
        QByteArray jsonBytes = m_buffer.left(m_expectedSize);
        m_buffer.remove(0, m_expectedSize);
        m_expectedSize = -1;
        // 4. 处理包
        if (!processPacket(jsonBytes)) {
            qWarning() << "[ConnectionHandler] Failed to process packet";
        }
    }
}

// 2. 处理 JSON 包
bool ConnectionHandler::processPacket(const QByteArray &jsonBytes)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[ConnectionHandler] JSON parse error:" << err.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "JSON is not object:" << jsonBytes;
        return false;
    }

    QJsonObject obj = doc.object();
    QString cmd = obj.value("cmd").toString();
    QString format = obj.value("format").toString("json");

    // command: AUTH

    if (cmd == "AUTH") {
        QString user = obj.value("user").toString();
        QString pwd  = obj.value("pwd").toString();

        bool ok = authenticate(user, pwd);

        QJsonObject reply;
        reply["cmd"] = "AUTH_REPLY";
        reply["ok"]  = ok;

        if (ok)
            reply["token"] = m_token;

        return sendJson(reply);
    }

    // 未登录禁止执行任何其他命令
    if (!m_isAuthenticated) {
        QJsonObject reply;
        reply["cmd"] = "ERROR";
        reply["msg"] = "Not authenticated.";
        return sendJson(reply);
    }

    // 实时数据命令（使用SensorData）
    if (cmd == "GET_REALTIME_DATA") {
        const SensorData &d = SensorDataProvider::instance()->getLastData();
        QJsonObject js;
        js["cmd"] = "GET_REALTIME_DATA";
        js["time"] = d.time;
        js["display1"] = d.display1;
        js["display2"] = d.display2;
        js["display3"] = d.display3;
        js["display4"] = d.display4;
        return sendJson(js);
    }

    // 历史数据和审计数据命令
    if (cmd == "GET_HISDATA_BATCH" || cmd == "GET_HISDATA_TIME" ||
        cmd == "GET_TRAILDATA_BATCH" || cmd == "GET_TRAILDATA_TIME") {

        return handleDataRequest(cmd, obj, format);
    }

    QJsonObject reply;
    reply["cmd"] = "ERROR";
    reply["msg"] = QString("Unknown command: %1").arg(cmd);
    return sendJson(reply);
}

// 3. JSON 带长度封包发送
bool ConnectionHandler::sendJson(const QJsonObject &obj)
{
    if (!m_socket) return false;

    m_pingTimer.start();

    QJsonDocument doc(obj);
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    QByteArray pack;
    QDataStream stream(&pack, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << (quint32)body.size();
    pack.append(body);

    m_socket->write(pack);
    return true;
}

//4. 认证
bool ConnectionHandler::authenticate(const QString &user, const QString &pwd)
{
    if (user.isEmpty() || pwd.isEmpty()) {
        qWarning() << "[AUTH] Empty username or password";
        return false;
    }

    RemoteUserManager *mgr = RemoteUserManager::instance();
    QString token = mgr->login(user, pwd);

    if (token.isEmpty())
        return false;

    m_isAuthenticated = true;
    m_token = token;
    return true;
}

// 5. 连接断开、异常处理
void ConnectionHandler::onDisconnected()
{
    emit clientDisconnected();
    emit finished();
}

void ConnectionHandler::onSocketError(QAbstractSocket::SocketError)
{
    emit clientDisconnected();
    emit finished();
}

void ConnectionHandler::onPingTimeout()
{
    qWarning() << "[ConnectionHandler] Ping timeout, disconnecting...";
    m_socket->disconnectFromHost();
}

bool ConnectionHandler::handleDataRequest(const QString& cmd, const QJsonObject& obj, const QString& format)
{
    DatabaseManager &dbMgr = DatabaseManager::instance();

    if (cmd.startsWith("GET_HISDATA")) {
        // 处理历史数据
        if (!dbMgr.openDatabase(DatabaseManager::HistoryDatabase)) {
            qWarning() << "[" << cmd << "] 历史数据库未打开";
            return sendJson(QJsonObject{{"cmd", "ERROR"}, {"msg", "History database not open"}});
        }

        QList<HistoryData> dataList;
        if (cmd.endsWith("_BATCH")) {
            // 修改：按批号查询，而不是按数量
            QString batchNumber = obj.value("number").toString();
            dataList = dbMgr.queryHistoryByBatch(batchNumber);
        } else {
            QString t1 = obj.value("start").toString();
            QString t2 = obj.value("end").toString();
            dataList = dbMgr.queryHistoryByTime(t1, t2);
        }

        return format == "file" ?
                   sendHistoryDataAsFile(dataList, cmd, obj) :
                   sendHistoryDataAsJson(dataList, cmd, obj);

    } else {
        // 处理审计追踪数据
        if (!dbMgr.openDatabase(DatabaseManager::TrailDatabase)) {
            qWarning() << "[" << cmd << "] 审计追踪数据库未打开";
            return sendJson(QJsonObject{{"cmd", "ERROR"}, {"msg", "Trail database not open"}});
        }

        QList<TrailData> dataList;
        if (cmd.endsWith("_BATCH")) {
            // 修改：按批号查询
            QString batchNumber = obj.value("number").toString();
            dataList = dbMgr.queryTrailByBatch(batchNumber);
        } else {
            QString t1 = obj.value("start").toString();
            QString t2 = obj.value("end").toString();
            dataList = dbMgr.queryTrailByTime(t1, t2);
        }

        return format == "file" ?
                   sendTrailDataAsFile(dataList, cmd, obj) :
                   sendTrailDataAsJson(dataList, cmd, obj);
    }
}

// 在 ConnectionHandler.cpp 中添加
QJsonObject ConnectionHandler::historyDataToJson(const HistoryData& data)
{
    QJsonObject obj;
    obj["historytime"] = data.historytime;
    obj["SENSOR1"] = data.SENSOR1;
    obj["SENSOR2"] = data.SENSOR2;
    obj["SENSOR3"] = data.SENSOR3;
    obj["SENSOR4"] = data.SENSOR4;
    obj["number"] = data.number;
    obj["username"] = data.username;
    obj["level"] = data.level;
    return obj;
}

QByteArray ConnectionHandler::historyDataListToCsv(const QList<HistoryData>& dataList)
{
    // 添加UTF-8 BOM
    QByteArray csvData;
    csvData.append(0xEF);
    csvData.append(0xBB);
    csvData.append(0xBF);
    // CSV头部
    csvData.append("TIME,SENSOR1,SENSOR2,SENSOR3,SENSOR4,BATCHNUM,USERNAME,PERMISSION\n");
    for (const auto& data : dataList) {
        QString line = QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
        .arg(data.historytime)
            .arg(data.SENSOR1)
            .arg(data.SENSOR2)
            .arg(data.SENSOR3)
            .arg(data.SENSOR4)
            .arg(data.number)
            .arg(data.username)
            .arg(data.level);

        csvData.append(line.toUtf8());
    }
    return csvData;
}

QJsonObject ConnectionHandler::trailDataToJson(const TrailData& data)
{
    QJsonObject obj;
    obj["trailtime"] = data.trailtime;
    obj["username"] = data.username;
    obj["level"] = data.level;
    obj["operation"] = data.operation;
    return obj;
}

QByteArray ConnectionHandler::trailDataListToCsv(const QList<TrailData>& dataList)
{
    // 添加UTF-8 BOM
    QByteArray csvData;
    csvData.append(0xEF);
    csvData.append(0xBB);
    csvData.append(0xBF);
    // CSV头部
    csvData.append("TIME,USERNAME,PERMISSION,OPERAIONS\n");

    for (const auto& data : dataList) {
        QString line = QString("%1,%2,%3,%4\n")
        .arg(data.trailtime)
            .arg(data.username)
            .arg(data.level)
            .arg(data.operation);

        csvData.append(line.toUtf8());
    }
    return csvData;
}

bool ConnectionHandler::sendHistoryDataAsJson(const QList<HistoryData>& dataList, const QString& cmd, const QJsonObject& obj)
{
    QJsonArray arr;
    for (auto &d : dataList) {
        arr.append(historyDataToJson(d));
    }

    QJsonObject js;
    js["cmd"] = cmd;
    if (cmd.endsWith("_BATCH")) {
        // 修改：返回批号
        js["number"] = obj.value("number").toString();
    } else {
        js["start"] = obj.value("start").toString();
        js["end"] = obj.value("end").toString();
    }
    js["data"] = arr;
    js["count"] = dataList.size();

    return sendJson(js);
}

bool ConnectionHandler::sendHistoryDataAsFile(const QList<HistoryData>& dataList, const QString& cmd, const QJsonObject& obj)
{
    QString filename = QString("%1_%2.csv").arg(cmd).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QByteArray fileData = historyDataListToCsv(dataList);

    return sendFile(filename, fileData, "csv");
}

bool ConnectionHandler::sendTrailDataAsJson(const QList<TrailData>& dataList, const QString& cmd, const QJsonObject& obj)
{
    QJsonArray arr;
    for (auto &d : dataList) {
        arr.append(trailDataToJson(d));
    }

    QJsonObject js;
    js["cmd"] = cmd;
    if (cmd.endsWith("_BATCH")) {
        // 修改：返回批号
        js["number"] = obj.value("number").toString();
    } else {
        js["start"] = obj.value("start").toString();
        js["end"] = obj.value("end").toString();
    }
    js["data"] = arr;
    js["count"] = dataList.size();

    return sendJson(js);
}


bool ConnectionHandler::sendTrailDataAsFile(const QList<TrailData>& dataList, const QString& cmd, const QJsonObject& obj)
{
    QString filename = QString("%1_%2.csv").arg(cmd).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QByteArray fileData = trailDataListToCsv(dataList);

    return sendFile(filename, fileData, "csv");
}


// 发送文件信息头
bool ConnectionHandler::sendFileInfoHeader(qint64 fileSize, const QString& filename, const QString& fileType)
{
    QJsonObject fileInfo;
    fileInfo["cmd"] = "FILE_INFO";
    fileInfo["filename"] = filename;
    fileInfo["filesize"] = QString::number(fileSize);
    fileInfo["filetype"] = fileType;
    fileInfo["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    return sendJson(fileInfo);
}

// 发送文件数据
bool ConnectionHandler::sendFile(const QString& filename, const QByteArray& fileData, const QString& fileType)
{
    if (!m_socket || !m_socket->isOpen()) {
        qWarning() << "[sendFile] Socket is not available";
        return false;
    }

    m_pingTimer.start();

    // 1. 先发送文件信息头
    if (!sendFileInfoHeader(fileData.size(), filename, fileType)) {
        qWarning() << "[sendFile] Failed to send file info header";
        return false;
    }

    // 2. 发送文件数据包
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // 文件数据包格式：类型标识(1字节) + 数据长度(4字节) + 数据内容
    stream << (quint8)0x02; // 文件数据类型标识
    stream << (quint32)fileData.size();
    packet.append(fileData);

    // 3. 计算整个包的长度（不包括长度字段本身）
    QByteArray finalPacket;
    QDataStream finalStream(&finalPacket, QIODevice::WriteOnly);
    finalStream.setByteOrder(QDataStream::BigEndian);
    finalStream << (quint32)packet.size(); // 4字节长度前缀
    finalPacket.append(packet);

    qint64 bytesWritten = m_socket->write(finalPacket);
    if (bytesWritten == -1) {
        qWarning() << "[sendFile] Failed to write file data to socket";
        return false;
    }

    m_socket->flush(); // 确保数据立即发送

    qDebug() << "[sendFile] File sent successfully:" << filename
             << "Size:" << fileData.size() << "bytes";
    return true;
}
