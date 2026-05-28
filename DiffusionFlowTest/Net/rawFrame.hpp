#ifndef DATAFRAME_HPP
#define DATAFRAME_HPP

#include <QByteArray>
#include <optional>

//下位机原始帧格式
struct RawFrame
{
public:
    void Make(const QByteARray& header, const QByteARray& data)
    {
        header_ = header;
        data_ = data;
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
        checksum_ = static_cast<uint8_t>(frame.at(frame.size() - 1)); 
    }

    void SetHeader(const QByteArray& header) { header_ = header; } 
    QByteArray GetHeader() const { return header_; }
    void SetData(const QByteArray& data) { data_ = data; }
    QByteArray GetData() const { return data_; }
    

    //判断帧是否合法（帧头、长度、校验等）
    bool IsValid() const
    {
        if(header_.size() != 3) 
            return false; 
        if(header_.at(0) != char(0xAA) || header_.at(1) != char(0xBB)) 
            return false; 
        int len = ParseLength();
        if(len < 0 || len != data_.size()) 
            return false; 


        uint8_t calcChecksum = 0;
        for(const char byte : header_) 
            calcChecksum += static_cast<uint8_t>(byte);
        for(const char byte : data_) 
            calcChecksum += static_cast<uint8_t>(byte);
        return calcChecksum == checksum_;
    }

    //判断是否是空帧
    std::optional<bool> IsEmpty() const
    {
        if(!IsValid().has_value()) 
            return std::nullopt;
        return header_.isEmpty() && data_.isEmpty() && checksum_ == 0;
    }

    //解析帧长度
    std::optional<int> ParseLength() const 
    { 
        if(header_.size() < 3) 
            return std::nullopt; 
        return static_cast<uint8_t>(header_.at(2)); 
    }

    //解析时间戳(8B)
    std::optional<uint64_t> ParseTimeStamp() const 
    {
        if(data_.size() < 8) 
            return std::nullopt;
        return static_cast<uint64_t>(data_.mid(3, 8));
    }

    //解析ms时间段(4B)
    std::optional<uint32_t> ParseMsTime() const 
    {
        if(data_.size() < 4) 
            return std::nullopt; 
        return static_cast<uint32_t>(data_.mid(11, 4));
    }

    QByteArray header_; //帧头，[0]:0xAA, [1]:0xBB, [2]:len
    QByteArray data_;   //数据段（n 字节）
    uint8_t checksum_;  //校验位，1字节
};



#endif