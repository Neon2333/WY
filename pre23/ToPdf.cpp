#include "ToPdf.h"
#include "OverlayMessage.h"
#include "sqlite/sqlite3.h"
#include "DatabaseManager.h"
#include "SessionManager.h"

#include <QPdfWriter>
#include <QPainter>
#include <QFont>
#include <QPageSize>
#include <QFile>
#include <QFileDialog>
#include <numeric>
#include <QDebug>

QFont fontTitle("Microsoft YaHei", 14, QFont::Bold);   // 页面主标题
QFont fontSub("Microsoft YaHei", 10);                  // 副标题
QFont fontHeader("Microsoft YaHei", 8, QFont::Bold);  // 表头
QFont fontBody("Microsoft YaHei", 8);                  // 表格数据

// 通用绘制函数：绘制带边框的单行表格
// 修改后的通用绘制函数
void drawRowWithBorder(QPainter &painter, const QStringList &values, int &y,
                       int marginLeft, const QVector<int> &columnWidths, int lineHeight) {
    int x = marginLeft;
    const int tableWidth = std::accumulate(columnWidths.begin(), columnWidths.end(), 0);

    // 绘制所有垂直边框
    painter.drawLine(x, y, x, y + lineHeight);  // 首列左边框
    for (int i = 0; i < values.size(); ++i) {
        // 绘制单元格内容
        QRect cell(x, y, columnWidths[i], lineHeight);
        painter.drawText(cell.adjusted(5, 5, -5, -5),
                         Qt::AlignLeft | Qt::TextWordWrap,
                         values[i]);

        // 绘制右边框
        const int right = x + columnWidths[i];
        painter.drawLine(right, y, right, y + lineHeight);

        x += columnWidths[i];
    }

    // 绘制整行底部边框
    painter.drawLine(marginLeft, y + lineHeight,
                     marginLeft + tableWidth, y + lineHeight);

    y += lineHeight;
}



// 导出操作记录 PDF（t_table），风格与 exportHisToPdf 保持一致
void exportTrailToPdf(const QDateTime &start,
                      const QDateTime &end,
                      const QString &filePath)
{
    // 1. 打开数据库
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qCritical() << "[审计追踪] 获取数据库连接失败";
        return;
    }

    // 2. 准备查询
    const char *sql = R"(
        SELECT trailtime, number, username, level, operation
        FROM t_table
        WHERE datetime(trailtime) BETWEEN ?1 AND ?2
        ORDER BY datetime(trailtime) DESC
    )";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qDebug() << "exportTrailToPdf 错误: SQL 准备失败:" << sqlite3_errmsg(db);
        return;
    }

    QString startStr = start.toString("yyyy-MM-dd HH:mm:ss");
    QString endStr   = end.toString("yyyy-MM-dd HH:mm:ss");
    sqlite3_bind_text(stmt, 1, startStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 3. 设置 PDF 输出
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qDebug() << "exportTrailToPdf 错误: 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);
        return;
    }

    // PDF 页面参数
    int marginLeft   = 100;
    int marginTop    = 100;
    int y = marginTop;

    // ************ 新增：页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司审计追踪数据表");
        int titleHeight = 120;
        painter.setFont(fontTitle);
        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);
        y += titleHeight + 50; // 行间距

        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString user      = SessionManager::instance().userName();
        QString level     = SessionManager::instance().userLevel();

        QString subTitle = QObject::tr("打印时间：%1     打印人：%2 (%3)")
                               .arg(printTime,user,level);

        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;

        QString timeArea = QObject::tr("时间范围：%1 到 %2")
                               .arg(start.toString("yyyy-MM-dd HH:mm:ss"),
                                    end.toString("yyyy-MM-dd HH:mm:ss"));
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, timeArea);
        y += 60;
    }
    // *************************************************************

    // 表格参数
    int lineHeight   = 60;
    QVector<int> columnWidths = {380, 240, 200, 220, 1200};
    QStringList headers = { "TIME", "BatchNumber", "UserName", "Permission", "Operations" };
    int pageHeight = writer.height() - marginTop;

    auto drawHeader = [&]() {
        painter.save();

        painter.setFont(fontHeader);   // 表头字体
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));

        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);

        painter.restore();
    };


    // ************ 打印表格标题行 ************
    drawHeader();

    // 数据绘制
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            row << (txt ? QString::fromUtf8(txt) : "");
        }

        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }

        painter.setFont(fontBody);    // 内部字体
        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }


    // 结束
    painter.end();
    sqlite3_finalize(stmt);

    // 检查
    QFile file(filePath);
    if (!file.exists() || file.size() == 0) {
        if (file.exists()) file.remove();
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}



// 导出历史记录 PDF（h_table）
void exportHisToPdf(const QDateTime &start,
                    const QDateTime &end,
                    const QString &filePath)
{
    // 1. 获取用户数据库句柄
    sqlite3 *db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qCritical() << "[历史记录] 获取数据库连接失败";
        return;
    }

    // 2. 准备 SQL 查询
    const char *sql = R"(
        SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level
        FROM h_table
        WHERE datetime(historytime) BETWEEN ?1 AND ?2
        ORDER BY datetime(historytime) DESC
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[历史记录] SQL 准备失败:" << sqlite3_errmsg(db);
        return;
    }

    // 3. 绑定查询时间参数
    QString startStr = start.toString("yyyy-MM-dd HH:mm:ss");
    QString endStr   = end.toString("yyyy-MM-dd HH:mm:ss");
    sqlite3_bind_text(stmt, 1, startStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endStr.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 4. 初始化 PDF 写入
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qCritical() << "[历史记录] 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);
        return;
    }

    // 页面参数
    int marginLeft = 100;
    int marginTop = 100;
    int y = marginTop;

    // ************ 新增：页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司历史数据表");

        painter.setFont(fontTitle);
        int titleHeight = 120;

        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);

        y += titleHeight + 50;  // 行距

        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString user      = SessionManager::instance().userName();
        QString level     = SessionManager::instance().userLevel();

        QString subTitle = QObject::tr("打印时间：%1     打印人：%2 (%3)")
                               .arg(printTime,user,level);

        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;

        QString timeArea = QObject::tr("时间范围：%1 到 %2")
                               .arg(start.toString("yyyy-MM-dd HH:mm:ss"),
                                    end.toString("yyyy-MM-dd HH:mm:ss"));
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, timeArea);
        y += 60;
    }
    // *************************************************************

    // 表格样式配置
    int lineHeight = 60;
    QVector<int> columnWidths = {440,185,185,185,185,340,300,340};
    QStringList headers = { "TIME", "SENSOR1", "SENSOR2", "SENSOR3", "SENSOR4", "BatchNum", "UserName", "Permission" };

    int pageHeight = writer.height() - marginTop;

    auto drawHeader = [&]() {
        painter.save();
        painter.setFont(fontHeader);
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));
        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);
        painter.restore();
    };

    // 5. 绘制表格标题行
    drawHeader();

    // 6. 绘制数据
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        painter.setFont(fontBody);
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            row << (text ? QString::fromUtf8(text) : "");
        }

        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }

        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }

    // 7. 收尾
    painter.end();
    sqlite3_finalize(stmt);

    if (QFileInfo(filePath).size() == 0) {
        QFile::remove(filePath);
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}


// 导出历史记录 PDF（按用户过滤）
void exportHisByUserToPdf(const QString &user, const QString &filePath)
{
    // 1. 获取用户数据库句柄
    sqlite3 *db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qCritical() << "[历史记录] 获取数据库连接失败";
        return;
    }
    // 2. 准备 SQL 查询（按用户过滤，无时间范围）
    const char *sql = R"(
        SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level
        FROM h_table
        WHERE username = ?
        ORDER BY datetime(historytime) DESC
    )";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[历史记录] SQL 准备失败:" << sqlite3_errmsg(db);

        return;
    }
    // 3. 绑定用户参数
    sqlite3_bind_text(stmt, 1, user.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    // 4. 初始化 PDF 写入
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qCritical() << "[历史记录] 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);

        return;
    }
    // 页面参数
    int marginLeft = 100;
    int marginTop = 100;
    int y = marginTop;
    // ************ 页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司历史数据表");
        painter.setFont(fontTitle);
        int titleHeight = 120;
        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);
        y += titleHeight + 50; // 行距
        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString currentUser = SessionManager::instance().userName();
        QString level = SessionManager::instance().userLevel();
        QString subTitle = QObject::tr("打印时间：%1     打印人：%2 (%3)     按用户名：%4导出")
                               .arg(printTime,
                                    currentUser,
                                    level,
                                    user);
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;
    }
    // *************************************************************
    // 表格样式配置
    int lineHeight = 60;
    QVector<int> columnWidths = {440,185,185,185,185,340,300,340};
    QStringList headers = { "TIME", "SENSOR1", "SENSOR2", "SENSOR3", "SENSOR4", "BatchNum", "UserName", "Permission" };
    int pageHeight = writer.height() - marginTop;
    auto drawHeader = [&]() {
        painter.save();
        painter.setFont(fontHeader);
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));
        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);
        painter.restore();
    };
    // 5. 绘制表格标题行
    drawHeader();
    // 6. 绘制数据
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        painter.setFont(fontBody);
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            row << (text ? QString::fromUtf8(text) : "");
        }
        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }
        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }
    // 7. 收尾
    painter.end();
    sqlite3_finalize(stmt);

    if (QFileInfo(filePath).size() == 0) {
        QFile::remove(filePath);
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}

// 导出历史记录 PDF（按批号过滤）
void exportHisByNumberToPdf(const QString &number, const QString &filePath)
{
    // 1. 获取用户数据库句柄
    sqlite3 *db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qCritical() << "[历史记录] 获取数据库连接失败";
        return;
    }
    // 2. 准备 SQL 查询（按批号过滤，无时间范围）
    const char *sql = R"(
        SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level
        FROM h_table
        WHERE number = ?
        ORDER BY datetime(historytime) DESC
    )";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[历史记录] SQL 准备失败:" << sqlite3_errmsg(db);

        return;
    }
    // 3. 绑定批号参数
    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    // 4. 初始化 PDF 写入
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qCritical() << "[历史记录] 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);

        return;
    }
    // 页面参数
    int marginLeft = 100;
    int marginTop = 100;
    int y = marginTop;
    // ************ 页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司历史数据表");
        painter.setFont(fontTitle);
        int titleHeight = 120;
        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);
        y += titleHeight + 50; // 行距
        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString currentUser = SessionManager::instance().userName();
        QString level = SessionManager::instance().userLevel();
        QString subTitle = QObject::tr("打印时间：%1     打印人：%2 (%3)     按批号：%4导出")
                               .arg(printTime,
                                    currentUser,
                                    level,
                                    number);
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;
    }
    // *************************************************************
    // 表格样式配置
    int lineHeight = 60;
    QVector<int> columnWidths = {440,185,185,185,185,340,300,340};
    QStringList headers = { "TIME", "SENSOR1", "SENSOR2", "SENSOR3", "SENSOR4", "BatchNum", "UserName", "Permission" };
    int pageHeight = writer.height() - marginTop;
    auto drawHeader = [&]() {
        painter.save();
        painter.setFont(fontHeader);
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));
        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);
        painter.restore();
    };
    // 5. 绘制表格标题行
    drawHeader();
    // 6. 绘制数据
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        painter.setFont(fontBody);
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            row << (text ? QString::fromUtf8(text) : "");
        }
        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }
        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }
    // 7. 收尾
    painter.end();
    sqlite3_finalize(stmt);

    if (QFileInfo(filePath).size() == 0) {
        QFile::remove(filePath);
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}

// 导出审计追踪 PDF（按用户过滤）
void exportTrailByUserToPdf(const QString &user, const QString &filePath)
{
    // 1. 获取用户数据库句柄
    sqlite3 *db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qCritical() << "[审计追踪] 获取数据库连接失败";
        return;
    }
    // 2. 准备 SQL 查询（按用户过滤，无时间范围）
    const char *sql = R"(
        SELECT trailtime, number, username, level, operation
        FROM t_table
        WHERE username = ?
        ORDER BY datetime(trailtime) DESC
    )";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[审计追踪] SQL 准备失败:" << sqlite3_errmsg(db);
        return;
    }
    // 3. 绑定用户参数
    sqlite3_bind_text(stmt, 1, user.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    // 4. 初始化 PDF 写入
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qCritical() << "[审计追踪] 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);
        return;
    }
    // 页面参数
    int marginLeft = 100;
    int marginTop = 100;
    int y = marginTop;
    // ************ 页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司审计追踪数据表");
        painter.setFont(fontTitle);
        int titleHeight = 120;
        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);
        y += titleHeight + 50; // 行距
        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString currentUser = SessionManager::instance().userName();
        QString level = SessionManager::instance().userLevel();
        QString subTitle = QObject::tr("打印时间：%1 打印人：%2 (%3) 按用户名：%4导出")
                               .arg(printTime,
                                    currentUser,
                                    level,
                                    user);
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;
    }
    // *************************************************************
    // 表格样式配置
    int lineHeight = 60;
    QVector<int> columnWidths = {380, 240, 200, 220, 1200};
    QStringList headers = { "TIME", "BatchNumber", "UserName", "Permission", "Operations" };
    int pageHeight = writer.height() - marginTop;
    auto drawHeader = [&]() {
        painter.save();
        painter.setFont(fontHeader);
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));
        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);
        painter.restore();
    };
    // 5. 绘制表格标题行
    drawHeader();
    // 6. 绘制数据
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        painter.setFont(fontBody);
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            row << (text ? QString::fromUtf8(text) : "");
        }
        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }
        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }
    // 7. 收尾
    painter.end();
    sqlite3_finalize(stmt);
    if (QFileInfo(filePath).size() == 0) {
        QFile::remove(filePath);
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}
// 导出审计追踪 PDF（按批号过滤）
void exportTrailByNumberToPdf(const QString &number, const QString &filePath)
{
    // 1. 获取用户数据库句柄
    sqlite3 *db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::TrailDatabase);
    if (!db) {
        qCritical() << "[审计追踪] 获取数据库连接失败";
        return;
    }
    // 2. 准备 SQL 查询（按批号过滤，无时间范围）
    const char *sql = R"(
        SELECT trailtime, number, username, level, operation
        FROM t_table
        WHERE number = ?
        ORDER BY datetime(trailtime) DESC
    )";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[审计追踪] SQL 准备失败:" << sqlite3_errmsg(db);
        return;
    }
    // 3. 绑定批号参数
    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    // 4. 初始化 PDF 写入
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    QPainter painter(&writer);
    if (!painter.isActive()) {
        qCritical() << "[审计追踪] 无法初始化 PDF 绘图";
        sqlite3_finalize(stmt);
        return;
    }
    // 页面参数
    int marginLeft = 100;
    int marginTop = 100;
    int y = marginTop;
    // ************ 页面表头（第一行 + 第二行） ************
    {
        QString title = QObject::tr("xx公司审计追踪数据表");
        painter.setFont(fontTitle);
        int titleHeight = 120;
        painter.drawText(QRect(0, y, writer.width(), titleHeight),
                         Qt::AlignCenter,
                         title);
        y += titleHeight + 50; // 行距
        QString printTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString currentUser = SessionManager::instance().userName();
        QString level = SessionManager::instance().userLevel();
        QString subTitle = QObject::tr("打印时间：%1 打印人：%2 (%3) 按批号：%4导出")
                               .arg(printTime,
                                    currentUser,
                                    level,
                                    number);
        painter.setFont(fontSub);
        painter.drawText(marginLeft, y, subTitle);
        y += 60;
    }
    // *************************************************************
    // 表格样式配置
    int lineHeight = 60;
    QVector<int> columnWidths = {380, 240, 200, 220, 1200};
    QStringList headers = { "TIME", "BatchNumber", "UserName", "Permission", "Operations" };
    int pageHeight = writer.height() - marginTop;
    auto drawHeader = [&]() {
        painter.save();
        painter.setFont(fontHeader);
        painter.fillRect(QRect(marginLeft, y,
                               std::accumulate(columnWidths.begin(), columnWidths.end(), 0),
                               lineHeight),
                         QBrush(QColor(240, 240, 240)));
        drawRowWithBorder(painter, headers, y, marginLeft, columnWidths, lineHeight);
        painter.restore();
    };
    // 5. 绘制表格标题行
    drawHeader();
    // 6. 绘制数据
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        painter.setFont(fontBody);
        QStringList row;
        for (int col = 0; col < headers.size(); ++col) {
            const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            row << (text ? QString::fromUtf8(text) : "");
        }
        if (y + lineHeight > pageHeight) {
            writer.newPage();
            y = marginTop;
            drawHeader();
        }
        drawRowWithBorder(painter, row, y, marginLeft, columnWidths, lineHeight);
    }
    // 7. 收尾
    painter.end();
    sqlite3_finalize(stmt);
    if (QFileInfo(filePath).size() == 0) {
        QFile::remove(filePath);
        auto *msg = new OverlayMessage();
        msg->showMessage(QObject::tr("错误"), QObject::tr("PDF 内容生成失败"));
    }
}
