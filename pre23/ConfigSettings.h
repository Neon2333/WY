#ifndef CONFIGSETTINGS_H
#define CONFIGSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QList>

class ThresholdSettings : public QObject
{
    Q_OBJECT
public:
    static ThresholdSettings& instance();

    // 通道报警阈值（返回浮点数）
    double alarmUpper(int index);
    double alarmLower(int index);
    void setAlarmUpper(int index, double value);
    void setAlarmLower(int index, double value);

    // 压差报警阈值
    double pressureDiffUpper(int diffIndex);
    double pressureDiffLower(int diffIndex);
    void setPressureDiffUpper(int diffIndex, double value);
    void setPressureDiffLower(int diffIndex, double value);

    // 压差报警开关
    bool isDiffAlarmEnabled(int diffIndex) const;
    void setDiffAlarmEnabled(int diffIndex, bool enabled);

    // 获取压差名称（用于界面显示）
    QString getPressureDiffName(int diffIndex);

    // 批量保存
    void saveAll(const QList<double>& uppers, const QList<double>& lowers);
    void saveAllPressureDiffs(const QList<double>& upperDiffs, const QList<double>& lowerDiffs);

    // 批量获取
    QList<double> getAllAlarmUppers();
    QList<double> getAllAlarmLowers();
    QList<double> getAllPressureDiffUppers();
    QList<double> getAllPressureDiffLowers();

signals:
    void thresholdChanged();

private:
    explicit ThresholdSettings(QObject *parent = nullptr);
    void initializeDefaults();

    QSettings m_settings;
};

#endif // CONFIGSETTINGS_H
