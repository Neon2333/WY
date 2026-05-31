#ifndef SQLITEHELPER_HPP
#define SQLITEHELPER_HPP

#include <../3rdParty/sqlite3/sqlite3.h>
#include <QString>
#include <QList>
#include <QVariant>
#include <QByteArray>
#include <optional>
#include "../Common/config.hpp"
#include "../Common/utils.hpp"

class ResultSet
{
public:
    ResultSet(sqlite3* db, sqlite3_stmt* stmt) : db_(db), stmt_(stmt) {}

    ~ResultSet()
    {
        if(stmt_)
        {
            sqlite3_finalize(stmt_);
            stmt_ = nullptr;
        }
    }

    // Non-copyable (owns stmt_)
    ResultSet(const ResultSet&) = delete;
    ResultSet& operator=(const ResultSet&) = delete;

    // Movable
    ResultSet(ResultSet&& other) noexcept
        : db_(other.db_), stmt_(other.stmt_)
    {
        other.db_ = nullptr;
        other.stmt_ = nullptr;
    }
    ResultSet& operator=(ResultSet&& other) noexcept
    {
        if (this != &other)
        {
            if (stmt_) 
                sqlite3_finalize(stmt_);
            db_ = other.db_;
            stmt_ = other.stmt_;
            other.db_ = nullptr;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    // 移动到下一行，返回 false 表示没有更多数据或出错
    bool Next()
    {
        if (!stmt_) 
            return false;
        int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW)   //获取一行数据
            return true;
        if (rc != SQLITE_DONE)
            qout << "step failed:" << sqlite3_errmsg(db_);
        return false;
    }

    // 判断结果集是否为空（消耗第一行，再次调用 Next 会返回第二行）
    bool IsEmpty()
    {
        return !Next();
    }

    // 获取列名列表
    QList<QString> GetColumnNames() const
    {
        QList<QString> names;
        int count = ColumnCount();
        for (int i = 0; i < count; i++)
            names.append(QString::fromUtf8(sqlite3_column_name(stmt_, i)));
        return names;
    }

    // 获取列数
    int ColumnCount() const
    {
        return stmt_ ? sqlite3_column_count(stmt_) : 0;
    }

    // 由字段名获取 int 类型的值
    int GetInt(const QString& columnName) const
    {
        int idx = columnIndex(columnName);
        return idx >= 0 ? sqlite3_column_int(stmt_, idx) : 0;
    }

    // 由字段名获取 QString 类型的值
    QString GetString(const QString& columnName) const
    {
        int idx = columnIndex(columnName);
        if (idx < 0) return {};
        auto text = sqlite3_column_text(stmt_, idx);
        return text ? QString::fromUtf8(reinterpret_cast<const char*>(text)) : QString();
    }

    // 由字段名获取 float 类型的值
    float GetFloat(const QString& columnName) const
    {
        return static_cast<float>(GetDouble(columnName));
    }

    // 由字段名获取 double 类型的值
    double GetDouble(const QString& columnName) const
    {
        int idx = columnIndex(columnName);
        return idx >= 0 ? sqlite3_column_double(stmt_, idx) : 0.0;
    }

    // 按照列索引获取值，返回 QVariant 类型
    QVariant GetValue(int colIndex) const
    {
        if (!stmt_ || colIndex < 0 || colIndex >= ColumnCount())
            return {};

        switch (sqlite3_column_type(stmt_, colIndex))
        {
        case SQLITE_INTEGER:
            return static_cast<long long>(sqlite3_column_int64(stmt_, colIndex));
        case SQLITE_FLOAT:
            return sqlite3_column_double(stmt_, colIndex);
        case SQLITE_TEXT:
            return QString::fromUtf8(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt_, colIndex)));
        case SQLITE_BLOB:
        {
            int size = sqlite3_column_bytes(stmt_, colIndex);
            return QByteArray(static_cast<const char*>(sqlite3_column_blob(stmt_, colIndex)), size);
        }
        case SQLITE_NULL:
        default:
            return {};
        }
    }

private:
    // 根据列名查找列索引
    int columnIndex(const QString& name) const
    {
        if (!stmt_) return -1;
        int count = sqlite3_column_count(stmt_);
        for (int i = 0; i < count; i++)
        {
            const char* colName = sqlite3_column_name(stmt_, i);
            if (colName && name == QString::fromUtf8(colName))
                return i;
        }
        return -1;
    }

    sqlite3* db_{nullptr};
    sqlite3_stmt* stmt_{nullptr};
};

class SqliteConnection
{
public:
    SqliteConnection()
    {
        int rc = sqlite3_open(dbPath_.toStdString().c_str(), &db_);
        qout << "db path: " << dbPath_;
        if (rc)
        {
            qerr << "can not open db.." << sqlite3_errmsg(db_);
            sqlite3_close(db_);
            assert(false);
        }
        qout << "open db success..";
    }

    ~SqliteConnection()
    {
        if(db_)
        {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    // 执行非高频简单语句(建表、删表、改表等)
    bool Execute(const QString& sql)
    {
        char* errmsg = nullptr;
        int rc = sqlite3_exec(db_, sql.toStdString().c_str(), nullptr, nullptr, &errmsg);
        if(rc != SQLITE_OK)
        {
            qout << "sql failed on execute: (" << sql << "):    " << errmsg;
            sqlite3_free(errmsg);
            return false;
        }
        return true;
    }

    // 执行查询语句返回结果集（lazy 模式，ResultSet::Next 中逐行 step）
    template<typename... Args>
    std::optional<ResultSet> QueryDb(const QString& sql, Args... args)
    {
        std::string sql_str = sql.toStdString();
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql_str.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            qout << "prepare failed:" << sqlite3_errmsg(db_);
            return std::nullopt;
        }

        bind_all(stmt, std::forward<Args>(args)...);

        return ResultSet(db_, stmt);
    }

    // 获取受上次操作影响的行数
    int ChangedRowsCount() const
    {
        return sqlite3_changes(db_);
    }

    // 事务操作
    bool BeginTransaction() { return Execute("BEGIN TRANSACTION"); }
    bool Commit()          { return Execute("COMMIT"); }
    bool Rollback()        { return Execute("ROLLBACK"); }

private:
    sqlite3* db_{nullptr};
    QString dbPath_{Config::Instance().GetDbPath()};
};

template<typename... Args>
void bind_all(sqlite3_stmt* stmt, Args&&... args)
{
    int idx = 1;  // SQLite 参数索引从 1 开始
    (bind_param(stmt, idx++, std::forward<Args>(args)), ...);
}

// 绑定 int
void bind_param(sqlite3_stmt* stmt, int idx, int value) {
    sqlite3_bind_int(stmt, idx, value);
}

// 绑定 int64_t (long long)
void bind_param(sqlite3_stmt* stmt, int idx, sqlite3_int64 value) {
    sqlite3_bind_int64(stmt, idx, value);
}

// 绑定 double
void bind_param(sqlite3_stmt* stmt, int idx, double value) {
    sqlite3_bind_double(stmt, idx, value);
}

// 绑定 const char* (C 字符串)
void bind_param(sqlite3_stmt* stmt, int idx, const char* value) {
    sqlite3_bind_text(stmt, idx, value, -1, SQLITE_TRANSIENT);
}

// 绑定 QString (转换为 UTF-8)
void bind_param(sqlite3_stmt* stmt, int idx, const QString& value) {
    QByteArray utf8 = value.toUtf8();
    sqlite3_bind_text(stmt, idx, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
}

// 绑定 std::string
void bind_param(sqlite3_stmt* stmt, int idx, const std::string& value) {
    sqlite3_bind_text(stmt, idx, value.c_str(), value.size(), SQLITE_TRANSIENT);
}

// 绑定 BLOB (QByteArray)
void bind_param(sqlite3_stmt* stmt, int idx, const QByteArray& blob) {
    sqlite3_bind_blob(stmt, idx, blob.constData(), blob.size(), SQLITE_TRANSIENT);
}

// 绑定 NULL (std::nullopt / nullptr)
void bind_param(sqlite3_stmt* stmt, int idx, std::nullopt_t) {
    sqlite3_bind_null(stmt, idx);
}
void bind_param(sqlite3_stmt* stmt, int idx, std::nullptr_t) {
    sqlite3_bind_null(stmt, idx);
}

#endif
