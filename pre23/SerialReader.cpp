#include "SerialReader.h"
#include <QDebug>
#include <cstring>  // 用于 memcpy 结构体拷贝

SerialReader::SerialReader(QObject *parent)
    : QObject(parent), serial(new QSerialPort(this))
{
    // 当串口有新数据可读时，调用 handleReadyRead()
    connect(serial, &QSerialPort::readyRead, this, &SerialReader::handleReadyRead);
}

SerialReader::~SerialReader() {
    // 程序退出时关闭串口
    if (serial->isOpen())
        serial->close();
}

// 启动串口并设置参数
void SerialReader::start(const QString &portName, int baudRate) {
    if (serial->isOpen())
        serial->close();  // 若已打开则先关闭

    // 设置串口参数
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    // 尝试打开串口
    if (!serial->open(QIODevice::ReadWrite)) {
        qDebug() << "串口打开失败:" << serial->errorString();
    } else {
        qDebug() << "串口打开成功:" << portName;
    }
}

// 串口有新数据时触发
void SerialReader::handleReadyRead() {
    // 追加新读入的数据
    buffer.append(serial->readAll());
    // 处理接收到的数据
    processBuffer();
}

// 数据缓冲处理函数
void SerialReader::processBuffer() {
    constexpr int HEADER_SIZE = 2;  // AA 55
    constexpr int PACKET_SIZE = sizeof(SensorPacket);  // 1 + 4 + 2 = 7
    constexpr int FRAME_SIZE = HEADER_SIZE + PACKET_SIZE;  // 9 字节

    while (buffer.size() >= HEADER_SIZE) {
        // 查找帧头 "AA 55"
        int index = buffer.indexOf(QByteArray::fromHex("aa55"));

        if (index == -1) {
            // 没有找到帧头，清空缓冲
            buffer.clear();
            return;
        }

        // 如果剩余数据不足以构成完整帧（9字节）
        if (buffer.size() < index + FRAME_SIZE) {
            // 保留可能包含帧头的数据，等待后续
            buffer.remove(0, index);
            return;
        }

        // 提取出完整的一帧数据：从帧头后开始，共 7 字节的数据包
        QByteArray frameData = buffer.mid(index + HEADER_SIZE, PACKET_SIZE);

        if (frameData.size() != PACKET_SIZE) {
            // 防御性判断
            buffer.remove(0, index + HEADER_SIZE);
            continue;
        }

        // 解包数据到结构体
        SensorPacket packet;
        if (validateAndExtractPacket(frameData, packet)) {
            emit sensorPacketReceived(packet);  // 发出信号，传递数据
        } else {
            qWarning() << "CRC校验失败，丢弃数据包";

        }

        // 移除已处理的帧（包括帧头 + 数据包）
        buffer.remove(0, index + FRAME_SIZE);
    }
}

// 校验 CRC 并提取有效数据()
bool SerialReader::validateAndExtractPacket(QByteArray &frame, SensorPacket &packet) {
    if (frame.size() != sizeof(SensorPacket))  // 应该是 7
        return false;

    memcpy(&packet, frame.constData(), sizeof(SensorPacket));
    uint16_t crc = calculateCRC(reinterpret_cast<uint8_t*>(&packet), offsetof(SensorPacket, crc));
    return crc == packet.crc;
}

bool SerialReader::isWritable() const {
    return serial && serial->isOpen() && serial->isWritable();
}

void SerialReader::sendMessage(const QByteArray &data) {

    if (serial && serial->isOpen() && serial->isWritable()) {
        serial->write(data);
    } else {
        qWarning() << "串口未准备好，无法发送数据";
    }
}

void SerialReader::sendDisconnectedState()
{
    if (!serial || !serial->isOpen() || !serial->isWritable()) {
        qWarning() << "无法发送错误状态帧：串口未准备好";
        return;
    }

    // 创建错误状态帧：AA EE 00 00 01
    QByteArray errorFrame;
    errorFrame.append(static_cast<char>(0xAA));  // 帧头1
    errorFrame.append(static_cast<char>(0xEE));  // 帧头2
    errorFrame.append(static_cast<char>(0x00));  // 预留字节1
    errorFrame.append(static_cast<char>(0x00));  // 预留字节2
    errorFrame.append(static_cast<char>(0x01));  // 错误状态码

    // 发送帧数据
    serial->write(errorFrame);
    qDebug() << "已发送错误状态帧：" << errorFrame.toHex(' ').toUpper();
}

void SerialReader::sendMuteAlarm()
{
    if (!serial || !serial->isOpen() || !serial->isWritable()) {
        qWarning() << "无法发送静音状态帧：串口未准备好";
        return;
    }
    // AA EE 00 00 02  静音
    QByteArray frame;
    frame.append(static_cast<char>(0xAA));
    frame.append(static_cast<char>(0xEE));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x02));

    serial->write(frame);
    qDebug() << "已发送静音帧：" << frame.toHex(' ').toUpper();
}

void SerialReader::sendThresholdAlarm()
{
    if (!serial || !serial->isOpen() || !serial->isWritable()) {
        qWarning() << "无法发送错误状态帧：串口未准备好";
        return;
    }

    // 创建错误状态帧：AA EE 00 00 01
    QByteArray errorFrame;
    errorFrame.append(static_cast<char>(0xAA));  // 帧头1
    errorFrame.append(static_cast<char>(0xEE));  // 帧头2
    errorFrame.append(static_cast<char>(0x00));  // 预留字节1
    errorFrame.append(static_cast<char>(0x00));  // 预留字节2
    errorFrame.append(static_cast<char>(0x01));  // 错误状态码

    // 发送帧数据
    serial->write(errorFrame);
    qDebug() << "已发送错误状态帧：" << errorFrame.toHex(' ').toUpper();
}


void SerialReader::sendNormalState()
{
    if (!serial || !serial->isOpen() || !serial->isWritable()) {
        qWarning() << "无法发送正常状态帧：串口未准备好";
        return;
    }

    // 创建正常状态帧：AA EE 00 00 00
    QByteArray normalFrame;
    normalFrame.append(static_cast<char>(0xAA));  // 帧头1
    normalFrame.append(static_cast<char>(0xEE));  // 帧头2
    normalFrame.append(static_cast<char>(0x00));  // 预留字节1
    normalFrame.append(static_cast<char>(0x00));  // 预留字节2
    normalFrame.append(static_cast<char>(0x00));  // 正常状态码

    // 发送帧数据
    serial->write(normalFrame);
    qDebug() << "已发送正常状态帧：" << normalFrame.toHex(' ').toUpper();
}


// CRC-CCITT-FALSE 算法 (Poly: 0x1021, 初始值: 0xFFFF)
uint16_t SerialReader::calculateCRC(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
