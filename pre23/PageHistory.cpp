// PageHistory.cpp
#include "PageHistory.h"
#include "ui_PageHistory.h"
#include "OverlayMessage.h"
#include "ToPdf.h"
#include "AuditLogger.h"
#include "DatabaseManager.h"
#include "LanguageManager.h"
#include "GlobalDefines.h"
#include "PermissionManager.h"
#include <QDebug>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>

PageHistory::PageHistory(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageHistory)
{
    ui->setupUi(this);

    //下拉框变化触发重绘
    connect(ui->selectDateRange,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,&PageHistory::filterByTime);

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageHistory::onUserChanged);

    // 初始化UI权限状态
    updateUIByPermission();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageHistory::retranslateUi);

}

PageHistory::~PageHistory()
{
    delete ui;
}

void PageHistory::retranslateUi()
{
    ui->retranslateUi(this);
}

void PageHistory::init()
{
    // 1. 打开历史数据库
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::HistoryDatabase)) {
        qCritical() << "[历史数据] 打开历史数据库失败";
        qWarning() << "错误,无法打开历史数据库" << this;
        return;
    }

    setupTable();  // 初始化表格结构

    // 2. 设置默认时间范围（当前时间和一天前）
    QDateTime now = QDateTime::currentDateTime();
    QDateTime oneDayAgo = now.addDays(-1);

    // 设置默认开始时间
    ui->startYear->setText(oneDayAgo.toString("yyyy"));
    ui->startMon->setText(oneDayAgo.toString("MM"));
    ui->startDay->setText(oneDayAgo.toString("dd"));
    ui->startHour->setText(oneDayAgo.toString("HH"));
    ui->startMin->setText(oneDayAgo.toString("mm"));
    ui->startSec->setText(oneDayAgo.toString("ss"));

    // 设置默认结束时间
    ui->endYear->setText(now.toString("yyyy"));
    ui->endMon->setText(now.toString("MM"));
    ui->endDay->setText(now.toString("dd"));
    ui->endHour->setText(now.toString("HH"));
    ui->endMin->setText(now.toString("mm"));
    ui->endSec->setText(now.toString("ss"));

    // 3. 更新下拉框
    refreshUserCombos();
    refreshNumberCombos();

    // 4. 连接按钮信号槽
    connect(ui->hisFilterBtn, &QPushButton::clicked,this, &PageHistory::filterClicked,Qt::UniqueConnection);
    connect(ui->hisPdfBtn, &QPushButton::clicked,this, &PageHistory::exportPdfClicked,Qt::UniqueConnection);

    connect(ui->selectUserRange, &QComboBox::currentTextChanged,this, &PageHistory::filterByUser);
    connect(ui->selectNumberRange, &QComboBox::currentTextChanged,this, &PageHistory::filterByNumber);

    // 5. 加载最近数据
    loadRecentRows();
    // 从数据库读总行数，更新到 Label
    ui->dataRows->setText(QString::number(fetchTotalHistoryCount()));
}

void PageHistory::setupTable()
{
    if (!ui->historyTable) {
        qCritical() << "[历史数据] 表格组件未初始化";
        return;
    }

    // 设置表头
    QStringList headers = {tr("时间"), "SENSOR1", "SENSOR2","SENSOR3","SENSOR4",tr("批号"),tr("用户名"),tr("权限等级")};
    ui->historyTable->setColumnCount(headers.size());
    ui->historyTable->setHorizontalHeaderLabels(headers);

    // 设置列宽
    ui->historyTable->setColumnWidth(0, 250);  // 时间
    ui->historyTable->setColumnWidth(1, 130);  // SENSOR1
    ui->historyTable->setColumnWidth(2, 130);  // SENSOR2
    ui->historyTable->setColumnWidth(3, 130);  // SENSOR3
    ui->historyTable->setColumnWidth(4, 130);  // SENSOR4
    ui->historyTable->setColumnWidth(5, 200);  // 批号
    ui->historyTable->setColumnWidth(6, 200);  // 用户名
    ui->historyTable->horizontalHeader()->setStretchLastSection(true); // 最后权限等级一列自适应

    ui->historyTable->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical {"
        "    width: 30px;"       // 设置滚动条宽度
        "}"
        );
    ui->historyTable->horizontalScrollBar()->setStyleSheet(
        "QScrollBar:horizontal {"
        "    height: 30px;"      // 设置滚动条高度
        "}"
        );
    // 禁止编辑
    ui->historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 整行选中
    ui->historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

// 辅助函数：从输入框获取日期时间
QDateTime PageHistory::getDateTimeFromLineEdits(
    QLineEdit* year, QLineEdit* mon, QLineEdit* day,
    QLineEdit* hour, QLineEdit* min, QLineEdit* sec,
    bool* ok) const
{
    if (ok) *ok = false;
    bool conv;
    int y = year->text().toInt(&conv); if (!conv) return QDateTime();
    int m = mon->text().toInt(&conv); if (!conv) return QDateTime();
    int d = day->text().toInt(&conv); if (!conv) return QDateTime();
    int h = hour->text().toInt(&conv); if (!conv) return QDateTime();
    int mi = min->text().toInt(&conv); if (!conv) return QDateTime();
    int s = sec->text().toInt(&conv); if (!conv) return QDateTime();
    if (!QDate::isValid(y, m, d)) return QDateTime();
    QDate date(y, m, d);
    QTime time(h, mi, s);
    if (!date.isValid() || !time.isValid()) return QDateTime();
    if (ok) *ok = true;
    return QDateTime(date, time);
}

// 修改后的 loadRecentRows()
void PageHistory::loadRecentRows()
{
    // 获取历史数据库连接
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qCritical() << "[历史数据] 获取数据库连接失败";
        return;
    }
    // 查询最近30条记录
    const char *sql = "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
                      "FROM h_table ORDER BY datetime(historytime) DESC LIMIT 30";
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        qCritical() << "[历史数据] SQL准备失败：" << sqlite3_errmsg(db);
        return;
    }
    // 清空旧数据
    ui->historyTable->setRowCount(0);

    // 读取数据
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->historyTable->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->historyTable->setItem(row, col, item);
        }
        row++;
    }

    // 释放资源
    sqlite3_finalize(stmt);

    // qDebug() << "[历史数据] 加载了" << row << "条记录";
}

void PageHistory::filterClicked()
{
    bool startOk, endOk;
    QDateTime start = getDateTimeFromLineEdits(
        ui->startYear, ui->startMon, ui->startDay,
        ui->startHour, ui->startMin, ui->startSec, &startOk);

    QDateTime end;

    // 结束时间若空，设为当前时间
    if (ui->endYear->text().isEmpty() || ui->endMon->text().isEmpty() || ui->endDay->text().isEmpty())
    {
        end = QDateTime::currentDateTime();
        endOk = true;
    } else
    {
        end = getDateTimeFromLineEdits(
            ui->endYear, ui->endMon, ui->endDay,
            ui->endHour, ui->endMin, ui->endSec, &endOk);
    }
    if (!startOk)
    {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("警告"), tr("开始时间输入无效"));
        return;
    }
    if (!endOk)
    {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("警告"), tr("结束时间输入无效"));
        return;
    }
    if (start > end)
    {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("警告"), tr("开始时间不能晚于结束时间"));
        return;
    }

    loadRange(start, end);
}

//下方导出按钮导出PDF
void PageHistory::exportPdfClicked()
{
    // 获取时间范围
    bool startOk, endOk;
    QDateTime start = getDateTimeFromLineEdits(
        ui->startYear, ui->startMon, ui->startDay,
        ui->startHour, ui->startMin, ui->startSec, &startOk);

    QDateTime end;
    if (ui->endYear->text().isEmpty() || ui->endMon->text().isEmpty() || ui->endDay->text().isEmpty()) {
        end = QDateTime::currentDateTime();
        endOk = true;
    } else {
        end = getDateTimeFromLineEdits(
            ui->endYear, ui->endMon, ui->endDay,
            ui->endHour, ui->endMin, ui->endSec, &endOk);
    }

    if (!startOk || !endOk || start > end) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("警告"), tr("结束时间输入无效"));
        return;
    }

    // 创建一次性提示框
    auto *msg = new OverlayMessage(this);
    msg->setShowCancelButton(true);
    msg->showMessage(tr("导出PDF"), tr("是否要导出历史记录为 PDF？"));

    // 关键点：信号连接到 msg，自然不会累积！
    QObject::connect(msg, &OverlayMessage::closed, msg, [=]() {

        QString outputDir = historyOutputDir;

        QDir dir(outputDir);
        if (!dir.exists() && !dir.mkpath(outputDir)) {
            auto *err = new OverlayMessage(this);
            err->showMessage(tr("错误"), tr("无法创建目录：") + outputDir);
            return;
        }

        QString fileName = "history_" +
                           QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
                           ".pdf";

        QString filePath = outputDir + fileName;

        // 执行导出 PDF
        exportHisToPdf(start, end, filePath);

        QFileInfo fileInfo(filePath);
        if (fileInfo.exists() && fileInfo.size() > 0) {

            // 写审计日志
            QString op = QString(tr("导出历史数据：从 %1 到 %2"))
                             .arg(start.toString("yyyy-MM-dd HH:mm:ss"))
                             .arg(end.toString("yyyy-MM-dd HH:mm:ss"));

            AuditLogger::instance().log(op);

            auto *ok = new OverlayMessage(this);
            ok->showMessage(tr("成功"), tr("文件已保存到：historyData目录"));

        } else {
            auto *err = new OverlayMessage(this);
            err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
        }

    });

    // 用户取消（不做任何事）
    QObject::connect(msg, &OverlayMessage::cancelled, msg, []() {
        // nothing
    });
}

// 修改后的 loadRange()
void PageHistory::loadRange(const QDateTime &start, const QDateTime &end)
{
    // 获取历史数据库连接
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qCritical() << "[历史数据] 获取数据库连接失败";
        return;
    }

    // 参数化查询
    const char *sql = "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
                      "FROM 'h_table' "
                      "WHERE datetime(historytime) BETWEEN ? AND ? "
                      "ORDER BY datetime(historytime) DESC";
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[历史数据] SQL准备失败：" << sqlite3_errmsg(db);
        return;
    }

    // 绑定时间参数
    QString startStr = start.toString("yyyy-MM-dd HH:mm:ss");
    QString endStr = end.toString("yyyy-MM-dd HH:mm:ss");
    sqlite3_bind_text(stmt, 1, startStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 清空旧数据
    ui->historyTable->setRowCount(0);

    // 读取数据
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->historyTable->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->historyTable->setItem(row, col, item);
        }
        row++;
    }

    // 释放资源
    sqlite3_finalize(stmt);

    if (row == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("该时间段内无记录"));
    } /*else {
        qDebug() << "[历史数据] 筛选到" << row << "条记录";
    }*/
}

// 根据时间下拉框的文字来调整开始时间
QDateTime PageHistory::getStartTime(int index) const
{
    QDateTime now = QDateTime::currentDateTime();

    switch (index)
    {
    case 0: // 今天
        return QDateTime(now.date(), QTime(0, 0));

    case 1: // 过去1小时
        return now.addSecs(-3600);

    case 2: // 过去1天
        return now.addDays(-1);

    case 3: // 过去1周
        return now.addDays(-7);

    case 4: // 过去1月
        return now.addMonths(-1);

    default:
        return now.addDays(-1);
    }
}

// 更新用户的下拉框
void PageHistory::refreshUserCombos()
{
    ui->selectUserRange->clear();

    // 添加默认的提示选项
    ui->selectUserRange->addItem(tr("请选择用户"));

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    const char *sql = "SELECT DISTINCT username FROM h_table ORDER BY username ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        QString username = text ? QString(text) : "";
        // 排除空字符串和只包含空白字符的选项
        if (!username.trimmed().isEmpty()) {
            ui->selectUserRange->addItem(username);
        }
    }

    sqlite3_finalize(stmt);

    // 设置默认选中第一个选项（"请选择用户"）
    ui->selectUserRange->setCurrentIndex(0);
}

// 更新批号的下拉框
void PageHistory::refreshNumberCombos()
{
    ui->selectNumberRange->clear();

    // 添加默认的提示选项
    ui->selectNumberRange->addItem(tr("请选择批号"));

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    const char *sql = "SELECT DISTINCT number FROM h_table ORDER BY number ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        QString number = text ? QString(text) : "";

        // 排除空字符串和只包含空白字符的选项
        if (!number.trimmed().isEmpty()) {
            ui->selectNumberRange->addItem(number);
        }
    }

    sqlite3_finalize(stmt);
    // 设置默认选中第一个选项（"请选择批号"）
    ui->selectNumberRange->setCurrentIndex(0);
}

void PageHistory::filterByTime(int index)
{
    QDateTime start = getStartTime(index);
    QDateTime end = QDateTime::currentDateTime();

    loadRange(start, end);
}

// 根据用户的下拉框来筛选
void PageHistory::filterByUser(const QString &uname)
{
    if (uname.isEmpty()) return;

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    const char *sql =
        "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
        "FROM h_table WHERE username = ? ORDER BY datetime(historytime) DESC;";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    sqlite3_bind_text(stmt, 1, uname.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    ui->historyTable->setRowCount(0);

    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->historyTable->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->historyTable->setItem(row, col, item);
        }
        row++;
    }

    sqlite3_finalize(stmt);
}

// 根据批号的下拉框来筛选
void PageHistory::filterByNumber(const QString &number)
{
    if (number.isEmpty() || (ui->selectNumberRange->currentIndex() == 0)) return;

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    const char *sql =
        "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
        "FROM h_table WHERE number = ? ORDER BY datetime(historytime) DESC;";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    ui->historyTable->setRowCount(0);

    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->historyTable->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->historyTable->setItem(row, col, item);
        }
        row++;
    }

    sqlite3_finalize(stmt);
}

// 根据用户来导出PDF
void PageHistory::on_toPdfByUser_clicked()
{
    if (ui->selectUserRange->currentIndex() == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择一个用户"));
        return;
    }
    QString user = ui->selectUserRange->currentText();

    QString fileName = "history_" + user + ".pdf";

    QString filePath = historyOutputDir + fileName;

    if (filePath.isEmpty()) return;
    exportHisByUserToPdf(user, filePath);

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {

        // 写审计日志
        QString op = QString(tr("导出用户：%1历史数据")).arg(user);

        AuditLogger::instance().log(op);

        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：historyData目录"));

    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}

// 根据批号来导出PDF
void PageHistory::on_toPdfByNumber_clicked()
{
    if (ui->selectNumberRange->currentIndex() == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择一个批号"));
        return;
    }
    QString number = ui->selectNumberRange->currentText();

    QString fileName = "history_" + number + ".pdf";

    QString filePath = historyOutputDir + fileName;

    if (filePath.isEmpty()) return;
    exportHisByNumberToPdf(number, filePath);
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {

        // 写审计日志
        QString op = QString(tr("导出批号：%1历史数据")).arg(number);

        AuditLogger::instance().log(op);

        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：historyData目录"));

    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}

// 根据下拉框时间来导出PDF
void PageHistory::on_toPdfByTime_clicked()
{
    int index = ui->selectDateRange->currentIndex();

    // 未选择（第0项）
    if (index <= 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择一个时间范围"));
        return;
    }

    QDateTime start = getStartTime(index);
    QDateTime end = QDateTime::currentDateTime();

    QString fileName = "history_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
                       ".pdf";

    QString filePath = historyOutputDir + fileName;

    if (filePath.isEmpty()) return;

    exportHisToPdf(start, end, filePath);

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {

        QString op = QString(tr("导出历史数据：从 %1 到 %2"))
                         .arg(start.toString("yyyy-MM-dd HH:mm:ss"))
                         .arg(end.toString("yyyy-MM-dd HH:mm:ss"));

        AuditLogger::instance().log(op);

        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：historyData目录"));

    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}

// 取得 history 对应的数据库row数
int PageHistory::fetchTotalHistoryCount() const
{
    // 取得 history 对应的数据库连接
    sqlite3* db = DatabaseManager::instance()
                      .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return 0;

    const char* sql = "SELECT COUNT(*) FROM h_table;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

// 删除历史表中最早的1000条记录
void PageHistory::on_clearData_clicked()
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    int totalCount = fetchTotalHistoryCount();
    if (totalCount < 1000) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"),
                         QString(tr("当前数据不足 1000 条，无法执行删除。"))
                             .arg(totalCount));
        return;
    }

    const char* sql =
        "DELETE FROM h_table WHERE datetime(historytime) IN ("
        "    SELECT datetime(historytime) FROM h_table ORDER BY datetime(historytime) ASC LIMIT 1000"
        ");";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            auto* msg = new OverlayMessage(this);
            msg->setShowCancelButton(true);
            msg->showMessage(tr("警告"), tr("是否删除最早的 1000 条数据？\n注意：删除后不可恢复"));
            AuditLogger::instance().log(tr("删除最早的 1000 条历史数据"));
            ui->dataRows->setText(QString::number(fetchTotalHistoryCount()));
        } else {
            auto* msg = new OverlayMessage(this);
            msg->showMessage(tr("失败"), tr("删除历史数据失败！"));
        }
    }
    sqlite3_finalize(stmt);
}

// 改变用户重新应用权限
void PageHistory::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态************************************************
void PageHistory::updateUIByPermission()
{
    // 超级管理员权限控制的UI元素
    QList<QWidget*> superAdminWidgets = {
        ui->clearData
    };

    // 设置超级管理员权限控件
    for (auto widget : superAdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::SuperAdmin, widget);
    }

    // QA权限控制的UI元素（需要QA及以上权限）
    QList<QWidget*> QAWidgets = {
        ui->hisPdfBtn,ui->toPdfByNumber,ui->toPdfByTime,ui->toPdfByUser
    };

    // 设置Supervisor权限控件
    for (auto widget : QAWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::QA, widget);
    }
}
