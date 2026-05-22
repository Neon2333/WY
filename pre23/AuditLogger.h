#ifndef AUDITLOGGER_H
#define AUDITLOGGER_H

#include <QString>
#include <QMutex>

/**
 * @brief The AuditLogger class
 * 单例模式，全局唯一。负责往审计表 t_table 插入日志记录。
 */
class AuditLogger
{
public:
    /// 获取单例
    static AuditLogger& instance();

    /// 设置审计数据库文件路径，必须在第一次 log 之前调用
    void setDatabasePath(const QString &dbPath);

    /**
     * @brief log
     * 插入一条审计记录：
     *  - 当前系统时间
     *  - SessionManager::userName()
     *  - SessionManager::userLevel()
     *  - operation 描述
     */
    void log(const QString &operation);

private:
    AuditLogger();
    ~AuditLogger();

    // 禁止拷贝、赋值
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger& operator=(const AuditLogger&) = delete;

    QString m_dbPath;
    QMutex  m_mutex;  // 线程安全

    QString batchNumber;
};

#endif // AUDITLOGGER_H
