#ifndef DATAFRAME_HPP
#define DATAFRAME_HPP

#include <QByteArray>
#include <optional>
#include <cstring>  //memcpy

//下位机原始帧格式
struct RawFrame
{
public:
    void Make(const QByteArray& header, const QByteArray& data, const uint8_t checksum)
    {
        header_ = header;
        data_ = data;
        checksum_ = checksum;
    }

    void Make(const QByteArray& frame) 
    {
        if(frame.size() < 4) 
        {
            header_.clear();
            data_.clear();
            checksum_ = 0;
            return; 
        }
        header_ = frame.left(3); 
        data_ = frame.mid(3, frame.size() - 4); 
        memcpy(&checksum_, frame.constData() + frame.size() - 1, 1);
    }

    QByteArray GetHeader() const { return header_; }
    QByteArray GetData() const { return data_; }
    

    //判断帧是否合法（帧头、长度、校验等）
    bool IsValid() const
    {
        if(header_.size() != 3) 
            return false; 
        if(header_.at(0) != char(0xAA) || header_.at(1) != char(0xBB)) 
            return false; 
        auto len = ParseLength();
        if(!len.has_value() || len.value() != data_.size()) 
            return false; 


        uint8_t calcChecksum = 0;
        for(const char byte : header_) 
            calcChecksum += static_cast<uint8_t>(byte);
        for(const char byte : data_) 
            calcChecksum += static_cast<uint8_t>(byte);
        return calcChecksum == checksum_;
    }

    // //判断是否是空帧
    // std::optional<bool> IsEmpty() const
    // {
    //     if(!IsValid())
    //         return std::nullopt;
    //     return data_.isEmpty();
    // }

    //解析帧长度
    std::optional<uint8_t> ParseLength() const 
    { 
        if(header_.size() < 3) 
            return std::nullopt; 
        const char* pHeader = header_.constData();
        uint8_t len = 0;
        memcpy(&len, pHeader + 2, 1);
        return len;
    }

    //解析时间戳(8B)
    std::optional<uint64_t> ParseTimeStamp() const 
    {
        if(data_.size() < 8) 
            return std::nullopt;
        const char* pData = data_.constData();
        uint64_t timestamp = 0;
        memcpy(&timestamp, pData, 8);
        return timestamp;
    }

    //解析ms时间段(4B)
    std::optional<uint32_t> ParseMsTime() const 
    {
        if(data_.size() < 4) 
            return std::nullopt; 
        const char* pData = data_.constData();
        uint32_t msTime = 0;
        memcpy(&msTime, pData + 8, 4);
        return msTime;
    }

    //解析P1压力值(4B)，单位mbar
    std::optional<uint32_t> ParseP1() const 
    {
        if(data_.size() < 4) 
            return std::nullopt; 
        const char* pData = data_.constData();
        uint32_t p1 = 0;
        memcpy(&p1, pData + 12, 4);
        return p1;
    }

    
    //解析P2压力值(4B)，单位mbar
    std::optional<uint32_t> ParseP2() const 
    {
        if(data_.size() < 4) 
            return std::nullopt; 
        const char* pData = data_.constData();
        uint32_t p2 = 0;
        memcpy(&p2, pData + 16, 4);
        return p2;
    }

    QByteArray header_; //帧头，[0]:0xAA, [1]:0xBB, [2]:len
    QByteArray data_;   //数据段（n 字节）
    uint8_t checksum_;  //校验位，1字节
};



#endif