#pragma once

#include <QObject>
#include <QMutex>

class BackupManager : public QObject
{
    Q_OBJECT
public:
    explicit BackupManager(QObject *parent = nullptr);

    void setBackupParameters(const QString &historyDbPath,
                             const QString &trailDbPath,
                             const QString &backupDir);

public slots:
    void startBackup();
    void cancelBackup();

signals:
    void backupStarted();
    void backupProgress(int percent, const QString &message);
    void backupFinished(bool success, const QString &message);

private:
    bool performBackup(const QString &backupFileName,
                       const QString &databasePath);

private:
    QString m_historyDbPath;
    QString m_trailDbPath;
    QString m_backupDir;

    bool m_cancelRequested = false;
    QMutex m_mutex;
};
