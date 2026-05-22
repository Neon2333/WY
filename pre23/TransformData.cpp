#include "TransformData.h"
#include "GlobalDefines.h"
// 初始化静态成员
PressureUnit TransformData::m_currentUnit = MILLIBAR;
double TransformData::m_conversionFactor = 1.0;
QFileSystemWatcher *TransformData::m_configWatcher = nullptr;
QMutex TransformData::m_configMutex;

TransformData::TransformData(QObject *parent) : QObject(parent)
{
    // 初始化配置
    loadConfiguration();
    initializeConversionFactors();

    // 创建配置监视器（首次创建）
    if (!m_configWatcher) {
        m_configWatcher = new QFileSystemWatcher();
        m_configWatcher->addPath(systemConfig);
        QObject::connect(m_configWatcher, &QFileSystemWatcher::fileChanged,
                         []{ reloadConfiguration(); });
    }
}

double TransformData::convertFromRaw(int16_t rawValue)
{
    // 处理传感器断开特殊值
    if (rawValue == SENSOR_DISCONNECTED_VALUE) {
        return SENSOR_DISCONNECTED_VALUE;
    }

    QMutexLocker locker(&m_configMutex);
    return static_cast<double>(rawValue) * m_conversionFactor;
}

QString TransformData::unitSymbol()
{
    QMutexLocker locker(&m_configMutex);
    switch (m_currentUnit) {
    case MILLIBAR: return "mbar";
    case KILOPASCAL: return "kPa";
    case PSI: return "psi";
    case MMHG: return "mmHg";
    default: return "mbar";
    }
}

PressureUnit TransformData::currentUnit()
{
    QMutexLocker locker(&m_configMutex);
    return m_currentUnit;
}

bool TransformData::isUnitActive(PressureUnit unit)
{
    QMutexLocker locker(&m_configMutex);
    return m_currentUnit == unit;
}

void TransformData::reloadConfiguration()
{
    QMutexLocker locker(&m_configMutex);
    loadConfiguration();
    initializeConversionFactors();
    // 如果文件被删除又恢复，重新添加监视
    if (!m_configWatcher->files().contains(systemConfig)) {
        m_configWatcher->addPath(systemConfig);
    }
}

void TransformData::loadConfiguration()
{
    QSettings settings(systemConfig, QSettings::IniFormat);
    QString unitStr = settings.value("PressureUnit", "mbar").toString().toLower();

    if (unitStr == "kpa") {
        m_currentUnit = KILOPASCAL;
    } else if (unitStr == "psi") {
        m_currentUnit = PSI;
    } else if (unitStr == "mmhg") {
        m_currentUnit = MMHG;
    } else {
        m_currentUnit = MILLIBAR;
    }
}

void TransformData::initializeConversionFactors()
{
    switch (m_currentUnit) {
    case MILLIBAR:
        m_conversionFactor = 1.0;   // 原始单位就是mbar
        break;
    case KILOPASCAL:
        m_conversionFactor = 0.1;   // 1 kPa = 10 mbar
        break;
    case PSI:
        m_conversionFactor = 0.01450377377; // 1 psi ≈ 68.9476 mbar
        break;
    case MMHG:
        m_conversionFactor = 0.750061683;   // 1 mmHg ≈ 1.33322 mbar
        break;
    }
}
