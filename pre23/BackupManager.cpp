#include "BackupManager.h"
#include <QDateTime>
#include <QDir>
#include <QDebug>

#include <sqlite\sqlite3.h>

/*
1 UI 线程创建 BackupManager
2 BackupManager 被 move 到子线程
3 UI 用 invokeMethod 请求开始备份
4 子线程执行数据库复制
5 过程中 emit 信号
6 Qt 自动把信号投递回 UI 线程
7 PageSystem 更新界面
8 QMutex 保证跨线程状态一致性
*/

BackupManager::BackupManager(QObject *parent)
    : QObject(parent)
{
}

void BackupManager::setBackupParameters(const QString &historyDbPath,
                                        const QString &trailDbPath,
                                        const QString &backupDir)
{
    QMutexLocker locker(&m_mutex);
    m_historyDbPath = historyDbPath;
    m_trailDbPath = trailDbPath;
    m_backupDir = backupDir;
}

void BackupManager::startBackup()
{
    emit backupStarted();//发信号通知，在system接收，执行关联的函数

    QString backupDate = QDate::currentDate().toString("yyyyMMdd");
    QString outputDir = m_backupDir + "/" + backupDate;

    QDir dir;
    if (!dir.mkpath(outputDir)) {
        emit backupFinished(false, tr("无法创建备份目录"));
        return;
    }

    emit backupProgress(20, tr("备份中……"));
    if (!performBackup(outputDir + "/history.db", m_historyDbPath)) {
        emit backupFinished(false, tr("历史数据库备份失败"));
        return;
    }

    emit backupProgress(60, tr("备份中……"));
    if (!performBackup(outputDir + "/trail.db", m_trailDbPath)) {
        emit backupFinished(false, tr("审计数据备份失败"));
        return;
    }

    emit backupProgress(100, tr("备份完成"));
    emit backupFinished(true, tr("备份完成！"));
}

void BackupManager::cancelBackup()
{
    QMutexLocker locker(&m_mutex);
    m_cancelRequested = true;
}

bool BackupManager::performBackup(const QString &backupFileName,
                                  const QString &databasePath)
{
    sqlite3 *src = nullptr;
    sqlite3 *dst = nullptr;

    if (sqlite3_open(databasePath.toUtf8().constData(), &src) != SQLITE_OK)
        return false;

    if (sqlite3_open(backupFileName.toUtf8().constData(), &dst) != SQLITE_OK) {
        sqlite3_close(src);
        return false;
    }

    sqlite3_backup *backup = sqlite3_backup_init(dst, "main", src, "main");
    if (!backup) {
        sqlite3_close(src);
        sqlite3_close(dst);
        return false;
    }

    sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);

    sqlite3_close(src);
    sqlite3_close(dst);
    return true;
}
