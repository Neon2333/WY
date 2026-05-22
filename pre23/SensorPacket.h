// SensorTypes.h
#ifndef SENSORPACKET_H
#define SENSORPACKET_H

#include <cstdint> // 包含 uint8_t, int16_t, uint16_t 等类型

#pragma pack(push, 1) // 保存当前对齐方式，并设置对齐为1字节
struct SensorPacket {
    uint8_t sensor_id;
    float  pressure;
    uint16_t crc;
};
#pragma pack(pop) // 恢复之前的对齐方式


#endif // SENSORPACKET_H
