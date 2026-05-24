#ifndef EXCELEXPORTER_H
#define EXCELEXPORTER_H

#include <QObject>
#include <QList>
#include "DatabaseManager.h"

class ExcelExporter : public QObject
{
    Q_OBJECT
public:
    explicit ExcelExporter(QObject *parent = nullptr);

    bool exportTestResults(const QString &filePath, const QList<TestResult> &results, const QList<FilterParams> &paramsList);
};

#endif
