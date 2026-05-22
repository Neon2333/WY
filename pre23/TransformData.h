#ifndef TRANSFORMDATA_H
#define TRANSFORMDATA_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QMutex>

// 支持的压力量级单位
enum PressureUnit {
    MILLIBAR = 0,   // 毫巴 (mbar)
    KILOPASCAL,     // 千帕 (kPa)
    PSI,            // 磅力每平方英寸 (psi)
    MMHG            // 毫米汞柱 (mmHg)
};

// 传感器特殊值
static constexpr int16_t SENSOR_DISCONNECTED_VALUE = 9999;

class TransformData : public QObject
{
    Q_OBJECT

public:
    explicit TransformData(QObject *parent = nullptr);

    // 从原始数据转换为配置单位（处理特殊值）
    static double convertFromRaw(int16_t rawValue);

    // 获取当前配置单位的符号（"mbar", "kPa", "psi", "mmHg"）
    static QString unitSymbol();

    // 获取当前配置单位
    static PressureUnit currentUnit();

    // 检查特定单位是否激活
    static bool isUnitActive(PressureUnit unit);

    // 重新加载配置
    static void reloadConfiguration();

private:
    // 加载配置
    static void loadConfiguration();

    // 初始化单位换算系数
    static void initializeConversionFactors();

    // 当前配置的单位
    static PressureUnit m_currentUnit;

    // 单位转换系数（从mbar到目标单位）
    static double m_conversionFactor;

    // 配置文件监视器
    static QFileSystemWatcher *m_configWatcher;

    // 配置锁
    static QMutex m_configMutex;
};

#endif // TRANSFORMDATA_H
