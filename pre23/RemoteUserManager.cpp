/*
*用户登录认证: 验证用户提供用户名和密码的有效性。
*会话管理 (基于 Token):
*在成功登录后生成一个唯一的、有时效性的访问令牌 (token)。
*提供方法来验证传入的 token 是否有效（是否存在且未过期）。
*允许用户通过 token 获取对应的用户名。
*支持用户登出，即手动使 token 失效。
*Token 生成: 使用加密哈希函数创建安全的、唯一的 token。
 */
//RemoteUserManager.cpp
#include "RemoteUserManager.h" // 包含自身的声明头文件
#include "DatabaseManager.h"   // 依赖数据库管理器进行用户数据查询

#include "sqlite/sqlite3.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>

RemoteUserManager* RemoteUserManager::instance()
{
    static RemoteUserManager mgr; // 局部静态变量，在首次调用时构造
    return &mgr;                 // 返回指向该唯一实例的指针
}

// 私有构造函数，防止外部直接实例化。
// 接受 QObject* parent 参数是为了符合 Qt 对象树机制。
RemoteUserManager::RemoteUserManager(QObject *parent)
    : QObject(parent)
{}

// --- 1. 登录验证 ---
QString RemoteUserManager::login(const QString &user, const QString &pwd)
{
    sqlite3* db; // 指向 SQLite 数据库的指针

    // 通过 DatabaseManager 打开并获取用户数据库句柄
    DatabaseManager::instance().openDatabase(DatabaseManager::UserDatabase);
    db = DatabaseManager::instance()
             .getDatabaseHandle(DatabaseManager::UserDatabase);

    // 检查数据库是否成功打开
    if (!db) {
        qCritical() << "RemoteUserManager: userinfo.db 未打开";
        return ""; // 返回空字符串表示登录失败
    }

    // --- SQL 注入防护 ---
    // 对用户名中的单引号进行转义，防止恶意输入破坏 SQL 查询结构。
    QString escapedUser = user;
    escapedUser.replace("'", "''");

    // 构造 SQL 查询语句，查找对应用户名的密码
    QString sql = QString("SELECT password FROM userinfo WHERE username = '%1'")
                      .arg(escapedUser); // 使用转义后的用户名


    sqlite3_stmt* stmt = nullptr; // SQLite 预处理语句对象指针

    // 准备 SQL 语句
    int rc = sqlite3_prepare_v2(
        db,
        sql.toUtf8().constData(), // 将 QString 转换为 C 字符串
        -1,                       // 让 SQLite 自行计算长度
        &stmt,                    // 输出参数，指向预处理后的语句对象
        nullptr                   // 不需要尾部指针
        );

    // 检查 SQL 语句准备是否成功
    if (rc != SQLITE_OK) {
        qCritical() << "SQL prepare error:" << sqlite3_errmsg(db); // 输出错误信息
        return ""; // 登录失败
    }

    // 执行查询，并移动到第一行结果
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) { // 如果没有返回一行数据，则说明用户不存在
        qWarning() << "RemoteUserManager: 用户不存在:" << user;
        sqlite3_finalize(stmt); // 清理资源
        return ""; // 登录失败
    }

    // 从结果集中获取存储在数据库中的密码
    QString dbPwd = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    sqlite3_finalize(stmt); // 获取完数据后立即清理资源

    // 比较数据库中的密码与用户提供的密码是否一致
    // 注意：此实现假设数据库中存储的是明文密码。实际应用中应存储密码的哈希值，
    // 并将用户输入的密码也进行相同哈希处理后再比较。
    if (dbPwd != pwd) {
        qWarning() << "RemoteUserManager: 密码错误 用户:" << user;
        return ""; // 登录失败
    }

    // -----------------------
    // 密码正确 → 生成 token
    // -----------------------
    QString token = generateToken(); // 调用内部方法生成新 token

    // 创建会话信息结构体
    SessionInfo info;
    info.username = user; // 关联用户名
    // 设置过期时间：当前时间 + 预定义的过期秒数 (TOKEN_EXPIRE_SEC)
    info.expireTime = QDateTime::currentDateTime().addSecs(TOKEN_EXPIRE_SEC);

    // 将新生成的 token 和其关联的会话信息存入内存映射表 m_sessions 中
    m_sessions[token] = info;

    qDebug() << "[RemoteUserManager] 登录成功 User:" << user << ", Token:" << token;

    return token; // 返回生成的 token，表示登录成功
}

// --- 2. Token 验证 ---
bool RemoteUserManager::validateToken(const QString &token)
{
    // 检查 token 是否存在于会话映射表中
    if (!m_sessions.contains(token))
        return false; // 不存在则无效

    // 获取对应的会话信息引用
    SessionInfo &s = m_sessions[token];

    // 检查 token 是否已过期
    if (QDateTime::currentDateTime() > s.expireTime) {
        qWarning() << "[RemoteUserManager] Token 过期:" << token;
        m_sessions.remove(token); // 从映射表中移除已过期的 token
        return false;             // 返回无效
    }

    // --- 自动续期逻辑 ---
    // 如果 token 有效且未过期，则更新其过期时间为当前时间加上 TOKEN_EXPIRE_SEC
    // 这使得只要用户在活动（持续使用有效的 token），其会话就不会因为超时而被终止。
    s.expireTime = QDateTime::currentDateTime().addSecs(TOKEN_EXPIRE_SEC);
    return true; // 返回有效
}

// --- 3. Token 换用户名 ---
QString RemoteUserManager::userFromToken(const QString &token)
{
    // 首先验证 token 的有效性
    if (!validateToken(token))
        return ""; // 如果无效，返回空字符串

    // 如果有效，从会话映射表中获取并返回关联的用户名
    return m_sessions[token].username;
}

// --- 4. 登出 ---
void RemoteUserManager::logout(const QString &token)
{
    // 直接从会话映射表中移除指定的 token
    // 下次再使用此 token 进行验证时，将因找不到而被判为无效。
    m_sessions.remove(token);
}

// --- 5. 生成 Token ---
QString RemoteUserManager::generateToken() const
{
    // 创建一个原始数据块，包含：
    // 1. 一个 64 位的随机数
    // 2. 当前时间的毫秒数（时间戳）
    // 这样可以保证每次生成的数据块都几乎唯一。
    QByteArray raw = QByteArray::number(QRandomGenerator::global()->generate64())
                     + QByteArray::number(QDateTime::currentMSecsSinceEpoch());

    // 使用 SHA-256 哈希算法对原始数据进行加密哈希运算
    // SHA-256 能够产生一个固定长度（256位/32字节）的、不可逆的摘要。
    QByteArray hash = QCryptographicHash::hash(raw, QCryptographicHash::Sha256);

    // 将二进制哈希值转换为十六进制字符串形式，作为最终的 Token 返回。
    // toHex() 方法非常适用于生成可读性强的 Token。
    return hash.toHex();
}
