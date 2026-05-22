// TrailData.h
#ifndef TRAILDATA_H
#define TRAILDATA_H

#include <QString>

struct TrailData
{
    QString trailtime;    // 时间戳
    QString username;     // 用户名
    QString level;        // 权限级别
    QString operation;    // 操作内容
    QString number;       // 批号
    TrailData() = default;
};

#endif // TRAILDATA_H
