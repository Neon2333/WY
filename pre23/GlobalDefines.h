#ifndef GLOBALDEFINES_H
#define GLOBALDEFINES_H

#include <QString>

#pragma once

namespace Constants {
constexpr double DISCONNECTED_VALUE = 9999.0;
}

struct DiffItem {
    QString name;   // "Δ12"
    double value;   // 实际压差
};

#define userDataBasePath "D:/pressureMonitor/home/database/userinfo.db"
#define hisDataBasePath "D:/pressureMonitor/home/database/history.db"
#define trailDataBasePath "D:/pressureMonitor/home/database/trail.db"
#define dataBasePath "D:/pressureMonitor/home/database"

#define trailOutputDir "D:/SensorData/AuditTrail/"
#define historyOutputDir "D:/SensorData/HistoryData/"
#define reportOutputDir "D:/SensorData/Report/"

#define dataBackupDir "D:/DataBackup/"

#define mainConfig "D:/pressureMonitor/home/configs/mainConfig.ini"
#define systemConfig "D:/pressureMonitor/home/configs/systemConfig.ini"
#define chartConfig "D:/pressureMonitor/home/configs/chartConfig.ini"


#endif // GLOBALDEFINES_H
