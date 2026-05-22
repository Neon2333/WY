// PageMain.cpp
#include "PageMain.h"
#include "ui_PageMain.h"
#include "SessionManager.h"
#include "DatabaseManager.h"
#include "SensorDataProvider.h"
#include "LanguageManager.h"
#include "GlobalDefines.h"
#include "AuditLogger.h"
#include "ConfigSettings.h"
#include "OverlayMessage.h"
#include "MathUtils.h"    //道格拉斯简化线函数
#include "PermissionManager.h"
#include "DiffAlarmManager.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QTextEdit>
#include <QtMath>
#include <limits>
double lastV1 = 0, lastV2 = 0, lastV3 = 0, lastV4 = 0;
bool hasLast = false;

PageMain::PageMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageMain)
    , saveTimer(new QTimer(this))
    , serialReader(new SerialReader(this))
{
    ui->setupUi(this);

    //收数据
    connect(serialReader, &SerialReader::sensorPacketReceived, this, &PageMain::updatePressure);
    serialReader->start("COM4", 115200);
    SensorDataProvider::instance()->connectToSerialReader(serialReader);

    //定时保存传感器数据到历史库
    connect(saveTimer, &QTimer::timeout, this, &PageMain::saveDataToDB);

    // 创建压差图表并加入container
    diffWidget = new DiffBarWidget(ui->widgetDiff);
    ui->widgetDiff->layout()->addWidget(diffWidget);

    //图表
    initTrendChart();

    //记录压力-监控阈值
    {
        connect(ui->record1, &QCheckBox::stateChanged, this, &PageMain::recordStateChanged);
        connect(ui->record2, &QCheckBox::stateChanged, this, &PageMain::recordStateChanged);
        connect(ui->record3, &QCheckBox::stateChanged, this, &PageMain::recordStateChanged);
        connect(ui->record4, &QCheckBox::stateChanged, this, &PageMain::recordStateChanged);
        connect(ui->recordAll, &QCheckBox::stateChanged, this, &PageMain::recordAllChanged);
        connect(ui->alarm1, &QCheckBox::stateChanged, this, &PageMain::alarmStateChanged);
        connect(ui->alarm2, &QCheckBox::stateChanged, this, &PageMain::alarmStateChanged);
        connect(ui->alarm3, &QCheckBox::stateChanged, this, &PageMain::alarmStateChanged);
        connect(ui->alarm4, &QCheckBox::stateChanged, this, &PageMain::alarmStateChanged);
        connect(ui->alarmAll, &QCheckBox::stateChanged, this, &PageMain::alarmAllChanged);
    }
    //单位
    connect(SensorDataProvider::instance(), &SensorDataProvider::pressureUnitChanged,this, &PageMain::updateUnitLabels);
    //采样间隔
    connect(&SessionManager::instance(), &SessionManager::intervalChanged,this, &PageMain::samplingIntervalChanged);

    connect(ui->toZero1, &QPushButton::clicked, this, [this]() { calibrateSensor(1); });
    connect(ui->toZero2, &QPushButton::clicked, this, [this]() { calibrateSensor(2); });
    connect(ui->toZero3, &QPushButton::clicked, this, [this]() { calibrateSensor(3); });
    connect(ui->toZero4, &QPushButton::clicked, this, [this]() { calibrateSensor(4); });

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageMain::onUserChanged);

    // 连接阈值变化信号
    connect(&ThresholdSettings::instance(),&ThresholdSettings::thresholdChanged,this,&PageMain::reloadThreshold);

    connect(ui->btnMuteAlarm,&QPushButton::clicked,this,&PageMain::btnMuteAlarm);
    // 初始化UI权限状态
    updateUIByPermission();

    //语言
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageMain::retranslateUi);
}

PageMain::~PageMain()
{
    delete ui;
}

void PageMain::retranslateUi()
{
    ui->retranslateUi(this);
    loadUserSettings();
}

void PageMain::init()
{
    m_username  = SessionManager::instance().userName();
    m_userLevel = SessionManager::instance().userLevel();
    qDebug() << "[PageMain] 用户：" << m_username << " 权限：" << m_userLevel;

    if (!DatabaseManager::instance().openDatabase(DatabaseManager::HistoryDatabase)) {
        QMessageBox::critical(this, "错误", "打开历史数据库失败");
    }

    updateUnitLabels();
    loadUserSettings();
    reloadThreshold();
    // 新增：检查是否已初始化，避免重复绘制
    if (!m_trendInitialized) {
        initTrendFrame(ui->frameChart1, trendCharts[0]);
        initTrendFrame(ui->frameChart2, trendCharts[1]);
        initTrendFrame(ui->frameChart3, trendCharts[2]);
        initTrendFrame(ui->frameChart4, trendCharts[3]);
        m_trendInitialized = true;
    }

    //低频刷新定时器（1 秒一次）
    auto* trendTimer = new QTimer(this);
    trendTimer->setInterval(1000);
    connect(trendTimer, &QTimer::timeout, this, [&]{

        m_flashOn = !m_flashOn;   // 控制闪烁节奏

        for (int i = 0; i < 4; ++i)
            refreshTrendChart(i);

        updatePressureStyle();    //  刷新颜色
    });
    trendTimer->start();

}

//控制颜色
void PageMain::updatePressureStyle()
{
    QLabel* labels[4] = {
        ui->pressure1,
        ui->pressure2,
        ui->pressure3,
        ui->pressure4
    };

    for (int i = 0; i < 4; ++i) {

        // ===== 1 断线优先 =====
        if (m_sDisconnected[i]) {
            labels[i]->setStyleSheet("color: rgb(255, 80, 0);");
            continue;
        }

        // ===== 2️ 报警闪烁 =====
        if (m_channelAlarm[i]) {

            if (m_flashOn) {
                labels[i]->setStyleSheet("color: red;");
            } else {
                labels[i]->setStyleSheet("color: gray");
            }

            continue;
        }

        // ===== 正常状态 =====
        labels[i]->setStyleSheet("color:rgb(34, 149, 144)");
    }
}
// 读值更新压力-压差-波动-趋势-提示信息-显示
void PageMain::updatePressure(const SensorPacket &packet)
{
    using namespace Constants;
    int id = static_cast<int>(packet.sensor_id);

    // 直接从SensorDataProvider获取数据
    auto& data = SensorDataProvider::instance()->getLastData();

    // 统一使用DISCONNECTED_VALUE进行断线判断
    bool s1Disconnected = (data.display1 == DISCONNECTED_VALUE);
    bool s2Disconnected = (data.display2 == DISCONNECTED_VALUE);
    bool s3Disconnected = (data.display3 == DISCONNECTED_VALUE);
    bool s4Disconnected = (data.display4 == DISCONNECTED_VALUE);

    // 保存为成员变量（关键）
    m_sDisconnected[0] = s1Disconnected;
    m_sDisconnected[1] = s2Disconnected;
    m_sDisconnected[2] = s3Disconnected;
    m_sDisconnected[3] = s4Disconnected;

    // 更新标签显示
    switch (id) {
    case 1:
        if (s1Disconnected) {
            ui->pressure1->setText("---");
        } else {
            ui->pressure1->setText(QString::number(data.display1, 'f', 1));
        }
        break;

    case 2:
        if (s2Disconnected) {
            ui->pressure2->setText("---");
        } else {
            ui->pressure2->setText(QString::number(data.display2, 'f', 1));
        }
        break;

    case 3:
        if (s3Disconnected) {
            ui->pressure3->setText("---");
        } else {
            ui->pressure3->setText(QString::number(data.display3, 'f', 1));
        }
        break;

    case 4:
        if (s4Disconnected) {
            ui->pressure4->setText("---");
        } else {
            ui->pressure4->setText(QString::number(data.display4, 'f', 1));
        }
        break;

    default:
        qWarning() << "未知 sensor_id：" << id;
        return;
    }

    lastS1_mbar = data.display1;
    lastS2_mbar = data.display2;
    lastS3_mbar = data.display3;
    lastS4_mbar = data.display4;

    //------------6组压差更新到条形图---------------
    QVector<DiffItem> diffs;

    // 记录每个传感器是否有效
    bool sValid[4] = {data.display1 != DISCONNECTED_VALUE,data.display2 != DISCONNECTED_VALUE,data.display3 != DISCONNECTED_VALUE,data.display4 != DISCONNECTED_VALUE
    };

    double sVal[4] = {data.display1,data.display2,data.display3,data.display4
    };

    // 组合所有“已插入传感器对”
    for (int i = 0; i < 4; ++i) {
        if (!sValid[i]) continue;

        for (int j = i + 1; j < 4; ++j) {
            if (!sValid[j]) continue;

            QString name = QString("Δ%1%2").arg(i + 1).arg(j + 1);
            double value = sVal[i] - sVal[j];
            diffs.append({name, value});
        }
    }

    diffWidget->setDiffValues(diffs);// 显示到条形图    // 这里会触发 diffBarWidget 内部的 update() 调用 从而触发 paintEvent()

    DiffAlarmManager::instance().checkAndTriggerDiffAlarm(diffs);   //压差报警

    //大图表数据更新
    updateTempBuffer(lastS1_mbar, lastS2_mbar, lastS3_mbar, lastS4_mbar);

    //--------------波动趋势------------------------
    updateFluctuationAndTrend(data);// 显示到波动趋势

    /**********四个趋势小图表******************/
    appendTrendSample(0, lastS1_mbar);
    appendTrendSample(1, lastS2_mbar);
    appendTrendSample(2, lastS3_mbar);
    appendTrendSample(3, lastS4_mbar);

    // ===== 通道报警判断 =====
    checkAndTriggerAlarm(1, data.display1);
    checkAndTriggerAlarm(2, data.display2);
    checkAndTriggerAlarm(3, data.display3);
    checkAndTriggerAlarm(4, data.display4);

    // ===== 系统状态评估 & 串口 =====
    evaluateSystemState();
    updatePressureStyle();   //  刷新颜色（关键）
}

//////////////////////大图表///////////////////////

void PageMain::initTrendChart()
{
    chart = new QChart();
    chart->setMargins(QMargins(0, 0, 0, 0));// 设置图表的边距为0，减少空白
    // 四个系列
    s1 = new QLineSeries();
    s1->setName("SENSOR1");
    s2 = new QLineSeries();
    s2->setName("SENSOR2");
    s3 = new QLineSeries();
    s3->setName("SENSOR3");
    s4 = new QLineSeries();
    s4->setName("SENSOR4");
    s1->setPen(QPen(QColor(0, 128, 255), 2)); // 2px，适当增粗
    s2->setPen(QPen(QColor(0, 255, 0), 2));
    s3->setPen(QPen(QColor(255, 165, 0), 2));
    s4->setPen(QPen(QColor(128, 0, 128), 2));
    chart->addSeries(s1);
    chart->addSeries(s2);
    chart->addSeries(s3);
    chart->addSeries(s4);
    // X 轴：0.5 小时时间轴
    xAxis = new QDateTimeAxis();
    xAxis->setFormat("HH:mm");
    xAxis->setRange(QDateTime::currentDateTime().addSecs(-1800),QDateTime::currentDateTime());
    chart->addAxis(xAxis, Qt::AlignBottom);
    // Y 轴（初始固定范围，后续可自动调整）
    yAxis = new QValueAxis();
    yAxis->setRange(-10, 3000);
    chart->addAxis(yAxis, Qt::AlignLeft);
    for (auto s : {s1, s2, s3, s4})
    {
        s->attachAxis(xAxis);
        s->attachAxis(yAxis);
        // s->setUseOpenGL(true);开了opengl之后，一些控件比如combobox会导致屏幕闪屏
    }
    chartView = new QChartView(chart, ui->frameChart);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setFrameShape(QFrame::NoFrame);
    auto layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->addWidget(chartView);
    ui->frameChart->setLayout(layout);
    // 初始化缓存值（新增 tlastX 初始化）
    tmin1 = tmin2 = tmin3 = tmin4 = 1e9;
    tmax1 = tmax2 = tmax3 = tmax4 = -1e9;
    tlast1 = tlast2 = tlast3 = tlast4 = 0; // 修复：初始化 tlastX
    // 1000ms 压缩一次
    compressTimer = new QTimer(this);
    compressTimer->setInterval(250);
    connect(compressTimer, &QTimer::timeout, this, &PageMain::appendCompressedPoint);
    compressTimer->start();
}

void PageMain::updateTempBuffer(double v1, double v2, double v3, double v4)
{
    auto push = [](double val, double &mn, double &mx, double &last){
        if (val == Constants::DISCONNECTED_VALUE) return; // 跳过断线值
        if (val < mn) mn = val;
        if (val > mx) mx = val;
        last = val;
    };
    push(v1, tmin1, tmax1, tlast1);
    push(v2, tmin2, tmax2, tlast2);
    push(v3, tmin3, tmax3, tlast3);
    push(v4, tmin4, tmax4, tlast4);
}
/**
 * @brief 每隔一段时间（由compressTimer触发）压缩并追加数据点到图表
 *
 * 这个函数是实时趋势图的核心更新逻辑：
 * 1. 把最近1秒内的 min/max/last 值打包成一个 TrendPoint，追加到缓冲区
 * 2. 根据缓冲区点数，决定是否需要对曲线进行 Douglas-Peucker 简化
 * 3. 更新 QLineSeries（真正画图用的点集）
 * 4. 自适应调整 X 轴和 Y 轴范围
 */
void PageMain::appendCompressedPoint()
{
    // 当前时间戳（毫秒），作为所有新点的 x 坐标
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    // 缓冲区最大容量（滚动窗口大小），超过后自动移除最老点
    const int MAX_POINTS = 2340;            //720 + (30-3) * 60,720 = 4*60*3(4point/s * 60s * 3min)
    // Douglas-Peucker 简化触发参数
    const int DOUGLAS_START_POINTS = 900; // 点数达到这个值开始考虑简化
    const int DOUGLAS_STEP_POINTS = 900; // 每增加多少点触发一次全量简化
    // Lambda：把最近1秒的 min/max/last 打包成 TrendPoint 并追加到缓冲区
    auto addPoint = [&](QVector<TrendPoint>& buf, double& mn, double& mx, double last) {
        // 如果这一秒没有有效数据（mn > mx），就不追加（防止无效点）
        if (mn > mx) return;
        TrendPoint tp{now, mn, mx, last};
        buf.append(tp);
        // 保持缓冲区不超过 MAX_POINTS，移除最老的点（实现滚动窗口）
        while (buf.size() > MAX_POINTS)
            buf.removeFirst();
        // 重置这一秒的累积极值，为下一秒准备
        mn = 1e9;
        mx = -1e9;
    };
    // ===== 1. 更新四个传感器的缓冲区 =====
    // 只在有有效数据（tmin <= tmax）时才追加，避免无效点干扰
    if (tmin1 <= tmax1) addPoint(buffer1, tmin1, tmax1, tlast1);
    if (tmin2 <= tmax2) addPoint(buffer2, tmin2, tmax2, tlast2);
    if (tmin3 <= tmax3) addPoint(buffer3, tmin3, tmax3, tlast3);
    if (tmin4 <= tmax4) addPoint(buffer4, tmin4, tmax4, tlast4);
    // ===== 2. 对每个系列进行简化 & 更新 QLineSeries =====
    // 这个 lambda 处理单个系列的简化逻辑
    auto simplifyAndReplace = [&](QLineSeries* series, QVector<TrendPoint>& buf, int& lastCount ,int& lastDouglasLevel) {
        // 缓冲区为空，直接返回（无数据可画）
        if (buf.isEmpty())
            return;
        // 从缓冲区构建当前所有原始点（只用 ts 和 lastVal 画曲线）
        QVector<QPointF> rawPoints;
        for (const auto& tp : buf)
            rawPoints.append(QPointF(tp.ts, tp.lastVal));
        const int pointCount = rawPoints.size(); // 当前缓冲区点数
        // 阶段1：点数还很少，直接用原始点替换整个系列（不简化）
        if (pointCount < DOUGLAS_START_POINTS) {
            series->replace(rawPoints); // 完全替换，保持实时性
            lastCount = pointCount; // 记录已处理的 raw 点数
            return;
        }
        // 计算当前处于第几个简化“阶梯”
        // 例如：5~9点 level=1，10~14点 level=2，以此类推
        const int currentLevel = (pointCount - DOUGLAS_START_POINTS) / DOUGLAS_STEP_POINTS + 1;
        // 关键检测：缓冲区是否发生了滚动（removeFirst）？
        // 如果 series 的最早时间戳 < buffer 的最早时间戳，说明 buffer 踢掉了旧点，但 series 还没同步
        bool forceReplace = false;
        if (series->count() > 0) {
            const QVector<QPointF> seriesPoints = series->pointsVector();
            qint64 bufFirstTs = buf.first().ts;
            qreal seriesFirstX = seriesPoints[0].x();
            // 时间戳落后 → 说明 buffer 已滚动，需要强制全量更新 series
            if (seriesFirstX < bufFirstTs) {
                forceReplace = true;
            }
        }
        // 阶段2：如果跨级（level变化）或检测到滚动，则对整个当前缓冲区进行一次完整简化
        if (forceReplace || currentLevel != lastDouglasLevel) {
            // 更新 level（只在真正跨级时更新）
            if (currentLevel != lastDouglasLevel) {
                lastDouglasLevel = currentLevel;
            }
            // 执行 Douglas-Peucker 简化，得到关键点
            QVector<QPointF> simplified = simplifyCurve(rawPoints, 0.001, 0.2);
            // 用简化后的点完全替换系列 → 抹掉旧点，加入新点
            series->replace(simplified);
            // 更新 lastCount 为当前 raw 点数（用于后续判断新增了多少点）
            lastCount = pointCount;
            return;
        }
        // 阶段3：还在同一个阶梯，且有新点 → 只处理新增部分（增量更新，性能更好）
        if (pointCount > lastCount) {
            // 取出从 lastCount 开始的新点（即最近新增的 raw 点）
            QVector<QPointF> newRawPoints = rawPoints.mid(lastCount);
            // 对新增部分也进行简化（保持整体风格一致）
            QVector<QPointF> newSimplified = simplifyCurve(newRawPoints, 0.001, 0.2);
            // 逐点追加到系列末尾（连接上之前的简化曲线）
            for (const auto& pt : newSimplified) {
                series->append(pt);
            }
            // 更新 lastCount，记录已处理的 raw 点数（用增量方式，避免漏掉）
            lastCount += newRawPoints.size();
        }
        // 注意：如果 pointCount == lastCount，什么都不做（正常情况，无新数据）
    };
    // 分别对四个系列执行简化 & 更新
    simplifyAndReplace(s1, buffer1, lastCount1, lastDouglasLevel1);
    simplifyAndReplace(s2, buffer2, lastCount2, lastDouglasLevel2);
    simplifyAndReplace(s3, buffer3, lastCount3, lastDouglasLevel3);
    simplifyAndReplace(s4, buffer4, lastCount4, lastDouglasLevel4);
    // ===== 3. X 轴自适应（只考虑活跃传感器的数据窗口，从最短活跃缓冲开始） =====
    qint64 minTs = LLONG_MIN; // 初始化为最小可能值
    qint64 maxTs = LLONG_MIN; // 同上，用于maxTs
    // 认为多久没更新就算“断线/不活跃”，单位：毫秒
    // 建议值：10000(10秒) ~ 60000(1分钟)
    const qint64 ACTIVE_THRESHOLD_MS = 30000;
    bool hasActiveData = false;
    for (auto& buf : {&buffer1, &buffer2, &buffer3, &buffer4}) {
        if (!buf->isEmpty()) {
            qint64 lastTs = buf->last().ts;
            // 如果最后一条数据在“最近一段时间”内更新过，视为活跃
            if (now - lastTs <= ACTIVE_THRESHOLD_MS) {
                // minTs 取所有活跃first.ts的最大值（起点最新的）
                minTs = qMax(minTs, buf->first().ts);
                // maxTs 取所有活跃last.ts的最大值（通常接近now）
                maxTs = qMax(maxTs, lastTs);
                hasActiveData = true;
            }
        }
    }
    // 如果没有任何活跃传感器，就保持当前范围，或者强制显示最近 N 秒
    if (!hasActiveData) {
        // 这里简单处理：至少显示最近 2 分钟
        minTs = now - 120000;
        maxTs = now;
    } else {
        // 防止 minTs >= maxTs（无数据或单点）
        if (minTs >= maxTs) {
            maxTs = minTs + 1000;
        }
        // 如果 maxTs 还是小（异常），强制用now
        if (maxTs < now) {
            maxTs = now;
        }
    }
    xAxis->setRange(QDateTime::fromMSecsSinceEpoch(minTs),
                    QDateTime::fromMSecsSinceEpoch(maxTs));
    // ===== 4. Y 轴自适应（根据所有缓冲区中的 min/max 极值） =====
    double gmin = 1e9, gmax = -1e9;
    for (auto& buf : {&buffer1, &buffer2, &buffer3, &buffer4}) {
        for (const auto& tp : *buf) {
            // 使用每个压缩点的 minVal 和 maxVal（比只用 lastVal 更能捕捉波动）
            gmin = qMin(gmin, tp.minVal);
            gmax = qMax(gmax, tp.maxVal);
        }
    }
    // 有有效范围才调整，并加 5% 上下裕量，美观且避免贴边
    if (gmin < gmax) {
        double margin = (gmax - gmin) * 0.05;
        yAxis->setRange(gmin - margin, gmax + margin);
    }

    // ===== 5. 动态调整 compressTimer 的 interval（基于缓冲区点数） =====
    // 取所有缓冲区的最大点数作为参考
    int maxBufSize = 0;
    for (auto& buf : {&buffer1, &buffer2, &buffer3, &buffer4}) {
        maxBufSize = qMax(maxBufSize, buf->size());
    }
    // 如果点数 <= 720 250ms；> 720 ，用 1000ms
    const int threshold = 720;
    int newInterval = (maxBufSize <= threshold) ? 250 : 1000;
    if (compressTimer->interval() != newInterval) {
        compressTimer->setInterval(newInterval);
    }
}

//////////////////////四个趋势小图表////////////////////
// 只创建曲线系列，并设置颜色
void PageMain::initTrendFrame(QFrame* frame, TrendChart& tc)
{
    tc.chart = new QChart();
    tc.chart->setBackgroundVisible(false);  // 不显示背景
    tc.chart->setMargins(QMargins(0, 0, 0, 0));
    tc.chart->legend()->hide();  // 隐藏图例

    tc.series = new QLineSeries();
    // tc.series->setUseOpenGL(true);
    tc.chart->addSeries(tc.series);

    // 创建坐标轴但不显示
    tc.xAxis = new QDateTimeAxis();
    tc.yAxis = new QValueAxis();
    tc.chart->addAxis(tc.xAxis, Qt::AlignBottom);
    tc.chart->addAxis(tc.yAxis, Qt::AlignLeft);
    tc.series->attachAxis(tc.xAxis);
    tc.series->attachAxis(tc.yAxis);

    // 完全隐藏坐标轴
    tc.xAxis->setVisible(false);
    tc.yAxis->setVisible(false);
    tc.xAxis->setGridLineVisible(false);
    tc.yAxis->setGridLineVisible(false);

    // 设置曲线颜色
    QColor colors[4] = {
        QColor(0, 128, 255),  // 蓝色
        QColor(0, 255, 0),    // 绿色
        QColor(255, 165, 0),  // 橙色
        QColor(128, 0, 128)   // 紫色
    };
    tc.series->setPen(QPen(colors[&tc - trendCharts], 2));

    // 创建视图
    auto* view = new QChartView(tc.chart, frame);
    view->setRenderHint(QPainter::Antialiasing, true);  // 开启抗锯齿
    view->setFrameShape(QFrame::NoFrame);

    // 设置布局
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(view);
}

// 绘图函数
void PageMain::refreshTrendChart(int ch)
{
    if (ch < 0 || ch >= 4) return; // 防越界

    auto& buf = trendBuf[ch];
    auto& tc  = trendCharts[ch];

    tc.series->clear();
    if (buf.isEmpty())
        return;

    // ===== 1. 构建原始点序列，并计算 Y 轴 min/max =====
    QVector<QPointF> rawPoints;
    bool hasValidData = false;
    double dataMin = 0.0, dataMax = 0.0;

    for (const auto& b : buf) {
        if (!b.hasData) continue;

        double yValue = (b.minVal + b.maxVal) * 0.5;
        if (qIsNaN(yValue) || qIsInf(yValue)) continue;

        rawPoints.append(QPointF(b.tsStart, yValue));

        if (!hasValidData) {
            dataMin = dataMax = yValue;
            hasValidData = true;
        } else {
            dataMin = qMin(dataMin, yValue);
            dataMax = qMax(dataMax, yValue);
        }
    }

    if (!hasValidData || rawPoints.isEmpty()) return;

    // ===== 2. 防止 min == max =====
    if (qFuzzyCompare(dataMin, dataMax)) {
        dataMin -= 1.0;
        dataMax += 1.0;
    }

    // ===== 3. 设置 Y 轴自适应 =====
    const double padding = qMax(0.1, (dataMax - dataMin) * 0.05);
    tc.yAxis->setRange(dataMin - padding, dataMax + padding);

    // ===== 4. 设置 X 轴自适应（根据 buffer 的最早/最新时间） =====
    qint64 minTs = rawPoints.first().x();
    qint64 maxTs = rawPoints.last().x();
    if (minTs >= maxTs) maxTs = minTs + 1000; // 避免区间为0
    tc.xAxis->setRange(QDateTime::fromMSecsSinceEpoch(minTs),
                       QDateTime::fromMSecsSinceEpoch(maxTs));

    // ===== 6. 绘制曲线 =====
    tc.series->replace(rawPoints);
}

void PageMain::appendTrendSample(int ch, double val)
{
    // ===== 1. 彻底过滤非法浮点 =====
    if (qIsNaN(val) || qIsInf(val))
        return; // 如果值是 NaN (Not a Number) 或 Inf (Infinity)，则直接丢弃，不处理

    if (val == Constants::DISCONNECTED_VALUE)
        return; // 如果值是预定义的断开连接值，也直接丢弃

    auto& buf = trendBuf[ch];// 获取对应通道 ch 的趋势数据缓冲区引用

    // ===== 2. 时间桶计算逻辑 =====
    const qint64 now = QDateTime::currentMSecsSinceEpoch();// 获取当前时间戳（毫秒）
    // 计算每个时间桶的持续时间（毫秒）
    // 总显示窗口时间 (TREND_WINDOW_SEC 秒) / 总共需要多少个桶 (TREND_BUCKET_CNT)
    const qint64 bucketMs = (TREND_WINDOW_SEC * 1000LL) / TREND_BUCKET_CNT;
    // 计算当前时间点应该属于哪个桶的起始时间戳
    // 1. 将当前时间戳 now 除以桶的大小 bucketMs (整数除法，会截断余数)
    // 2. 再乘以桶的大小 bucketMs
    // 这样就得到了当前时间点所属桶的精确起始时间戳 (例如，如果桶大小是1000ms，
    // 那么 1001ms, 1500ms, 1999ms 都会落在起始时间为 1000ms 的那个桶里)
    const qint64 bucketStart = (now / bucketMs) * bucketMs;

    // ===== 3. 检查是否需要创建新桶 =====
    // 如果缓冲区为空，或者当前最新的桶的起始时间戳不是本次采样的目标桶
    if (buf.isEmpty() || buf.last().tsStart != bucketStart) {
        // 这意味着当前采样点落在了一个新的时间桶里，需要创建一个新桶
        TrendBucket b; // 创建一个新的桶实例
        b.tsStart = bucketStart; // 设置这个新桶的起始时间戳
        buf.append(b); // 将新桶追加到缓冲区末尾

        // ===== 4. 保持缓冲区大小固定 =====
        // 确保缓冲区中的桶数量不超过预设的最大数量 (TREND_BUCKET_CNT)
        while (buf.size() > TREND_BUCKET_CNT)
            buf.removeFirst(); // 如果超出了，就移除最旧的桶
    }

    // ===== 5. 更新当前桶的数据 =====
    // 获取刚刚确认或创建的、对应当前采样时间的桶的引用
    auto& b = buf.last();
    // 在这个桶内，更新聚合统计信息：
    b.minVal  = qMin(b.minVal, val); // 更新此桶内遇到的最小值
    b.maxVal  = qMax(b.maxVal, val); // 更新此桶内遇到的最大值
    b.lastVal = val;                 // 更新此桶内遇到的最后一个值（覆盖旧的lastVal）
    b.hasData = true;                // 标记此桶已有有效数据
}

//波动和箭头趋势
void PageMain::updateFluctuationAndTrend(const SensorData &data)
{
    using namespace Constants;

    auto calcTrend = [&](QVector<TrendPoint> &buf) -> int {
        if (buf.size() < 2) return 0;

        double last = buf.last().lastVal;
        double prev = buf[buf.size() - 2].lastVal;

        if (last - prev > 0.5) return +1;   // 上升
        if (prev - last > 0.5) return -1;   // 下降
        return 0;                           // 平稳
    };

    auto setTrendLabel = [&](QLabel *lab, int trend) {
        if (!lab) return;

        switch (trend) {
        case +1:
            lab->setText("↑");
            lab->setStyleSheet("color: #00AA00; font-weight:bold;");
            break;
        case -1:
            lab->setText("↓");
            lab->setStyleSheet("color: #AA0000; font-weight:bold;");
            break;
        default:
            lab->setText("→");
            lab->setStyleSheet("color: #666666; font-weight:bold;");
            break;
        }
    };

    auto setFlucLabel = [&](QLabel *lab, double fluc, bool disconnected) {
        if (!lab) return;

        if (disconnected) {
            lab->setText("---");
            lab->setStyleSheet("color:#555;");
            return;
        }

        lab->setText(QString::number(fluc, 'f', 1));

        if (fluc > 20)
            lab->setStyleSheet("color:#CC0000; font-weight:bold;");
        else if (fluc > 5)
            lab->setStyleSheet("color:#CC8800;");
        else
            lab->setStyleSheet("color:#008800;");
    };

    // 获取波动
    double f1 = calcFluctuationFromTrend(trendBuf[0]);
    double f2 = calcFluctuationFromTrend(trendBuf[1]);
    double f3 = calcFluctuationFromTrend(trendBuf[2]);
    double f4 = calcFluctuationFromTrend(trendBuf[3]);

    // 获取趋势
    int t1 = calcTrend(buffer1);
    int t2 = calcTrend(buffer2);
    int t3 = calcTrend(buffer3);
    int t4 = calcTrend(buffer4);

    // 设置 UI（含断线判断）
    setFlucLabel(ui->fluctuation1, f1, data.display1 == DISCONNECTED_VALUE);
    setFlucLabel(ui->fluctuation2, f2, data.display2 == DISCONNECTED_VALUE);
    setFlucLabel(ui->fluctuation3, f3, data.display3 == DISCONNECTED_VALUE);
    setFlucLabel(ui->fluctuation4, f4, data.display4 == DISCONNECTED_VALUE);

    setTrendLabel(ui->trend1, t1);
    setTrendLabel(ui->trend2, t2);
    setTrendLabel(ui->trend3, t3);
    setTrendLabel(ui->trend4, t4);
}

//计算波动
double PageMain::calcFluctuationFromTrend(const QVector<TrendBucket>& buf)
{
    if (buf.size() < 2)
        return 0.0;

    double mn = 1e9;
    double mx = -1e9;

    for (const auto& b : buf) {
        if (!b.hasData) continue;
        mn = qMin(mn, b.minVal);
        mx = qMax(mx, b.maxVal);
    }

    if (mn > mx)
        return 0.0;

    return mx - mn;
}

//保存数据到库
void PageMain::saveDataToDB()
{
    using namespace Constants;

    sqlite3* db = DatabaseManager::instance()
                      .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "[PageMain] 历史数据库未打开";
        return;
    }

    // 如果4路都未开启记录，则不用保存
    if (!rec1 && !rec2 && !rec3 && !rec4)
        return;

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString batchNum = ui->batchNumber->text().trimmed();

    auto makeVal = [&](bool rec, double mbar){
        if (!rec) return QVariant("OFF");
        if (mbar == DISCONNECTED_VALUE) return QVariant("-");
        return QVariant(QString::number(mbar, 'f', 1));
    };

    QVariant v1 = makeVal(rec1, lastS1_mbar);
    QVariant v2 = makeVal(rec2, lastS2_mbar);
    QVariant v3 = makeVal(rec3, lastS3_mbar);
    QVariant v4 = makeVal(rec4, lastS4_mbar);

    bool ok = DatabaseManager::instance().insertSensorData(
        timeStr, v1, v2, v3, v4,
        batchNum, m_username, m_userLevel);

    if (!ok)
        qDebug() << "[PageMain] 插入历史数据失败";
}

// 开关分路记录
void PageMain::recordStateChanged(int)
{
    bool prev1 = rec1;
    bool prev2 = rec2;
    bool prev3 = rec3;
    bool prev4 = rec4;

    // 更新当前状态
    rec1 = ui->record1->isChecked();
    rec2 = ui->record2->isChecked();
    rec3 = ui->record3->isChecked();
    rec4 = ui->record4->isChecked();

    // ====== 如果是从全部关闭 → 任意开启，先做批号校验 ======
    if (!prev1 && !prev2 && !prev3 && !prev4 &&
        (rec1 || rec2 || rec3 || rec4))
    {
        if (!checkBatchBeforeRecording(prev1, prev2, prev3, prev4))
            return;
    }

    // ====== 每一路发生变化都记录一次 ======
    if (prev1 != rec1)
        saveDataToDB();

    if (prev2 != rec2)
        saveDataToDB();

    if (prev3 != rec3)
        saveDataToDB();

    if (prev4 != rec4)
        saveDataToDB();

    // ---- 审计日志 ----
    if (sender() == ui->record1)
        logChannel(1, rec1 ? tr("开始记录") : tr("停止记录"));
    else if (sender() == ui->record2)
        logChannel(2, rec2 ? tr("开始记录") : tr("停止记录"));
    else if (sender() == ui->record3)
        logChannel(3, rec3 ? tr("开始记录") : tr("停止记录"));
    else if (sender() == ui->record4)
        logChannel(4, rec4 ? tr("开始记录") : tr("停止记录"));

    // 更新文字
    ui->record1->setText(rec1 ? tr("记录中") : tr("暂停中"));
    ui->record2->setText(rec2 ? tr("记录中") : tr("暂停中"));
    ui->record3->setText(rec3 ? tr("记录中") : tr("暂停中"));
    ui->record4->setText(rec4 ? tr("记录中") : tr("暂停中"));

    // recordAll 联动
    ui->recordAll->blockSignals(true);
    if (rec1 && rec2 && rec3 && rec4) {
        ui->recordAll->setChecked(true);
        ui->recordAll->setText(tr("全部记录中"));
    }
    else {
        ui->recordAll->setChecked(false);
        ui->recordAll->setText(tr("全运行关闭"));
    }
    ui->recordAll->blockSignals(false);

    // ====== 全部关闭时提示生成报告 ======
    if (prev1 || prev2 || prev3 || prev4) {
        if (!rec1 && !rec2 && !rec3 && !rec4) {

            auto *msg = new OverlayMessage(this);
            msg->setShowCancelButton(true);
            msg->showMessage(
                tr("生成报告"),
                tr("检测到当前批号 [%1] 已停止记录，是否生成报告？")
                    .arg(ui->batchNumber->text())
                );

            connect(msg, &OverlayMessage::closed, this, [=]() {
                tryGenerateReportForBatch(ui->batchNumber->text());
            });
        }
    }
}

// 开关数据记录总控
void PageMain::recordAllChanged(int state)
{
    bool prev1 = rec1;
    bool prev2 = rec2;
    bool prev3 = rec3;
    bool prev4 = rec4;

    bool on = (state == Qt::Checked);

    rec1 = rec2 = rec3 = rec4 = on;

    ui->record1->blockSignals(true);
    ui->record2->blockSignals(true);
    ui->record3->blockSignals(true);
    ui->record4->blockSignals(true);

    ui->record1->setChecked(on);
    ui->record2->setChecked(on);
    ui->record3->setChecked(on);
    ui->record4->setChecked(on);

    ui->record1->blockSignals(false);
    ui->record2->blockSignals(false);
    ui->record3->blockSignals(false);
    ui->record4->blockSignals(false);

    // ====== 如果是从全部关闭 → 开启，先校验 ======
    if (!prev1 && !prev2 && !prev3 && !prev4 && on)
    {
        if (!checkBatchBeforeRecording(prev1, prev2, prev3, prev4))
            return;
    }

    // ====== 总控模式只写一次 ======
    if (prev1 != rec1 || prev2 != rec2 || prev3 != rec3 || prev4 != rec4)
    {
        saveDataToDB();
    }

    if (on) {
        ui->recordAll->setText(tr("全部记录中"));
        ui->record1->setText(tr("记录中"));
        ui->record2->setText(tr("记录中"));
        ui->record3->setText(tr("记录中"));
        ui->record4->setText(tr("记录中"));
        AuditLogger::instance().log(tr("开启全部通道记录"));
    } else {
        ui->recordAll->setText(tr("全运行关闭"));
        ui->record1->setText(tr("暂停中"));
        ui->record2->setText(tr("暂停中"));
        ui->record3->setText(tr("暂停中"));
        ui->record4->setText(tr("暂停中"));
        AuditLogger::instance().log(tr("关闭全部通道记录"));

        auto *msg = new OverlayMessage(this);
        msg->setShowCancelButton(true);
        msg->showMessage(
            tr("生成报告"),
            tr("检测到当前批号 [%1] 已停止记录，是否生成报告？")
                .arg(ui->batchNumber->text())
            );

        connect(msg, &OverlayMessage::closed, this, [=]() {
            tryGenerateReportForBatch(ui->batchNumber->text());
        });
    }
}

//记录间隔改变信号
void PageMain::samplingIntervalChanged(float sec)
{
    saveTimer->start(static_cast<int>(sec * 1000));
    ui->samplingInterval->setText(QString::number(sec, 'f', 1));
}

//开关分路报警
void PageMain::alarmStateChanged(int)
{
    alarm_enable1 = ui->alarm1->isChecked();
    alarm_enable2 = ui->alarm2->isChecked();
    alarm_enable3 = ui->alarm3->isChecked();
    alarm_enable4 = ui->alarm4->isChecked();

    // ---- 审计日志（基于当前状态）----
    if (sender() == ui->alarm1)
        logChannel(1, alarm_enable1 ? tr("开始阈值报警监测") : tr("停止阈值报警监测"));
    else if (sender() == ui->alarm2)
        logChannel(2, alarm_enable2 ? tr("开始阈值报警监测") : tr("停止阈值报警监测"));
    else if (sender() == ui->alarm3)
        logChannel(3, alarm_enable3 ? tr("开始阈值报警监测") : tr("停止阈值报警监测"));
    else if (sender() == ui->alarm4)
        logChannel(4, alarm_enable4 ? tr("开始阈值报警监测") : tr("停止阈值报警监测"));

    // 更新按钮文字
    ui->alarm1->setText(alarm_enable1 ? tr("监测中") : tr("暂停中"));
    ui->alarm2->setText(alarm_enable2 ? tr("监测中") : tr("暂停中"));
    ui->alarm3->setText(alarm_enable3 ? tr("监测中") : tr("暂停中"));
    ui->alarm4->setText(alarm_enable4 ? tr("监测中") : tr("暂停中"));

    // 如果有任意一个通道取消，则 alarm_enableordAll 必须取消
    if (!(alarm_enable1 && alarm_enable2 && alarm_enable3 && alarm_enable4)) {
        ui->alarmAll->blockSignals(true);
        ui->alarmAll->setChecked(false);
        ui->alarmAll->setText(tr("全运行关闭"));
        ui->alarmAll->blockSignals(false);
    }

    // 如果全部都勾选了 → alarm_enableordAll 自动选中
    if (alarm_enable1 && alarm_enable2 && alarm_enable3 && alarm_enable4) {
        ui->alarmAll->blockSignals(true);
        ui->alarmAll->setChecked(true);
        ui->alarmAll->setText(tr("全部监测中"));
        ui->alarmAll->blockSignals(false);
    }
}

//开关报警总控
void PageMain::alarmAllChanged(int state)
{
    bool on = (state == Qt::Checked);

    // 更新 4 个单独通道的状态变量
    alarm_enable1 = alarm_enable2 = alarm_enable3 = alarm_enable4 = on;

    // 更新 UI 勾选状态（不会递归触发）
    ui->alarm1->blockSignals(true);
    ui->alarm2->blockSignals(true);
    ui->alarm3->blockSignals(true);
    ui->alarm4->blockSignals(true);

    ui->alarm1->setChecked(on);
    ui->alarm2->setChecked(on);
    ui->alarm3->setChecked(on);
    ui->alarm4->setChecked(on);

    ui->alarm1->blockSignals(false);
    ui->alarm2->blockSignals(false);
    ui->alarm3->blockSignals(false);
    ui->alarm4->blockSignals(false);

    // 更新文字
    if (on) {
        ui->alarmAll->setText(tr("全部监测中"));
        ui->alarm1->setText(tr("监测中"));
        ui->alarm2->setText(tr("监测中"));
        ui->alarm3->setText(tr("监测中"));
        ui->alarm4->setText(tr("监测中"));
        AuditLogger::instance().log(tr("开启全部报警监测"));
    } else {
        ui->alarmAll->setText(tr("全运行关闭"));
        ui->alarm1->setText(tr("暂停中"));
        ui->alarm2->setText(tr("暂停中"));
        ui->alarm3->setText(tr("暂停中"));
        ui->alarm4->setText(tr("暂停中"));
        AuditLogger::instance().log(tr("关闭全部报警监测"));
    }
}

// 从配置文件加载阈值与批次号
void PageMain::loadUserSettings()
{
    // 1. 批号（SessionManager）
    ui->batchNumber->setText(SessionManager::instance().batchNumber());

    // 2. 采样间隔（SessionManager）
    float intervalSec = SessionManager::instance().samplingInterval();

    // 检查有效性：0~600 秒
    if ((intervalSec < 0.0) || (intervalSec > 600.0)) {
        ui->samplingInterval->setText(QString::number(5.0, 'f', 1));
    } else {
        ui->samplingInterval->setText(QString::number(intervalSec, 'f', 1));
    }
    // 启动定时器（毫秒）
    if (!saveTimer->isActive()) {
        saveTimer->start(static_cast<int>(intervalSec * 1000));
    }

    // 3. 阈值min/max 标签（事件算法依赖此值）
    auto &ts = ThresholdSettings::instance();

    ui->min1->setText(QString::number(ts.alarmLower(1), 'f', 1));
    ui->max1->setText(QString::number(ts.alarmUpper(1), 'f', 1));
    ui->min2->setText(QString::number(ts.alarmLower(2), 'f', 1));
    ui->max2->setText(QString::number(ts.alarmUpper(2), 'f', 1));
    ui->min3->setText(QString::number(ts.alarmLower(3), 'f', 1));
    ui->max3->setText(QString::number(ts.alarmUpper(3), 'f', 1));
    ui->min4->setText(QString::number(ts.alarmLower(4), 'f', 1));
    ui->max4->setText(QString::number(ts.alarmUpper(4), 'f', 1));
}

//改变单位
void PageMain::updateUnitLabels()
{
    // 1. 获取当前单位并检查有效性
    QString unit = SensorDataProvider::instance()->pressureUnit();
    if (unit.isEmpty()) {
        qWarning() << "Pressure unit is empty!";
        return;
    }
    // 3. 直接更新已知的标签（避免动态查找）
    ui->preUnit->setText(unit);
}

//上下限刷新
void PageMain::reloadThreshold()
{
    auto &ts = ThresholdSettings::instance();

    for (int i = 0; i < 4; ++i) {
        m_min[i] = ts.alarmLower(i + 1);
        m_max[i] = ts.alarmUpper(i + 1);
    }
}

//报警判断
void PageMain::checkAndTriggerAlarm(int channel, double pressure)
{
    using namespace Constants;

    int idx = channel - 1;
    if (idx < 0 || idx >= 4)
        return;

    QCheckBox* record = nullptr;
    QCheckBox* alarm  = nullptr;


    switch (channel) {
    case 1: record = ui->record1; alarm = ui->alarm1; break;
    case 2: record = ui->record2; alarm = ui->alarm2; break;
    case 3: record = ui->record3; alarm = ui->alarm3; break;
    case 4: record = ui->record4; alarm = ui->alarm4; break;
    default: return;
    }

    bool recordChecked = record->isChecked();
    bool alarmChecked  = alarm->isChecked();

    // ===== 记录未开启：直接清除通道报警 =====
    if (!recordChecked) {
        if (m_channelAlarm[idx]) {
            AlarmType oldType = m_channelAlarmType[idx];
            m_channelAlarm[idx] = false;
            m_channelAlarmType[idx] = AlarmType::None;
            m_channelAbnormalSince[idx] = 0;
            m_channelNormalSince[idx] = 0;

            logChannel(channel,
                       oldType == AlarmType::Disconnected
                           ? tr("断线报警恢复")
                           : tr("阈值报警恢复"));
        }
        return;
    }

    bool disconnected = (pressure == DISCONNECTED_VALUE);

    double minV = m_min[idx];
    double maxV = m_max[idx];

    bool overHigh = false;
    bool overLow  = false;

    // 阈值报警必须：记录 + 阈值开关
    if (alarmChecked && !disconnected) {
        overHigh = pressure > maxV;
        overLow  = pressure < minV;
    }

    bool abnormal = disconnected || overHigh || overLow;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (abnormal) {
        if (m_channelAbnormalSince[idx] == 0)
            m_channelAbnormalSince[idx] = now;
    } else {
        m_channelAbnormalSince[idx] = 0;
    }

    bool abnormalStable =
        m_channelAbnormalSince[idx] != 0 && (now - m_channelAbnormalSince[idx] >= 500); //500ms的延时判断

    bool prevAlarm = m_channelAlarm[idx];

    AlarmType newType = AlarmType::None;
    if (abnormalStable) {
        newType = disconnected ? AlarmType::Disconnected : AlarmType::Threshold;
    }

    // ===== 报警进入 =====
    if (!prevAlarm && abnormalStable) {
        m_alarmMuted = false;       //新报警必须重新响
        emit alarmMuteChanged(false);  // 通知外部静音状态已取消
        m_channelAlarm[idx] = true;
        m_channelAlarmType[idx] = newType;
        m_channelNormalSince[idx] = 0;

        if (newType == AlarmType::Disconnected) {
            logChannel(channel, tr("断线报警"));
        } else {
            QString msg = overHigh? tr("超上限报警，报警值 %1，上限 %2")
                                    .arg(QString::number(pressure, 'f', 1),QString::number(maxV, 'f', 1))
                              : tr("超下限报警，报警值 %1，下限 %2")
                                    .arg(QString::number(pressure, 'f', 1),QString::number(minV, 'f', 1));
            logChannel(channel, msg);
            saveDataToDB();
        }
        return;
    }

    // ===== 报警恢复 =====
    if (prevAlarm && !abnormalStable) {

        if (m_channelNormalSince[idx] == 0)
            m_channelNormalSince[idx] = now;

        if (now - m_channelNormalSince[idx] >= 500) {

            AlarmType oldType = m_channelAlarmType[idx];

            m_channelAlarm[idx] = false;
            m_channelAlarmType[idx] = AlarmType::None;
            m_channelNormalSince[idx] = 0;

            logChannel(channel,oldType == AlarmType::Disconnected
                               ? tr("断线报警恢复")
                               : tr("阈值报警恢复"));
            saveDataToDB();
        }
    }
}

//系统评估 + 串口发送
void PageMain::evaluateSystemState()
{
    bool anyAlarm = false;

    for (int i = 0; i < 4; ++i) {
        if (m_channelAlarm[i]) {
            anyAlarm = true;
            break;
        }
    }

    // 压差
    if (DiffAlarmManager::instance().anyDiffAlarmActive()) {
        anyAlarm = true;
    }

    SerialState newState =
        anyAlarm ? SerialState::Threshold : SerialState::Normal;

    if (newState == m_lastSerialState)
        return;

    m_lastSerialState = newState;

    if (newState == SerialState::Threshold) {
        serialReader->sendThresholdAlarm();     // 报警
    } else {
        m_alarmMuted = false;                   // 恢复正常时，清掉静音状态
        emit alarmMuteChanged(false);           // 通知外部静音状态已取消
        serialReader->sendNormalState();        // 报警取消
    }
}

//静音操作
void PageMain::btnMuteAlarm()
{
    bool anyAlarm = false;
    for (int i = 0; i < 4; ++i) {
        if (m_channelAlarm[i]) {
            anyAlarm = true;
            break;
        }
    }
    if (!anyAlarm) return;
    if (m_alarmMuted) return;

    m_alarmMuted = true;
    serialReader->sendMuteAlarm();

    // 发射信号，通知外部静音状态已变为 true
    emit alarmMuteChanged(true);

    qDebug() << "已静音（报警仍然存在）";
}
// 判断是否刚刚开始一次记录会话
bool PageMain::justStartedRecording(bool prev1, bool prev2, bool prev3, bool prev4)
{
    bool noneRecordingBefore = !prev1 && !prev2 && !prev3 && !prev4;
    bool anyRecordingNow = rec1 || rec2 || rec3 || rec4;
    return noneRecordingBefore && anyRecordingNow;
}

// 判断是否刚刚停止全部通道
bool PageMain::justStoppedAllChannels(bool prev1, bool prev2, bool prev3, bool prev4)
{
    bool anyRecordingBefore = prev1 || prev2 || prev3 || prev4;
    bool allStoppedNow = !rec1 && !rec2 && !rec3 && !rec4;
    return anyRecordingBefore && allStoppedNow;
}

// 开始记录前检查函数（只检查批号）
bool PageMain::checkBatchBeforeRecording(bool prev1, bool prev2, bool prev3, bool prev4)
{
    QString batchNum = ui->batchNumber->text().trimmed();

    if (batchNum.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("*警告"), tr("批号不能为空，无法开始记录"));
        rollbackRecordingState(prev1, prev2, prev3, prev4);
        return false;
    }

    if (batchExistsInHistory(batchNum)) {
        auto *msg = new OverlayMessage(this);
        msg->setShowCancelButton(true);
        msg->showMessage(
            tr("*警告"),
            tr("批号 [%1] 已存在，是否继续使用该批号记录数据？").arg(batchNum)
            );

        connect(msg, &OverlayMessage::closed, this, [=]() {
            AuditLogger::instance().log(tr("批号已存在，但用户选择继续录入数据"));
        });

        connect(msg, &OverlayMessage::cancelled, this, [=]() {
            AuditLogger::instance().log(tr("批号已存在，用户取消录入数据"));
            rollbackRecordingState(prev1, prev2, prev3, prev4);
        });

        return true;   // 允许继续
    }

    return true;
}

// 判断批号是否已存在（history 表）
bool PageMain::batchExistsInHistory(const QString &batchNum)
{
    sqlite3* db = DatabaseManager::instance()
    .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) return false;

    const char *sql =
        "SELECT 1 FROM h_table WHERE number = ? LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1,
                          batchNum.toUtf8().constData(),
                          -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = true;
        }
    }
    sqlite3_finalize(stmt);
    return exists;
}

// 校验失败后，恢复 UI 和内部状态
void PageMain::rollbackRecordingState(bool prev1, bool prev2,
                                      bool prev3, bool prev4)
{
    // 1. 回滚内部状态
    rec1 = prev1;
    rec2 = prev2;
    rec3 = prev3;
    rec4 = prev4;

    // 2. 推导 recordAll 状态（派生状态）
    bool allOn = rec1 && rec2 && rec3 && rec4;

    // 3. 阻断信号，防止递归触发
    ui->record1->blockSignals(true);
    ui->record2->blockSignals(true);
    ui->record3->blockSignals(true);
    ui->record4->blockSignals(true);
    ui->recordAll->blockSignals(true);

    // 4. 回滚分路 UI
    ui->record1->setChecked(rec1);
    ui->record2->setChecked(rec2);
    ui->record3->setChecked(rec3);
    ui->record4->setChecked(rec4);

    ui->record1->setText(rec1 ? tr("记录中") : tr("暂停中"));
    ui->record2->setText(rec2 ? tr("记录中") : tr("暂停中"));
    ui->record3->setText(rec3 ? tr("记录中") : tr("暂停中"));
    ui->record4->setText(rec4 ? tr("记录中") : tr("暂停中"));

    // 5. 回滚 recordAll UI（关键）
    ui->recordAll->setChecked(allOn);
    ui->recordAll->setText(allOn
                               ? tr("全部记录中")
                               : tr("全运行关闭"));

    // 6. 恢复信号
    ui->record1->blockSignals(false);
    ui->record2->blockSignals(false);
    ui->record3->blockSignals(false);
    ui->record4->blockSignals(false);
    ui->recordAll->blockSignals(false);
}

bool PageMain::reportExists(const QString &batchNum)
{
    sqlite3* db = DatabaseManager::instance()
    .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        return false;
    }

    const char *sql = "SELECT 1 FROM report WHERE number = ? LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, batchNum.toUtf8().constData(),
                          -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = true;
        }
    }
    sqlite3_finalize(stmt);
    return exists;
}

void PageMain::tryGenerateReportForBatch(const QString &batchNum)
{
    QString currentUnit =
        SensorDataProvider::instance()->pressureUnit();

    // 批号已存在
    if (reportExists(batchNum)) {
        auto *msg = new OverlayMessage(this);
        msg->setShowCancelButton(true);
        msg->showMessage(
            tr("*生成报告"),
            tr("批号 %1（单位：%2）已经存在，是否覆盖？")
                .arg(batchNum,currentUnit)
            );

        connect(msg, &OverlayMessage::closed, this, [=]() {
            generateReport(batchNum);
        });
        return;
    }

    // 不存在直接生成
    generateReport(batchNum);
}

inline double round1(double v)
{
    return std::round(v * 10.0) / 10.0;
}

void PageMain::generateReport(const QString &batchNum)
{
    auto &ts = ThresholdSettings::instance();

    QMap<QString, QPair<double, double>> alarmLimits = {
        {"SENSOR1", {ts.alarmUpper(1), ts.alarmLower(1)}},
        {"SENSOR2", {ts.alarmUpper(2), ts.alarmLower(2)}},
        {"SENSOR3", {ts.alarmUpper(3), ts.alarmLower(3)}},
        {"SENSOR4", {ts.alarmUpper(4), ts.alarmLower(4)}}
    };

    sqlite3* db = DatabaseManager::instance()
                      .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "[PageMain] 无法连接历史数据库";
        return;
    }

    QMap<QString, QMap<QString, QVariant>> sensorDataForReport;
    QStringList sensors = {"SENSOR1","SENSOR2","SENSOR3","SENSOR4"};

    for (const QString &sensor : sensors) {

        float alarmUpper = alarmLimits[sensor].first;
        float alarmLower = alarmLimits[sensor].second;

        QString query =
            QString("SELECT %1 FROM h_table "
                    "WHERE number = ? AND %1 IS NOT NULL AND %1 != '' "
                    "AND CAST(%1 AS REAL) IS NOT NULL "
                    "ORDER BY historytime ASC").arg(sensor);

        sqlite3_stmt* stmt = nullptr;
        QVector<float> values;

        if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {

            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                values.append(static_cast<float>(sqlite3_column_double(stmt, 0)));
            }
        }
        sqlite3_finalize(stmt);

        float minVal = 0, maxVal = 0, avgVal = 0;
        int alarmCount = 0;

        if (!values.isEmpty()) {

            minVal = maxVal = values[0];
            float sum = 0;
            bool inAlarm = false;

            for (float v : values) {

                minVal = qMin(minVal, v);
                maxVal = qMax(maxVal, v);
                sum += v;

                bool outOfRange = (v < alarmLower || v > alarmUpper);

                if (outOfRange && !inAlarm) {
                    alarmCount++;
                    inAlarm = true;
                } else if (!outOfRange) {
                    inAlarm = false;
                }
            }

            avgVal = sum / values.size();
        }

        QMap<QString, QVariant> data;
        data["min_value"] = round1(minVal);
        data["max_value"] = round1(maxVal);
        data["avg_value"] = round1(avgVal);
        data["alarm_upper"] = alarmUpper;
        data["alarm_lower"] = alarmLower;
        data["alarm_count"] = alarmCount;

        sensorDataForReport[sensor] = data;
    }

    if (!saveToReportTable(batchNum, sensorDataForReport)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("更新报表"), tr("报表更新失败"));
    } else {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("更新报表"), tr("报表已更新"));
        AuditLogger::instance().log(tr("所有通道关闭，按提示生成报告"));
    }
}

// 保存数据到report表，并更新unit表
bool PageMain::saveToReportTable(const QString& batchNum,
                                 const QMap<QString, QMap<QString, QVariant>>& sensorData)
{
    sqlite3* db = DatabaseManager::instance()
    .getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接历史数据库";
        return false;
    }

    char* errMsg = nullptr;
    int rc;

    // ✅ 开启事务
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "开启事务失败:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    bool success = true;

    // ===== 时间范围 =====
    QPair<QString, QString> timeRange = getBatchTimeRange(batchNum);
    QString startTime = timeRange.first;
    QString endTime = timeRange.second;

    if (startTime.isEmpty() || endTime.isEmpty()) {
        startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        endTime = startTime;
    }

    // ===== 删除旧数据 =====
    {
        sqlite3_stmt* stmt = nullptr;
        QString sql = "DELETE FROM report WHERE number = ?";

        if (sqlite3_prepare_v2(db, sql.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE)
                success = false;
        }
        sqlite3_finalize(stmt);
    }

    // ===== unit 表 =====
    QString currentUnit = SensorDataProvider::instance()->pressureUnit();

    {
        sqlite3_stmt* stmt = nullptr;
        QString sql = "INSERT OR IGNORE INTO unit (number, unit) VALUES (?, ?)";

        if (sqlite3_prepare_v2(db, sql.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, currentUnit.toUtf8(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(stmt) != SQLITE_DONE)
                success = false;
        }
        sqlite3_finalize(stmt);
    }

    // ===== 插入 report =====
    sqlite3_stmt* insertStmt = nullptr;

    QString insertSql =
        "INSERT INTO report "
        "(sname, max, min, avg, upLimit, lowLimit, number, startTime, endTime, alarmCount) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, insertSql.toUtf8(), -1, &insertStmt, nullptr) != SQLITE_OK) {
        qDebug() << "prepare INSERT failed:" << sqlite3_errmsg(db);
        success = false;
    } else {

        for (auto it = sensorData.begin(); it != sensorData.end(); ++it) {

            const QString &sensorName = it.key();
            const auto &data = it.value();

            sqlite3_bind_text(insertStmt, 1, sensorName.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(insertStmt, 2, data["max_value"].toDouble());
            sqlite3_bind_double(insertStmt, 3, data["min_value"].toDouble());
            sqlite3_bind_double(insertStmt, 4, data["avg_value"].toDouble());
            sqlite3_bind_double(insertStmt, 5, data["alarm_upper"].toDouble());
            sqlite3_bind_double(insertStmt, 6, data["alarm_lower"].toDouble());
            sqlite3_bind_text(insertStmt, 7, batchNum.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStmt, 8, startTime.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insertStmt, 9, endTime.toUtf8(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insertStmt, 10, data["alarm_count"].toInt());

            if (sqlite3_step(insertStmt) != SQLITE_DONE) {
                qDebug() << "插入失败:" << sqlite3_errmsg(db);
                success = false;
                break;
            }

            sqlite3_reset(insertStmt);
            sqlite3_clear_bindings(insertStmt);
        }
    }

    sqlite3_finalize(insertStmt);

    // ===== 提交 / 回滚 =====
    if (success) {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }

    return success;
}

// 获取批号的开始时间和结束时间：查询数据库获取指定批号的时间范围
QPair<QString, QString> PageMain::getBatchTimeRange(const QString& batchNum)
{
    QPair<QString, QString> timeRange;  // 存储时间范围的配对

    // 获取历史数据库句柄
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::HistoryDatabase);
    if (!db) {
        qDebug() << "无法连接数据库";
        return timeRange;
    }

    // SQL查询：获取指定批号的最小和最大时间
    QString query = QString(
                        "SELECT MIN(historytime), MAX(historytime) FROM h_table "
                        "WHERE number = '%1' AND historytime IS NOT NULL AND historytime != ''")
                        .arg(batchNum);

    qDebug() << "查询时间范围:" << query;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.toUtf8(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* minTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));  // 最小时间
            const char* maxTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));  // 最大时间

            if (minTime) {
                timeRange.first = QString(minTime);
                // 确保时间格式正确
                if (!timeRange.first.contains(":")) {
                    // 如果时间格式不正确，尝试转换
                    QDateTime dt = QDateTime::fromString(timeRange.first, Qt::ISODate);
                    if (dt.isValid()) {
                        timeRange.first = dt.toString("yyyy-MM-dd HH:mm:ss");
                    }
                }
            }

            if (maxTime) {
                timeRange.second = QString(maxTime);
                // 确保时间格式正确
                if (!timeRange.second.contains(":")) {
                    // 如果时间格式不正确，尝试转换
                    QDateTime dt = QDateTime::fromString(timeRange.second, Qt::ISODate);
                    if (dt.isValid()) {
                        timeRange.second = dt.toString("yyyy-MM-dd HH:mm:ss");
                    }
                }
            }

            qDebug() << "批号" << batchNum << "的时间范围:" << timeRange.first << "到" << timeRange.second;
        } else {
            qDebug() << "未找到批号" << batchNum << "的数据";

            // 如果找不到数据，尝试检查数据库中是否有这个批号
            QString checkQuery = QString("SELECT COUNT(*) FROM h_table WHERE number = '%1'").arg(batchNum);
            sqlite3_stmt* checkStmt = nullptr;
            if (sqlite3_prepare_v2(db, checkQuery.toUtf8(), -1, &checkStmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(checkStmt) == SQLITE_ROW) {
                    int count = sqlite3_column_int(checkStmt, 0);
                    qDebug() << "批号" << batchNum << "在h_table中的记录数:" << count;
                }
                sqlite3_finalize(checkStmt);
            }
        }
        sqlite3_finalize(stmt);
    } else {
        qDebug() << "查询时间范围失败:" << sqlite3_errmsg(db);
    }

    return timeRange;
}

//审计日志辅助函数
void PageMain::logChannel(int channel, const QString& msg)
{
    QString op = tr("[通道%1]%2").arg(channel).arg(msg);
    AuditLogger::instance().log(op);
}

//校零功能
void PageMain::calibrateSensor(int sensorId) {
    SensorDataProvider::instance()->calibrateSensor(sensorId);

    AuditLogger::instance().log(QString(tr("传感器%1校零")).arg(sensorId));

    auto *msg = new OverlayMessage(this);
    msg->showMessage(tr("成功"), QString(tr("传感器%1校零完成")).arg(sensorId));

}

// 改变用户重新应用权限
void PageMain::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态************************************************
void PageMain::updateUIByPermission()
{
    // Standard权限控制的UI元素
    QList<QWidget*> Widgets = {
        ui->record1,ui->record2,ui->record3,ui->record4,ui->recordAll,
        ui->alarm1,ui->alarm2,ui->alarm3,ui->alarm4,ui->alarmAll,
        ui->toZero1,ui->toZero2,ui->toZero3,ui->toZero4
    };

    // 设置Standard权限控件
    for (auto widget : Widgets) {
        PermissionManager::setUIByPermission(PermissionManager::Standard, widget,{PermissionManager::QA});
    }
}

