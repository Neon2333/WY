// SensorDataProvider.cpp
#include "SensorDataProvider.h"
#include <QDebug>
#include "GlobalDefines.h" // 包含常量定义

SensorDataProvider* SensorDataProvider::instance() {
    static SensorDataProvider inst;
    return &inst;
}

SensorDataProvider::SensorDataProvider(QObject *parent)
    : QObject(parent)
{
    m_settings = new QSettings(systemConfig, QSettings::IniFormat, this);
    m_currentUnit = m_settings->value("PressureUnit", "mbar").toString();       //没有找到该键，则使用默认值"mbar"
    m_lastUnit = m_currentUnit;

    // 初始化校零偏移量
    data.offset1 = 0.0;
    data.offset2 = 0.0;
    data.offset3 = 0.0;
    data.offset4 = 0.0;

    connect(this, &SensorDataProvider::pressureUnitChanged, [this](const QString& newUnit){
        m_currentUnit = newUnit;
        // 单位改变时更新所有显示值
        updateAllDisplayValues();
    });
}


// ==== 数据流实现 ====
void SensorDataProvider::connectToSerialReader(QObject *reader) {
    if (auto serial = qobject_cast<SerialReader*>(reader)) {
        connect(serial, &SerialReader::sensorPacketReceived,
                this, &SensorDataProvider::onRawPacketReceived);
    }
}


// 校准函数（直接操作结构体）
void SensorDataProvider::calibrateSensor(int sensorId)
{
    using namespace Constants;
    if ((sensorId < 1) || (sensorId > 4)) return;

    double* valuePtr = nullptr;
    switch(sensorId) {
    case 1: valuePtr = &data.sensor1; break;
    case 2: valuePtr = &data.sensor2; break;
    case 3: valuePtr = &data.sensor3; break;
    case 4: valuePtr = &data.sensor4; break;
    }

    // 检测断线
    if (qFuzzyCompare(*valuePtr, DISCONNECTED_VALUE)) {
        qWarning() << "传感器" << sensorId << "断线，无法校零";
        return;
    }

    // 直接更新偏移量（增加第4路）
    switch(sensorId) {
    case 1: data.offset1 = *valuePtr; break;
    case 2: data.offset2 = *valuePtr; break;
    case 3: data.offset3 = *valuePtr; break;
    case 4: data.offset4 = *valuePtr; break;
    }

    qDebug() << "传感器" << sensorId << "校零完成，偏移量：" << *valuePtr;
}


void SensorDataProvider::onRawPacketReceived(const SensorPacket &packet)
{
    using namespace Constants;

    int id = packet.sensor_id;
    double value = packet.pressure;

    // 当前时间
    QDateTime now = QDateTime::currentDateTime();

    // ================== 断线处理 ==================
    if (value == DISCONNECTED_VALUE)
    {
        // 标记断线：立即生效
        m_isStable[id] = false;
        m_connectTime.remove(id);

        // 直接更新数据
        switch (id) {
        case 1: data.display1 = DISCONNECTED_VALUE; break;
        case 2: data.display2 = DISCONNECTED_VALUE; break;
        case 3: data.display3 = DISCONNECTED_VALUE; break;
        case 4: data.display4 = DISCONNECTED_VALUE; break;
        }

        emit convertedSensorDataReady(data);
        return;
    }

    // ================== 插入检测 ==================
    if (!m_connectTime.contains(id))
    {
        // 第一次看到非断线值 → 认为是“刚插入”
        m_connectTime[id] = now;
        m_isStable[id] = false;
        return;   //  稳定期内不向上游传
    }

    // ================== 稳定期判断 ==================
    if (!m_isStable.value(id))
    {
        if (m_connectTime[id].msecsTo(now) < STABLE_DELAY_MS)
        {
            // 还在 1 秒稳定期
            return;   //  丢弃数据
        }
        else
        {
            // 稳定期结束
            m_isStable[id] = true;
            qDebug() << "传感器" << id << "稳定完成";
        }
    }

    // ================== 稳定后，正常数据流 ==================
    data.time = now.toString("yyyy-MM-dd HH:mm:ss");

    switch(id) {
    case 1:
        data.sensor1 = value;
        data.calibrated1 = value - data.offset1;
        data.display1 = convertPressure(data.calibrated1, m_currentUnit);
        break;
    case 2:
        data.sensor2 = value;
        data.calibrated2 = value - data.offset2;
        data.display2 = convertPressure(data.calibrated2, m_currentUnit);
        break;
    case 3:
        data.sensor3 = value;
        data.calibrated3 = value - data.offset3;
        data.display3 = convertPressure(data.calibrated3, m_currentUnit);
        break;
    case 4:
        data.sensor4 = value;
        data.calibrated4 = value - data.offset4;
        data.display4 = convertPressure(data.calibrated4, m_currentUnit);
        break;
    }

    emit convertedSensorDataReady(data);
}


double SensorDataProvider::convertPressure(double mbarValue, const QString &unit)
{
    using namespace Constants;

    // 如果是断线值，直接返回
    if (mbarValue == DISCONNECTED_VALUE) return DISCONNECTED_VALUE;

    if (unit == "kpa") return mbarValue / 10.0;
    if (unit == "psi") return mbarValue / 68.9476;
    if (unit == "mmhg") return mbarValue / 1.33322;
    return mbarValue; // 默认返回原始值
}

// 辅助函数（直接操作结构体）
void SensorDataProvider::updateAllDisplayValues() {
    data.display1 = convertPressure(data.calibrated1, m_currentUnit);
    data.display2 = convertPressure(data.calibrated2, m_currentUnit);
    data.display3 = convertPressure(data.calibrated3, m_currentUnit);
    data.display4 = convertPressure(data.calibrated4, m_currentUnit);
}

double SensorDataProvider::getCalibratedValue(int sensorId, double rawValue) const
{
    if (qFuzzyCompare(rawValue, 9999.0)) return rawValue; // 断线不校准
    return rawValue - m_calibrationOffsets.value(sensorId, 0.0);
}

double SensorDataProvider::getCalibratedValueInCurrentUnit(int sensorId, double rawValue) const
{
    // 1. 应用校零校准
    double calibrated = getCalibratedValue(sensorId, rawValue);

    // 2. 转换为当前单位
    return convertPressure(calibrated, m_currentUnit);
}

void SensorDataProvider::resetCalibrations() {
    data.offset1 = 0.0;
    data.offset2 = 0.0;
    data.offset2 = 0.0;
    data.offset4 = 0.0;
    qDebug() << "所有校零偏移量已重置";
}

// ==== 单位管理实现 ====
QString SensorDataProvider::pressureUnit() const {
    return m_currentUnit;
}

bool SensorDataProvider::isValidUnit(const QString &unit) const {
    return unit == "mbar" || unit == "kpa" || unit == "psi" || unit == "mmhg";
}

void SensorDataProvider::setPressureUnit(const QString &unit) {
    if (!isValidUnit(unit)) {
        qDebug() << "无效的压力单位：" << unit;
        return;
    }

    if (unit != m_currentUnit) {
        m_lastUnit = m_currentUnit;
        m_currentUnit = unit;
        m_settings->setValue("PressureUnit", unit);
        emit pressureUnitChanged(unit);
    }
}

// ==== 数据处理 ====
void SensorDataProvider::onPressureUnitChanged(const QString &newUnit) {
    m_currentUnit = newUnit;
}

