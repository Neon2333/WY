// SensorData.h
#pragma once

#include <QString>
#include <limits>

struct SensorData {
    QString time;

    // 传感器原始值 (mbar)
    double sensor1 = std::numeric_limits<double>::quiet_NaN();  // 传感器1原始值 (mbar)
    double sensor2 = std::numeric_limits<double>::quiet_NaN();  // 传感器2原始值 (mbar)
    double sensor3 = std::numeric_limits<double>::quiet_NaN();  // 传感器3原始值 (mbar)
    double sensor4 = std::numeric_limits<double>::quiet_NaN();  // 传感器4原始值 (mbar)


    // 校准后值 (mbar)
    double calibrated1 = std::numeric_limits<double>::quiet_NaN();
    double calibrated2 = std::numeric_limits<double>::quiet_NaN();
    double calibrated3 = std::numeric_limits<double>::quiet_NaN();
    double calibrated4 = std::numeric_limits<double>::quiet_NaN();


    // 单位转换后的值
    double display1 = std::numeric_limits<double>::quiet_NaN();
    double display2 = std::numeric_limits<double>::quiet_NaN();
    double display3 = std::numeric_limits<double>::quiet_NaN();
    double display4 = std::numeric_limits<double>::quiet_NaN();


    // 校准偏移量
    double offset1 = 0.0;
    double offset2 = 0.0;
    double offset3 = 0.0;
    double offset4 = 0.0;

};

struct hisSensorData {
    QString time;

    // 传感器原始值 (mbar) - 历史数据
    double hisSensor1 = std::numeric_limits<double>::quiet_NaN();  // 传感器1数据库原始值 (mbar)
    double hisSensor2 = std::numeric_limits<double>::quiet_NaN();  // 传感器2数据库原始值 (mbar)
    double hisSensor3 = std::numeric_limits<double>::quiet_NaN();  // 传感器3数据库原始值 (mbar)
    double hisSensor4 = std::numeric_limits<double>::quiet_NaN();  // 传感器4数据库原始值 (mbar)

};
