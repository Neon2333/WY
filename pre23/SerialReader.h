#ifndef SERIALREADER_H
#define SERIALREADER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include "SensorPacket.h"


class SerialReader : public QObject {
    Q_OBJECT

public:
    explicit SerialReader(QObject *parent = nullptr);
    ~SerialReader();

    void start(const QString &portName, int baudRate);


    // 串口写接口
    bool isWritable() const;
    void sendMessage(const QByteArray &data);

    void sendMuteAlarm();

    // 发送错误状态帧 (AA EE 00 00 01)
    void sendDisconnectedState();

    // 发送错误状态帧 (AA EE 00 00 02)
    void sendThresholdAlarm();

    // 发送恢复正常状态帧 (AA EE 00 00 00)
    void sendNormalState();




signals:
    void sensorPacketReceived(const SensorPacket &packet);  // 新信号，发送结构体数据

private slots:
    void handleReadyRead();

private:
    void processBuffer();
    bool validateAndExtractPacket(QByteArray &frame, SensorPacket &packet);
    uint16_t calculateCRC(const uint8_t *data, size_t len);

    QSerialPort *serial;
    QByteArray buffer;

    static constexpr uint8_t SYNC_BYTE1 = 0xAA;
    static constexpr uint8_t SYNC_BYTE2 = 0x55;

};

#endif // SERIALREADER_H
