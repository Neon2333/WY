#include "AuditLogger.h"
#include "GlobalDefines.h"
#include "SessionManager.h"
#include "sqlite/sqlite3.h"
#include <QDateTime>
#include <QDebug>
#include <QSettings>

AuditLogger& AuditLogger::instance()
{
    static AuditLogger inst;
    return inst;
}

AuditLogger::AuditLogger() = default;
AuditLogger::~AuditLogger() = default;

void AuditLogger::setDatabasePath(const QString &dbPath)
{
    m_dbPath = dbPath;
    qDebug() << "[AuditLogger] 设置审计数据库：" << dbPath;
}

void AuditLogger::log(const QString &operation)
{
    // 确保路径已设置
    if (m_dbPath.isEmpty()) {
        qWarning() << "[AuditLogger] 未设置数据库路径，无法记录审计日志";
        return;
    }

    QSettings settings(mainConfig, QSettings::IniFormat);

    QMutexLocker locker(&m_mutex);

    // 打开 SQLite
    sqlite3 *db = nullptr;
    if (sqlite3_open(m_dbPath.toUtf8().constData(), &db) != SQLITE_OK) {
        qWarning() << "[AuditLogger] 打开数据库失败：" << sqlite3_errmsg(db);
        sqlite3_close(db);
        return;
    }

    // 构造插入语句
    QString timeStr = QDateTime::currentDateTime()
                          .toString("yyyy-MM-dd HH:mm:ss");
    QString user  = SessionManager::instance().userName();
    QString level = SessionManager::instance().userLevel();
    // 注意：operation 要对单引号做简单转义
    QString opEscaped = operation;
    opEscaped.replace("'", "''");

    batchNumber = settings.value("BatchNumber", "num001").toString();

    QString sql = QString(
                      "INSERT INTO t_table (trailtime, username, level, number, operation) "
                      "VALUES ('%1','%2','%3','%4','%5');"
                      ).arg(timeStr, user, level, batchNumber, opEscaped);

    char *err = nullptr;
    if (sqlite3_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &err) != SQLITE_OK) {
        qWarning() << "[AuditLogger] 插入审计日志失败：" << err;
        sqlite3_free(err);
    }
    sqlite3_close(db);
}
