#ifndef SENSORDATAPROVIDER_H
#define SENSORDATAPROVIDER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDateTime>
#include "SerialReader.h" // 包含传感器数据包定义
#include "SensorData.h"
#include "SensorPacket.h"

class SensorDataProvider : public QObject
{
    Q_OBJECT
public:
    // 单例模式获取实例
    static SensorDataProvider* instance();

    // 数据接口
    void connectToSerialReader(QObject *reader);

    const SensorData& getLastData() const { return data; } // 直接返回数据引用

    // 单位管理接口
    QString pressureUnit() const;
    void setPressureUnit(const QString &unit);
    bool isValidUnit(const QString &unit) const;


    // ==== 校零功能 ====
    void calibrateSensor(int sensorId);
    double getCalibratedValue(int sensorId, double rawValue) const;
    void resetCalibrations();

    // 新增接口：直接获取校准并转换后的值
    double getCalibratedValueInCurrentUnit(int sensorId, double rawValue) const;

    // 基础单位转换函数
    static double convertPressure(double mbarValue, const QString &targetUnit);

private:
    // 私有构造函数（单例模式）
    explicit SensorDataProvider(QObject *parent = nullptr);

    void updateAllDisplayValues();

    QHash<int, double> tempCalibratedValues;            // 临时存储校准后的值 (mbar)
    QHash<int, double> tempCalibratedInCurrentUnit;     // 临时存储校准后并转换单位的数值

    // 应用校零校准
    double applyCalibration(int sensorId, double rawValue) const;
    QMap<int, double> tempValues;
    QDateTime lastTimestamp;
    int receivedCount = 0;




private slots:
    void onPressureUnitChanged(const QString &newUnit);
    void onRawPacketReceived(const SensorPacket &packet);



signals:
    // 当压力单位发生变更时发出
    void pressureUnitChanged(const QString &newUnit);

    // 当传感器数据转换完成并准备使用时发出
    void convertedSensorDataReady(const SensorData &data);

    // void sensorDisconnected(int sensorId, QDateTime disconnectTime);

private:
    QSettings* m_settings;    // 配置管理器
    QString m_currentUnit;    // 当前单位 ("mbar", "kpa" 等)
    QString m_lastUnit;       // 上一次使用的单位
    SensorData data;        // 唯一数据存储      // 最新保存的传感器数据

    QMap<int, double> m_calibrationOffsets;// 校零偏移量（单位：mbar）
    QMap<int, QDateTime> m_connectTime; // 每个传感器的插入时间
    QMap<int, bool> m_isStable;         // 每个传感器是否已稳定
    const int STABLE_DELAY_MS = 200;   // 稳定等待时间（毫秒）


};

#endif // SENSORDATAPROVIDER_H
