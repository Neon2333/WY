// PageMain.h
#ifndef PAGEMAIN_H
#define PAGEMAIN_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include "SerialReader.h"
#include "DiffBarWidget.h"
#include "SensorData.h"
#include "GlobalDefines.h"


#include <QWidget>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QScatterSeries>
#include <QDateTime>

class SerialReader;

QT_CHARTS_USE_NAMESPACE

    struct TrendPoint
{
    qint64 ts;
    double minVal;
    double maxVal;
    double lastVal;
};

//数据结构
struct TrendBucket
{
    qint64 tsStart = 0;
    double minVal  = std::numeric_limits<double>::max();
    double maxVal  = std::numeric_limits<double>::lowest();
    double lastVal = 0.0;
    bool hasData   = false;
};

static constexpr int TREND_WINDOW_SEC  = 1800;   // 小图表，30 分钟
// static constexpr int TREND_BUCKET_CNT  = 120;   // 5 秒一个桶，120个桶
static constexpr int TREND_BUCKET_CNT  = 50;   // 36 秒一个桶，50个桶

//图表对象
struct TrendChart {
    QChart* chart = nullptr;
    QLineSeries* series = nullptr;
    QScatterSeries* minPts = nullptr;
    QScatterSeries* maxPts = nullptr;
    QDateTimeAxis* xAxis = nullptr;
    QValueAxis* yAxis = nullptr;
};


namespace Ui {
class PageMain;
}

class PageMain : public QWidget
{
    Q_OBJECT

public:
    explicit PageMain(QWidget *parent = nullptr);
    ~PageMain();

    void init();

private slots:
    void onUserChanged(const QString& name, const QString& level);
    void retranslateUi();
    void recordAllChanged(int state);
    void recordStateChanged(int);

    void alarmAllChanged(int state);
    void alarmStateChanged(int);

    void samplingIntervalChanged(float sec);
    void calibrateSensor(int sensorId);

    // void sensorDisconnected(int sensorId, QDateTime disconnectTime);

private:

    void updatePressure(const SensorPacket &packet);
    void initTrendChart();
    void updateTempBuffer(double v1, double v2, double v3, double v4);
    void appendCompressedPoint();
    void updateFluctuationAndTrend(const SensorData &data);
    void updateEventNotifications(int channel, double pressure, double fluctuation);
    void saveDataToDB();

    void saveUserSettings();
    void loadUserSettings();
    void updateUnitLabels();
    void reloadThreshold();
    void checkAndTriggerAlarm(int channel, double pressure);
    void logChannel(int channel, const QString& msg);  //审计日志辅助函数

    void appendTrendSample(int ch, double val);
    void initTrendFrame(QFrame* frame, TrendChart& tc);
    void refreshTrendChart(int ch);

    void evaluateSystemState();
    void updatePressureStyle();
    void btnMuteAlarm();

    double calcFluctuationFromTrend(const QVector<TrendBucket>& buf);
    void updateUIByPermission();

    // ====== 报告生成相关（从 PageAnalysis 复制过来）======

    bool reportExists(const QString &batchNum);// 批号是否已存在

    void tryGenerateReportForBatch(const QString &batchNum);// 生成报告主入口（带提示逻辑）

    void generateReport(const QString &batchNum);// 真正生成报告（复制 PageAnalysis::generateReport）
    bool saveToReportTable(const QString& batchNum,
                                     const QMap<QString, QMap<QString, QVariant>>& sensorData);
    QPair<QString, QString> getBatchTimeRange(const QString& batchNum);
    bool justStartedRecording(bool prev1, bool prev2, bool prev3, bool prev4);
    // 内部辅助：判断是否刚刚停止全部通道
    bool justStoppedAllChannels(bool prev1, bool prev2, bool prev3, bool prev4);
    void rollbackRecordingState(bool prev1, bool prev2, bool prev3, bool prev4);
    bool checkBatchBeforeRecording(bool prev1, bool prev2, bool prev3, bool prev4);
    bool batchExistsInHistory(const QString &batchNum);

private:
    Ui::PageMain *ui;
    QTimer       *saveTimer;
    SerialReader *serialReader;
    QString m_username;
    QString m_userLevel;

    DiffBarWidget *diffWidget;

    QChart *chart;
    QChartView *chartView;

    QLineSeries *s1;
    QLineSeries *s2;
    QLineSeries *s3;
    QLineSeries *s4;

    // lastCount 跟踪 raw buffer 的位置，使最后数据是拟合过的数据而不是实时读取的数据
    int lastCount1 = 0, lastCount2 = 0, lastCount3 = 0, lastCount4 = 0;

    int lastDouglasLevel1 = 0;
    int lastDouglasLevel2 = 0;
    int lastDouglasLevel3 = 0;
    int lastDouglasLevel4 = 0;

    QVector<QPointF> recentBuffer1;  // 实时未简化点缓冲，固定大小 e.g. 60 (1min if 1s/point)
    int recentMax = 60;  // 实时缓冲最大点数，根据需求调（太大影响性能）

    QValueAxis *yAxis;
    QDateTimeAxis *xAxis;

    QVector<TrendPoint> buffer1;
    QVector<TrendPoint> buffer2;
    QVector<TrendPoint> buffer3;
    QVector<TrendPoint> buffer4;

    QTimer *compressTimer;

    double tmin1, tmax1, tlast1;
    double tmin2, tmax2, tlast2;
    double tmin3, tmax3, tlast3;
    double tmin4, tmax4, tlast4;


    // 传感器原始值（统一以 mbar 存储）
    double lastS1_mbar = Constants::DISCONNECTED_VALUE;
    double lastS2_mbar = Constants::DISCONNECTED_VALUE;
    double lastS3_mbar = Constants::DISCONNECTED_VALUE;
    double lastS4_mbar = Constants::DISCONNECTED_VALUE;

    //数据记录变量
    bool rec1 = false;
    bool rec2 = false;
    bool rec3 = false;
    bool rec4 = false;

    bool alarm_enable1 = false;
    bool alarm_enable2 = false;
    bool alarm_enable3 = false;
    bool alarm_enable4 = false;

    bool   m_alarmActive = false;                                       // 当前系统报警状态
    bool m_channelAlarm[4] = { false, false, false, false };           // 通道是否处于报警
    bool m_sDisconnected[4] = {false, false, false, false};

    bool m_alarmMuted = false;  //静音
    bool m_flashOn = false;     //闪烁状态变量

    // ---- 3 秒稳定计时 ----
    qint64 m_channelAbnormalSince[4] = { 0, 0, 0, 0 };
    qint64 m_channelNormalSince[4]   = { 0, 0, 0, 0 };

    bool m_trendInitialized = false;  // 防止小趋势图重复初始化

    QVector<TrendBucket> trendBuf[4];
    TrendChart trendCharts[4];


    enum class AlarmType {
        None,
        Disconnected,
        Threshold
    };

    AlarmType m_channelAlarmType[4] = {
        AlarmType::None,
        AlarmType::None,
        AlarmType::None,
        AlarmType::None
    };

    // ---- 串口状态 ----
    enum class SerialState {
        Normal,
        Disconnected,
        Threshold
    };

    SerialState m_lastSerialState = SerialState::Normal;

    static const int PRE_DISCONNECT_CLEAR_MS = 500;   // 断开前清除的时间范围（毫秒）

    double m_min[4];
    double m_max[4];

signals:
    void alarmMuteChanged(bool muted);  // muted = true 表示已静音，false 表示未静音
};

#endif // PAGEMAIN_H
