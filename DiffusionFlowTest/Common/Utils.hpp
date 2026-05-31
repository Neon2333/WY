#ifndef UTILS_HPP
#define UTILS_HPP
#include <QDebug>
#include <chrono>
#include <sstream>
#include <iomanip>  //std::put_time
#include <ctime> //std::tm, std::time_t, localtime
#include <QDir>

#define qout qDebug()<<"[Info] (" << GetCurrentTimeFormatted() << ") "
#define qerr qWarning()<<"[Error] (" << GetCurrentTimeFormatted() << ") "
#define qdebug qWarning()<<"[Debug] ("<<__FILE__<<":"<<__LINE__<<") "


// 时间串转换函数
inline QString GetCurrentTimeFormatted()
{
    try
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
    catch(const std::exception& e)
    {
        qerr << e.what();
        return QString("error..");
    }
}



// 检查目录是否存在，存在返回true。不存在则创建目录，并返回false。
inline bool IsDirExists(const QString& dirPath)
{
    QDir dir(dirPath);
    if (dir.exists())
        return true;
    dir.mkpath(".");
    return false;
}

#endif