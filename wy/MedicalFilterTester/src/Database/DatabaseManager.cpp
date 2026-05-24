#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_connectionName("medical_filter_db")
{
}

bool DatabaseManager::initialize()
{
    QString dbPath = QCoreApplication::applicationDirPath() + "/medical_filter.db";
    
    // Delete old database to ensure clean start
    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    if (!createTables()) {
        return false;
    }

    // Insert admin user
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", "admin");
    query.bindValue(":password", "admin123");
    
    if (query.exec()) {
        qDebug() << "Admin user created successfully";
    } else {
        qDebug() << "Failed to create admin user:" << query.lastError().text();
    }

    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "username TEXT UNIQUE NOT NULL,"
                   "password TEXT NOT NULL,"
                   "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)")) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS filter_params ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "model TEXT NOT NULL,"
                   "area REAL NOT NULL,"
                   "test_pressure REAL NOT NULL,"
                   "coa_threshold REAL NOT NULL,"
                   "test_duration INTEGER NOT NULL,"
                   "tank_volume REAL NOT NULL,"
                   "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                   "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)")) {
        qDebug() << "Failed to create filter_params table:" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS test_results ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "filter_param_id INTEGER NOT NULL,"
                   "test_method TEXT NOT NULL,"
                   "test_data TEXT NOT NULL,"
                   "result TEXT NOT NULL,"
                   "operator TEXT NOT NULL,"
                   "test_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                   "FOREIGN KEY (filter_param_id) REFERENCES filter_params(id))")) {
        qDebug() << "Failed to create test_results table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::verifyUser(const QString& username, const QString& password)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString storedPassword = query.value(0).toString();
        qDebug() << "Verifying user" << username << "stored:" << storedPassword << "input:" << password;
        return storedPassword == password;
    }
    qDebug() << "User" << username << "not found";
    return false;
}

bool DatabaseManager::saveFilterParams(const FilterParams& params)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO filter_params (model, area, test_pressure, coa_threshold, test_duration, tank_volume) "
                 "VALUES (:model, :area, :test_pressure, :coa_threshold, :test_duration, :tank_volume)");
    query.bindValue(":model", params.model);
    query.bindValue(":area", params.area);
    query.bindValue(":test_pressure", params.testPressure);
    query.bindValue(":coa_threshold", params.coaThreshold);
    query.bindValue(":test_duration", params.testDuration);
    query.bindValue(":tank_volume", params.tankVolume);

    if (!query.exec()) {
        qDebug() << "Failed to save filter params:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateFilterParams(const FilterParams& params)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE filter_params SET model = :model, area = :area, test_pressure = :test_pressure, "
                 "coa_threshold = :coa_threshold, test_duration = :test_duration, tank_volume = :tank_volume, "
                 "updated_at = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":id", params.id);
    query.bindValue(":model", params.model);
    query.bindValue(":area", params.area);
    query.bindValue(":test_pressure", params.testPressure);
    query.bindValue(":coa_threshold", params.coaThreshold);
    query.bindValue(":test_duration", params.testDuration);
    query.bindValue(":tank_volume", params.tankVolume);

    if (!query.exec()) {
        qDebug() << "Failed to update filter params:" << query.lastError().text();
        return false;
    }
    return true;
}

FilterParams DatabaseManager::getLatestFilterParams()
{
    FilterParams params;
    QSqlQuery query(m_db);
    query.exec("SELECT id, model, area, test_pressure, coa_threshold, test_duration, tank_volume, "
              "created_at, updated_at FROM filter_params ORDER BY updated_at DESC LIMIT 1");

    if (query.next()) {
        params.id = query.value(0).toInt();
        params.model = query.value(1).toString();
        params.area = query.value(2).toDouble();
        params.testPressure = query.value(3).toDouble();
        params.coaThreshold = query.value(4).toDouble();
        params.testDuration = query.value(5).toInt();
        params.tankVolume = query.value(6).toDouble();
        params.createdAt = query.value(7).toString();
        params.updatedAt = query.value(8).toString();
    }
    return params;
}

QList<FilterParams> DatabaseManager::getAllFilterParams()
{
    QList<FilterParams> list;
    QSqlQuery query(m_db);
    query.exec("SELECT id, model, area, test_pressure, coa_threshold, test_duration, tank_volume, "
              "created_at, updated_at FROM filter_params ORDER BY updated_at DESC");

    while (query.next()) {
        FilterParams params;
        params.id = query.value(0).toInt();
        params.model = query.value(1).toString();
        params.area = query.value(2).toDouble();
        params.testPressure = query.value(3).toDouble();
        params.coaThreshold = query.value(4).toDouble();
        params.testDuration = query.value(5).toInt();
        params.tankVolume = query.value(6).toDouble();
        params.createdAt = query.value(7).toString();
        params.updatedAt = query.value(8).toString();
        list.append(params);
    }
    return list;
}

bool DatabaseManager::saveTestResult(const TestResult& result)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO test_results (filter_param_id, test_method, test_data, result, operator) "
                 "VALUES (:filter_param_id, :test_method, :test_data, :result, :operator)");
    query.bindValue(":filter_param_id", result.filterParamId);
    query.bindValue(":test_method", result.testMethod);
    query.bindValue(":test_data", result.testData);
    query.bindValue(":result", result.result);
    query.bindValue(":operator", result.operatorName);

    if (!query.exec()) {
        qDebug() << "Failed to save test result:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<TestResult> DatabaseManager::getAllTestResults()
{
    QList<TestResult> list;
    QSqlQuery query(m_db);
    query.exec("SELECT id, filter_param_id, test_method, test_data, result, operator, test_at "
              "FROM test_results ORDER BY test_at DESC");

    while (query.next()) {
        TestResult result;
        result.id = query.value(0).toInt();
        result.filterParamId = query.value(1).toInt();
        result.testMethod = query.value(2).toString();
        result.testData = query.value(3).toString();
        result.result = query.value(4).toString();
        result.operatorName = query.value(5).toString();
        result.testAt = query.value(6).toString();
        list.append(result);
    }
    return list;
}

QList<TestResult> DatabaseManager::getTestResultsByFilterId(int filterId)
{
    QList<TestResult> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, filter_param_id, test_method, test_data, result, operator, test_at "
                  "FROM test_results WHERE filter_param_id = :filter_id ORDER BY test_at DESC");
    query.bindValue(":filter_id", filterId);

    if (query.exec()) {
        while (query.next()) {
            TestResult result;
            result.id = query.value(0).toInt();
            result.filterParamId = query.value(1).toInt();
            result.testMethod = query.value(2).toString();
            result.testData = query.value(3).toString();
            result.result = query.value(4).toString();
            result.operatorName = query.value(5).toString();
            result.testAt = query.value(6).toString();
            list.append(result);
        }
    }
    return list;
}

bool DatabaseManager::deleteTestResult(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM test_results WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Failed to delete test result:" << query.lastError().text();
        return false;
    }
    return true;
}