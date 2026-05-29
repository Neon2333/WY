#ifndef SERIALWORKER_HPP
#define SERIALWORKER_HPP

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QString>
#include "ringBuffer.hpp"
#include "../DataStructure/rawFrame.hpp"
#include "../Common/utils.hpp"

const int DEFAULT_RING_BUFFER_SIZE = 4096; //默认环形缓冲区大小

//写串口类
class SerialWorker : public QObject 
{
    Q_OBJECT
public:
    class Builder;

    SerialWorker(const QString portName, 
                qint32 baudRate, 
                QSerialPort::DataBits dataBits, 
                QSerialPort::Parity parity, 
                QSerialPort::StopBits stopBits, 
                QSerialPort::FlowControl flowControl, 
                int ringBufferSize);
    ~SerialWorker();    

    bool Connect();     //连接串口
    bool Disconnect();  //断开串口连接
    bool Send(const QByteArray& data);    //写数据到串口
    bool Read(QByteArray& data);          //从串口读取数据
    RawFrame ExtractFrame(const QByteArray& data);    //从原始数据中提取一个RawFrame
    
public slots:
    void onReadyRead();    //串口有数据可读时的槽函数
signals:
    void signal_sendData(const QByteArray& data);       //串口发送了数据的信号
    void signal_readData(const QByteArray& data);       //串口读取到数据的信号
    void signal_error(const QString& errorString);      //错误信号
    void signal_frameReady(const RawFrame& rawFrame);   //1个RawFrame准备好的信号

private:
    QSerialPort serialPort_;    //串口对象
    QThread thread_;            //串口线程

	QString portName_;	//串口名称,windows一般是COM端口
	qint32 baudRate_;	//波特率
	QSerialPort::DataBits dataBits_;    //数据位
	QSerialPort::Parity parity_;	    //奇偶校验
	QSerialPort::StopBits stopBits_;	//停止位
	QSerialPort::FlowControl flowControl_;	//流控制

    bool isConnected_ = false;	//串口连接状态
    int ringBufferSize_ = 4096;
    RingBuffer ringBuffer_{ringBufferSize_};  //环形缓冲区，容量默认大小
};

#endif