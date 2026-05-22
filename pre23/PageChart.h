// PageChart.h
#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include "sqlite/sqlite3.h"
#include <QLineEdit>
#include <QDateTime>
#include <QtCharts/QChart> // Qt 5.12 使用 QtCharts 命名空间
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries> // Qt 5.12 使用 QtCharts 命名空间
#include <QSlider>

#include "SensorData.h"

QT_CHARTS_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class PageChart; }
QT_END_NAMESPACE

class PageChart : public QWidget
{
    Q_OBJECT

public:
    explicit PageChart(QWidget *parent = nullptr);
    ~PageChart();

    void init();
    void setDatabasePath(const QString &dbPath);

private:
    Ui::PageChart *ui;
    void retranslateUi();
    QString currentUnit; // 当前单位
    QChart *currentChart = nullptr;

    sqlite3 *db;
    QString currentDbPath;
    QList<hisSensorData> lastDataList;  //历史数据
    QDateTime startTimeFromCombo(const QString& option) const;
    QList<hisSensorData> readSensorData(const QString &startTime, const QString &endTime);
    void drawLineChart(const QList<hisSensorData>& dataList,const QString& title);

    // 辅助函数
    QDateTime getDateTimeFromLineEdits(
        QLineEdit* year, QLineEdit* mon, QLineEdit* day,
        QLineEdit* hour, QLineEdit* min, QLineEdit* sec,
        bool* ok) const;


    QDateTime lastProcessedSecond; // 记录上一次处理数据的时间（秒级）

    enum TitleMode { ComboBoxMode, FilterMode };
    TitleMode currentTitleMode;

    QDateTime manualStartTime;
    QDateTime manualEndTime;

    QLineSeries* findSeriesByName(QChart* chart, const QString &name);


    // 历史模式专用数据结构
    struct HistoricalChartData {
        QList<hisSensorData> dataList;
        QDateTime startTime;
        QDateTime endTime;
        QString title;
    };

    HistoricalChartData historicalData;

    void showTimeRangeError(bool startInvalid, bool endInvalid, bool reverse);
    void refreshNumberCombos();

    QDateTimeAxis *axisX = nullptr;
    QValueAxis *axisY = nullptr;

    QSlider *sliderStart = nullptr;
    QSlider *sliderEnd   = nullptr;

    bool internalAxisChange = false;  // 关键：防递归

    QDateTime sliderBaseStart;
    QDateTime sliderBaseEnd;

    void updateSlidersFromAxis(const QDateTime &min,const QDateTime &max);
    void autoAdjustYAxis(const QDateTime &xMin,const QDateTime &xMax);

    void resetSlidersToFullRange();
private slots:

    void onFilterClicked();         // 筛选按钮点击
    void redrawChart();
    void updateChartVisibility();   // 复选按钮点击
    void onPressureUnitChanged(const QString &newUnit); // 单位变化处理

    void onXAxisRangeChanged(const QDateTime &min,const QDateTime &max);
    void onSliderChanged();
    void onComboDateRangeChanged(const QString& text);
    void resetChart();
public slots:

    void onSelectNumberRangeChanged(const QString &number);  // 批号选择历史筛选

protected:
    void showEvent(QShowEvent *event) override;

};
