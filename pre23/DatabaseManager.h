#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "sqlite/sqlite3.h"
#include <QObject>
#include <QString>
#include <QHash>
#include "HistoryData.h"
#include "TrailData.h"
#include "PermissionManager.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    enum DatabaseType {
        UserDatabase,   // 用户认证数据库
        HistoryDatabase, // 历史记录数据库
        TrailDatabase    // 轨迹记录数据库
    };

    static DatabaseManager& instance();

    // 数据库连接管理
    bool openDatabase(DatabaseType dbType);
    void closeDatabase(DatabaseType dbType);
    void closeAllDatabases();

    // 设置数据库路径
    void setHistoryDatabasePath(const QString &path);
    void setTrailDatabasePath(const QString &path);

    // 获取数据库路径
    QString getDatabasePath(DatabaseType dbType) const {
        switch (dbType) {
        case UserDatabase: return m_userDbPath;
        case HistoryDatabase: return m_historyDbPath;
        case TrailDatabase: return m_trailDbPath;
        default: return "";
        }
    }

    bool userExists(const QString &username);
    // 用户管理功能
    bool validateUser(const QString &username, const QString &password);
    bool getUserInfo(const QString &username, QString &outUsername, QString &outLevel);

    bool getLockStatus(const QString &username, QString &outLockStatus);
    bool firstLogin(const QString &username, QString &isFirstLogin);

    bool incrementFailedAttempts(const QString &username, int &outAttempts); // 增加并返回当前尝试次数
    bool resetFailedAttempts(const QString &username);
    bool lockAccount(const QString &username);
    bool isValidPasswordComplexity(const QString &password); // 系统级密码复杂度校验函数
    bool getUserCreateTime(const QString &username, QDateTime &outTime);
    bool updatePasswd_Time_FirstLogin(const QString &username,
                               const QString &newPassword,
                               const QDateTime &now);

    // 记录管理
    bool addHistoryRecord(const QString &user, const QString &action);
    bool addTrailRecord(const QString &user, const QString &event, const QString &details = "");
    bool insertSensorData(const QString& timestamp,
                          const QVariant& s1, const QVariant& s2, const QVariant& s3,const QVariant& s4,
                          const QString& batchNumber,
                          const QString& username,
                          const QString& userLevel);

    PermissionManager::PermissionLevel getUserPermissionLevel(const QString& username);

    // 获取数据库句柄（供需要直接操作数据库的组件使用）
    sqlite3* getDatabaseHandle(DatabaseType dbType);

    QList<HistoryData> queryHistoryByBatch(const QString& batchNumber);
    QList<HistoryData> queryHistoryByTime(const QString& start, const QString& end);
    QList<TrailData> queryTrailByBatch(const QString& batchNumber);
    QList<TrailData> queryTrailByTime(const QString& start, const QString& end);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    const char* dbTypeToString(DatabaseType dbType);
    void initDatabase(DatabaseType dbType);

    QHash<DatabaseType, sqlite3*> m_databases; // 数据库连接映射
    QString m_userDbPath;    // 用户数据库路径
    QString m_historyDbPath; // 历史数据库路径
    QString m_trailDbPath;   // 轨迹数据库路径

    // 禁止拷贝和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    const char* dbTypeToString(DatabaseType dbType) const;
};

#endif // DATABASEMANAGER_H
