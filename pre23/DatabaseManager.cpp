//DatabaseManager.cpp
#include "DatabaseManager.h"
#include <QDir>
#include <QMutex>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include "GlobalDefines.h"
#include <QDateTime>

// ——— 单例方法实现 ———
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;  // 首次调用时构造，以后一直用这一份
    return inst;
}
const char* DatabaseManager::dbTypeToString(DatabaseType dbType) const // 添加 const
{
    switch (dbType) {
    case UserDatabase: return "用户";
    case HistoryDatabase: return "历史记录";
    case TrailDatabase: return "审计追踪";
    default: return "未知";
    }
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    // 初始化默认数据库路径
    m_userDbPath = userDataBasePath;
    m_historyDbPath = hisDataBasePath;
    m_trailDbPath = trailDataBasePath;

    // 检查并创建数据库目录（如果不存在）
    QDir dbDir(dataBasePath);
    if (!dbDir.exists()) {
        dbDir.mkpath(".");
    }

    qDebug() << "数据库管理器初始化完成";
}

DatabaseManager::~DatabaseManager() {
    closeAllDatabases();
}

sqlite3* DatabaseManager::getDatabaseHandle(DatabaseType dbType)
{
    // 1. 已存在连接，直接返回
    if (m_databases.contains(dbType)) {
        sqlite3* db = m_databases.value(dbType);
        if (db) {
            return db;
        }
    }

    qWarning() << "数据库未打开，尝试自动打开:" << dbTypeToString(dbType);

    // 2. 尝试自动打开
    if (!openDatabase(dbType)) {
        qCritical() << "自动打开数据库失败:" << dbTypeToString(dbType);
        return nullptr;
    }

    // 3. 再次获取
    if (m_databases.contains(dbType)) {
        return m_databases.value(dbType);
    }

    qCritical() << "数据库打开后仍然获取失败:" << dbTypeToString(dbType);
    return nullptr;
}

// 打开指定类型的数据库
bool DatabaseManager::openDatabase(DatabaseType dbType) {
    // 检查是否已打开
    if (m_databases.contains(dbType) && m_databases.value(dbType)) {
        return true;
    }

    // 确定数据库路径
    QString path = "";
    switch (dbType) {
    case UserDatabase:
        path = m_userDbPath;
        break;
    case HistoryDatabase:
        path = m_historyDbPath;
        break;
    case TrailDatabase:
        path = m_trailDbPath;
        break;
    default:
        qCritical() << "无效的数据库类型:" << dbType;
        return false;
    }

    // 检查路径是否设置
    if (path.isEmpty()) {
        qCritical() << dbTypeToString(dbType) << "路径未设置";
        return false;
    }
    qDebug() << "正在打开数据库:" << dbTypeToString(dbType) << "路径:" << path;

    // 检查文件是否存在
    if (!QFile::exists(path)) {
        qWarning() << "数据库文件不存在，尝试创建:" << path;
    }

    // 打开新数据库连接
    sqlite3* db = nullptr;
    int rc = sqlite3_open(path.toUtf8().constData(), &db);
    if (rc != SQLITE_OK) {
        qCritical() << "无法打开" << dbTypeToString(dbType)
                    << "数据库:" << sqlite3_errmsg(db);
        if (db) {
            sqlite3_close(db);
        }
        return false;
    }

    // 存储连接
    m_databases[dbType] = db;

    // 初始化表结构
    initDatabase(dbType);

    qDebug() << dbTypeToString(dbType) << "数据库连接成功, 路径:" << path;
    return true;
}

// 设置历史数据库路径
void DatabaseManager::setHistoryDatabasePath(const QString &path) {
    // 如果数据库已经打开，需要先关闭
    if (m_databases.contains(HistoryDatabase)) {
        qWarning() << "历史数据库已打开，无法更改路径";
        return;
    }

    m_historyDbPath = path;
    qDebug() << "历史数据库路径设置为:" << m_historyDbPath;
}

// 设置审计追踪数据库路径
void DatabaseManager::setTrailDatabasePath(const QString &path) {
    // 如果数据库已经打开，需要先关闭
    if (m_databases.contains(TrailDatabase)) {
        qWarning() << "审计追踪数据库已打开，无法更改路径";
        return;
    }

    m_trailDbPath = path;
    qDebug() << "审计追踪数据库路径设置为:" << m_trailDbPath;
}

// 关闭所有数据库连接
void DatabaseManager::closeAllDatabases() {
    // 关闭所有连接
    for (auto it = m_databases.begin(); it != m_databases.end(); ++it) {
        if (it.value()) {
            sqlite3_close(it.value());
        }
    }
    m_databases.clear();
    qDebug() << "已关闭所有数据库连接";
}

// 关闭特定数据库
void DatabaseManager::closeDatabase(DatabaseType dbType) {
    if (m_databases.contains(dbType)) {
        sqlite3_close(m_databases[dbType]);
        m_databases.remove(dbType);
        qDebug() << "已关闭" << dbTypeToString(dbType) << "数据库";
    }
}

// 初始化数据库表结构
void DatabaseManager::initDatabase(DatabaseType dbType) {
    sqlite3* db = m_databases[dbType];
    if (!db) {
        qWarning() << "无法初始化" << dbTypeToString(dbType) << "数据库: 无效的连接";
        return;
    }

    const char* sql = nullptr;
    const char* tableName = "";

    switch (dbType) {
    case UserDatabase:
        sql = "CREATE TABLE IF NOT EXISTS userinfo ("
              "username TEXT PRIMARY KEY, "
              "level TEXT NOT NULL, "
              "password TEXT NOT NULL)";
        tableName = "userinfo";
        break;

    case HistoryDatabase:
        sql = "CREATE TABLE IF NOT EXISTS h_table ("
              "historytime TEXT NOT NULL, "
              "SENSOR1 INTEGER, "
              "SENSOR2 INTEGER, "
              "SENSOR3 INTEGER, "
              "SENSOR4 INTEGER, "
              "number TEXT, "
              "username TEXT, "
              "level TEXT)";
        tableName = "h_table";
        break;

    case TrailDatabase:
        sql = "CREATE TABLE IF NOT EXISTS t_table ("
              "trailtime TEXT NOT NULL, "
              "username TEXT, "
              "level TEXT, "
              "operation TEXT)";
        tableName = "t_table";
        break;
    }

    if (!sql) {
        qWarning() << dbTypeToString(dbType) << "数据库无需初始化";
        return;
    }

    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qCritical() << "在" << dbTypeToString(dbType) << "数据库创建"
                    << tableName << "表失败:" << (errMsg ? errMsg : "未知错误");
        sqlite3_free(errMsg);
    } else {
        qDebug() << dbTypeToString(dbType) << "数据库表结构初始化完成";
    }
}

// 数据库类型转字符串
const char* DatabaseManager::dbTypeToString(DatabaseType dbType) {
    switch (dbType) {
    case UserDatabase: return "用户";
    case HistoryDatabase: return "历史记录";
    case TrailDatabase: return "审计追踪";
    default: return "未知";
    }
}

// 用户验证（仅校验密码，不涉及锁定）
bool DatabaseManager::validateUser(const QString &username, const QString &password) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "验证用户失败: 无法打开用户数据库";
        return false;
    }
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password FROM userinfo WHERE username = ?";
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    bool isValid = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        QString storedPassword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        isValid = (storedPassword == password.trimmed());
    }
    sqlite3_finalize(stmt);
    return isValid;
}

// 获取用户信息
bool DatabaseManager::getUserInfo(const QString &username, QString &outUsername, QString &outLevel) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "获取用户信息失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "SELECT username, level FROM userinfo WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outUsername = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        outLevel = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

// 检查用户是否存在
bool DatabaseManager::userExists(const QString &username) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "检查用户存在失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "SELECT COUNT(*) FROM userinfo WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0);
    }
    sqlite3_finalize(stmt);
    return exists;
}

// 获取锁定状态
bool DatabaseManager::getLockStatus(const QString &username, QString &outLockStatus) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "获取锁定状态失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "SELECT islocked FROM userinfo WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outLockStatus = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

// 获取初次登录状态
bool DatabaseManager::firstLogin(const QString &username, QString &isFirstLogin) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "获取登录状态失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "SELECT firstlogin FROM userinfo WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        isFirstLogin = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

// 增加失败尝试次数（分开 UPDATE 和 SELECT）
bool DatabaseManager::incrementFailedAttempts(const QString &username, int &outAttempts) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "增加失败尝试失败: 无法打开用户数据库";
        return false;
    }

    // 第一步: 执行 UPDATE
    const char *updateSql = "UPDATE userinfo SET failed_attempts = failed_attempts + 1 WHERE username = ?";
    sqlite3_stmt *updateStmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], updateSql, -1, &updateStmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "更新准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(updateStmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    rc = sqlite3_step(updateStmt);
    if (rc != SQLITE_DONE) {
        qCritical() << "更新失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        sqlite3_finalize(updateStmt);
        return false;
    }
    sqlite3_finalize(updateStmt);

    // 第二步: 执行 SELECT
    const char *selectSql = "SELECT failed_attempts FROM userinfo WHERE username = ?";
    sqlite3_stmt *selectStmt;
    rc = sqlite3_prepare_v2(m_databases[UserDatabase], selectSql, -1, &selectStmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "查询准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    sqlite3_bind_text(selectStmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    rc = sqlite3_step(selectStmt);
    if (rc == SQLITE_ROW) {
        outAttempts = sqlite3_column_int(selectStmt, 0);
    } else {
        qCritical() << "查询失败: 未找到用户或其它错误 -" << sqlite3_errmsg(m_databases[UserDatabase]);
        sqlite3_finalize(selectStmt);
        return false;  // 如果未找到，返回 false
    }
    sqlite3_finalize(selectStmt);
    return true;
}

// 重置失败尝试次数
bool DatabaseManager::resetFailedAttempts(const QString &username) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "重置失败尝试失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "UPDATE userinfo SET failed_attempts = 0 WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "更新准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        qCritical() << "更新失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

// 锁定账户
bool DatabaseManager::lockAccount(const QString &username) {
    if (!openDatabase(UserDatabase)) {
        qCritical() << "锁定账户失败: 无法打开用户数据库";
        return false;
    }
    const char *sql = "UPDATE userinfo SET islocked = 'Y' WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "更新准备失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        return false;
    }
    QByteArray usernameBytes = username.trimmed().toUtf8();
    sqlite3_bind_text(stmt, 1, usernameBytes.constData(), usernameBytes.size(), SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        qCritical() << "更新失败:" << sqlite3_errmsg(m_databases[UserDatabase]);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

// 系统级密码复杂度校验函数（在设置或修改密码的地方调用，例如注册或改密界面）
#include <QRegularExpression>
bool DatabaseManager::isValidPasswordComplexity(const QString &password) {

    static const QRegularExpression reLower("[a-z]");
    static const QRegularExpression reUpper("[A-Z]");
    static const QRegularExpression reDigit("[0-9]");
    static const QRegularExpression reSpecial("[^a-zA-Z0-9]");

    if (password.length() < 8) {
        return false;
    }
    int typeCount = 0;
    // 检查小写字母
    if (password.contains(reLower)) {
        typeCount++;
    }
    // 检查大写字母
    if (password.contains(reUpper)) {
        typeCount++;
    }
    // 检查数字
    if (password.contains(reDigit)) {
        typeCount++;
    }
    // 检查特殊字符
    if (password.contains(reSpecial)) {
        typeCount++;
    }
    return typeCount >= 3;
}

bool DatabaseManager::getUserCreateTime(const QString &username, QDateTime &outTime)
{
    const char *sql = "SELECT usertime FROM userinfo WHERE username = ?";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(m_databases[UserDatabase], sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    QByteArray u = username.toUtf8();
    sqlite3_bind_text(stmt, 1, u.constData(), u.size(), SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        QString timeStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        outTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
        sqlite3_finalize(stmt);
        return outTime.isValid();
    }

    sqlite3_finalize(stmt);
    return false;
}
bool DatabaseManager::updatePasswd_Time_FirstLogin(const QString &username,
                                            const QString &newPassword,
                                            const QDateTime &now)
{
    const char *sql =
        "UPDATE userinfo SET password = ?, usertime = ?, firstlogin = "
        "   CASE WHEN firstlogin = 'Y' THEN 'N' "
        "   ELSE firstlogin "
        " END "
        " WHERE username = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_databases[UserDatabase],sql,-1,&stmt,nullptr) != SQLITE_OK) {
        return false;
    }

    const QByteArray pwd  = newPassword.toUtf8();
    const QByteArray time = now.toString("yyyy-MM-dd HH:mm:ss").toUtf8();
    const QByteArray user = username.toUtf8();

    sqlite3_bind_text(stmt, 1, pwd.constData(), pwd.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, time.constData(), time.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.constData(), user.size(), SQLITE_TRANSIENT);

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}
// 查询某个用户的权限等级
PermissionManager::PermissionLevel DatabaseManager::getUserPermissionLevel(const QString& username)
{
    sqlite3* db = getDatabaseHandle(UserDatabase);
    if (!db) return PermissionManager::Standard;

    const char* sql =
        "SELECT level FROM userinfo WHERE username = ? LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    PermissionManager::PermissionLevel level = PermissionManager::Standard;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1,
                          username.toUtf8().constData(),
                          -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            QString levelStr =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            level = static_cast<PermissionManager::PermissionLevel>(
                PermissionManager::levelFromString(levelStr));
        }
    }
    sqlite3_finalize(stmt);
    return level;
}


// 添加历史记录
bool DatabaseManager::insertSensorData(const QString& timestamp,
                                       const QVariant& s1, const QVariant& s2, const QVariant& s3,const QVariant& s4,
                                       const QString& batchNumber,
                                       const QString& username,
                                       const QString& userLevel)
{
    if (!openDatabase(HistoryDatabase)) {
        qWarning() << "[DatabaseManager] 历史数据库未打开";
        return false;
    }

    const char* sql = "INSERT INTO h_table(historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level) "
                      "VALUES(?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_databases[HistoryDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[DatabaseManager] prepare failed:" << sqlite3_errmsg(m_databases[HistoryDatabase]);
        return false;
    }

    // 1) 时间戳
    sqlite3_bind_text(stmt, 1, timestamp.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 2) 传感器1~3：QVariant::isNull => NULL，否则一律以文本形式存入 QVariant::toString()
    auto bindVariantAsText = [&](int idx, const QVariant& v) {
        if (v.isNull()) {
            sqlite3_bind_null(stmt, idx);
        } else {
            QByteArray utf = v.toString().toUtf8();
            sqlite3_bind_text(stmt, idx, utf.constData(), utf.size(), SQLITE_TRANSIENT);
        }
    };

    bindVariantAsText(2, s1);
    bindVariantAsText(3, s2);
    bindVariantAsText(4, s3);
    bindVariantAsText(5, s4);
    // 3) 批次号
    if (batchNumber.isEmpty()) {
        sqlite3_bind_null(stmt, 6);
    } else {
        sqlite3_bind_text(stmt, 6, batchNumber.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }

    // 4) 用户名、权限级别
    sqlite3_bind_text(stmt, 7, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, userLevel.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 执行
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        qCritical() << "[DatabaseManager] insert failed:" << sqlite3_errmsg(m_databases[HistoryDatabase]);
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// 查询历史数据 - 按批号
QList<HistoryData> DatabaseManager::queryHistoryByBatch(const QString& batchNumber)
{
    QList<HistoryData> result;

    if (!openDatabase(HistoryDatabase)) {
        qWarning() << "[queryHistoryByBatch] 历史数据库未打开";
        return result;
    }

    const char* sql = "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
                      "FROM h_table WHERE number = ? ORDER BY historytime ASC";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_databases[HistoryDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[queryHistoryByBatch] prepare failed:" << sqlite3_errmsg(m_databases[HistoryDatabase]);
        return result;
    }

    sqlite3_bind_text(stmt, 1, batchNumber.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryData data;

        // 读取每一列数据
        data.historytime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        // 处理可能为NULL的传感器数据
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            data.SENSOR1 = QString::number(sqlite3_column_int(stmt, 1));
        } else {
            data.SENSOR1 = "";
        }

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            data.SENSOR2 = QString::number(sqlite3_column_int(stmt, 2));
        } else {
            data.SENSOR2 = "";
        }

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            data.SENSOR3 = QString::number(sqlite3_column_int(stmt, 3));
        } else {
            data.SENSOR3 = "";
        }

        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
            data.SENSOR4 = QString::number(sqlite3_column_int(stmt, 4));
        } else {
            data.SENSOR4 = "";
        }

        // 处理可能为NULL的文本字段
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
            data.number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        }

        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
            data.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        }

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            data.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        }

        result.append(data);
    }

    sqlite3_finalize(stmt);
    qDebug() << "[queryHistoryByBatch] 查询到批号" << batchNumber << "的" << result.size() << "条记录";
    return result;
}

// 查询历史数据 - 按时间范围
QList<HistoryData> DatabaseManager::queryHistoryByTime(const QString& start, const QString& end)
{
    QList<HistoryData> result;

    if (!openDatabase(HistoryDatabase)) {
        qWarning() << "[queryHistoryByTime] 历史数据库未打开";
        return result;
    }

    const char* sql = "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4, number, username, level "
                      "FROM h_table WHERE historytime BETWEEN ? AND ? ORDER BY historytime ASC";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_databases[HistoryDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[queryHistoryByTime] prepare failed:" << sqlite3_errmsg(m_databases[HistoryDatabase]);
        return result;
    }

    sqlite3_bind_text(stmt, 1, start.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, end.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryData data;

        data.historytime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            data.SENSOR1 = QString::number(sqlite3_column_int(stmt, 1));
        }

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            data.SENSOR2 = QString::number(sqlite3_column_int(stmt, 2));
        }

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            data.SENSOR3 = QString::number(sqlite3_column_int(stmt, 3));
        }

        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
            data.SENSOR4 = QString::number(sqlite3_column_int(stmt, 4));
        }

        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
            data.number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        }

        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
            data.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        }

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            data.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        }

        result.append(data);
    }

    sqlite3_finalize(stmt);
    qDebug() << "[queryHistoryByTime] 查询到" << result.size() << "条记录，时间范围:" << start << "到" << end;
    return result;
}

// 查询审计追踪数据 - 按批号
QList<TrailData> DatabaseManager::queryTrailByBatch(const QString& batchNumber)
{
    QList<TrailData> result;

    if (!openDatabase(TrailDatabase)) {
        qWarning() << "[queryTrailByBatch] 审计追踪数据库未打开";
        return result;
    }

    // 注意：审计追踪表可能没有批号字段，这里假设有number字段
    // 如果审计追踪表没有批号字段，这个查询可能返回空结果
    const char* sql = "SELECT trailtime, username, level, operation, number "
                      "FROM t_table WHERE number = ? ORDER BY trailtime ASC";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_databases[TrailDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[queryTrailByBatch] prepare failed:" << sqlite3_errmsg(m_databases[TrailDatabase]);
        return result;
    }

    sqlite3_bind_text(stmt, 1, batchNumber.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrailData data;

        data.trailtime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            data.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        }

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            data.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        }

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            data.operation = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        }

        // 如果有批号字段
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
            data.number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        }

        result.append(data);
    }

    sqlite3_finalize(stmt);
    qDebug() << "[queryTrailByBatch] 查询到批号" << batchNumber << "的" << result.size() << "条审计记录";
    return result;
}

// 查询审计追踪数据 - 按时间范围
QList<TrailData> DatabaseManager::queryTrailByTime(const QString& start, const QString& end)
{
    QList<TrailData> result;

    if (!openDatabase(TrailDatabase)) {
        qWarning() << "[queryTrailByTime] 审计追踪数据库未打开";
        return result;
    }

    const char* sql = "SELECT trailtime, username, level, operation "
                      "FROM t_table WHERE trailtime BETWEEN ? AND ? ORDER BY trailtime ASC";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_databases[TrailDatabase], sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        qCritical() << "[queryTrailByTime] prepare failed:" << sqlite3_errmsg(m_databases[TrailDatabase]);
        return result;
    }

    sqlite3_bind_text(stmt, 1, start.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, end.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrailData data;

        data.trailtime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            data.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        }

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            data.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        }

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
            data.operation = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        }

        result.append(data);
    }

    sqlite3_finalize(stmt);
    qDebug() << "[queryTrailByTime] 查询到" << result.size() << "条审计记录，时间范围:" << start << "到" << end;
    return result;
}

