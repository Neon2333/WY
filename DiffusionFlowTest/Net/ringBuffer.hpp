#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <QByteArray>
#include <mutex>

//环形缓冲区，处理半包、粘包问题
class RingBuffer
{
public:
    explicit RingBuffer(int capacity) 
    {
        buffer_.resize(capacity);
    }
    void Write(const QByteArray& data)        // 写入原始数据
    {
        std::unique_lock<std::mutex> lock(mtx_);
        for (const char byte& : data) 
        {
            buffer[writePos] = byte;
            writePos = (writePos + 1) % capacity;
            // 如果写指针追上了读指针，说明缓冲区满了，丢弃最老的数据（读指针前移）
            if (writePos == readPos) 
            {
                readPos = (readPos + 1) % capacity;
            }
        }
    }

    // 提取一个RawFrame
    QByteArray ExtractRawFrame()                 
    {
        int avail = (writePos - readPos + capacity) % capacity;
        if (avail < 4) return QByteArray();          // 连最小帧头都不够

        // 1. 寻找帧头 0xAA, 0xBB
        int start = -1;
        int tmp = readPos;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (int i = 0; i < avail - 1; ++i) 
            {
                if (buffer[tmp] == char(0xAA) && buffer[(tmp+1)%capacity] == char(0xBB)) 
                {
                    start = tmp;
                    break;
                }
                tmp = (tmp + 1) % capacity;
            }
        }
        
        if (start == -1) 
        {
            // 没有找到帧头，丢弃所有数据（防止无效数据堆积）
            readPos = writePos;
            return QByteArray();
        }

        // 2. 从 start 开始读取长度字段（第3个字节）
        int lenPos = (start + 2) % capacity;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            quint8 dataLen = static_cast<quint8>(buffer[lenPos]);
        }
        int totalFrameLen = 4 + dataLen;     // 头2 + 长度1 + 数据n + 校验1

        // 3. 检查环形缓冲区中从 start 开始是否有足够字节
        int bytesFromStart = (writePos - start + capacity) % capacity;
        if (bytesFromStart < totalFrameLen) 
        {
            return QByteArray();             // 数据不够，等待更多数据
        }

        // 4. 拷贝整个帧
        QByteArray frame;
        frame.reserve(totalFrameLen);
        int pos = start;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (int i = 0; i < totalFrameLen; ++i) 
            {
                frame.append(buffer[pos]);
                pos = (pos + 1) % capacity;
            }
        }

        // 5. 可选：校验帧（异或校验）
        quint8 xorSum = 0;
        for (int i = 0; i < totalFrameLen - 1; ++i) 
        {
            xorSum ^= static_cast<quint8>(frame[i]);
        }
        if (xorSum != static_cast<quint8>(frame[totalFrameLen-1])) 
        {
            // 校验失败，丢弃该帧，移动读指针跳过帧头继续寻找下一帧
            readPos = (start + 1) % capacity;
            return QByteArray();
        }

        // 6. 校验通过，移动读指针到帧末尾
        readPos = (start + totalFrameLen) % capacity;
        return frame; 
    }

private:
    int readPos_ = 0;                // 读指针位置
    int writePos_ = 0;               // 写指针位置
    int capacity_ = 4096;
    QByteArray buffer_{capacity, 0}; // 底层存储
    std::mutex mtx_;    
};

#endif