#include "serialWorker.hpp"

class Builder
{
public:
    Builder& SetPortName(const QString& portName) { portName_ = portName;  return *this; }
    Builder& SetBaudRate(qint32 baudRate) { baudRate_ = baudRate;  return *this; }
    Builder& SetDataBits(QSerialPort::DataBits dataBits) { dataBits_ = dataBits;  return *this; }
    Builder& SetParity(QSerialPort::Parity parity) { parity_ = parity;  return *this; }
    Builder& SetStopBits(QSerialPort::StopBits stopBits) { stopBits_ = stopBits;  return *this; }
    Builder& SetFlowControl(QSerialPort::FlowControl flowControl) { flowControl_ = flowControl;  return *this; }
    Builder& SetRingBufferSize(int size) { ringBufferSize_ = size; return *this; }

    SerialWorker Build() 
    {
        return SerialWorker(portName_, baudRate_, dataBits_, parity_, stopBits_, flowControl_, ringBufferSize_);
    }
private:
    QString portName_;
    qint32 baudRate_;
    QSerialPort::DataBits dataBits_;
    QSerialPort::Parity parity_;
    QSerialPort::StopBits stopBits_;
    QSerialPort::FlowControl flowControl_;
    int ringBufferSize_; 
};



SerialWorker::SerialWorker(const QString portName, 
                           qint32 baudRate, 
                           QSerialPort::DataBits dataBits, 
                           QSerialPort::Parity parity, 
                           QSerialPort::StopBits stopBits, 
                           QSerialPort::FlowControl flowControl,
                           int ringBufferSize)
    : portName_(portName), baudRate_(baudRate), dataBits_(dataBits), 
    parity_(parity), stopBits_(stopBits), flowControl_(flowControl), ringBuffer_(ringBufferSize)
{
    this->moveToThread(&thread_);  
    //连接串口数据可读信号到槽函数
    connect(&serialPort_, &QSerialPort::readyRead, this, &SerialWorker::onReadyRead);   

    //连接错误信号到槽函数
    connect(&serialPort_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit signal_error(serialPort_.errorString());
        }
    });

    //确保线程结束时删除worker对象
    connect(&thread_, &QThread::finished, this, &QObject::deleteLater); 
    //确保线程结束时删除线程对象
    connect(&thread_, &QThread::finished, &thread_, &QObject::deleteLater);

    // 启动线程
    thread_.start();  
}

SerialWorker::~SerialWorker() 
{
    Disconnect();  // 确保在析构时断开连接
}


bool SerialWorker::Connect()
{
   if(isConnected_) 
       return true;
    if(!serialPort_.open(QIODevice::ReadWrite))
    {
        qerr << "failed to open serial port:" << serialPort_.errorString();
        return false;
    }
    isConnected_ = true;
    return true;
}

bool SerialWorker::Disconnect()
{
    if(!isConnected_)
        return true; 
    if (serialPort_.isOpen()) 
        serialPort_.close(); 
    isConnected_ = false;
    return true; 
}

bool SerialWorker::Send(const QByteArray& data)
{
    if (serialPort_.write(data) == -1) //失败返回-1
    {
        emit signal_error(serialPort_.errorString());
        return false;
    }
    emit signal_sendData(data); 
    return serialPort_.waitForBytesWritten();	//阻塞直到所有数据写入 
}

bool SerialWorker::Read(QByteArray& data)
{
    if(!isConnected_)
    {
        emit signal_error("Serial port not connected");
        return false;       
    }
    QByteArray dataTmp = serialPort_.readAll();
    if(dataTmp.isEmpty())
    {
        return false;
    }
    data.append(dataTmp);
    return true;
}

//循环提取RawFrame
RawFrame SerialWorker::ExtractFrame(const QByteArray& data)
{
    RawFrame rawFrame;
    while (true)
    {
        QByteArray frameData = ringBuffer_.ExtractRawFrame();
        if (frameData.isEmpty())
            break;
        rawFrame.Make(frameData);
        emit signal_frameReady(rawFrame);
    }
    return rawFrame;
}

void SerialWorker::onReadyRead()
{
    QByteArray data;
    Read(data);                 //从串口读取数据
    ringBuffer_.Write(data);    //存入环形缓冲区 
    emit signal_readData(data); 
    ExtractFrame(data); 
}


