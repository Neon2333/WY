#pragma once
#include <QObject>
#include <QSettings>

class SessionManager : public QObject
{
    Q_OBJECT
public:
    static SessionManager& instance();

    void setUser(const QString& name, const QString& level);
    QString userName() const;
    QString userLevel() const;

    void setBatchNumber(const QString& batch);
    QString batchNumber() const;

    void setInterval(float interval);
    float samplingInterval() const;
    void setLockTime(int time) ;
    int lockTime() const;
signals:
    void userChanged(const QString &userName, const QString &userLevel);
    void intervalChanged(float interval);
    void lockTimeChanged(int time);

private:
    explicit SessionManager(QObject *parent = nullptr);

    // 新增：配置文件读写
    void loadSettings();
    void saveSettings();

private:
    QString m_userName;
    QString m_userLevel;

    QString m_batchNumber;
    float m_interval = 1.0f;
    int m_lockTime;

    QSettings m_settings;     // 新增
};
