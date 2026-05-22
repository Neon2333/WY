#pragma once
#include <QObject>

class TimeRangeSliderBridge : public QObject
{
    Q_OBJECT
public:
    explicit TimeRangeSliderBridge(QObject *parent = nullptr);

    Q_INVOKABLE void onRangeChanged(qint64 left, qint64 right);

signals:
    void rangeChanged(qint64 left, qint64 right);
};
