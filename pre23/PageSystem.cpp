// PageSystem.cpp
#include "PageSystem.h"
#include "ui_PageSystem.h"
#include "PermissionManager.h"
#include "SessionManager.h"
#include "AuditLogger.h"
#include "OverlayMessage.h"
#include "SensorDataProvider.h"
#include "LanguageManager.h"
#include "DatabaseManager.h"
#include "ConfigSettings.h"
#include "GlobalDefines.h"

#include <QSettings>
#include <QDebug>
#include <QVBoxLayout>
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <QScrollBar>
/*
 * ​自动连接机制​：Qt的ui->setupUi(this)会自动连接遵循on_<objectName>_<signalName>命名规范的槽函数
 * 我们在下面写的连接函数都是以这个格式去写的，所以会自动触发。
 */

PageSystem::PageSystem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageSystem)
{
    ui->setupUi(this);

    // 设置当前选项为 ini 中保存的单位
    QString currentUnit = SensorDataProvider::instance()->pressureUnit();
    ui->sysCoBoxUnit->setCurrentText(currentUnit);

    connect(ui->sysUnitBtn, &QPushButton::clicked, this, [this]() {
        SensorDataProvider::instance()->setPressureUnit(getCurrentPressureUnit());
    });

    connect(ui->sysZeroBtn, &QPushButton::clicked, this, &PageSystem::offsetToZero);

    connect(ui->modifyBatchNum, &QPushButton::clicked, this, [=]() {saveBatchNumber();});
    connect(ui->modifyInterval, &QPushButton::clicked, this, [=]() {saveSamplingInterval();});

    connect(ui->modifyAlarm1, &QPushButton::clicked, this, [=]() { saveAlarmSetting(1); });
    connect(ui->modifyAlarm2, &QPushButton::clicked, this, [=]() { saveAlarmSetting(2); });
    connect(ui->modifyAlarm3, &QPushButton::clicked, this, [=]() { saveAlarmSetting(3); });
    connect(ui->modifyAlarm4, &QPushButton::clicked, this, [=]() { saveAlarmSetting(4); });

    connect(ui->modifyAll, &QPushButton::clicked, this, &PageSystem::saveAllAlarmSettings);
    connect(ui->modifyAllDiffs, &QPushButton::clicked, this, &PageSystem::saveAllDiffSettings);

    // 连接单个压差按钮
    connect(ui->modifyAlarm12, &QPushButton::clicked, this, &PageSystem::saveAlarm12Settings);
    connect(ui->modifyAlarm13, &QPushButton::clicked, this, &PageSystem::saveAlarm13Settings);
    connect(ui->modifyAlarm14, &QPushButton::clicked, this, &PageSystem::saveAlarm14Settings);
    connect(ui->modifyAlarm23, &QPushButton::clicked, this, &PageSystem::saveAlarm23Settings);
    connect(ui->modifyAlarm24, &QPushButton::clicked, this, &PageSystem::saveAlarm24Settings);
    connect(ui->modifyAlarm34, &QPushButton::clicked, this, &PageSystem::saveAlarm34Settings);

    connect(ui->modifyNet, &QPushButton::clicked, this, &PageSystem::saveNetworkConfig);

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageSystem::retranslateUi);

    loadUserSettings();

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageSystem::onUserChanged);

    connect(ui->changeLanguage, &QPushButton::clicked,this,&PageSystem::changeLanguage);
    // 初始化UI权限状态
    updateUIByPermission();

    /* ================= 备份管理器初始化（关键） ================= */
    m_backupThread = new QThread(this);
    m_backupManager = new BackupManager;              // 先 new
    m_backupManager->moveToThread(m_backupThread);    // 移入子线程，BackupManager 的“槽函数”，会在 m_backupThread 线程中执行

    connect(m_backupThread, &QThread::finished,m_backupManager, &QObject::deleteLater);

    /* ================= 设置备份路径 ================= */
    QString backupDir = "D:/pressureMonitor/backups";

    m_backupManager->setBackupParameters(hisDataBasePath,trailDataBasePath,backupDir);

    /* ================= 信号连接（UI安全） ================= */
    connect(m_backupManager, &BackupManager::backupStarted,this, &PageSystem::onBackupStarted, Qt::QueuedConnection);
    connect(m_backupManager, &BackupManager::backupProgress,this, &PageSystem::onBackupProgress, Qt::QueuedConnection);
    connect(m_backupManager, &BackupManager::backupFinished,this, &PageSystem::onBackupFinished, Qt::QueuedConnection);

    /* ================= 备份按钮 ================= */
    connect(ui->backupNow, &QPushButton::clicked, this, [this]() {
        m_currentBackupTrigger = BackupTrigger::Manual;

        ui->backupNow->setEnabled(false);
        ui->backupNow->setText(tr("备份中..."));
        QMetaObject::invokeMethod(
            m_backupManager,
            "startBackup",
            Qt::QueuedConnection
            );
    });
    m_backupThread->start();

    /* ================= 自动备份 ================= */
    connect(ui->autoBackup, &QPushButton::clicked,this, &PageSystem::autoBackupClicked);

    m_autoBackupTimer = new QTimer(this);
    connect(m_autoBackupTimer, &QTimer::timeout,this, &PageSystem::checkAutoBackup);

    m_autoBackupTimer->start(60 * 1000); // 每分钟检测

    initAutoBackup(); // 程序启动立即检测一次

    connect(ui->delBackup, &QPushButton::clicked,this, &PageSystem::onDeleteBackupClicked);
    connect(ui->copyBackup, &QPushButton::clicked,this, &PageSystem::onCopyBackupClicked);

    connect(ui->alarmStatus12, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);
    connect(ui->alarmStatus13, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);
    connect(ui->alarmStatus14, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);
    connect(ui->alarmStatus23, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);
    connect(ui->alarmStatus24, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);
    connect(ui->alarmStatus34, &QCheckBox::stateChanged, this, &PageSystem::alarmStateChanged);

}

PageSystem::~PageSystem()
{
    //1停止自动备份定时器
    if (m_autoBackupTimer) {
        m_autoBackupTimer->stop();
    }
    //2让备份线程退出
    if (m_backupThread) {
        m_backupThread->quit();   // 让线程事件循环退出
        m_backupThread->wait();   // 阻塞等待线程真正结束
    }
    delete ui;
}

void PageSystem::retranslateUi()
{
    ui->retranslateUi(this);
    loadUserSettings();
}

//获取单位
QString PageSystem::getCurrentPressureUnit()
{
    QString unit = ui->sysCoBoxUnit->currentText();

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("切换单位"), QString(tr("切换单位为：%1")).arg(unit));

    AuditLogger::instance().log(QString(tr("切换单位为：%1")).arg(unit));

    return unit;
}

//重置校零
void PageSystem::offsetToZero()
{
    SensorDataProvider::instance()->resetCalibrations();

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("重置校零"), tr("已重置校零"));
    AuditLogger::instance().log(tr("重置校零"));
}

//保存批号
void PageSystem::saveBatchNumber()
{
    QString newBatch = ui->batchNumber->text().trimmed();

    if (newBatch.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("批号不能为空"));
        return;
    }

    // 检查数据库
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "历史数据库未打开";
        return;
    }

    QString query = QString("SELECT COUNT(*) FROM h_table WHERE number = '%1'")
                        .arg(newBatch.replace("'", "''"));

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qDebug() << "SQL prepare error:" << sqlite3_errmsg(db);
        return;
    }

    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0);
    sqlite3_finalize(stmt);

    if (exists) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"),
                         tr("修改批号失败：批号 %1 已存在").arg(newBatch));
        AuditLogger::instance().log(tr("修改批号失败：批号 %1 已存在").arg(newBatch));
        return;
    }

    // 使用 SessionManager 保存
    SessionManager::instance().setBatchNumber(newBatch);

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("批号已保存"));

    AuditLogger::instance().log(tr("保存批号: %1").arg(newBatch));
}

//保存采样间隔
void PageSystem::saveSamplingInterval()
{
    QString intervalStr = ui->samplingInterval->text();
    bool ok = false;
    double intervalSec = intervalStr.toDouble(&ok);

    if (!ok || (intervalSec < 0) || (intervalSec > 600) ) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请输入 0~600 秒之间的数字"));
        return;
    }

    // 使用 SessionManager 保存
    SessionManager::instance().setInterval(static_cast<float>(intervalSec));

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("采样间隔已保存"));

    AuditLogger::instance().log(tr("保存采样间隔: %1 秒").arg(intervalSec, 0, 'f', 1));
}

// 从配置文件加载阈值、批次号、采样间隔、备份参数、网络
void PageSystem::loadUserSettings()
{
    // 1. 批号（SessionManager）
    ui->batchNumber->setText(SessionManager::instance().batchNumber());

    // 2. 采样间隔（SessionManager）
    float intervalSec = SessionManager::instance().samplingInterval();

    // 检查有效性：0~600 秒
    if ( (intervalSec < 0) || (intervalSec > 600) ) {
        ui->samplingInterval->setText(QString::number(5.0, 'f', 1));
    } else {
        ui->samplingInterval->setText(QString::number(intervalSec, 'f', 1));
    }

    // 3. 阈值min/max 标签
    auto &ts = ThresholdSettings::instance();

    ui->AlarmLower1->setText(QString::number(ts.alarmLower(1), 'f', 1));
    ui->AlarmUper1 ->setText(QString::number(ts.alarmUpper(1), 'f', 1));
    ui->AlarmLower2->setText(QString::number(ts.alarmLower(2), 'f', 1));
    ui->AlarmUper2 ->setText(QString::number(ts.alarmUpper(2), 'f', 1));
    ui->AlarmLower3->setText(QString::number(ts.alarmLower(3), 'f', 1));
    ui->AlarmUper3 ->setText(QString::number(ts.alarmUpper(3), 'f', 1));
    ui->AlarmLower4->setText(QString::number(ts.alarmLower(4), 'f', 1));
    ui->AlarmUper4 ->setText(QString::number(ts.alarmUpper(4), 'f', 1));
    // S12 压差
    ui->S12Lower->setText(QString::number(ts.pressureDiffLower(1), 'f', 1));
    ui->S12Upper->setText(QString::number(ts.pressureDiffUpper(1), 'f', 1));

    // S13 压差
    ui->S13Lower->setText(QString::number(ts.pressureDiffLower(2), 'f', 1));
    ui->S13Upper->setText(QString::number(ts.pressureDiffUpper(2), 'f', 1));

    // S14 压差
    ui->S14Lower->setText(QString::number(ts.pressureDiffLower(3), 'f', 1));
    ui->S14Upper->setText(QString::number(ts.pressureDiffUpper(3), 'f', 1));

    // S23 压差
    ui->S23Lower->setText(QString::number(ts.pressureDiffLower(4), 'f', 1));
    ui->S23Upper->setText(QString::number(ts.pressureDiffUpper(4), 'f', 1));

    // S24 压差
    ui->S24Lower->setText(QString::number(ts.pressureDiffLower(5), 'f', 1));
    ui->S24Upper->setText(QString::number(ts.pressureDiffUpper(5), 'f', 1));

    // S34 压差
    ui->S34Lower->setText(QString::number(ts.pressureDiffLower(6), 'f', 1));
    ui->S34Upper->setText(QString::number(ts.pressureDiffUpper(6), 'f', 1));

    //网络
    loadNetworkConfig();
    loadBackupTimeConfig(); //加载备份的时间
    initTable();
    loadBackupTable();      //加载备份数据表


}

//开关分路报警
void PageSystem::alarmStateChanged(int)
{
    QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
    if (!cb) return;

    int diffIndex = -1;

    if (cb == ui->alarmStatus12) diffIndex = 1;
    else if (cb == ui->alarmStatus13) diffIndex = 2;
    else if (cb == ui->alarmStatus14) diffIndex = 3;
    else if (cb == ui->alarmStatus23) diffIndex = 4;
    else if (cb == ui->alarmStatus24) diffIndex = 5;
    else if (cb == ui->alarmStatus34) diffIndex = 6;

    if (diffIndex == -1)
        return;

    ThresholdSettings::instance().setDiffAlarmEnabled(diffIndex, cb->isChecked());

    QString diffName = ThresholdSettings::instance().getPressureDiffName(diffIndex);

    AuditLogger::instance()
        .log(tr("压差 %1 报警 %2").arg(diffName,cb->isChecked() ? tr("开启") : tr("关闭")));

    cb->setText(cb->isChecked() ? tr("监测中") : tr("暂停中"));
}

//审计日志辅助函数
void PageSystem::logDiff(int diff, const QString& msg)
{
    QString op = QString(tr("通道压差Δ%1%2")).arg(diff).arg(msg);
    AuditLogger::instance().log(op);
}

// 保存阈值到配置文件-单通道
void PageSystem::saveAlarmSetting(int channel)
{
    // 1. 获取对应 UI 控件文本
    QString upperStr, lowerStr;

    switch (channel) {
    case 1:
        upperStr = ui->AlarmUper1->text().trimmed();
        lowerStr = ui->AlarmLower1->text().trimmed();
        break;
    case 2:
        upperStr = ui->AlarmUper2->text().trimmed();
        lowerStr = ui->AlarmLower2->text().trimmed();
        break;
    case 3:
        upperStr = ui->AlarmUper3->text().trimmed();
        lowerStr = ui->AlarmLower3->text().trimmed();
        break;
    case 4:
        upperStr = ui->AlarmUper4->text().trimmed();
        lowerStr = ui->AlarmLower4->text().trimmed();
        break;
    default:
        return;
    }

    // 2. 字段校验
    bool ok1, ok2;
    double up = upperStr.toDouble(&ok1);
    double lo = lowerStr.toDouble(&ok2);

    if (!ok1 || !ok2) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("请输入有效的数字"));
        return;
    }
    if (lo > up) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("下限不能大于上限"));
        return;
    }

    // 3. 保存到全局 ThresholdSettings（传入 double 值）
    ThresholdSettings::instance().setAlarmUpper(channel, up);
    ThresholdSettings::instance().setAlarmLower(channel, lo);

    // 4. 提示
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("通道 %1 的阈值已保存").arg(channel));

    AuditLogger::instance().log(tr("保存通道 %1 阈值：上限=%2，下限=%3")
                                    .arg(channel)
                                    .arg(up, 0, 'f', 1)   // 保留一位小数
                                    .arg(lo, 0, 'f', 1));
}

void PageSystem::saveAllAlarmSettings()
{
    // 获取所有4个通道的阈值字符串
    QString upper1Str = ui->AlarmUper1->text().trimmed();
    QString lower1Str = ui->AlarmLower1->text().trimmed();
    QString upper2Str = ui->AlarmUper2->text().trimmed();
    QString lower2Str = ui->AlarmLower2->text().trimmed();
    QString upper3Str = ui->AlarmUper3->text().trimmed();
    QString lower3Str = ui->AlarmLower3->text().trimmed();
    QString upper4Str = ui->AlarmUper4->text().trimmed();
    QString lower4Str = ui->AlarmLower4->text().trimmed();

    // 转换为 double 并验证
    bool ok1, ok2, ok3, ok4, ok5, ok6, ok7, ok8;
    double upper1 = upper1Str.toDouble(&ok1);
    double lower1 = lower1Str.toDouble(&ok2);
    double upper2 = upper2Str.toDouble(&ok3);
    double lower2 = lower2Str.toDouble(&ok4);
    double upper3 = upper3Str.toDouble(&ok5);
    double lower3 = lower3Str.toDouble(&ok6);
    double upper4 = upper4Str.toDouble(&ok7);
    double lower4 = lower4Str.toDouble(&ok8);

    // 验证所有输入是否有效
    if (!ok1 || !ok2 || !ok3 || !ok4 || !ok5 || !ok6 || !ok7 || !ok8) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("请输入有效的数字"));
        return;
    }

    // 验证下限不能大于上限
    if (lower1 > upper1 || lower2 > upper2 || lower3 > upper3 || lower4 > upper4) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("下限不能大于上限"));
        return;
    }

    // 构建 double 列表
    QList<double> uppers = { upper1, upper2, upper3, upper4 };
    QList<double> lowers = { lower1, lower2, lower3, lower4 };

    // 批量保存
    ThresholdSettings::instance().saveAll(uppers, lowers);

    // 提示
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("所有通道阈值已保存"));

    // 审计日志（保留一位小数）
    AuditLogger::instance().log(tr("更新阈值：[%1,%2][%3,%4][%5,%6][%7,%8]")
                                    .arg(lower1, 0, 'f', 1)
                                    .arg(upper1, 0, 'f', 1)
                                    .arg(lower2, 0, 'f', 1)
                                    .arg(upper2, 0, 'f', 1)
                                    .arg(lower3, 0, 'f', 1)
                                    .arg(upper3, 0, 'f', 1)
                                    .arg(lower4, 0, 'f', 1)
                                    .arg(upper4, 0, 'f', 1));
}

void PageSystem::saveAlarm12Settings()
{
    saveSingleDiffSettings(1, "S12");
}

void PageSystem::saveAlarm13Settings()
{
    saveSingleDiffSettings(2, "S13");
}

void PageSystem::saveAlarm14Settings()
{
    saveSingleDiffSettings(3, "S14");
}

void PageSystem::saveAlarm23Settings()
{
    saveSingleDiffSettings(4, "S23");
}

void PageSystem::saveAlarm24Settings()
{
    saveSingleDiffSettings(5, "S24");
}

void PageSystem::saveAlarm34Settings()
{
    saveSingleDiffSettings(6, "S34");
}

void PageSystem::saveAllDiffSettings()
{
    // 获取所有6个压差阈值的字符串
    QString s12UpperStr = ui->S12Upper->text().trimmed();
    QString s12LowerStr = ui->S12Lower->text().trimmed();
    QString s13UpperStr = ui->S13Upper->text().trimmed();
    QString s13LowerStr = ui->S13Lower->text().trimmed();
    QString s14UpperStr = ui->S14Upper->text().trimmed();
    QString s14LowerStr = ui->S14Lower->text().trimmed();
    QString s23UpperStr = ui->S23Upper->text().trimmed();
    QString s23LowerStr = ui->S23Lower->text().trimmed();
    QString s24UpperStr = ui->S24Upper->text().trimmed();
    QString s24LowerStr = ui->S24Lower->text().trimmed();
    QString s34UpperStr = ui->S34Upper->text().trimmed();
    QString s34LowerStr = ui->S34Lower->text().trimmed();

    // 转换为 double 并验证
    bool ok[12];
    double s12Upper = s12UpperStr.toDouble(&ok[0]);
    double s12Lower = s12LowerStr.toDouble(&ok[1]);
    double s13Upper = s13UpperStr.toDouble(&ok[2]);
    double s13Lower = s13LowerStr.toDouble(&ok[3]);
    double s14Upper = s14UpperStr.toDouble(&ok[4]);
    double s14Lower = s14LowerStr.toDouble(&ok[5]);
    double s23Upper = s23UpperStr.toDouble(&ok[6]);
    double s23Lower = s23LowerStr.toDouble(&ok[7]);
    double s24Upper = s24UpperStr.toDouble(&ok[8]);
    double s24Lower = s24LowerStr.toDouble(&ok[9]);
    double s34Upper = s34UpperStr.toDouble(&ok[10]);
    double s34Lower = s34LowerStr.toDouble(&ok[11]);

    // 检查所有转换是否成功
    for (int i = 0; i < 12; ++i) {
        if (!ok[i]) {
            auto* msg = new OverlayMessage(this);
            msg->showMessage(tr("错误"), tr("请输入有效的数字"));
            return;
        }
    }

    // 验证下限不能大于上限
    if (s12Lower > s12Upper || s13Lower > s13Upper || s14Lower > s14Upper ||
        s23Lower > s23Upper || s24Lower > s24Upper || s34Lower > s34Upper) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("下限不能大于上限"));
        return;
    }

    // 构建 double 列表
    QList<double> upperDiffs = { s12Upper, s13Upper, s14Upper, s23Upper, s24Upper, s34Upper };
    QList<double> lowerDiffs = { s12Lower, s13Lower, s14Lower, s23Lower, s24Lower, s34Lower };

    // 批量保存
    ThresholdSettings::instance().saveAllPressureDiffs(upperDiffs, lowerDiffs);

    // 提示
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("所有压差阈值已保存"));

    // 审计日志（保留一位小数）
    AuditLogger::instance().log(tr("保存压差阈值：")
                                + tr("[%1,%2][%3,%4][%5,%6][%7,%8][%9,%10][%11,%12]")
                                      .arg(s12Lower, 0, 'f', 1).arg(s12Upper, 0, 'f', 1)
                                      .arg(s13Lower, 0, 'f', 1).arg(s13Upper, 0, 'f', 1)
                                      .arg(s14Lower, 0, 'f', 1).arg(s14Upper, 0, 'f', 1)
                                      .arg(s23Lower, 0, 'f', 1).arg(s23Upper, 0, 'f', 1)
                                      .arg(s24Lower, 0, 'f', 1).arg(s24Upper, 0, 'f', 1)
                                      .arg(s34Lower, 0, 'f', 1).arg(s34Upper, 0, 'f', 1));
}

// 辅助函数：验证字符串是否为有效数字
bool PageSystem::isValidNumber(const QString& str)
{
    bool ok;
    str.toDouble(&ok);
    return ok && !str.isEmpty();
}

// 保存单个压差设置的通用函数
void PageSystem::saveSingleDiffSettings(int diffIndex, const QString& diffName)
{
    QString upper, lower;

    // 根据diffIndex获取对应的UI控件
    switch (diffIndex) {
    case 1:  // S12
        upper = ui->S12Upper->text().trimmed();
        lower = ui->S12Lower->text().trimmed();
        break;
    case 2:  // S13
        upper = ui->S13Upper->text().trimmed();
        lower = ui->S13Lower->text().trimmed();
        break;
    case 3:  // S14
        upper = ui->S14Upper->text().trimmed();
        lower = ui->S14Lower->text().trimmed();
        break;
    case 4:  // S23
        upper = ui->S23Upper->text().trimmed();
        lower = ui->S23Lower->text().trimmed();
        break;
    case 5:  // S24
        upper = ui->S24Upper->text().trimmed();
        lower = ui->S24Lower->text().trimmed();
        break;
    case 6:  // S34
        upper = ui->S34Upper->text().trimmed();
        lower = ui->S34Lower->text().trimmed();
        break;
    default:
        return;
    }

    // 字段校验
    bool ok1, ok2;
    double up = upper.toDouble(&ok1);
    double lo = lower.toDouble(&ok2);

    if (!ok1 || !ok2) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), diffName + tr(": 请输入有效的数字"));
        return;
    }
    if (lo > up) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), diffName + tr(": 下限不能大于上限"));
        return;
    }

    // 保存到配置
    ThresholdSettings::instance().setPressureDiffUpper(diffIndex, up);
    ThresholdSettings::instance().setPressureDiffLower(diffIndex, lo);

    // 提示
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), diffName + tr(" 压差阈值已保存"));

    // 审计日志
    AuditLogger::instance().log(tr("保存%1压差阈值：上限=%2，下限=%3")
                                    .arg(diffName)
                                    .arg(up, 0, 'f', 1)
                                    .arg(lo, 0, 'f', 1));
}

//网络配置
void PageSystem::loadNetworkConfig()
{
    QSettings settings(systemConfig, QSettings::IniFormat);
    ui->lineEditPort->setText(settings.value("Port", 502).toString());
}

// 保存配置
void PageSystem::saveNetworkConfig()
{
    QSettings settings(systemConfig, QSettings::IniFormat);
    settings.setValue("Port", ui->lineEditPort->text());
    // 立即保存
    settings.sync();
    // 提示
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("端口已保存"));
    // 审计日志
    AuditLogger::instance().log(tr("保存网络端口号：%1").arg(ui->lineEditPort->text()));
}

// 实现槽函数
void PageSystem::onBackupStarted()
{
    // 可以显示进度条或状态信息
    ui->statusLabel->setText(tr("开始备份..."));
}

void PageSystem::onBackupProgress(int percent, const QString &message)
{
    // 更新进度条和状态信息
    ui->progressBar->setValue(percent);
    ui->statusLabel->setText(message);
}

void PageSystem::onBackupFinished(bool success, const QString &message)
{
    ui->backupNow->setEnabled(true);
    ui->backupNow->setText(tr("立即备份"));

    auto* msg = new OverlayMessage(this);

    if (success) {
        msg->showMessage(tr("备份成功"), message);

        if (m_currentBackupTrigger == BackupTrigger::Auto) {
            AuditLogger::instance().log(tr("自动备份成功"));
        } else {
            AuditLogger::instance().log(tr("手动备份成功"));
        }

        // 自动备份更新下次备份时间
        if (m_currentBackupTrigger == BackupTrigger::Auto) {
            QSettings settings(systemConfig, QSettings::IniFormat);
            int days = settings.value("BackupIntervalDays", 15).toInt();
            settings.setValue("NextBackupTime",
                              QDateTime::currentDateTime().addDays(days).toString("yyyy-MM-dd hh:mm:ss"));
            settings.sync();
        }
    } else {
        msg->showMessage(tr("备份失败"), message);

        if (m_currentBackupTrigger == BackupTrigger::Auto) {
            AuditLogger::instance().log(tr("自动备份失败"));
        } else {
            AuditLogger::instance().log(tr("手动备份失败"));
        }
    }

    ui->progressBar->setValue(0);
    ui->statusLabel->setText(tr("就绪"));
    loadBackupTable();
}

/* ================= 自动备份初始化 ================= */
void PageSystem::initAutoBackup()
{
    checkAutoBackup();
}

/* ================= autoBackup 按钮 ================= */
void PageSystem::autoBackupClicked()
{
    bool ok = false;
    int days = ui->backupTime->text().toInt(&ok);   // 获取备份的时长
    if (!ok || days <= 0)
        return;

    QDateTime nextTime = QDateTime::currentDateTime().addDays(days);  // 当前时间加上备份时长

    QSettings settings(systemConfig, QSettings::IniFormat);

    settings.setValue("BackupIntervalDays", days);

    // 将 QDateTime 转换为字符串保存
    settings.setValue("NextBackupTime",nextTime.toString("yyyy-MM-dd hh:mm:ss"));
    settings.sync();

    AuditLogger::instance().log(tr("设置自动备份周期：%1 天").arg(days));
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("提示"), tr("自动备份设置已保存"));
    // checkAutoBackup(); // 立即检查一次
}

/* ================= 自动检测逻辑（核心） ================= */
void PageSystem::checkAutoBackup()
{
    QSettings settings(systemConfig, QSettings::IniFormat);

    QDateTime nextTime =
        QDateTime::fromString(
            settings.value("NextBackupTime").toString(),
            "yyyy-MM-dd hh:mm:ss");

    if (!nextTime.isValid())
        return;

    if (QDateTime::currentDateTime() >= nextTime) {
        m_currentBackupTrigger = BackupTrigger::Auto;

        QMetaObject::invokeMethod(
            m_backupManager,
            "startBackup",
            Qt::QueuedConnection
            );
    }
}

void PageSystem::loadBackupTimeConfig()
{
    QSettings settings(systemConfig, QSettings::IniFormat);

    /* ---------- 1. 用户配置：备份周期 ---------- */
    int intervalDays = settings.value("BackupIntervalDays", 15).toInt();
    ui->backupTime->setText(QString::number(intervalDays));

    /* ---------- 2. 系统状态：下一次备份时间 ---------- */
    QDateTime nextBackupTime = QDateTime::fromString(
        settings.value("NextBackupTime").toString(),
        "yyyy-MM-dd hh:mm:ss"
        );

    /* ---------- 3. 计算剩余天数 ---------- */
    int leftDays = 0;

    if (nextBackupTime.isValid()) {
        QDateTime now = QDateTime::currentDateTime();

        if (now < nextBackupTime) {
            qint64 secondsLeft = now.secsTo(nextBackupTime);

            // 向上取整，保证“还有 1.x 天”显示为 2 天
            leftDays = static_cast<int>(
                std::ceil(secondsLeft / 86400.0)
                );
        } else {
            leftDays = 0;
        }
    }

    ui->leftDays->setText(QString::number(leftDays));
}

/* ================= 备份数据表  ================= */
void PageSystem::initTable(){
    ui->backupTable->setColumnCount(2);
    ui->backupTable->setHorizontalHeaderLabels(
        { tr("备份日期"), tr("数据量（行）") }
        );
    ui->backupTable->horizontalHeader()->setStretchLastSection(true);
    ui->backupTable->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical {"
        "    width: 30px;"       // 设置滚动条宽度
        "}"
        );
    ui->backupTable->horizontalScrollBar()->setStyleSheet(
        "QScrollBar:horizontal {"
        "    height: 30px;"      // 设置滚动条高度
        "}"
        );
    // 禁止编辑
    ui->backupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 整行选中
    ui->backupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

int countTableRows(const QString &dbPath, const QString &tableName)
{
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;
    int count = 0;

    if (sqlite3_open_v2(dbPath.toUtf8().constData(),
                        &db,
                        SQLITE_OPEN_READONLY,
                        nullptr) != SQLITE_OK) {
        return 0;
    }

    QString sql = QString("SELECT COUNT(*) FROM %1").arg(tableName);

    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(),
                           -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return count;
}

int PageSystem::calculateBackupRows(const QString &backupDir)
{
    QString historyDb = backupDir + "/history.db";
    QString trailDb   = backupDir + "/trail.db";

    int historyRows = 0;
    int trailRows   = 0;

    if (QFile::exists(historyDb)) {
        historyRows = countTableRows(historyDb, "h_table");
    }

    if (QFile::exists(trailDb)) {
        trailRows = countTableRows(trailDb, "t_table");
    }

    return historyRows + trailRows;
}

void PageSystem::loadBackupTable()
{
    ui->backupTable->setRowCount(0);

    QDir rootDir("D:/pressureMonitor/backups");
    QStringList backupDirs = rootDir.entryList(
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::Reversed
        );

    //删除备份和拷贝备份的下拉框
    ui->selectDelData->clear();
    ui->selectCopyData->clear();
    ui->selectDelData->addItems(backupDirs);
    ui->selectCopyData->addItems(backupDirs);

    int row = 0;

    for (const QString &dirName : backupDirs) {
        QString fullPath = rootDir.absoluteFilePath(dirName);

        int rows = calculateBackupRows(fullPath);

        ui->backupTable->insertRow(row);

        QTableWidgetItem *dateItem = new QTableWidgetItem(dirName);
        dateItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *rowsItem =
            new QTableWidgetItem(QString::number(rows));
        rowsItem->setTextAlignment(Qt::AlignCenter);

        ui->backupTable->setItem(row, 0, dateItem);
        ui->backupTable->setItem(row, 1, rowsItem);


        row++;
    }
}

/* ================= 数据拷贝  ================= */
bool copyDirectory(const QString &srcPath, const QString &dstPath)//工具函数
{
    QDir srcDir(srcPath);
    if (!srcDir.exists())
        return false;

    QDir dstDir(dstPath);
    if (!dstDir.exists()) {
        if (!dstDir.mkpath(".")) {
            return false;
        }
    }

    QFileInfoList fileList = srcDir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
        );

    for (const QFileInfo &info : fileList) {
        QString srcFilePath = info.absoluteFilePath();
        QString dstFilePath = dstPath + "/" + info.fileName();

        if (info.isDir()) {
            if (!copyDirectory(srcFilePath, dstFilePath))
                return false;
        } else {
            if (QFile::exists(dstFilePath))
                QFile::remove(dstFilePath);

            if (!QFile::copy(srcFilePath, dstFilePath))
                return false;
        }
    }
    return true;
}

void PageSystem::doCopyBackup(const QString &srcPath,const QString &dstPath)
{
    if (copyDirectory(srcPath, dstPath)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("成功"), tr("备份拷贝完成"));  
    } else {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("失败"), tr("备份拷贝失败"));
    }
}

void PageSystem::onCopyBackupClicked()
{
    QString dirName = ui->selectCopyData->currentText();
    if (dirName.isEmpty())
        return;

    QString srcRoot = "D:/pressureMonitor/backups";
    QString dstRoot = dataBackupDir;

    QString srcPath = srcRoot + "/" + dirName;
    QString dstPath = dstRoot + "/" + dirName;

    if (!QDir(srcPath).exists()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("源备份目录不存在"));
        return;
    }

    // ---------- 目标不存在：直接拷贝 ----------
    if (!QDir(dstPath).exists()) {
        doCopyBackup(srcPath, dstPath);
        AuditLogger::instance().log(tr("数据 %1 向外拷贝完成").arg(dirName));
        return;
    }

    // ---------- 目标已存在：确认覆盖 ----------
    auto *confirmMsg = new OverlayMessage(this);
    confirmMsg->setShowCancelButton(true);
    confirmMsg->showMessage(
        tr("确认覆盖"),
        tr("目标备份已存在，是否覆盖？\n%1").arg(dirName)
        );

    connect(confirmMsg, &OverlayMessage::closed, this,
            [this, srcPath, dstPath, dirName]() {
                QDir dstDir(dstPath);
                if (!dstDir.removeRecursively()) {
                    auto *failMsg = new OverlayMessage(this);
                    failMsg->showMessage(tr("失败"), tr("无法删除已有备份"));
                    return;
                }
                doCopyBackup(srcPath, dstPath);
                AuditLogger::instance().log(tr("数据 %1 向外拷贝完成").arg(dirName));
            });
}

/* ================= 数据删除 ================= */
void PageSystem::onDeleteBackupClicked()
{
    QString dirName = ui->selectDelData->currentText();
    if (dirName.isEmpty())
        return;

    QString rootPath = "D:/pressureMonitor/backups";
    QString fullPath = rootPath + "/" + dirName;

    QDir dir(fullPath);
    if (!dir.exists()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("源备份目录不存在"));
        return;
    }

    // 使用 OverlayMessage 作为确认框
    auto *msg = new OverlayMessage(this);
    msg->setShowCancelButton(true);
    msg->showMessage(
        tr("确认删除"),
        tr("确定要删除该备份吗？\n%1").arg(dirName)
        );

    // 点击“确定”
    connect(msg, &OverlayMessage::closed, this, [this, fullPath, dirName]() {
        QDir dir(fullPath);
        if (dir.removeRecursively()) {
            auto *okMsg = new OverlayMessage(this);
            okMsg->showMessage(tr("成功"), tr("备份删除完成"));
            AuditLogger::instance().log(tr("备份 %1 删除完成").arg(dirName));
            loadBackupTable();
        } else {
            auto *failMsg = new OverlayMessage(this);
            failMsg->showMessage(tr("失败"), tr("备份删除失败"));
        }
    });

}

// 改变用户重新应用权限
void PageSystem::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态************************************************
void PageSystem::updateUIByPermission()
{
    // 超级管理员权限控制的UI元素（需要SuperAdmin权限）
    QList<QWidget*> superAdminWidgets = {
        ui->copyBackup, ui->selectCopyData, ui->delBackup, ui->selectDelData
    };

    // 设置超级管理员权限控件
    for (auto widget : superAdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::SuperAdmin, widget);
    }

    // Admin权限控制的UI元素（需要Admin及以上权限）
    QList<QWidget*> AdminWidgets = {
        ui->changeLanguage,
        ui->backupNow,ui->autoBackup,ui->sysUnitBtn,ui->sysCoBoxUnit,ui->modifyNet
    };

    // 设置Admin权限控件
    for (auto widget : AdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::Admin, widget);
    }

    // Supervisor权限控制的UI元素（需要Supervisor及以上权限）
    QList<QWidget*> SupervisorWidgets = {
        ui->modifyBatchNum, ui->batchNumber,ui->modifyInterval,ui->samplingInterval,
        ui->modifyAlarm1, ui->modifyAlarm2, ui->modifyAlarm3, ui->modifyAlarm4,ui->modifyAll,
        ui->modifyAlarm12, ui->modifyAlarm13, ui->modifyAlarm14, ui->modifyAlarm23, ui->modifyAlarm24, ui->modifyAlarm34, ui->modifyAllDiffs
    };

    // 设置Supervisor权限控件
    for (auto widget : SupervisorWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::Supervisor, widget);
    }

    // Standard权限控制的UI元素
    QList<QWidget*> Widgets = {
        ui->sysZeroBtn,
        ui->alarmStatus12,ui->alarmStatus13,ui->alarmStatus14,
        ui->alarmStatus23,ui->alarmStatus24,ui->alarmStatus34
    };

    // 设置Standard权限控件
    for (auto widget : Widgets) {
        PermissionManager::setUIByPermission(PermissionManager::Standard, widget,{PermissionManager::QA});
    }
}

void PageSystem::changeLanguage()
{
    auto *msg = new OverlayMessage(this);
    msg->setShowCancelButton(true);
    msg->showMessage("提示Warning", "是否切换语言\n Switch the language?");

    QObject::connect(msg, &OverlayMessage::closed, msg, [=]() {
        QString current = LanguageManager::instance().currentLanguage();
        if (current == "default"){
            LanguageManager::instance().loadLanguage("en");
            AuditLogger::instance().log("切换到英文Switch to english");
        }else{
            LanguageManager::instance().loadLanguage("default");
            AuditLogger::instance().log("切换到中文Switch to chinese");
        }
    });
    // 用户取消（不做任何事）
    QObject::connect(msg, &OverlayMessage::cancelled, msg, []() {
        // nothing
    });

}

