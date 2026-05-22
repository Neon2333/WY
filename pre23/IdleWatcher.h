#pragma once

#include <QObject>
#include <QTimer>

class IdleWatcher : public QObject
{
    Q_OBJECT
public:
    explicit IdleWatcher(int msecs, QObject *parent = nullptr);

    void startWatching();
    void stopWatching();
    void setTimeout(int msecs);

    void resetRemainingTime();

signals:
    void idleTimeout();
    void activity();                     // 用户活动改变计时
    void remainingTimeChanged(int sec);  // 剩余时间

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTimeout();
    void onTick();

private:
    QTimer _timer;       // 超时定时器（singleShot）
    QTimer _tickTimer;   // 1 秒倒计时
    int    _timeoutMs;   // 总超时时间
    int    _remainingSec;

    int remainingSeconds;
    int timeoutMs;
};
