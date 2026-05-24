#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QMap>

struct FilterParams {
    int id;
    QString model;
    double area;
    double testPressure;
    double coaThreshold;
    int testDuration;
    double tankVolume;
    QString createdAt;
    QString updatedAt;
};

struct TestResult {
    int id;
    int filterParamId;
    QString testMethod;
    QString testData;
    QString result;
    QString operatorName;
    QString testAt;
};

struct User {
    int id;
    QString username;
    QString password;
    QString createdAt;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();
    explicit DatabaseManager(QObject *parent = nullptr);

    bool initialize();
    bool verifyUser(const QString& username, const QString& password);

    bool saveFilterParams(const FilterParams& params);
    bool updateFilterParams(const FilterParams& params);
    FilterParams getLatestFilterParams();
    QList<FilterParams> getAllFilterParams();

    bool saveTestResult(const TestResult& result);
    QList<TestResult> getAllTestResults();
    QList<TestResult> getTestResultsByFilterId(int filterId);
    bool deleteTestResult(int id);

private:
    bool createTables();
    QSqlDatabase m_db;
    QString m_connectionName;
};

#endif
