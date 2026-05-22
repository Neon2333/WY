#include "ConfigSettings.h"
#include "GlobalDefines.h"
#include <QDebug>
#include <QDir>

ThresholdSettings::ThresholdSettings(QObject *parent)
    : QObject(parent)
    , m_settings(mainConfig, QSettings::IniFormat)
{
    initializeDefaults();
}

ThresholdSettings& ThresholdSettings::instance()
{
    static ThresholdSettings inst;
    return inst;
}

void ThresholdSettings::initializeDefaults()
{
    // 检查并设置通道阈值默认值
    for (int i = 1; i <= 4; ++i) {
        QString upperKey = QString("AlarmUpper%1").arg(i);
        QString lowerKey = QString("AlarmLower%1").arg(i);

        if (!m_settings.contains(upperKey)) {
            m_settings.setValue(upperKey, 4136.0); // 浮点数
        }

        if (!m_settings.contains(lowerKey)) {
            m_settings.setValue(lowerKey, 100.0);  // 浮点数
        }
    }

    // 检查并设置压差阈值默认值
    QStringList pressureDiffNames = {"Δ12", "Δ13", "Δ14", "Δ23", "Δ24", "Δ34"};

    for (int i = 0; i < pressureDiffNames.size(); ++i) {
        int index = i + 1;

        QString upperKey = QString("PressureDiffUpper%1").arg(index);
        QString lowerKey = QString("PressureDiffLower%1").arg(index);

        if (!m_settings.contains(upperKey)) {
            m_settings.setValue(upperKey, 1000.0); // 浮点数
        }

        if (!m_settings.contains(lowerKey)) {
            m_settings.setValue(lowerKey, 10.0);   // 浮点数
        }
    }

    // IP地址和端口默认值
    if (!m_settings.contains("Port")) {
        m_settings.setValue("Port", 6000);
    }

    m_settings.sync();
    qDebug() << "配置文件路径:" << m_settings.fileName();
}

double ThresholdSettings::alarmUpper(int index)
{
    if ((index < 1) || (index > 4)) {
        qWarning() << "无效的通道索引:" << index;
        return 0.0;
    }

    QString key = QString("AlarmUpper%1").arg(index);
    bool ok;
    double value = m_settings.value(key).toDouble(&ok);
    if (!ok) {
        double defaultValue = 4136.0;
        m_settings.setValue(key, defaultValue);
        return defaultValue;
    }
    return value;
}

double ThresholdSettings::alarmLower(int index)
{
    if ((index < 1) || (index > 4)) {
        qWarning() << "无效的通道索引:" << index;
        return 0.0;
    }

    QString key = QString("AlarmLower%1").arg(index);
    bool ok;
    double value = m_settings.value(key).toDouble(&ok);
    if (!ok) {
        double defaultValue = -100.0;
        m_settings.setValue(key, defaultValue);
        return defaultValue;
    }
    return value;
}

void ThresholdSettings::setAlarmUpper(int index, double value)
{
    if ((index < 1) || (index > 4)) {
        qWarning() << "设置无效的通道索引:" << index;
        return;
    }

    m_settings.setValue(QString("AlarmUpper%1").arg(index), value);
    m_settings.sync();
    emit thresholdChanged();
}

void ThresholdSettings::setAlarmLower(int index, double value)
{
    if ((index < 1) || (index > 4)) {
        qWarning() << "设置无效的通道索引:" << index;
        return;
    }

    m_settings.setValue(QString("AlarmLower%1").arg(index), value);
    m_settings.sync();
    emit thresholdChanged();
}

double ThresholdSettings::pressureDiffUpper(int diffIndex)
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "无效的压差索引:" << diffIndex;
        return 0.0;
    }

    QString key = QString("PressureDiffUpper%1").arg(diffIndex);
    bool ok;
    double value = m_settings.value(key).toDouble(&ok);
    if (!ok) {
        double defaultValue = 1000.0;
        m_settings.setValue(key, defaultValue);
        return defaultValue;
    }
    return value;
}

double ThresholdSettings::pressureDiffLower(int diffIndex)
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "无效的压差索引:" << diffIndex;
        return 0.0;
    }

    QString key = QString("PressureDiffLower%1").arg(diffIndex);
    bool ok;
    double value = m_settings.value(key).toDouble(&ok);
    if (!ok) {
        double defaultValue = -100.0;
        m_settings.setValue(key, defaultValue);
        return defaultValue;
    }
    return value;
}

void ThresholdSettings::setPressureDiffUpper(int diffIndex, double value)
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "设置无效的压差索引:" << diffIndex;
        return;
    }

    m_settings.setValue(QString("PressureDiffUpper%1").arg(diffIndex), value);
    m_settings.sync();
    emit thresholdChanged();
}

void ThresholdSettings::setPressureDiffLower(int diffIndex, double value)
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "设置无效的压差索引:" << diffIndex;
        return;
    }

    m_settings.setValue(QString("PressureDiffLower%1").arg(diffIndex), value);
    m_settings.sync();
    emit thresholdChanged();
}

bool ThresholdSettings::isDiffAlarmEnabled(int diffIndex) const
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "无效的压差索引:" << diffIndex;
        return false;
    }

    QString key = QString("PressureDiffEnable%1").arg(diffIndex);
    return m_settings.value(key, false).toBool();
}

void ThresholdSettings::setDiffAlarmEnabled(int diffIndex, bool enabled)
{
    if ((diffIndex < 1) || (diffIndex > 6)) {
        qWarning() << "设置无效的压差索引:" << diffIndex;
        return;
    }

    m_settings.setValue(QString("PressureDiffEnable%1").arg(diffIndex), enabled);
    m_settings.sync();
    emit thresholdChanged();
}

QString ThresholdSettings::getPressureDiffName(int diffIndex)
{
    static QStringList names = {"Δ12", "Δ13", "Δ14", "Δ23", "Δ24", "Δ34"};
    if (diffIndex >= 1 && diffIndex <= names.size()) {
        return names[diffIndex - 1];
    }
    return QString("%1").arg(diffIndex);
}

void ThresholdSettings::saveAll(const QList<double>& uppers, const QList<double>& lowers)
{
    for (int i = 0; i < uppers.size() && i < 4; ++i) {
        m_settings.setValue(QString("AlarmUpper%1").arg(i+1), uppers[i]);
        m_settings.setValue(QString("AlarmLower%1").arg(i+1), lowers[i]);
    }
    m_settings.sync();
    emit thresholdChanged();
}

void ThresholdSettings::saveAllPressureDiffs(const QList<double>& upperDiffs, const QList<double>& lowerDiffs)
{
    for (int i = 0; i < upperDiffs.size() && i < 6; ++i) {
        m_settings.setValue(QString("PressureDiffUpper%1").arg(i+1), upperDiffs[i]);
        m_settings.setValue(QString("PressureDiffLower%1").arg(i+1), lowerDiffs[i]);
    }
    m_settings.sync();
    emit thresholdChanged();
}

QList<double> ThresholdSettings::getAllAlarmUppers()
{
    QList<double> list;
    for (int i = 1; i <= 4; ++i) {
        list << alarmUpper(i);
    }
    return list;
}

QList<double> ThresholdSettings::getAllAlarmLowers()
{
    QList<double> list;
    for (int i = 1; i <= 4; ++i) {
        list << alarmLower(i);
    }
    return list;
}

QList<double> ThresholdSettings::getAllPressureDiffUppers()
{
    QList<double> list;
    for (int i = 1; i <= 6; ++i) {
        list << pressureDiffUpper(i);
    }
    return list;
}

QList<double> ThresholdSettings::getAllPressureDiffLowers()
{
    QList<double> list;
    for (int i = 1; i <= 6; ++i) {
        list << pressureDiffLower(i);
    }
    return list;
}
