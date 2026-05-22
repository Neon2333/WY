// PageSystem.h
#ifndef PAGESYSTEM_H
#define PAGESYSTEM_H

#include <QWidget>
#include <QDateTime>
#include <QInputDialog>
#include <QFrame>

#include "BackupManager.h"
namespace Ui {
class PageSystem;
}

enum class BackupTrigger {
    Manual,
    Auto
};

class PageSystem : public QWidget
{
    Q_OBJECT

public:
    explicit PageSystem(QWidget *parent = nullptr);
    ~PageSystem();

private slots:

    QString getCurrentPressureUnit();
    void offsetToZero();
    void onUserChanged(const QString& name, const QString& level);

    void saveAlarmSetting(int channel);
    void saveAllAlarmSettings();  // 保存所有通道阈值
    void saveAllDiffSettings();   // 保存所有压差阈值

    // 单个压差保存函数
    void saveAlarm12Settings();
    void saveAlarm13Settings();
    void saveAlarm14Settings();
    void saveAlarm23Settings();
    void saveAlarm24Settings();
    void saveAlarm34Settings();

    void onBackupStarted();
    void onBackupProgress(int percent, const QString &message);
    void onBackupFinished(bool success, const QString &message);

    void autoBackupClicked();     // 点击 autoBackup
    void checkAutoBackup();         // 自动检测
    void onCopyBackupClicked();
    void onDeleteBackupClicked();

    void changeLanguage();

private:
    void initPermission();
    void AdminCanControlState(bool enabled);
    void SuperAdminCanControlState(bool enabled);
    void saveBatchNumber();
    void retranslateUi();
    void saveSamplingInterval();
    void loadUserSettings();
    bool isValidNumber(const QString& str);
    void saveSingleDiffSettings(int diffIndex, const QString& diffName);
    void loadNetworkConfig();
    void saveNetworkConfig();
    void initAutoBackup();          // 启动时检测
    void loadBackupTimeConfig();

    void logDiff(int diff, const QString& msg);
    void alarmStateChanged(int);

    void initTable();
    int calculateBackupRows(const QString &backupDir);
    void loadBackupTable();
    void doCopyBackup(const QString &srcPath,const QString &dstPath);
    void updateUIByPermission();
private:
    Ui::PageSystem *ui;

    BackupManager *m_backupManager = nullptr; // 明确初始化,防野指针
    QThread *m_backupThread = nullptr;         // 备份线程

    QTimer *m_autoBackupTimer = nullptr;

    BackupTrigger m_currentBackupTrigger = BackupTrigger::Manual;

};

#endif // PAGESYSTEM_H
