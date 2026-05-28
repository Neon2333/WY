#ifndef DATAFRAME_HPP
#define DATAFRAME_HPP

//下位机原始帧格式
class RawFrame
{
public:
    RawFrame();
    ~RawFrame();

    RawFrame(const RawFrame& other);
    RawFrame& operator=(const RawFrame& other);

    RawFrame(const RawFrame&& other);
    RawFrame& operator=(const RawFrame&& other);

    bool operator==(const RawFrame& other) const;
    bool isEmpty() const; //判断是否是空帧
private:
};



#endif