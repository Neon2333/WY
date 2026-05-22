#ifndef DIFFALARMMANAGER_H
#define DIFFALARMMANAGER_H

#include <QObject>
#include <QSet>
#include <QVector>
#include <QDateTime>
#include "GlobalDefines.h"

enum class DiffAlarmType {
    None,
    Threshold
};


class DiffAlarmManager : public QObject
{
    Q_OBJECT
public:
    static DiffAlarmManager& instance();

    void checkAndTriggerDiffAlarm(const QVector<DiffItem>& diffs);

    bool anyDiffAlarmActive() const;

signals:
    void diffAlarmStateChanged(bool alarm);

private:
    explicit DiffAlarmManager(QObject* parent = nullptr);

    bool     m_diffAlarm[6] = {false};
    DiffAlarmType m_diffAlarmType[6] = {DiffAlarmType::None};

    qint64   m_diffAbnormalSince[6] = {0};
    qint64   m_diffNormalSince[6]   = {0};

};

#endif // DIFFALARMMANAGER_H
