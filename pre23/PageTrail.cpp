#include "PageTrail.h"
#include "ui_PageTrail.h"
#include "OverlayMessage.h"
#include "sqlite/sqlite3.h"
#include "ToPdf.h"
#include "AuditLogger.h"
#include "DatabaseManager.h"
#include "LanguageManager.h"
#include "GlobalDefines.h"
#include "PermissionManager.h"

#include <QtDebug>
#include <QFile>
#include <QScrollBar>
#include <QFileDialog>

PageTrail::PageTrail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageTrail)
{
    ui->setupUi(this);
    //下拉框变化触发重绘
    connect(ui->selectDateRange,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,&PageTrail::filterByTime);
    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageTrail::onUserChanged);

    // 初始化UI权限状态
    updateUIByPermission();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageTrail::retranslateUi);
}

PageTrail::~PageTrail()
{
    delete ui;
}
void PageTrail::retranslateUi()
{
    ui->retranslateUi(this);
}

void PageTrail::init()
{
    // 1. 打开轨迹数据库
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::TrailDatabase)) {
        qCritical() << "[审计追踪] 打开数据库失败";
        return;
    }
    qDebug() << "PageTrail init called";
    // 2. 初始化表格结构
    setupTable();

    // 3. 设置默认时间范围（当前时间和一天前）
    QDateTime now = QDateTime::currentDateTime();
    QDateTime oneDayAgo = now.addDays(-1);
    ui->startYear->setText(oneDayAgo.toString("yyyy"));
    ui->startMon->setText(oneDayAgo.toString("MM"));
    ui->startDay->setText(oneDayAgo.toString("dd"));
    ui->startHour->setText(oneDayAgo.toString("HH"));
    ui->startMin->setText(oneDayAgo.toString("mm"));
    ui->startSec->setText(oneDayAgo.toString("ss"));
    ui->endYear->setText(now.toString("yyyy"));
    ui->endMon->setText(now.toString("MM"));
    ui->endDay->setText(now.toString("dd"));
    ui->endHour->setText(now.toString("HH"));
    ui->endMin->setText(now.toString("mm"));
    ui->endSec->setText(now.toString("ss"));

    //4. 更新下拉框
    refreshUserCombos();
    refreshNumberCombos();

    // 5. 信号槽绑定
    connect(ui->trailFilterBtn, &QPushButton::clicked,this, &PageTrail::filterClicked,Qt::UniqueConnection);
    connect(ui->trailPdfBtn, &QPushButton::clicked,this, &PageTrail::exportPdfClicked,Qt::UniqueConnection);

    connect(ui->selectUserRange, &QComboBox::currentTextChanged,this, &PageTrail::filterByUser);
    connect(ui->selectNumberRange, &QComboBox::currentTextChanged,this, &PageTrail::filterByNumber);
    // 6. 加载最近数据
    loadRecentRows();
    // 从数据库读总行数，更新到 Label
    ui->dataRows->setText(QString::number(fetchTotalTrailCount()));

}

void PageTrail::setupTable()
{
    QStringList headers = {tr("时间"), tr("用户名"), tr("权限等级"), tr("批号"),tr("操作详情")};
    ui->trailTable->setColumnCount(headers.size());
    ui->trailTable->setHorizontalHeaderLabels(headers);
    ui->trailTable->setColumnWidth(0, 240);
    ui->trailTable->setColumnWidth(1, 130);
    ui->trailTable->setColumnWidth(2, 130);
    ui->trailTable->setColumnWidth(3, 130);
    ui->trailTable->horizontalHeader()->setStretchLastSection(true);
    ui->trailTable->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width: 30px; }");
    ui->trailTable->horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal { height: 30px; }");
    ui->trailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->trailTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

// 辅助函数：从输入框获取日期时间
QDateTime PageTrail::getDateTimeFromLineEdits(
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

void PageTrail::loadRecentRows()
{
    // 获取单例连接
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qCritical() << "[审计追踪] 获取数据库连接失败";
        return;
    }

    const char* sql =
        "SELECT trailtime, username, level, number, operation "
        "FROM t_table ORDER BY datetime(trailtime) DESC LIMIT 30;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[审计追踪] SQL 准备失败：" << sqlite3_errmsg(db);
        return;
    }

    ui->trailTable->setRowCount(0);
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->trailTable->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem* item = new QTableWidgetItem(txt ? txt : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->trailTable->setItem(row, col, item);
        }
        row++;
    }
    sqlite3_finalize(stmt);

    // qDebug() << "[审计追踪] 加载了" << row << "条记录";
}

void PageTrail::filterClicked()
{
    bool startOk, endOk;
    QDateTime start = getDateTimeFromLineEdits(
        ui->startYear, ui->startMon, ui->startDay,
        ui->startHour, ui->startMin, ui->startSec, &startOk);
    QDateTime end;
    if (ui->endYear->text().isEmpty() ||
        ui->endMon->text().isEmpty() ||
        ui->endDay->text().isEmpty())
    {
        end = QDateTime::currentDateTime();
        endOk = true;
    } else {
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

void PageTrail::exportPdfClicked()
{
    bool startOk, endOk;
    QDateTime start = getDateTimeFromLineEdits(
        ui->startYear, ui->startMon, ui->startDay,
        ui->startHour, ui->startMin, ui->startSec, &startOk);
    QDateTime end;
    if (ui->endYear->text().isEmpty() ||
        ui->endMon->text().isEmpty() ||
        ui->endDay->text().isEmpty())
    {
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

    // 弹出确认对话框
    auto *msg = new OverlayMessage(this);
    msg->setShowCancelButton(true);  // 显示“取消”按钮
    msg->showMessage(tr("导出PDF"), tr("是否要导出审计追踪记录为 PDF？"));

    connect(msg, &OverlayMessage::closed, this, [=]() {
        // 固定输出目录
        QString outputDir = trailOutputDir;

        // 确保目录存在
        QDir dir(outputDir);
        if (!dir.exists() && !dir.mkpath(outputDir)) {
            auto *err = new OverlayMessage(this);
            err->showMessage(tr("错误"), tr("无法创建目录：") + outputDir);
            return;
        }

        // 生成文件名
        QString fileName = "trail_" +
                           QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
                           ".pdf";
        QString filePath = outputDir + fileName;

        // 导出PDF
        exportTrailToPdf(start, end, filePath);

        // 检查是否成功
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists() && fileInfo.size() > 0) {
            QString op = QString(tr("导出审计追踪：从 %1 到 %2"))
                             .arg(start.toString("yyyy-MM-dd HH:mm:ss"))
                             .arg(end.toString("yyyy-MM-dd HH:mm:ss"));
            AuditLogger::instance().log(op);

            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("成功"), tr("文件已保存到：") + filePath);
        } else {
            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
        }
    });

    connect(msg, &OverlayMessage::cancelled, this, [=]() {
        // 用户点击了“取消”，不执行任何操作
        return;
    });
}

void PageTrail::loadRange(const QDateTime &start, const QDateTime &end)
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qCritical() << "[审计追踪] 获取数据库连接失败";
        return;
    }

    const char* sql =
        "SELECT trailtime, username, level, number, operation "
        "FROM t_table WHERE datetime(trailtime) BETWEEN ? AND ? "
        "ORDER BY datetime(trailtime) DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[审计追踪] SQL 准备失败：" << sqlite3_errmsg(db);
        return;
    }

    QString startStr = start.toString("yyyy-MM-dd HH:mm:ss");
    QString endStr   = end.toString("yyyy-MM-dd HH:mm:ss");
    sqlite3_bind_text(stmt, 1, startStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    ui->trailTable->setRowCount(0);
    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->trailTable->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            const char* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem* item = new QTableWidgetItem(txt ? txt : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->trailTable->setItem(row, col, item);
        }
        row++;
    }

    sqlite3_finalize(stmt);

    if (row == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("该时间段内无记录"));
    } /*else {
        qDebug() << "[审计追踪] 筛选到" << row << "条记录";
    }*/
}

// 根据时间下拉框的文字来调整开始时间
QDateTime PageTrail::getStartTime(int index) const
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
void PageTrail::refreshUserCombos()
{
    ui->selectUserRange->clear();

    // 添加默认的提示选项
    ui->selectUserRange->addItem(tr("请选择用户"));

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return;

    const char *sql = "SELECT DISTINCT username FROM t_table ORDER BY username ASC;";
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
void PageTrail::refreshNumberCombos()
{
    ui->selectNumberRange->clear();

    // 添加默认的提示选项
    ui->selectNumberRange->addItem(tr("请选择批号"));

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return;

    const char *sql = "SELECT DISTINCT number FROM t_table ORDER BY number ASC;";
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

// 根据时间的下拉框来筛选
void PageTrail::filterByTime(int index)
{
    QDateTime start = getStartTime(index);
    QDateTime end = QDateTime::currentDateTime();
    loadRange(start, end);
}

// 根据用户的下拉框来筛选
void PageTrail::filterByUser(const QString &uname)
{
    if (uname.isEmpty()) return;

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return;

    const char *sql =
        "SELECT trailtime, username, level, number, operation "
        "FROM t_table WHERE username = ? ORDER BY datetime(trailtime) DESC;";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    sqlite3_bind_text(stmt, 1, uname.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    ui->trailTable->setRowCount(0);

    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->trailTable->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->trailTable->setItem(row, col, item);
        }
        row++;
    }

    sqlite3_finalize(stmt);
}

// 根据批号的下拉框来筛选
void PageTrail::filterByNumber(const QString &number)
{
    if (number.isEmpty() || (ui->selectNumberRange->currentIndex() == 0)) return;

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return;

    const char *sql =
        "SELECT trailtime, username, level, number, operation "
        "FROM t_table WHERE number = ? ORDER BY datetime(trailtime) DESC;";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    ui->trailTable->setRowCount(0);

    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->trailTable->insertRow(row);
        for (int col = 0; col < 5; ++col) {
            const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            QTableWidgetItem *item = new QTableWidgetItem(text ? text : "N/A");
            item->setTextAlignment(Qt::AlignCenter);
            ui->trailTable->setItem(row, col, item);
        }
        row++;
    }

    sqlite3_finalize(stmt);
}

// 根据用户来导出PDF
void PageTrail::on_toPdfByUser_clicked()
{
    if (ui->selectUserRange->currentIndex() == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择一个用户"));
        return;
    }
    QString user = ui->selectUserRange->currentText();
    QString fileName = "trail_" + user + ".pdf";
    QString filePath = trailOutputDir + fileName;
    if (filePath.isEmpty()) return;
    exportTrailByUserToPdf(user, filePath);
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {
        // 写审计日志
        QString op = QString(tr("导出用户：%1审计追踪数据")).arg(user);
        AuditLogger::instance().log(op);
        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：trailData目录"));
    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}
// 根据批号来导出PDF
void PageTrail::on_toPdfByNumber_clicked()
{
    if (ui->selectNumberRange->currentIndex() == 0) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择一个批号"));
        return;
    }
    QString number = ui->selectNumberRange->currentText();
    QString fileName = "trail_" + number + ".pdf";
    QString filePath = trailOutputDir + fileName;
    if (filePath.isEmpty()) return;
    exportTrailByNumberToPdf(number, filePath);
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {
        // 写审计日志
        QString op = QString(tr("导出批号：%1审计追踪数据")).arg(number);
        AuditLogger::instance().log(op);
        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：trailData目录"));
    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}
// 根据下拉框时间来导出PDF
void PageTrail::on_toPdfByTime_clicked()
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
    QString fileName = "trail_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
                       ".pdf";
    QString filePath = trailOutputDir + fileName;
    if (filePath.isEmpty()) return;
    exportTrailToPdf(start, end, filePath);
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.size() > 0) {
        // 写审计日志
        QString op = QString(tr("导出审计追踪数据：从 %1 到 %2"))
                         .arg(start.toString("yyyy-MM-dd HH:mm:ss"))
                         .arg(end.toString("yyyy-MM-dd HH:mm:ss"));
        AuditLogger::instance().log(op);
        auto *ok = new OverlayMessage(this);
        ok->showMessage(tr("成功"), tr("文件已保存到：trailData目录"));
    } else {
        auto *err = new OverlayMessage(this);
        err->showMessage(tr("错误"), tr("文件创建失败，请检查权限或路径"));
    }
}

// 取得 trail 对应的数据库row数
int PageTrail::fetchTotalTrailCount() const
{
    // 取得 trail 对应的数据库连接
    sqlite3* db = DatabaseManager::instance()
                      .getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return 0;

    const char* sql = "SELECT COUNT(*) FROM t_table;";
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


// 删除追踪表中最早的1000条记录
void PageTrail::on_clearData_clicked()
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) return;

    int totalCount = fetchTotalTrailCount();
    if (totalCount < 1000) {
        auto* msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"),
                         tr("当前数据不足 1000 条，无法执行删除。"));
        return;
    }

    const char* sql =
        "DELETE FROM t_table WHERE datetime(trailtime) IN ("
        "    SELECT datetime(trailtime) FROM t_table ORDER BY datetime(trailtime) ASC LIMIT 1000"
        ");";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            auto* msg = new OverlayMessage(this);
            msg->setShowCancelButton(true);
            msg->showMessage(tr("警告"), tr("是否删除最早的 1000 条数据？\n注意：删除后不可恢复"));
            AuditLogger::instance().log(tr("删除最早的 1000 条追踪数据"));
            ui->dataRows->setText(QString::number(fetchTotalTrailCount()));
        } else {
            auto* msg = new OverlayMessage(this);
            msg->showMessage(tr("失败"), tr("删除追踪数据失败！"));
        }
    }
    sqlite3_finalize(stmt);
}



// 改变用户重新应用权限
void PageTrail::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态************************************************
void PageTrail::updateUIByPermission()
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
        ui->trailPdfBtn,ui->toPdfByNumber,ui->toPdfByTime,ui->toPdfByUser
    };

    // 设置Supervisor权限控件
    for (auto widget : QAWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::QA, widget);
    }
}

