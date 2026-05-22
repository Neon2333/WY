#include "TimeRangeSliderBridge.h"

TimeRangeSliderBridge::TimeRangeSliderBridge(QObject *parent)
    : QObject(parent)
{
}

void TimeRangeSliderBridge::onRangeChanged(qint64 left, qint64 right)
{
    emit rangeChanged(left, right);
}
