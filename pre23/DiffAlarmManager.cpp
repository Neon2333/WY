#include "DiffAlarmManager.h"
#include "ConfigSettings.h"
#include "GlobalDefines.h"

#include "AuditLogger.h"
#include <QtMath>

using namespace Constants;

DiffAlarmManager& DiffAlarmManager::instance()
{
    static DiffAlarmManager inst;
    return inst;
}

DiffAlarmManager::DiffAlarmManager(QObject* parent)
    : QObject(parent)
{
}

void DiffAlarmManager::checkAndTriggerDiffAlarm(const QVector<DiffItem>& diffs)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // diff 名称 → index（1~6）
    auto diffNameToIndex = [](const QString& name) -> int {
        if (name == "Δ12") return 1;
        if (name == "Δ13") return 2;
        if (name == "Δ14") return 3;
        if (name == "Δ23") return 4;
        if (name == "Δ24") return 5;
        if (name == "Δ34") return 6;
        return -1;
    };

    for (int i = 0; i < diffs.size(); ++i) {

        int diffIndex = diffNameToIndex(diffs[i].name);
        if (diffIndex == -1)
            continue;

        int idx = diffIndex - 1;   // 内部数组索引 0~5

        // ===== 是否启用该压差报警 =====
        if (!ThresholdSettings::instance().isDiffAlarmEnabled(diffIndex)) {

            // 关闭即强制清报警
            if (m_diffAlarm[idx]) {
                m_diffAlarm[idx] = false;
                m_diffAlarmType[idx] = DiffAlarmType::None;
                m_diffAbnormalSince[idx] = 0;
                m_diffNormalSince[idx] = 0;

                emit diffAlarmStateChanged(anyDiffAlarmActive());
            }
            continue;
        }

        double value = diffs[i].value;

        // ===== 非法值兜底 =====
        if (qIsNaN(value) || qIsInf(value))
            continue;

        double upper =
            ThresholdSettings::instance().pressureDiffUpper(diffIndex);
        double lower =
            ThresholdSettings::instance().pressureDiffLower(diffIndex);

        bool abnormal = (value > upper || value < lower);

        // ===== 稳定计时 =====
        if (abnormal) {
            if (m_diffAbnormalSince[idx] == 0)
                m_diffAbnormalSince[idx] = now;
            m_diffNormalSince[idx] = 0;
        } else {
            if (m_diffNormalSince[idx] == 0)
                m_diffNormalSince[idx] = now;
            m_diffAbnormalSince[idx] = 0;
        }

        bool abnormalStable =
            m_diffAbnormalSince[idx] != 0 &&
            (now - m_diffAbnormalSince[idx] >= 3000);

        bool normalStable =
            m_diffNormalSince[idx] != 0 &&
            (now - m_diffNormalSince[idx] >= 3000);

        // ===== 报警触发 =====
        if (!m_diffAlarm[idx] && abnormalStable) {
            m_diffAlarm[idx] = true;
            m_diffAlarmType[idx] = DiffAlarmType::Threshold;
            emit diffAlarmStateChanged(true);

            QString diffName =
                ThresholdSettings::instance().getPressureDiffName(diffIndex);

            QString msg = tr("压差报警 %1，实际值 %2，阈值 [%3,%4]")
                              .arg(diffName)
                              .arg(value, 0, 'f', 1)
                              .arg(lower)
                              .arg(upper);

            AuditLogger::instance().log(msg);
        }

        // ===== 报警恢复 =====
        if (m_diffAlarm[idx] && normalStable) {
            m_diffAlarm[idx] = false;
            m_diffAlarmType[idx] = DiffAlarmType::None;
            emit diffAlarmStateChanged(anyDiffAlarmActive());

            QString diffName =
                ThresholdSettings::instance().getPressureDiffName(diffIndex);

            AuditLogger::instance().log(tr("压差报警 %1 已恢复").arg(diffName));
        }
    }
}

bool DiffAlarmManager::anyDiffAlarmActive() const
{
    for (bool a : m_diffAlarm) {
        if (a)
            return true;
    }
    return false;
}


