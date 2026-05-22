#include "SessionManager.h"
#include "GlobalDefines.h"
// 单例获取
SessionManager& SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

// 构造函数
SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , m_settings(mainConfig, QSettings::IniFormat)   // 新增
{
    loadSettings();   // 构造时自动加载配置文件
}

// 用户信息
void SessionManager::setUser(const QString &name, const QString &level)
{
    if (name != m_userName || level != m_userLevel) {
        m_userName  = name;
        m_userLevel = level;
        emit userChanged(m_userName, m_userLevel);
    }
}

QString SessionManager::userName() const { return m_userName; }
QString SessionManager::userLevel() const { return m_userLevel; }

// 批号
void SessionManager::setBatchNumber(const QString &batch)
{
    if (batch != m_batchNumber) {
        m_batchNumber = batch;
        saveSettings();        // 自动写入配置文件
    }
}

QString SessionManager::batchNumber() const
{
    return m_batchNumber;
}

// 采样间隔
void SessionManager::setInterval(float interval)
{
    if (!qFuzzyCompare(interval, m_interval)) {
        m_interval = interval;
        saveSettings();        // 新增：自动写入配置文件
        emit intervalChanged(interval);
    }
}

float SessionManager::samplingInterval() const
{
    return m_interval;
}

// 配置文件读取
void SessionManager::loadSettings()
{
    m_batchNumber = m_settings.value("BatchNumber", "").toString();
    // 从字符串读取浮点数
    QString intervalStr = m_settings.value("SamplingInterval", "1.0").toString();
    bool ok = false;
    m_interval = intervalStr.toFloat(&ok);
    if (!ok) {
        m_interval = 1.0f;  // 转换失败时使用默认值
    }
    m_lockTime    = m_settings.value("lockTime",15).toInt();
}

// 配置文件写入
void SessionManager::saveSettings()
{
    m_settings.setValue("BatchNumber", m_batchNumber);
    // 将浮点数转换为字符串，避免二进制格式
    m_settings.setValue("SamplingInterval", QString::number(m_interval, 'f', 1));
    m_settings.setValue("lockTime", m_lockTime);
}

// 锁定时间
void SessionManager::setLockTime(int time)
{
    if (time != m_lockTime) {
        m_lockTime = time;
        saveSettings();
        emit lockTimeChanged(time);
    }
}

int SessionManager::lockTime() const
{
    return m_lockTime;
}


