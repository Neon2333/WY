// HistoryData.h
#ifndef HISTORYDATA_H
#define HISTORYDATA_H

#include <QString>

struct HistoryData
{
    QString historytime;  // 时间戳
    QString SENSOR1;      // 传感器1数据
    QString SENSOR2;      // 传感器2数据
    QString SENSOR3;      // 传感器3数据
    QString SENSOR4;      // 传感器3数据
    QString number;       // 批次号
    QString username;     // 用户名
    QString level;        // 权限级别

    HistoryData() = default;
};

#endif // HISTORYDATA_H
