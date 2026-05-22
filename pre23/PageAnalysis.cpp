#include "PageAnalysis.h"
#include "ui_PageAnalysis.h"
#include "DatabaseManager.h"
#include "SessionManager.h"
#include "LanguageManager.h"
#include "ConfigSettings.h"
#include "SensorDataProvider.h"
#include "OverlayMessage.h"
#include "GlobalDefines.h"
#include "PermissionManager.h"
#include "AuditLogger.h"
#include "MathUtils.h"    //道格拉斯简化线函数
#include <QtDebug>
#include <QPrinterInfo>
#include <QMessageBox>
#include <QPainter>
#include <QtMath>
#include <QSettings>
#include <QVBoxLayout>
#include <QRectF>
#include <QPrintDialog>

// PageAnalysis类构造函数：初始化UI界面并连接信号槽
PageAnalysis::PageAnalysis(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageAnalysis)
{
    ui->setupUi(this);

    setupTable();              // 初始化数据表格
    initPrinterList();         // 初始化打印机列表
    setupChart();              // 初始化图表

    // 将选择的批号最新数据保存到报表
    connect(ui->reportBtn,&QPushButton::clicked,this,&PageAnalysis::reportBtnClicked);
    // 从历史数据刷新到显示
    connect(ui->hisReportBtn,&QPushButton::clicked,this,&PageAnalysis::refreshAll);

    //打印输出
    connect(ui->exToPrinterBtn,&QPushButton::clicked,this,&PageAnalysis::printReport);

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageAnalysis::onUserChanged);

    // 初始化UI权限状态
    updateUIByPermission();

    // 连接全选checkbox的点击信号
    connect(ui->checkBoxAll, &QCheckBox::clicked, this, &PageAnalysis::selectAllCheckBox);
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageAnalysis::retranslateUi);
}

// PageAnalysis类析构函数：释放UI资源
PageAnalysis::~PageAnalysis()
{
    delete ui;
}

void PageAnalysis::retranslateUi()
{
    ui->retranslateUi(this);
}


// 初始化函数：设置表格、刷新批号下拉列表、初始化打印机列表、填充文本浏览器
void PageAnalysis::init()
{
    refreshNumberCombos();     // 刷新批号下拉列表
    fillTextBrowser();         // 填充文本浏览器内容
    ui->outputUnit->setText(SensorDataProvider::instance()->pressureUnit());
}

/*******************UI显示****************************/

// 初始化表格：设置表格列数、标题、列宽、编辑模式等
void PageAnalysis::setupTable()
{
    QStringList headers = {tr("时间"),tr("操作详情")};  // 定义表格标题
    ui->trailTable->setColumnCount(headers.size());  // 设置列数
    ui->trailTable->setHorizontalHeaderLabels(headers);  // 设置标题
    ui->trailTable->setColumnWidth(0, 200);  // 设置第一列宽度
    ui->trailTable->horizontalHeader()->setStretchLastSection(true);  // 最后一列拉伸
    ui->trailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 禁止编辑
    ui->trailTable->setSelectionBehavior(QAbstractItemView::SelectRows);  // 行选择模式

    ui->trailTable->
        verticalScrollBar()->setStyleSheet("QScrollBar:vertical {width: 30px;}");// 设置滚动条宽度
}

// 更新文本浏览器：获取批号、用户信息、当前时间并填充到UI组件
void PageAnalysis::fillTextBrowser()
{
    // ======= 1. 获取 batchNum（来自下拉框）=======
    QString batchNum = ui->selectNumberRange->currentText();

    // ======= 2. 获取用户信息 =======
    QString user  = SessionManager::instance().userName();  // 获取当前用户名
    QString level = SessionManager::instance().userLevel(); // 获取当前用户级别

    // operationUser 显示： user(level)
    ui->operationUser->setText(QString("%1(%2)").arg(user).arg(level));

    // ======= 3. 输出时间（当前时间）=======
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ui->outputTime->setText(now);

    // ======= 4. 根据勾选框确定标题 =======
    ui->batchNumTimeTitle->setText(tr("批号:"));
    ui->batchNumTimeText->setText(batchNum);
}

//从 unit 表读取单位
QString PageAnalysis::getUnitFromUnitTable(const QString& batchNum)
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return QString();

    QString query = QString("SELECT unit FROM unit WHERE number = '%1'").arg(batchNum);

    sqlite3_stmt* stmt = nullptr;
    QString unit;

    if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmt, 0);
            if (text)
                unit = QString::fromUtf8(reinterpret_cast<const char*>(text));
        }
        sqlite3_finalize(stmt);
    }

    return unit; // 可能为空
}

// 批号下拉列表更新：从数据库获取所有批号并填充到下拉框
void PageAnalysis::refreshNumberCombos()
{
    ui->selectNumberRange->clear();  // 清空现有项
    ui->selectNumberRange->addItem(tr("请选择批号"));  // 添加默认选项

    // 获取历史数据库句柄
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    // SQL查询：获取所有不同的批号并按升序排列
    const char *sql = "SELECT DISTINCT number FROM h_table ORDER BY number ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    // 遍历查询结果，将批号添加到下拉框
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        QString number = text ? QString(text) : "";

        // 排除空字符串和只包含空白字符的选项
        if (!number.trimmed().isEmpty()) {
            ui->selectNumberRange->addItem(number);
        }
    }
    sqlite3_finalize(stmt);  // 释放语句句柄
}


// 获取批号的开始时间和结束时间：查询数据库获取指定批号的时间范围
QPair<QString, QString> PageAnalysis::getBatchTimeRange(const QString& batchNum)
{
    QPair<QString, QString> timeRange;  // 存储时间范围的配对

    // 获取历史数据库句柄
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接数据库";
        return timeRange;
    }

    // SQL查询：获取指定批号的最小和最大时间
    QString query = QString(
                        "SELECT MIN(historytime), MAX(historytime) FROM h_table "
                        "WHERE number = '%1' AND historytime IS NOT NULL AND historytime != ''")
                        .arg(batchNum);

    qDebug() << "查询时间范围:" << query;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* minTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));  // 最小时间
            const char* maxTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));  // 最大时间

            if (minTime) {
                timeRange.first = QString(minTime);
                // 确保时间格式正确
                if (!timeRange.first.contains(":")) {
                    // 如果时间格式不正确，尝试转换
                    QDateTime dt = QDateTime::fromString(timeRange.first, Qt::ISODate);
                    if (dt.isValid()) {
                        timeRange.first = dt.toString("yyyy-MM-dd HH:mm:ss");
                    }
                }
            }

            if (maxTime) {
                timeRange.second = QString(maxTime);
                // 确保时间格式正确
                if (!timeRange.second.contains(":")) {
                    // 如果时间格式不正确，尝试转换
                    QDateTime dt = QDateTime::fromString(timeRange.second, Qt::ISODate);
                    if (dt.isValid()) {
                        timeRange.second = dt.toString("yyyy-MM-dd HH:mm:ss");
                    }
                }
            }

            qDebug() << "批号" << batchNum << "的时间范围:" << timeRange.first << "到" << timeRange.second;
        } else {
            qDebug() << "未找到批号" << batchNum << "的数据";

            // 如果找不到数据，尝试检查数据库中是否有这个批号
            QString checkQuery = QString("SELECT COUNT(*) FROM h_table WHERE number = '%1'").arg(batchNum);
            sqlite3_stmt* checkStmt = nullptr;
            if (sqlite3_prepare_v2(db, checkQuery.toUtf8(), -1, &checkStmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(checkStmt) == SQLITE_ROW) {
                    int count = sqlite3_column_int(checkStmt, 0);
                    qDebug() << "批号" << batchNum << "在h_table中的记录数:" << count;
                }
                sqlite3_finalize(checkStmt);
            }
        }
        sqlite3_finalize(stmt);
    } else {
        qDebug() << "查询时间范围失败:" << sqlite3_errmsg(db);
    }
    return timeRange;
}

// 刷新所有数据：更新文本浏览器内容并触发历史报表按钮点击事件
void PageAnalysis::refreshAll()
{
    fillTextBrowser();        // 填充文本浏览器
    hisReportBtnClicked();    // 触发历史报表按钮点击事件
}

// hisReportBtn点击事件 - 从report表查询并显示数据，只显示勾选的列
void PageAnalysis::hisReportBtnClicked()
{
    QString batchNum = ui->selectNumberRange->currentText();
    if (batchNum == tr("请选择批号") || batchNum.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请选择批号！"));
            return;
    }

    // 定义列配置
    struct ColumnConfig {
        QString field;      // 数据库字段名
        QString title;       // 显示标题
        QCheckBox* checkbox; // 对应的复选框
    };

    QVector<ColumnConfig> columnConfig = {
        {"sname", tr("传感器名称"), ui->sensor1},
        {"min", tr("最小值"), ui->min},
        {"max", tr("最大值"), ui->max},
        {"avg", tr("平均值"), ui->avg},
        {"upLimit", tr("报警上限"), ui->alarmUpLimit},
        {"lowLimit", tr("报警下限"), ui->alarmLowLimit},
        {"alarmCount", tr("报警次数"), ui->alarm},
        {"startTime", tr("开始时间"), ui->startTime},
        {"endTime", tr("结束时间"), ui->endTime}
    };

    // 确定要显示的列
    QStringList selectedFields;
    QStringList selectedTitles;
    QStringList selectedSensors;

    for (const auto& config : columnConfig) {
        if (config.checkbox->isChecked()) {
            selectedFields << config.field;
            selectedTitles << config.title;
        }
    }

    if (ui->sensor1->isChecked()) selectedSensors << "SENSOR1";
    if (ui->sensor2->isChecked()) selectedSensors << "SENSOR2";
    if (ui->sensor3->isChecked()) selectedSensors << "SENSOR3";
    if (ui->sensor4->isChecked()) selectedSensors << "SENSOR4";

    if (selectedFields.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请至少选择一列显示！"));
        return;
    }

    // 查询report表
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << " 错误请检查历史数据库！";
        return;
    }

    QString sensorCondition;
    for (const QString& s : selectedSensors) {
        sensorCondition += "'" + s + "',";
    }
    sensorCondition.chop(1); // 去掉最后一个逗号

    // 构建查询语句
    QString fields = selectedFields.join(", ");
    QString query = QString(
                        "SELECT %1 FROM report "
                        "WHERE number = '%2' AND sname IN (%3) "
                        "ORDER BY sname"
                        ).arg(fields,batchNum,sensorCondition);

    // qDebug() << "执行查询:" << query;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        QString errorMsg = tr("查询失败: %1").arg(sqlite3_errmsg(db));
        qDebug() << "SQL错误:" << errorMsg;
        return;
    }

    // 设置表格
    ui->dataTable->setColumnCount(selectedFields.size());
    ui->dataTable->setHorizontalHeaderLabels(selectedTitles);
    ui->dataTable->setRowCount(0);

    // 填充数据
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->dataTable->insertRow(row);

        for (int col = 0; col < selectedFields.size(); ++col) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QString value = text ? QString(text) : "";

            QTableWidgetItem* item = new QTableWidgetItem(value);
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, "dynamic_item");   // 或者任何非空值
            ui->dataTable->setItem(row, col, item);
        }
        row++;
    }
    sqlite3_finalize(stmt);

    // 表格美化
    ui->dataTable->resizeColumnsToContents();

    // 为时间列设置更大的宽度
    for (int col = 0; col < selectedTitles.size(); ++col) {
        QString header = selectedTitles[col];
        if (header == tr("开始时间") || header == tr("结束时间")){
            ui->dataTable->setColumnWidth(col, 140); // 设置时间列为140像素宽
        }
    }

    ui->dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); // 改为交互模式
    ui->dataTable->setAlternatingRowColors(true);

    // 更新图表
    updateChart();

    QString unit = getUnitFromUnitTable(batchNum);

    if (!unit.isEmpty()) {
        ui->outputUnit->setText(unit);
    } else {
        ui->outputUnit->setText(tr("未知单位"));
    }

    // 如果勾选了operation选项，从TrailDatabase读取操作详情
    if (ui->operation->isChecked()) {
        loadOperationData(batchNum);
    }
}

// 批号是否已存在的判断函数
bool PageAnalysis::reportExists(const QString &batchNum)
{
    sqlite3* db = DatabaseManager::instance()
    .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        return false;
    }
    const char *sql =
        "SELECT 1 FROM report WHERE number = ? LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1,batchNum.toUtf8().constData(),-1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = true;
        }
    }
    sqlite3_finalize(stmt);
    return exists;
}

// 检查单位是否一致（或不存在）
bool PageAnalysis::checkUnitConsistency(const QString &batchNum, const QString &currentUnit)
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接历史数据库";
        return false;
    }
    // 查询 unit 表中该批号的现有单位
    QString selectUnitSql = QString("SELECT unit FROM unit WHERE number = '%1'").arg(batchNum);
    sqlite3_stmt* unitStmt = nullptr;
    QString existingUnit;
    bool unitExists = false;
    if (sqlite3_prepare_v2(db, selectUnitSql.toUtf8(), -1, &unitStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(unitStmt) == SQLITE_ROW) {
            const char* unitText = reinterpret_cast<const char*>(sqlite3_column_text(unitStmt, 0));
            existingUnit = unitText ? QString(unitText) : "";
            unitExists = true;
        }
        sqlite3_finalize(unitStmt);
    }
    if (unitExists) {
        if (existingUnit != currentUnit) {
            // 单位不一致，提示并返回 false
            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("*警告"), tr("两次保存压力单位不一致"));
            return false;
        } else {
            // 单位一致，返回 true
            return true;
        }
    } else {
        // 单位不存在（新批号），返回 true
        return true;
    }
}

inline double round1(double v)
{
    return std::round(v * 10.0) / 10.0;
}

// reportBtn点击事件 - 从UI获取值并保存到report表，generateReport
void PageAnalysis::reportBtnClicked()
{
    // QString batchNum = ui->selectNumberRange->currentText();
    QString batchNum = SessionManager::instance().batchNumber();
    QString currentUnit = SensorDataProvider::instance()->pressureUnit();

    // 先检查单位一致性
    if (!checkUnitConsistency(batchNum, currentUnit)) {
        return;  // 不一致，已提示，不继续
    }

    // 再判断 batch 是否已存在
    if (reportExists(batchNum)) {
        auto *msg = new OverlayMessage(this);
        msg->setShowCancelButton(true);
        msg->showMessage(tr("*警告"), tr("批号%1（单位：%2）已经存在，是否覆盖？").arg(batchNum).arg(currentUnit));
        QObject::connect(msg, &OverlayMessage::closed, msg, [=]() {
            // 用户确认 → 执行生成报表
            generateReport(batchNum);
        });
        QObject::connect(msg, &OverlayMessage::cancelled, msg, []() {
            // 用户取消 → 不做任何事
        });
        return;
    }
    // 不存在，直接生成
    generateReport(batchNum);
}

//根据当前批号生成报表到数据库函数
void PageAnalysis::generateReport(const QString &batchNum)
{
    auto &ts = ThresholdSettings::instance();

    QMap<QString, QPair<double, double>> alarmLimits = {
        {"SENSOR1", {ts.alarmUpper(1), ts.alarmLower(1)}},
        {"SENSOR2", {ts.alarmUpper(2), ts.alarmLower(2)}},
        {"SENSOR3", {ts.alarmUpper(3), ts.alarmLower(3)}},
        {"SENSOR4", {ts.alarmUpper(4), ts.alarmLower(4)}}
    };

    if (!createReportTable()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("警告"), tr("生成表失败！"));
        return;
    }

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "[analysis] 无法连接历史数据库";
        return;
    }

    QMap<QString, QMap<QString, QVariant>> sensorDataForReport;
    QStringList sensors = {"SENSOR1", "SENSOR2", "SENSOR3", "SENSOR4"};

    for (const QString &sensor : sensors) {

        float alarmUpper = alarmLimits[sensor].first;
        float alarmLower = alarmLimits[sensor].second;

        //  bind 参数
        QString query =
            QString("SELECT %1, historytime FROM h_table "
                    "WHERE number = ? AND %1 IS NOT NULL AND %1 != '' "
                    "AND CAST(%1 AS REAL) IS NOT NULL "
                    "ORDER BY historytime ASC").arg(sensor);

        sqlite3_stmt* stmt = nullptr;
        QVector<float> values;

        if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {

            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                float value = static_cast<float>(sqlite3_column_double(stmt, 0));
                values.append(value);
            }
        }
        sqlite3_finalize(stmt);

        float minVal = 0, maxVal = 0, avgVal = 0;
        int alarmCount = 0;

        if (!values.isEmpty()) {

            minVal = maxVal = values[0];
            float sum = 0;

            bool inAlarm = false;

            for (float v : values) {

                minVal = qMin(minVal, v);
                maxVal = qMax(maxVal, v);
                sum += v;

                bool outOfRange = (v < alarmLower || v > alarmUpper);

                // 只统计“进入报警”
                if (outOfRange && !inAlarm) {
                    alarmCount++;
                    inAlarm = true;
                }
                else if (!outOfRange) {
                    inAlarm = false;
                }
            }

            avgVal = sum / values.size();
        }

        // 全部用数值类型（不要转 QString）
        QMap<QString, QVariant> sensorData;
        sensorData["min_value"] = round1(minVal);
        sensorData["max_value"] = round1(maxVal);
        sensorData["avg_value"] = round1(avgVal);
        sensorData["alarm_upper"] = alarmUpper;
        sensorData["alarm_lower"] = alarmLower;
        sensorData["alarm_count"] = alarmCount;   // 0 或 N

        sensorDataForReport[sensor] = sensorData;
    }

    if (!saveToReportTable(batchNum, sensorDataForReport)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("更新报表"), tr("报表更新失败"));
    } else {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("更新报表"), tr("报表已更新"));
        AuditLogger::instance().log(tr("*生成报告"));
    }
}

// 创建report表和unit表：如果表不存在则创建
bool PageAnalysis::createReportTable()
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接历史数据库";
        return false;
    }
    const char* reportTableSql = "CREATE TABLE IF NOT EXISTS report ("
                                 "sname TEXT,"
                                 "max REAL,"
                                 "min REAL,"
                                 "avg REAL,"
                                 "upLimit REAL,"
                                 "lowLimit REAL,"
                                 "number TEXT,"
                                 "startTime TEXT,"
                                 "endTime TEXT,"
                                 "alarmCount INTEGER"
                                 ");";
    const char* unitTableSql = "CREATE TABLE IF NOT EXISTS unit ("
                               "number TEXT PRIMARY KEY,"
                               "unit TEXT"
                               ");";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, reportTableSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "创建report表失败:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    rc = sqlite3_exec(db, unitTableSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "创建unit表失败:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    qDebug() << "成功创建report表和unit表";
    return true;
}

// 保存数据到report表，并更新unit表
bool PageAnalysis::saveToReportTable(const QString& batchNum,
                                     const QMap<QString, QMap<QString, QVariant>>& sensorData)
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接历史数据库";
        return false;
    }

    char* errMsg = nullptr;
    int rc;

    // ✅ 开启事务
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "开启事务失败:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    bool success = true;

    // ================= 时间范围 =================
    QPair<QString, QString> timeRange = getBatchTimeRange(batchNum);
    QString startTime = timeRange.first;
    QString endTime = timeRange.second;

    if (startTime.isEmpty() || endTime.isEmpty()) {
        QString fallbackQuery =
            "SELECT historytime FROM h_table "
            "WHERE number = ? AND historytime IS NOT NULL AND historytime != '' LIMIT 1";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, fallbackQuery.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {

            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* fallbackTime =
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (fallbackTime) {
                    startTime = fallbackTime;
                    endTime = startTime;
                }
            }
        }
        sqlite3_finalize(stmt);

        if (startTime.isEmpty()) {
            startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            endTime = startTime;
        }
    }

    // ================= 删除旧数据 =================
    {
        sqlite3_stmt* stmt = nullptr;
        QString sql = "DELETE FROM report WHERE number = ?";

        if (sqlite3_prepare_v2(db, sql.toUtf8(), -1, &stmt, nullptr) != SQLITE_OK) {
            qDebug() << "prepare DELETE failed:" << sqlite3_errmsg(db);
            success = false;
        } else {
            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                qDebug() << "删除旧记录失败:" << sqlite3_errmsg(db);
                success = false;
            }
        }
        sqlite3_finalize(stmt);
    }

    // ================= unit 表 =================
    QString currentUnit = SensorDataProvider::instance()->pressureUnit();
    {
        sqlite3_stmt* stmt = nullptr;
        QString sql = "INSERT OR IGNORE INTO unit (number, unit) VALUES (?, ?)";

        if (sqlite3_prepare_v2(db, sql.toUtf8(), -1, &stmt, nullptr) != SQLITE_OK) {
            qDebug() << "prepare unit failed:" << sqlite3_errmsg(db);
            success = false;
        } else {
            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, currentUnit.toUtf8(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                qDebug() << "写入 unit 表失败:" << sqlite3_errmsg(db);
                success = false;
            }
        }
        sqlite3_finalize(stmt);
    }

    // ================= 插入 report =================
    sqlite3_stmt* insertStmt = nullptr;
    QString insertSql =
        "INSERT INTO report "
        "(sname, max, min, avg, upLimit, lowLimit, number, startTime, endTime, alarmCount) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, insertSql.toUtf8(), -1, &insertStmt, nullptr) != SQLITE_OK) {
        qDebug() << "prepare INSERT failed:" << sqlite3_errmsg(db);
        success = false;
    } else {

        for (auto it = sensorData.begin(); it != sensorData.end(); ++it) {

            QString sensorName = it.key();
            const auto& data = it.value();

            sqlite3_bind_text(insertStmt, 1, sensorName.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(insertStmt, 2, data["max_value"].toDouble());
            sqlite3_bind_double(insertStmt, 3, data["min_value"].toDouble());
            sqlite3_bind_double(insertStmt, 4, data["avg_value"].toDouble());
            sqlite3_bind_double(insertStmt, 5, data["alarm_upper"].toDouble());
            sqlite3_bind_double(insertStmt, 6, data["alarm_lower"].toDouble());
            sqlite3_bind_text(insertStmt, 7, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStmt, 8, startTime.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStmt, 9, endTime.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insertStmt, 10, data["alarm_count"].toInt());
            if (sqlite3_step(insertStmt) != SQLITE_DONE) {
                qDebug() << "插入 report 失败:" << sqlite3_errmsg(db);
                success = false;
                break;
            }

            sqlite3_reset(insertStmt);   // 复用 statement
            sqlite3_clear_bindings(insertStmt);
        }
    }
    sqlite3_finalize(insertStmt);

    // ================= 提交 / 回滚 =================
    if (success) {
        rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            qDebug() << "提交事务失败:" << errMsg;
            sqlite3_free(errMsg);
            return false;
        }

        qDebug() << "报表保存成功，批号:" << batchNum;
        qDebug() << "时间范围:" << startTime << "->" << endTime;
        qDebug() << "单位:" << currentUnit;

    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        qDebug() << "事务回滚（保存失败）";
    }

    return success;
}

// 从TrailDatabase加载操作详情数据：查询t_table获取指定批号的操作记录
void PageAnalysis::loadOperationData(const QString& batchNum)
{
    qDebug() << "开始加载操作详情数据，批号:" << batchNum;
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::TrailDatabase)) {
        qDebug() << "无法打开TrailDatabase";
        QMessageBox::warning(this, tr("错误"), tr("无法打开审计追踪数据库！"));
        return;
    }
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qDebug() << "无法连接TrailDatabase";
        QMessageBox::warning(this, tr("错误"), tr("无法打开审计追踪数据库！"));
        return;
    }
    qDebug() << "成功连接到TrailDatabase";
    QString query = QString(
                        "SELECT trailtime, operation FROM t_table "
                        "WHERE number = '%1' ORDER BY trailtime")
                        .arg(batchNum);
    // qDebug() << "执行查询:" << query;
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qDebug() << "查询操作详情失败:" << sqlite3_errmsg(db);
        QMessageBox::warning(this, tr("错误"), tr("查询失败: %1").arg(sqlite3_errmsg(db)));
        return;
    }
    qDebug() << "查询准备成功";
    ui->trailTable->setRowCount(0);
    int row = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->trailTable->insertRow(row);
        const char* timeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        QString timeValue = timeText ? QString(timeText) : "";
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeValue);
        timeItem->setTextAlignment(Qt::AlignCenter);
        ui->trailTable->setItem(row, 0, timeItem);
        const char* opText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        QString opValue = opText ? QString(opText) : "";
        QTableWidgetItem* opItem = new QTableWidgetItem(opValue);
        opItem->setTextAlignment(Qt::AlignCenter);
        ui->trailTable->setItem(row, 1, opItem);
        row++;
    }
    sqlite3_finalize(stmt);
    qDebug() << "从TrailDatabase加载了" << row << "条操作记录";
    if (row == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("未找到该批号的操作记录"));
    }
}

/************************UI图表部分********************************/

// 全选
void PageAnalysis::selectAllCheckBox()
{
    // 获取全选checkbox的当前状态
    bool isChecked = ui->checkBoxAll->isChecked();

    // 根据状态设置所有其他checkbox
    if (isChecked) {
        // 勾选所有其他checkbox
        ui->alarm->setChecked(true);
        ui->alarmLowLimit->setChecked(true);
        ui->alarmUpLimit->setChecked(true);
        ui->avg->setChecked(true);
        ui->charts->setChecked(true);
        ui->max->setChecked(true);
        ui->min->setChecked(true);
        ui->operation->setChecked(true);
        ui->sensor1->setChecked(true);
        ui->sensor2->setChecked(true);
        ui->sensor3->setChecked(true);
        ui->sensor4->setChecked(true);
        ui->endTime->setChecked(true);
        ui->startTime->setChecked(true);

    } else {
        // 取消勾选所有其他checkbox
        ui->alarm->setChecked(false);
        ui->alarmLowLimit->setChecked(false);
        ui->alarmUpLimit->setChecked(false);
        ui->avg->setChecked(false);
        ui->charts->setChecked(false);
        ui->max->setChecked(false);
        ui->min->setChecked(false);
        ui->operation->setChecked(false);
        ui->sensor1->setChecked(false);
        ui->sensor2->setChecked(false);
        ui->sensor3->setChecked(false);
        ui->sensor4->setChecked(false);
        ui->endTime->setChecked(false);
        ui->startTime->setChecked(false);
    }
}
// 初始化图表，只初始化一次
void PageAnalysis::setupChart()
{
    // 创建图表
    chart = new QChart();
    chart->setMargins(QMargins(0, 0, 0, 0));// 设置图表的边距为0，减少空白
    chart->setTitle(tr("压力变化趋势图"));

    // 增大标题字体
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(10);  // 增大标题字体
    titleFont.setBold(false);
    chart->setTitleFont(titleFont);

    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // 各系列创建，设置不同颜色
    series1 = new QLineSeries();
    series1->setName(tr("SENSOR1"));
    series2 = new QLineSeries();
    series2->setName(tr("SENSOR2"));
    series3 = new QLineSeries();
    series3->setName(tr("SENSOR3"));
    series4 = new QLineSeries();
    series4->setName(tr("SENSOR4"));
    // 添加到图表
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(series3);
    chart->addSeries(series4);
    // 绘制空图表
    buildEmptyChart();

    // QChartView
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    if (!ui->chartWidget->layout()) {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartWidget);
        layout->setContentsMargins(0,0,0,0);
    }
    ui->chartWidget->layout()->addWidget(chartView);

    // 信号连接
    connect(ui->sensor1, &QCheckBox::stateChanged, this, &PageAnalysis::updateChart);
    connect(ui->sensor2, &QCheckBox::stateChanged, this, &PageAnalysis::updateChart);
    connect(ui->sensor3, &QCheckBox::stateChanged, this, &PageAnalysis::updateChart);
    connect(ui->sensor4, &QCheckBox::stateChanged, this, &PageAnalysis::updateChart);
    connect(ui->selectNumberRange, SIGNAL(currentTextChanged(QString)),this, SLOT(updateChart()));
}

// 设置UI坐标轴
void PageAnalysis::buildEmptyChart()
{
    // 删除原坐标轴
    QList<QAbstractAxis*> axes = chart->axes();
    for (QAbstractAxis *ax : axes) {
        chart->removeAxis(ax);
        delete ax;
    }

    // 创建 X 轴
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setTitleText(tr("时间"));
    axisX->setFormat("MM-dd HH:mm");
    axisX->setTitleVisible(true);  // 确保标题可见

    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setBold(false);

    QFont axisLabelsFont;
    axisLabelsFont.setPointSize(8);

    axisX->setTitleFont(axisTitleFont);
    axisX->setLabelsFont(axisLabelsFont);

    // 默认最近 24 小时
    auto now = QDateTime::currentDateTime();
    axisX->setRange(now.addDays(-1), now);
    chart->addAxis(axisX, Qt::AlignBottom);

    // 创建 Y 轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(tr("压力"));
    axisY->setTitleVisible(true);  // 确保标题可见

    // 增大Y轴字体
    axisY->setTitleFont(axisTitleFont);
    axisY->setLabelsFont(axisLabelsFont);

    axisY->setRange(-1500, 4500);
    chart->addAxis(axisY, Qt::AlignLeft);

    // 设置图表边距，确保标题有足够空间显示
    chart->setMargins(QMargins(5, 5, 5, 5));
    chart->setContentsMargins(-1, -1, -1, -1);  // 减少内容边距

    // 将系列附加到坐标轴
    for (QAbstractSeries *s : chart->series()) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }
}

QVector<QPointF> PageAnalysis::adaptiveSampling(const QVector<QPointF>& data, int maxPoints = 1500)
{
    if (data.size() <= maxPoints) return data;

    if (data.isEmpty()) return {};

    // 计算y范围
    double minY = data[0].y();
    double maxY = data[0].y();
    for (const auto& p : data) {
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }
    double yRange = maxY - minY;
    if (yRange <= 0) return data;  // 常量数据，无需简化

    // 初始epsilon：y范围的百分比，min下限避免太小
    double tolerance = 0.04;  // 调整这个值测试效果，0.001更精细（更多点），0.01更粗糙（更少点）
    double epsilon = qMax(0.1, yRange * tolerance);  // 例如压力范围10，epsilon=0.05；范围1000，epsilon=5

    QVector<QPointF> result;
    result.append(data.first());

    // 递归简化
    douglasPeucker(data, 0, data.size() - 1, epsilon, result);

    result.append(data.last());

    // 如果仍超，增大epsilon并重试
    int iteration = 0;
    while (result.size() > maxPoints && iteration < 10) {  // 限迭代避免死循环
        epsilon *= 1.5;
        result.clear();
        result.append(data.first());
        douglasPeucker(data, 0, data.size() - 1, epsilon, result);
        result.append(data.last());
        ++iteration;
    }

    // 如果最终仍超（极罕见），强制均匀采样
    if (result.size() > maxPoints) {
        int step = data.size() / maxPoints;
        if (step < 1) step = 1;
        result.clear();
        for (int i = 0; i < data.size(); i += step) {
            result.append(data[i]);
        }
        if (!result.contains(data.last())) result.append(data.last());
    }

    // qDebug() << "采样后数据点数:" << result.size() << "(原始:" << data.size() << ") epsilon:" << epsilon;
    return result;
}

// 更新图表
void PageAnalysis::updateChart()
{
    QString batch = ui->selectNumberRange->currentText();
    if (batch.isEmpty() || batch == tr("请选择批号"))
        return;

    // 清空数据
    series1->clear();
    series2->clear();
    series3->clear();
    series4->clear();

    QVector<QPointF> s1, s2, s3, s4;
    bool used = false;

    if (ui->sensor1->isChecked()) {
        s1 = adaptiveSampling(getSensorData("SENSOR1", batch), 1000);
        used |= !s1.isEmpty();
    }
    if (ui->sensor2->isChecked()) {
        s2 = adaptiveSampling(getSensorData("SENSOR2", batch), 1000);
        used |= !s2.isEmpty();
    }
    if (ui->sensor3->isChecked()) {
        s3 = adaptiveSampling(getSensorData("SENSOR3", batch), 1000);
        used |= !s3.isEmpty();
    }
    if (ui->sensor4->isChecked()) {
        s4 = adaptiveSampling(getSensorData("SENSOR4", batch), 1000);
        used |= !s4.isEmpty();
    }

    // 重新绘制
    drawBatchChart(s1, s2, s3, s4, used, batch);
}

// 获取数据
QVector<QPointF> PageAnalysis::getSensorData(const QString& sensorName, const QString& batchNum) {
    QVector<QPointF> data;
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);

    if (!db) {
        qDebug() << "无法连接历史数据库";
        return data;
    }

    // 查询指定批号和传感器的数据，按时间排序
    QString query = QString(
                        "SELECT historytime, %1 FROM h_table "
                        "WHERE number = '%2' AND %1 IS NOT NULL AND %1 != '' "
                        "AND historytime IS NOT NULL AND historytime != '' "
                        "ORDER BY historytime"
                        ).arg(sensorName).arg(batchNum);

    // qDebug() << "查询传感器数据:" << query;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // 获取时间
            const char* timeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (!timeText) continue;

            QString timeStr = QString(timeText);
            QDateTime dateTime;

            // 尝试多种时间格式
            dateTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
            if (!dateTime.isValid()) {
                dateTime = QDateTime::fromString(timeStr, "yyyy-MM-dd hh:mm:ss");
            }
            if (!dateTime.isValid()) {
                dateTime = QDateTime::fromString(timeStr, Qt::ISODate);
            }
            if (!dateTime.isValid()) {
                continue; // 如果时间格式仍然无效，跳过此记录
            }

            // 获取传感器值
            int columnType = sqlite3_column_type(stmt, 1);
            double value = 0.0;
            switch (columnType) {
            case SQLITE_FLOAT:
                value = sqlite3_column_double(stmt, 1);
                break;
            case SQLITE_INTEGER:
                value = static_cast<double>(sqlite3_column_int(stmt, 1));
                break;
            case SQLITE_TEXT: {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                if (text) {
                    bool ok;
                    value = QString(text).toDouble(&ok);
                    if (!ok) continue;
                }
                break;
            }
            default:
                continue;
            }

            // 添加到数据向量
            data.append(QPointF(dateTime.toMSecsSinceEpoch(), value));
        }
        sqlite3_finalize(stmt);
    } else {
        qDebug() << "查询传感器数据失败:" << sqlite3_errmsg(db);
    }

    // qDebug() << "获取到" << sensorName << "数据点数:" << data.size();
    return data;
}

// 分批筛选
void PageAnalysis::drawBatchChart(const QVector<QPointF>& s1,const QVector<QPointF>& s2,const QVector<QPointF>& s3,const QVector<QPointF>& s4,
                                  bool hasData,const QString &batch)
{
    if (!hasData) {
        buildEmptyChart();
        chart->setTitle(tr("压力趋势(无数据)"));
        return;
    }

    // 检查并确保坐标轴存在
    if (chart->axes(Qt::Horizontal).isEmpty() || chart->axes(Qt::Vertical).isEmpty()) {
        buildEmptyChart(); // 如果坐标轴不存在，重新创建
    }

    // 添加点
    for (auto &p : s1) series1->append(p);
    for (auto &p : s2) series2->append(p);
    for (auto &p : s3) series3->append(p);
    for (auto &p : s4) series4->append(p);
    // 计算 XY 范围
    double minX=1e20, maxX=-1e20, minY=1e20, maxY=-1e20;

    auto calcRange = [&](const QVector<QPointF>& v){
        for (auto &p : v) {
            minX = qMin(minX, p.x());
            maxX = qMax(maxX, p.x());
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
    };
    calcRange(s1); calcRange(s2); calcRange(s3);calcRange(s4);

    // 安全获取坐标轴
    QDateTimeAxis *axisX = nullptr;
    QValueAxis *axisY = nullptr;

    if (!chart->axes(Qt::Horizontal).isEmpty()) {
        axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
    }
    if (!chart->axes(Qt::Vertical).isEmpty()) {
        axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
    }

    // 如果坐标轴获取失败，重新创建
    if (!axisX || !axisY) {
        buildEmptyChart();
        axisX = qobject_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal).first());
        axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
    }

    if (axisX && axisY) {
        // Y 辅助范围
        double yr = maxY - minY;
        if (yr < 1) yr = 100;
        minY -= yr * 0.1;
        maxY += yr * 0.1;

        // X 辅助范围
        double xr = maxX - minX;
        if (xr < 1000)
            xr = 3600000; // 至少 1 小时
        minX -= xr * 0.05;
        maxX += xr * 0.05;

        axisX->setRange(QDateTime::fromMSecsSinceEpoch(minX),
                        QDateTime::fromMSecsSinceEpoch(maxX));
        axisY->setRange(minY, maxY);
    }

    // 更新标题
    chart->setTitle(tr("传感器数据 - 批号：%1").arg(batch));
}

/*************************************打印上下文（统一 DPI / 缩放 / 尺寸）********************************************/

// 打印报告内容：绘制标题、批号、操作人员、输出时间、数据表和操作记录
void PageAnalysis::printReportContent(PrintContext& ctx)
{
    QPainter* painter = ctx.painter;

    int x = ctx.margin;
    int y = ctx.margin;

    // ===== 主标题 =====
    painter->setFont(ctx.titleFont);
    painter->drawText(
        QRect(0, y, ctx.pageRect.width(), ctx.headerHeight),
        Qt::AlignCenter,
        ui->titleEdit->text()
        );
    y += 100;

    // =====设备编号区域 =====
    painter->setFont(ctx.smallFont);

    painter->drawText(
        QRect(0, y, ctx.pageRect.width(), ctx.subHeaderHeight),
        Qt::AlignCenter,
        tr("设备编号：") + QStringLiteral("WYWPS-2X7ZB")
        );
    y += 50;

    ////////////////////===== 文本区域 =====///////////////////////
    // ===== 批号 / 单位 =====
    painter->setFont(ctx.textFont);

    painter->drawText(x, y,tr("批次: %1").arg(ui->batchNumTimeText->text()));

    QString reportUnit = getUnitFromUnitTable(ui->selectNumberRange->currentText());
    painter->drawText(ctx.pageRect.width() * 0.7, y,tr("当前单位: %1").arg(reportUnit));
    y += 50;

    painter->drawText(x, y,tr("操作人员: %1").arg(ui->operationUser->text()));
    painter->drawText(ctx.pageRect.width() * 0.7, y,tr("报告输出时间: %1").arg(ui->outputTime->text()));
    y += 50;

    // ===== 数据表格 =====
    y = drawTable(ctx, ui->dataTable, x, y);
    y += 40;

    // ===== 图表 =====
    if (ui->charts->isChecked())
        y = printCharts(ctx, x, y);

    y += 100;
    // ===== 操作记录 =====
    if (ui->operation->isChecked())
        y = drawTable(ctx, ui->trailTable, x, y);
}


// 初始化打印机列表：获取系统可用打印机并添加到下拉框
void PageAnalysis::initPrinterList()
{
    ui->exToPrinterComb->clear();

    QList<QPrinterInfo> printerList = QPrinterInfo::availablePrinters();

    // 虚拟打印机常见名称关键词
    // QStringList virtualPrinterKeywords = {
    //     "PDF", "pdf", "XPS", "OneNote", "Microsoft Print to PDF", "Microsoft XPS Document Writer",
    //     "Adobe PDF", "Foxit Reader PDF Printer", "WPS PDF", "Fax", "Send To OneNote", "CutePDF",
    //     "doPDF", "Bullzip PDF Printer", "PDFCreator", "Adobe", "Foxit", "WPS", "Virtual", "virtual"
    // };

    QStringList virtualPrinterKeywords = {
        "PDF", "pdf", "XPS", "OneNote", "Microsoft Print to PDF", "Microsoft XPS Document Writer",
        "Adobe PDF", "Foxit Reader PDF Printer", "WPS PDF", "Fax", "Send To OneNote", "CutePDF",
        "doPDF", "Bullzip PDF Printer", "PDFCreator", "Adobe", "Foxit", "WPS", "Virtual", "virtual"
    };
    for (const QPrinterInfo &info : printerList) {
        QString printerName = info.printerName();
        QString description = info.description();

        // 过滤虚拟打印机
        bool isVirtual = false;
        for (const QString &keyword : virtualPrinterKeywords) {
            if (printerName.contains(keyword, Qt::CaseInsensitive) ||
                description.contains(keyword, Qt::CaseInsensitive)) {
                isVirtual = true;
                break;
            }
        }

        // 排除虚拟打印机
        if (!isVirtual) {
            ui->exToPrinterComb->addItem(printerName);
        }
    }
}

///////////////////////打印输出table部分/////////////////////

// 计算宽度总和：计算指定索引前所有列宽的总和
int PageAnalysis::sumWidths(const QVector<int> &w, int idx)
{
    int s = 0;  // 累计宽度
    for (int i=0; i<idx; i++)
        s += w[i];  // 累加指定索引前的所有列宽
    return s;
}

// 绘制报表：绘制指定的表格到打印机，处理自动分页
int PageAnalysis::drawTable(PrintContext& ctx, QTableWidget* table, int x, int y)
{
    if (!table || table->rowCount() == 0)
        return y;

    int rowH = ctx.tableRowHeight;
    int colCount = table->columnCount();

    QVector<int> colWidths(colCount, (ctx.pageRect.width() - 2*x) / colCount);

    // === 表头 ===
    ctx.painter->setFont(ctx.headerFont);
    for (int c = 0; c < colCount; ++c) {
        QRect r(x + sumWidths(colWidths, c), y, colWidths[c], rowH);
        ctx.painter->drawRect(r);
        ctx.painter->drawText(r, Qt::AlignCenter, table->horizontalHeaderItem(c)->text());
    }
    y += rowH;

    // === 行内容 ===
    ctx.painter->setFont(ctx.bodyFont);
    for (int r = 0; r < table->rowCount(); ++r)
    {
        if (y + rowH > ctx.pageRect.height() - 80)
        {
            ctx.device->newPage();
            y = ctx.margin;

            // 重绘表头
            ctx.painter->setFont(ctx.headerFont);
            for (int c = 0; c < colCount; ++c) {
                QRect rct(x + sumWidths(colWidths, c), y, colWidths[c], rowH);
                ctx.painter->drawRect(rct);
                ctx.painter->drawText(rct, Qt::AlignCenter, table->horizontalHeaderItem(c)->text());
            }
            y += rowH;
            ctx.painter->setFont(ctx.bodyFont);
        }

        for (int c = 0; c < colCount; ++c)
        {
            QRect rct(x + sumWidths(colWidths, c), y, colWidths[c], rowH);
            ctx.painter->drawRect(rct);
            auto* item = table->item(r, c);
            ctx.painter->drawText(rct, Qt::AlignCenter, item ? item->text() : "");
        }
        y += rowH;
    }

    return y;
}

///////////////////////打印输出chart部分/////////////////////

//设置打印输出图表
QChart* PageAnalysis::buildPrintChart(QLineSeries* sourceSeries,const QString& title,const PrintContext& ctx)
{
    QChart *chart = new QChart();
    chart->setTitle(title);
    chart->legend()->hide();

    QFont titleFont;
    titleFont.setPointSizeF(22);
    titleFont.setBold(false);
    chart->setTitleFont(titleFont);

    QLineSeries *s = new QLineSeries();
    s->setName(sourceSeries->name());

    QPen pen = sourceSeries->pen();
    pen.setWidth(qMax(1, ctx.s(5)));
    s->setPen(pen);
    s->append(sourceSeries->points());
    chart->addSeries(s);

    auto *axisX = new QDateTimeAxis();
    axisX->setTitleText(tr("时间"));
    axisX->setFormat("MM-dd HH:mm");

    auto *axisY = new QValueAxis();
    axisY->setTitleText(tr("压力值"));

    QPen gridPen(Qt::gray);
    gridPen.setWidth(ctx.s(2));          // 网格线粗一点
    gridPen.setStyle(Qt::DashLine);      // 虚线，替换成 DashDot / DotLine 都行

    axisX->setGridLinePen(gridPen);
    axisY->setGridLinePen(gridPen);

    QFont axisTitleFont;
    axisTitleFont.setPointSizeF(18);
    axisTitleFont.setBold(false);

    QFont axisLabelFont;
    axisLabelFont.setPointSizeF(18);

    axisX->setTitleFont(axisTitleFont);
    axisY->setTitleFont(axisTitleFont);
    axisX->setLabelsFont(axisLabelFont);
    axisY->setLabelsFont(axisLabelFont);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    s->attachAxis(axisX);
    s->attachAxis(axisY);

    if (!s->points().isEmpty()) {
        auto pts = s->points();

        double minX = pts.first().x();
        double maxX = pts.last().x();
        double minY = pts.first().y();
        double maxY = minY;

        for (const auto& p : pts) {
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }

        double yRange = qMax(100.0, maxY - minY);
        double yMargin = yRange * 0.1;

        double xRange = qMax(3600000.0, maxX - minX);
        double xMargin = xRange * 0.01;                 //x轴左右各留1%空位

        axisX->setRange(
            QDateTime::fromMSecsSinceEpoch(minX - xMargin),
            QDateTime::fromMSecsSinceEpoch(maxX + xMargin)
            );
        axisY->setRange(minY - yMargin, maxY + yMargin);

        axisY->setTickCount(8);
        axisX->setTickCount(14);
    }

    chart->setMargins(QMargins(8, 0, 8, 0));


    return chart;
}

// 打印图表部分,图表间距
int PageAnalysis::printCharts(PrintContext& ctx, int xMargin, int y)
{
    int chartWidth = ctx.pageRect.width() - 2 * xMargin;
    int chartHeight = ctx.chartHeight;

    auto printOne = [&](QLineSeries* series, const QString& title)
    {
        if (!series || series->points().isEmpty())
            return;

        QChart* chart = buildPrintChart(series, title, ctx);

        QChartView view(chart);
        view.setRenderHint(QPainter::Antialiasing);
        view.resize(chartWidth, chartHeight);

        view.grab(); // 强制布局

        QRectF target(xMargin, y, chartWidth, chartHeight);
        view.render(ctx.painter, target);

        y += chartHeight + ctx.chartSpacing;
    };

    if (ui->sensor1->isChecked()) printOne(series1, "SENSOR1");
    if (ui->sensor2->isChecked()) printOne(series2, "SENSOR2");
    if (ui->sensor3->isChecked()) printOne(series3, "SENSOR3");
    if (ui->sensor4->isChecked()) printOne(series4, "SENSOR4");

    return y;
}

// PDF输出
void PageAnalysis::on_exToPdfBtn_clicked()
{
    QString outputDir = reportOutputDir;
    QDir dir(outputDir);

    QString batchNum = ui->selectNumberRange->currentText();
    if (batchNum.isEmpty() || batchNum == tr("请选择批号")) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("输出PDF"), tr("请先选择批号"));
        return;
    }

    QString fileName = QString("%1_%2.pdf")
                           .arg(batchNum,
                                QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString filePath = dir.filePath(fileName);

    // ==== 关键：使用 QPdfWriter 取代 QPrinter ====
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        qWarning() << "PDF painter failed!";
        return;
    }

    PrintContext ctx;
    ctx.init(writer, painter);

    printReportContent(ctx);

    painter.end();

    auto *msgOK = new OverlayMessage(this);
    msgOK->showMessage(tr("输出PDF完成"), tr("PDF 保存到：\n%1").arg(filePath));

    AuditLogger::instance().log(tr("导出PDF报告"));
}


// 打印报告：打开打印对话框并执行打印操作
void PageAnalysis::printReport()
{
    QString printerName = ui->exToPrinterComb->currentText();
    if (printerName.isEmpty()) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("打印报告"), tr("请先选择打印机"));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setPrinterName(printerName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setResolution(300);

    QPainter painter(&printer);
    if (!painter.isActive()) {
        qWarning() << "Printer painter failed!";
        return;
    }

    PrintContext ctx;
    ctx.init(printer, painter);

    printReportContent(ctx);

    painter.end();

    AuditLogger::instance().log(tr("打印机%1打印报告").arg(printerName));
}


// 改变用户重新应用权限
void PageAnalysis::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态************************************************
void PageAnalysis::updateUIByPermission()
{
    // Admin权限控制的UI元素（需要Admin及以上权限）
    QList<QWidget*> AdminWidgets = {
        ui->reportBtn
    };

    // 设置Admin权限控件
    for (auto widget : AdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::Admin, widget);
    }

    // QA r权限控制的UI元素（需要QA及以上权限）
    QList<QWidget*> QAWidgets = {

    ui->selectNumberRange,ui->exToPrinterComb,ui->exToPdfBtn,ui->exToPrinterBtn,ui->hisReportBtn

    };

    // 设置Supervisor权限控件
    for (auto widget : QAWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::QA, widget);
    }
}

