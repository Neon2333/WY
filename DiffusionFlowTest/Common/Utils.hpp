#ifndef UTILS_HPP
#define UTILS_HPP
#include <QDebug>
#include <chrono>
#include <sstream>
#include <iomanip>  //std::put_time
#include <ctime> //std::tm, std::time_t, localtime

#define qout qDebug()<<"[Info] (" << GetCurrentTimeFormatted() << ") "
#define qerr qWarning()<<"[Error] (" << GetCurrentTimeFormatted() << ") "
#define qdebug qWarning()<<"[Debug] ("<<__FILE__<<":"<<__LINE__<<") "


// 时间串转换函数
static QString GetCurrentTimeFormatted()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_time;
#if defined(_WIN32)
    localtime_s(&tm_time, &now_time);
#else
    localtime_r(&now_time, &tm_time);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
    return QString::fromStdString(oss.str());
}

#endif