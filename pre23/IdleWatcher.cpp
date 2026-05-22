// IdleWatcher.cpp
//无操作做超时弹窗处理程序
#include "IdleWatcher.h"
#include <QCoreApplication>
#include <QEvent>
#include <QDebug>

IdleWatcher::IdleWatcher(int msecs, QObject *parent)
    : QObject(parent)
    , _timeoutMs(msecs)
{
    // 超时定时器（真正锁定）
    _timer.setInterval(_timeoutMs);
    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout,this, &IdleWatcher::onTimeout);

    // 1 秒 tick 定时器（仅用于显示剩余时间）
    _tickTimer.setInterval(1000);
    connect(&_tickTimer, &QTimer::timeout,this, &IdleWatcher::onTick);

    // 安装事件过滤器
    QCoreApplication::instance()->installEventFilter(this);
}

void IdleWatcher::startWatching()
{
    _timer.stop();
    _tickTimer.stop();

    _timer.setInterval(_timeoutMs);
    _timer.start();

    _remainingSec = _timeoutMs / 1000;
    emit remainingTimeChanged(_remainingSec);  // 立即刷新 UI

    _tickTimer.start();
}

void IdleWatcher::stopWatching()
{
    _timer.stop();
    _tickTimer.stop();
}

void IdleWatcher::setTimeout(int msecs)
{
    _timeoutMs = msecs;
}

bool IdleWatcher::eventFilter(QObject *watched, QEvent *event)
{
    if (_timer.isActive()) {
        switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::KeyPress:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            // 用户有操作 → 重置计时
            _timer.start();

            _remainingSec = _timeoutMs / 1000;
            emit remainingTimeChanged(_remainingSec);

            emit activity();
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

void IdleWatcher::onTick()
{
    if (_remainingSec > 0) {
        _remainingSec--;
        emit remainingTimeChanged(_remainingSec);
    }
}

void IdleWatcher::onTimeout()
{
    // qDebug() << "用户已空闲超过设定时间";

    _tickTimer.stop();
    emit idleTimeout();
    // 不自动 startWatching，保持你原有语义
}


void IdleWatcher::resetRemainingTime()
{
    remainingSeconds = timeoutMs / 1000;
    emit remainingTimeChanged(remainingSeconds);
}
