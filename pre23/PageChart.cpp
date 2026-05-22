// PageChart.cpp
#include "PageChart.h"
#include "ui_PageChart.h"
#include "OverlayMessage.h"
#include "DatabaseManager.h"
#include "SensorDataProvider.h"
#include "OverlayMessage.h"
#include "LanguageManager.h"
#include "GlobalDefines.h"
#include "touchchartview.h"

#include <QVBoxLayout>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <limits>
#include <QPainter>
#include <QDateTime>
#include <cmath>
#include <QtQml/QQmlContext>
#include <QTimer>

PageChart::PageChart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageChart)
    , currentTitleMode(ComboBoxMode)
{
    ui->setupUi(this);

    // 2) 下拉框变化触发重绘
    connect(ui->selectDateRange,&QComboBox::currentTextChanged,this,&PageChart::onComboDateRangeChanged);

    // 4) 连接复选框状态变化信号到统一槽函数
    connect(ui->chkSensor1, SIGNAL(stateChanged(int)), this, SLOT(updateChartVisibility()));
    connect(ui->chkSensor2, SIGNAL(stateChanged(int)), this, SLOT(updateChartVisibility()));
    connect(ui->chkSensor3, SIGNAL(stateChanged(int)), this, SLOT(updateChartVisibility()));
    connect(ui->chkSensor4, SIGNAL(stateChanged(int)), this, SLOT(updateChartVisibility()));

    // 初始单位
    currentUnit = SensorDataProvider::instance()->pressureUnit();

    // 监听单位切换
    connect(SensorDataProvider::instance(), SIGNAL(pressureUnitChanged(QString)),
            this, SLOT(onPressureUnitChanged(QString)));

    connect(ui->hisFilterBtn, SIGNAL(clicked()), this, SLOT(onFilterClicked()));

    connect(ui->selectNumberRange, SIGNAL(currentTextChanged(QString)),this, SLOT(onSelectNumberRangeChanged(QString)));

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageChart::retranslateUi);
}

PageChart::~PageChart()
{
    delete ui;
}

void PageChart::retranslateUi()
{
    ui->retranslateUi(this);
}

// 【更新单位】
void PageChart::onPressureUnitChanged(const QString &newUnit)
{
    currentUnit = newUnit;
}

// 【获取数据】放到hisSensorData
QList<hisSensorData> PageChart::readSensorData(const QString &startTime,
                                               const QString &endTime)
{
    QList<hisSensorData> dataList;
    auto db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return dataList;

    const char* sql =
        "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4 "
        "FROM h_table WHERE datetime(historytime) BETWEEN ? AND ? "
        "ORDER BY datetime(historytime) ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "[图表页面] SQL 准备失败:" << sqlite3_errmsg(db);
        return dataList;
    }
    sqlite3_bind_text(stmt, 1, startTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hisSensorData d;
        d.time     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        //空值剔除
        auto parseValue = [](sqlite3_stmt* stmt, int col) -> double {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));

            if (!text) return std::numeric_limits<double>::quiet_NaN();
            QString str = QString(text).trimmed().toUpper();
            if (str == "N/A" || str == "INF" || str == "-INF") {
                return std::numeric_limits<double>::quiet_NaN();
            }
            bool ok = false;
            double val = str.toDouble(&ok);
            if (!ok || std::isinf(val) || std::isnan(val)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            return val;
        };

        d.hisSensor1 = parseValue(stmt, 1);
        d.hisSensor2 = parseValue(stmt, 2);
        d.hisSensor3 = parseValue(stmt, 3);
        d.hisSensor4 = parseValue(stmt, 4);
        dataList.append(d);
    }
    sqlite3_finalize(stmt);
    return dataList;
}

// 初始化：打开数据库、设置时间控件、触发首次绘制
void PageChart::init()
{
    // 1) 确保历史数据库已打开
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::HistoryDatabase)) {
        qWarning()<<"错误,无法打开历史数据库"<<this;
        return;
    }

    // 2) 设置默认选项并触发一次绘制
    ui->selectDateRange->setCurrentText(tr("过去1天"));

    refreshNumberCombos();

    // 3) 初始化开始/结束时间为"现在"和"一天前"
    QDateTime now = QDateTime::currentDateTime();
    QDateTime oneDayAgo = now.addDays(-1);
    ui->startYear->setText(oneDayAgo.toString("yyyy"));
    ui->startMon ->setText(oneDayAgo.toString("MM"));
    ui->startDay ->setText(oneDayAgo.toString("dd"));
    ui->startHour->setText(oneDayAgo.toString("HH"));
    ui->startMin ->setText(oneDayAgo.toString("mm"));
    ui->startSec ->setText(oneDayAgo.toString("ss"));
    ui->endYear  ->setText(now.toString("yyyy"));
    ui->endMon   ->setText(now.toString("MM"));
    ui->endDay   ->setText(now.toString("dd"));
    ui->endHour  ->setText(now.toString("HH"));
    ui->endMin   ->setText(now.toString("mm"));
    ui->endSec   ->setText(now.toString("ss"));
}

void PageChart::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    static bool isFirstShow = true;
    if (!isFirstShow) return;
    isFirstShow = false;

    sliderStart = ui->sliderStart;
    sliderEnd   = ui->sliderEnd;

    connect(sliderStart, &QSlider::valueChanged,this, &PageChart::onSliderChanged);
    connect(sliderEnd, &QSlider::valueChanged,this, &PageChart::onSliderChanged);

    connect(ui->resetChart,&QPushButton::clicked,this,&PageChart::resetChart);
}

// 按【批号】筛选
void PageChart::onSelectNumberRangeChanged(const QString &number)
{
    // 忽略提示文本
    if (number == tr("请选择批号") || number.trimmed().isEmpty())
        return;

    // 进入历史模式
    currentTitleMode = FilterMode;

    // 构造 SQL 查询
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qWarning() << "[批号查询] 数据库未打开";
        return;
    }

    QList<hisSensorData> list;

    const char *sql =
        "SELECT historytime, SENSOR1, SENSOR2, SENSOR3, SENSOR4 "
        "FROM h_table WHERE number = ? ORDER BY datetime(historytime) ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "[批号查询] SQL 错误:" << sqlite3_errmsg(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    // 读取
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hisSensorData d;
        d.time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        auto parseValue = [](sqlite3_stmt* s, int col) -> double {
            const char* t = reinterpret_cast<const char*>(sqlite3_column_text(s, col));
            if (!t) return std::numeric_limits<double>::quiet_NaN();
            QString str = QString(t).trimmed().toUpper();
            bool ok = false;
            double v = str.toDouble(&ok);
            if (!ok || std::isinf(v) || std::isnan(v))
                return std::numeric_limits<double>::quiet_NaN();
            return v;
        };

        d.hisSensor1 = parseValue(stmt, 1);
        d.hisSensor2 = parseValue(stmt, 2);
        d.hisSensor3 = parseValue(stmt, 3);
        d.hisSensor4 = parseValue(stmt, 4);
        list.append(d);
    }
    sqlite3_finalize(stmt);

    // 更新历史数据缓存
    historicalData.dataList = list;

    // 设置图表标题
    const QString title= tr("批号 [%1] 的历史曲线").arg(number);
    drawLineChart(
        historicalData.dataList,
        QString("<b><font size='5'>%1</font></b>").arg(title)
        );

}

// 点击【筛选】按钮筛选
void PageChart::onFilterClicked()
{
    const QString busyText = tr("日期筛选模式");
    int busyIndex = ui->selectDateRange->findText(busyText);
    if (busyIndex == -1) {
        // 如果还没这项，就插到最前面
        ui->selectDateRange->insertItem(0, busyText);
        busyIndex = 0;
    }
    ui->selectDateRange->setCurrentIndex(busyIndex);

    currentTitleMode = FilterMode;

    // 获取时间范围
    bool ok1, ok2;
    QDateTime start = getDateTimeFromLineEdits(
        ui->startYear, ui->startMon, ui->startDay,
        ui->startHour, ui->startMin, ui->startSec, &ok1);

    QDateTime end;
    if (ui->endYear->text().isEmpty() ||
        ui->endMon->text().isEmpty() ||
        ui->endDay->text().isEmpty())
    {
        end = QDateTime::currentDateTime();
        ok2 = true;
    } else {
        end = getDateTimeFromLineEdits(
            ui->endYear, ui->endMon, ui->endDay,
            ui->endHour, ui->endMin, ui->endSec, &ok2);
    }

    // 验证时间
    if (!ok1 || !ok2 || start > end) {
        showTimeRangeError(!ok1, !ok2, start > end);
        return;
    }

    // 存储历史数据
    historicalData.dataList = readSensorData(
        start.toString("yyyy-MM-dd HH:mm:ss"),
        end.toString("yyyy-MM-dd HH:mm:ss"));

    historicalData.startTime = start;
    historicalData.endTime = end;
    const QString title = QString(tr("%1 至 %2 曲线"))
                               .arg(start.toString("yyyy-MM-dd HH:mm"),
                                    end.toString("yyyy-MM-dd HH:mm"));
    drawLineChart(
        historicalData.dataList,
        QString("<b><font size='5'>%1</font></b>").arg(title)
        );

}

// 【时间下拉框】
void PageChart::onComboDateRangeChanged(const QString& text)
{
    if (text.isEmpty())
        return;

    QDateTime start = startTimeFromCombo(text);
    QDateTime end   = QDateTime::currentDateTime();

    historicalData.dataList = readSensorData(
        start.toString("yyyy-MM-dd HH:mm:ss"),
        end.toString("yyyy-MM-dd HH:mm:ss")
        );

    const QString title = tr("[%1]数据曲线").arg(text);

    drawLineChart(
        historicalData.dataList,
        QString("<b><font size='5'>%1</font></b>").arg(title)
        );
}

///////////////【滑块】///////////////
// 【滑块适配时间范围】
void PageChart::resetSlidersToFullRange()
{
    // 基本防护
    if (!sliderStart || !sliderEnd || !axisX) {
        qWarning() << "[resetSliders] UI 未就绪，跳过";
        return;
    }

    if (historicalData.dataList.isEmpty()) {
        qDebug() << "[滑块重置] 数据列表为空，使用默认范围";
        sliderStart->blockSignals(true);
        sliderEnd->blockSignals(true);
        sliderStart->setRange(0, 1000);  // 随便一个正数，避免0
        sliderEnd->setRange(0, 1000);
        sliderStart->setValue(0);
        sliderEnd->setValue(1000);
        sliderStart->blockSignals(false);
        sliderEnd->blockSignals(false);
        return;
    }

    QDateTime minTime;
    QDateTime maxTime;

    // 尝试从第一条和最后一条取（假设数据已按时间升序）
    if (!historicalData.dataList.isEmpty()) {
        const auto &first = historicalData.dataList.first();
        const auto &last  = historicalData.dataList.last();

        minTime = QDateTime::fromString(first.time, "yyyy-MM-dd HH:mm:ss");
        maxTime = QDateTime::fromString(last.time, "yyyy-MM-dd HH:mm:ss");
    }

    // 如果首尾无效，才完整遍历
    if (!minTime.isValid() || !maxTime.isValid() || minTime >= maxTime) {
        minTime = QDateTime();  // invalid
        maxTime = QDateTime();

        for (const auto &d : historicalData.dataList) {
            QDateTime t = QDateTime::fromString(d.time, "yyyy-MM-dd HH:mm:ss");
            if (!t.isValid()) continue;

            if (!minTime.isValid() || t < minTime) minTime = t;
            if (!maxTime.isValid() || t > maxTime) maxTime = t;
        }
    }

    // 还是无效 → 用当前时间前后一天兜底
    if (!minTime.isValid() || !maxTime.isValid() || minTime >= maxTime) {
        qWarning() << "[滑块重置] 无法从数据中获取有效时间范围，使用默认";
        minTime = QDateTime::currentDateTime().addDays(-1);
        maxTime = QDateTime::currentDateTime();
    }

    sliderBaseStart = minTime;
    sliderBaseEnd   = maxTime;

    qint64 totalMSecs = minTime.msecsTo(maxTime);
    if (totalMSecs <= 0) {
        totalMSecs = 3600000; // 1小时兜底
    }

    sliderStart->blockSignals(true);
    sliderEnd->blockSignals(true);

    sliderStart->setRange(0, totalMSecs);
    sliderEnd->setRange(0, totalMSecs);

    sliderStart->setValue(0);
    sliderEnd->setValue(totalMSecs);

    sliderStart->blockSignals(false);
    sliderEnd->blockSignals(false);

    // 只在 axisX 已经存在时才设置
    if (axisX && currentChart) {

        axisX->setRange(minTime, maxTime);// 避免在图表还没画完就崩溃
    }
}

void PageChart::onXAxisRangeChanged(const QDateTime &min,const QDateTime &max)
{
    if (internalAxisChange)
        return;

    internalAxisChange = true;

    // 更新滑块的时间基准，防止滑块永远以筛选之后的尺度来进行，而不是根据当前屏幕中的X轴的范围进行变换
    sliderBaseStart = min;
    sliderBaseEnd   = max;

    updateSlidersFromAxis(min, max);// 同步滑块 UI（只映射，不反向影响 axis）

    autoAdjustYAxis(min, max);// 根据当前可见区间自适应 Y 轴

    internalAxisChange = false;
}

// 【滑块适配时间范围】把 X 轴的可见时间段，映射成滑块的 [0 ~ span]
void PageChart::updateSlidersFromAxis(const QDateTime &min,const QDateTime &max)
{
    if (!sliderStart || !sliderEnd) return;

    qint64 span = min.msecsTo(max);
    if (span <= 0) return;

    sliderStart->blockSignals(true);
    sliderEnd->blockSignals(true);

    sliderStart->setRange(0, span);
    sliderEnd->setRange(0, span);
    sliderStart->setValue(0);
    sliderEnd->setValue(span);

    sliderStart->blockSignals(false);
    sliderEnd->blockSignals(false);
}

void PageChart::onSliderChanged()
{
    if (internalAxisChange) return;
    if (!axisX || !sliderBaseStart.isValid()) return;

    qint64 left  = sliderStart->value();
    qint64 right = sliderEnd->value();
    if (left >= right) return;

    internalAxisChange = true;

    axisX->setRange(
        sliderBaseStart.addMSecs(left),
        sliderBaseStart.addMSecs(right)
        );

    internalAxisChange = false;
}

// 【自动更新Y 轴】纵坐标
void PageChart::autoAdjustYAxis(const QDateTime &xMin, const QDateTime &xMax)
{
    if (historicalData.dataList.isEmpty()) {
        return;
    }

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    bool hasValidData = false;

    for (const auto &d : historicalData.dataList) {
        QDateTime t = QDateTime::fromString(d.time, "yyyy-MM-dd HH:mm:ss");
        if (!t.isValid() || t < xMin || t > xMax) {
            continue;
        }

        double values[4] = {
            SensorDataProvider::instance()->convertPressure(d.hisSensor1, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor2, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor3, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor4, currentUnit)
        };

        for (int i = 0; i < 4; ++i) {
            // 只考虑当前可见的传感器（重要！）
            if (std::isnan(values[i])) continue;

            // 根据曲线可见性过滤（避免隐藏曲线影响Y轴范围）
            QString sensorName = QString("SENSOR%1").arg(i+1);
            bool isVisible = false;
            if (auto* series = findSeriesByName(currentChart, sensorName)) {
                isVisible = series->isVisible();
            }
            // 如果你暂时不care可见性，也可以全部考虑： isVisible = true;

            if (isVisible) {
                minY = qMin(minY, values[i]);
                maxY = qMax(maxY, values[i]);
                hasValidData = true;
            }
        }
    }

    // 有有效数据才调整
    if (hasValidData && minY < maxY) {
        double range = maxY - minY;
        double margin = range * 0.12;  // 建议 10%~15% 余量，看视觉效果调整

        double newMin = minY - margin;
        double newMax = maxY + margin;

        // 可选：可以在这里做一些限制，比如最小范围不能太小
        // if (newMax - newMin < 10) { newMax = newMin + 10; }

        // 找到Y轴并设置（推荐只操作一个Y轴的情况）
        if (currentChart) {
            auto axes = currentChart->axes(Qt::Vertical);
            if (!axes.isEmpty()) {
                if (auto *yAxis = qobject_cast<QValueAxis*>(axes.first())) {
                    // 避免频繁无意义的setRange（可减少闪烁）
                    if (qAbs(yAxis->min() - newMin) > 0.01 || qAbs(yAxis->max() - newMax) > 0.01) {
                        currentChart->setAnimationOptions(QChart::NoAnimation);
                        yAxis->setRange(newMin, newMax);
                        currentChart->setAnimationOptions(QChart::SeriesAnimations);
                    }
                }
            }
        }
    }
}

// 【勾选曲线】仅更新 Y 轴范围，不重载数据
void PageChart::redrawChart()
{
    if (!currentChart) return;

    // 获取当前 X 轴范围
    auto axesX = currentChart->axes(Qt::Horizontal);
    if (axesX.isEmpty()) return;
    auto *axisX = qobject_cast<QDateTimeAxis*>(axesX.first());
    if (!axisX) return;

    QDateTime minX = axisX->min();
    QDateTime maxX = axisX->max();

    if (!minX.isValid() || !maxX.isValid()) return;

    // 直接调用自动调整 Y 轴
    autoAdjustYAxis(minX, maxX);
}

// 【勾选曲线】更新曲线可见性（不重载数据）
void PageChart::updateChartVisibility()
{
    auto layout = ui->chartsInOne->layout();
    if (!layout || layout->count() == 0) return; // Qt 5.12 使用 count()

    auto view = qobject_cast<QChartView*>(layout->itemAt(0)->widget());
    if (!view) return;

    QChart* chart = view->chart();
    if (!chart) return;

    if (!layout || layout->count() == 0) {
        redrawChart(); // 图表不存在时触发完整重绘
        return;
    }

    // 获取所有曲线系列
    auto seriesList = chart->series();
    for (auto* series : seriesList) {
        if (auto* lineSeries = qobject_cast<QLineSeries*>(series)) {
            // 根据曲线名称匹配复选框状态
            if (lineSeries->name() == "SENSOR1") {
                lineSeries->setVisible(ui->chkSensor1->isChecked());
            } else if (lineSeries->name() == "SENSOR2") {
                lineSeries->setVisible(ui->chkSensor2->isChecked());
            } else if (lineSeries->name() == "SENSOR3") {
                lineSeries->setVisible(ui->chkSensor3->isChecked());
            } else if (lineSeries->name() == "SENSOR4") {
                lineSeries->setVisible(ui->chkSensor4->isChecked());
            }
        }
    }

    // 确保至少一条曲线可见
    bool anyVisible = false;
    for (auto* series : seriesList) {
        if (series->isVisible()) anyVisible = true;
    }
    if (!anyVisible) {
        ui->chkSensor1->setChecked(true);  // 默认启用第一条
        for (auto* series : seriesList) {
            if (series->name() == "SENSOR1") series->setVisible(true);
        }
    }

    // 可见性改变后重新自适应 Y 轴
    if (currentChart)
    {
        redrawChart();
    }
}

// 【绘制并显示折线图】（唯一图表入口）
void PageChart::drawLineChart(const QList<hisSensorData>& dataList,const QString& title)
{
    /* === 1. 创建 Chart=== */
    QChart* chart = new QChart();
    currentChart = chart;
    chart->setTitle(title);

    /* === 2. 创建 4 条传感器曲线=== */
    QVector<QLineSeries*> sensors(4);
    QVector<QCheckBox*> checkboxes = {
        ui->chkSensor1,
        ui->chkSensor2,
        ui->chkSensor3,
        ui->chkSensor4
    };

    for (int i = 0; i < 4; ++i) {
        sensors[i] = new QLineSeries();
        sensors[i]->setName(QString("SENSOR%1").arg(i + 1));
        sensors[i]->setVisible(checkboxes[i]->isChecked());
        chart->addSeries(sensors[i]);
    }

    /* === 3. 填充数据点=== */
    for (const auto& d : dataList) {
        QDateTime dt = QDateTime::fromString(d.time, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid())
            continue;

        const qint64 x = dt.toMSecsSinceEpoch();

        const double values[4] = {
            SensorDataProvider::instance()->convertPressure(d.hisSensor1, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor2, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor3, currentUnit),
            SensorDataProvider::instance()->convertPressure(d.hisSensor4, currentUnit)
        };

        for (int i = 0; i < 4; ++i) {
            if (!std::isnan(values[i])) {
                sensors[i]->append(x, values[i]);
            }
        }
    }

    /* === 4. 创建 X 轴（时间轴）=== */
    axisX = new QDateTimeAxis();   // 成员变量
    axisX->setFormat("MM-dd HH:mm");
    axisX->setTitleText(tr("时间"));
    chart->addAxis(axisX, Qt::AlignBottom);

    // X 轴范围变化 → 同步 slider & Y 轴
    connect(axisX, &QDateTimeAxis::rangeChanged,this,  &PageChart::onXAxisRangeChanged);

    /* === 5. 创建 Y 轴=== */
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText(QString(tr("压力 (%1)")).arg(currentUnit));
    axisY->setRange(0, 5000);  // 初始兜底范围
    chart->addAxis(axisY, Qt::AlignLeft);

    /* === 6. 绑定坐标轴=== */
    for (auto* s : chart->series()) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }


    /* === 7. 创建 ChartView 并显示=== */
    QChartView* view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);

    //交互能力，鼠标触摸用
    view->setRubberBand(QChartView::RectangleRubberBand); // 框选缩放
    view->setDragMode(QGraphicsView::ScrollHandDrag);     // 拖动平移

    // TouchChartView* view = new TouchChartView(chart); //加双指触摸，没改好，目前用着不行
    // view->setRenderHint(QPainter::Antialiasing);
    // view->setRubberBand(QChartView::NoRubberBand);
    // view->setDragMode(QGraphicsView::NoDrag);

    /* === 8. 清空旧视图并添加新图=== */
    QLayout* layout = ui->chartsInOne->layout();
    if (!layout) {
        layout = new QVBoxLayout(ui->chartsInOne);
        ui->chartsInOne->setLayout(layout);
    } else {
        QLayoutItem* item;
        while ((item = layout->takeAt(0))) {
            delete item->widget();
            delete item;
        }
    }
    layout->addWidget(view);


    /* === 9. 延迟初始化滑块范围=== */
    // 确保 chart / axisX / view 已完全 ready
    QTimer::singleShot(0, this, [this]() {
        resetSlidersToFullRange();
    });
}

// 【更新下拉列表】
void PageChart::refreshNumberCombos()
{
    ui->selectNumberRange->clear();

    // 添加默认的提示选项
    ui->selectNumberRange->addItem(tr("请选择批号"));

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return;

    const char *sql = "SELECT DISTINCT number FROM h_table ORDER BY number ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        QString number = text ? QString(text) : "";

        // 排除空字符串和只包含空白字符的选项
        if (!number.trimmed().isEmpty()) {
            ui->selectNumberRange->addItem(number);
        }
    }

    sqlite3_finalize(stmt);
}

// 【重置图表】重置图表为缩放之前的样子
void PageChart::resetChart()
{
    if (!currentChart || !axisX || historicalData.dataList.isEmpty()) {
        qWarning() << "[resetChart] 图表或数据未就绪，无法重置";
        return;
    }

    //1 恢复 X 轴范围为整个数据的时间范围
    QDateTime minTime = QDateTime::fromString(historicalData.dataList.first().time, "yyyy-MM-dd HH:mm:ss");
    QDateTime maxTime = QDateTime::fromString(historicalData.dataList.last().time,  "yyyy-MM-dd HH:mm:ss");

    // 安全兜底
    if (!minTime.isValid() || !maxTime.isValid() || minTime >= maxTime) {
        minTime = QDateTime::currentDateTime().addDays(-1);
        maxTime = QDateTime::currentDateTime();
    }

    sliderBaseStart = minTime;
    sliderBaseEnd   = maxTime;

    internalAxisChange = true; // 避免触发 onXAxisRangeChanged

    axisX->setRange(minTime, maxTime);

    //2 重置滑块
    qint64 totalMSecs = minTime.msecsTo(maxTime);
    sliderStart->blockSignals(true);
    sliderEnd->blockSignals(true);

    sliderStart->setRange(0, totalMSecs);
    sliderEnd->setRange(0, totalMSecs);
    sliderStart->setValue(0);
    sliderEnd->setValue(totalMSecs);

    sliderStart->blockSignals(false);
    sliderEnd->blockSignals(false);

    internalAxisChange = false;

    //3 可选：重新自适应 Y 轴
    // autoAdjustYAxis(minTime, maxTime);
}


///////////////【辅助函数】///////////////
// 【辅助函数】显示时间范围错误
void PageChart::showTimeRangeError(bool startInvalid, bool endInvalid, bool reverse)
{
    QString message;
    if (startInvalid && endInvalid) {
        message = tr("开始时间和结束时间无效");
    } else if (startInvalid) {
        message = tr("开始时间无效");
    } else if (endInvalid) {
        message = tr("结束时间无效");
    } else if (reverse) {
        message = tr("开始时间不能晚于结束时间");
    }

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("警告"), message);
}

// 【辅助函数】【获取时间区间】从LineEdits获取时间
QDateTime PageChart::getDateTimeFromLineEdits(
    QLineEdit* year, QLineEdit* mon, QLineEdit* day,
    QLineEdit* hour, QLineEdit* min, QLineEdit* sec,
    bool* ok) const
{
    if (ok) *ok = false;
    auto toInt = [&](QLineEdit* le) -> QPair<bool, int> {
        bool c; int v = le->text().toInt(&c); return qMakePair(c, v);
    };
    auto result1 = toInt(year);  if (!result1.first) return QDateTime();
    auto result2 = toInt(mon);   if (!result2.first) return QDateTime();
    auto result3 = toInt(day);   if (!result3.first) return QDateTime();
    auto result4 = toInt(hour);  if (!result4.first) return QDateTime();
    auto result5 = toInt(min);   if (!result5.first) return QDateTime();
    auto result6 = toInt(sec);   if (!result6.first) return QDateTime();

    int y = result1.second;
    int m = result2.second;
    int d = result3.second;
    int h = result4.second;
    int mi = result5.second;
    int s = result6.second;

    if (!QDate::isValid(y, m, d)) return QDateTime();
    QDate date(y, m, d); QTime time(h, mi, s);
    if (!date.isValid()||!time.isValid()) return QDateTime();
    if (ok) *ok = true;
    return QDateTime(date, time);
}

// 【辅助函数】按名称查找系列
QLineSeries* PageChart::findSeriesByName(QChart* chart, const QString &name)
{
    for (auto* series : chart->series()) {
        if (auto* lineSeries = qobject_cast<QLineSeries*>(series)) {
            if (lineSeries->name() == name) {
                return lineSeries;
            }
        }
    }
    return nullptr;
}

// 【辅助函数】根据选下拉框生成起始时间字符串
QDateTime PageChart::startTimeFromCombo(const QString& option) const
{
    const QDateTime now = QDateTime::currentDateTime();

    if (option == tr("今天"))     return QDateTime(now.date(), QTime(0, 0));

    if (option == tr("过去1小时"))  return now.addSecs(-3600);

    if (option == tr("过去1天"))   return now.addDays(-1);

    if (option == tr("过去1周"))   return now.addDays(-7);

    if (option == tr("过去1月"))   return now.addMonths(-1);

    return now.addDays(-1); // 默认
}
