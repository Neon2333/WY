#ifndef UTILS_HPP
#define UTILS_HPP
#include <QDebug>

#define qout qDebug()<<"[Info]("<<__FILE__<<":"<<__LINE__<<")"
#define qwarning qWarning()<<"[Warning]("<<__FILE__<<":"<<__LINE__<<")"

#endif