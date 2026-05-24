#include "ExcelExporter.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QStringConverter>

ExcelExporter::ExcelExporter(QObject *parent)
    : QObject(parent)
{
}

bool ExcelExporter::exportTestResults(const QString &filePath, const QList<TestResult> &results, const QList<FilterParams> &paramsList)
{
    QFile file(filePath);
    if (file.exists()) {
        file.remove();
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    stream << QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5\u62A5\u8868\n");
    stream << QString::fromUtf8("\u5BFC\u51FA\u65F6\u95F4: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";

    stream << QString::fromUtf8("\u6D4B\u8BD5\u5387\u5F55\n");
    stream << "----------------------------------------------------------------------------------------------------------------\n";
    stream << QString::fromUtf8("\u5E8F\u53F7") << "\t" << QString::fromUtf8("\u6DA1\u5FC3\u578B\u53F7") << "\t" << QString::fromUtf8("\u6D4B\u8BD5\u65B9\u6CD5") << "\t"
           << QString::fromUtf8("\u6D4B\u8BD5\u65F6\u95F4") << "\t" << QString::fromUtf8("\u64CD\u4F5C\u5458") << "\t" << QString::fromUtf8("\u6D4B\u8BD5\u7ED3\u679C") << "\t"
           << QString::fromUtf8("Q\u503C(ml/min)") << "\t" << QString::fromUtf8("COA\u9608\u503C") << "\n";

    for (int i = 0; i < results.size(); ++i) {
        const TestResult &result = results[i];
        FilterParams params;
        for (const FilterParams &p : paramsList) {
            if (p.id == result.filterParamId) {
                params = p;
                break;
            }
        }

        double qValue = 0, coaValue = 0;
        QJsonObject testData = QJsonDocument::fromJson(result.testData.toUtf8()).object();
        if (!testData.isEmpty()) {
            qValue = testData["Q"].toDouble();
            coaValue = testData["COA"].toDouble();
        }

        QString testMethodName;
        if (result.testMethod == "diffusion_flow") {
            testMethodName = QString::fromUtf8("\u6269\u6563\u6D41\u6D4B\u8BD5");
        } else if (result.testMethod == "bubble_point") {
            testMethodName = QString::fromUtf8("\u6CE1\u70B9\u6D4B\u8BD5");
        } else {
            testMethodName = result.testMethod;
        }

        QString resultText = (result.result == "qualified") ? QString::fromUtf8("\u5408\u683C") : QString::fromUtf8("\u4E0D\u5408\u683C");

        stream << (i + 1) << "\t" << params.model << "\t" << testMethodName << "\t"
               << result.testAt << "\t" << result.operatorName << "\t" << resultText << "\t"
               << QString::number(qValue, 'f', 4) << "\t" << QString::number(coaValue, 'f', 4) << "\n";
    }

    stream << "\n\n";
    stream << QString::fromUtf8("\u6DA1\u5FC3\u89C4\u683C\u53C2\u6570\n");
    stream << "----------------------------------------------------------------------------------------------------------------\n";
    stream << QString::fromUtf8("\u5E8F\u53F7") << "\t" << QString::fromUtf8("\u578B\u53F7") << "\t" << QString::fromUtf8("\u9762\u79EF(cm\u00B2)")
           << "\t" << QString::fromUtf8("\u6D4B\u8BD5\u538B\u529B(mbar)") << "\t" << QString::fromUtf8("COA\u9608\u503C(ml/min)")
           << "\t" << QString::fromUtf8("\u6D4B\u8BD5\u65F6\u95F4(s)") << "\t" << QString::fromUtf8("\u6C14\u7F50\u4F53\u79EF(ml)") << "\n";

    for (int i = 0; i < paramsList.size(); ++i) {
        const FilterParams &p = paramsList[i];
        stream << (i + 1) << "\t" << p.model << "\t" << p.area << "\t" << p.testPressure << "\t"
               << p.coaThreshold << "\t" << p.testDuration << "\t" << p.tankVolume << "\n";
    }

    file.close();

    return true;
}
