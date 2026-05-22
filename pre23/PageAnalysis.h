#ifndef PAGEANALYSIS_H
#define PAGEANALYSIS_H

#include <QWidget>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include <QPrinter>

QT_CHARTS_USE_NAMESPACE

    namespace Ui {
    class PageAnalysis;
}

struct PrintContext
{
    QPainter* painter = nullptr;
    QPagedPaintDevice* device = nullptr;   // 支持 QPdfWriter + QPrinter

    QRect pageRect;

    // 字体
    QFont titleFont;
    QFont smallFont;
    QFont textFont;
    QFont headerFont;
    QFont bodyFont;

    // 常量
    int margin = 40;
    int headerHeight = 120;
    int subHeaderHeight = 60;
    int TextHeight = 80;
    int tableRowHeight = 50;
    int chartHeight = 650;
    int chartSpacing = 50;

    qreal scale = 1.0;   // 不再依赖 dpi！

    void init(QPagedPaintDevice& dev, QPainter& p)
    {
        painter = &p;
        device = &dev;

        pageRect = QRect(0, 0, dev.width(), dev.height());

        // PDF/Printer 通用字体（最稳定）
        QString fontFamily = "SimSun";  // 宋体（打印最安全）
        titleFont  = QFont(fontFamily, 16);
        smallFont    = QFont(fontFamily, 6);
        textFont    = QFont(fontFamily, 8);
        headerFont = QFont(fontFamily, 8);
        bodyFont   = QFont(fontFamily, 6);
    }

    int s(int v) const { return v; }   // 标准化，保持兼容性
};




class PageAnalysis : public QWidget
{
    Q_OBJECT

public:
    explicit PageAnalysis(QWidget *parent = nullptr);
    ~PageAnalysis();

    void init();

private slots:
    void on_exToPdfBtn_clicked();
    void hisReportBtnClicked();
    void reportBtnClicked();
    void refreshAll();
    void updateChart();
    void printReport();
    void onUserChanged(const QString& name, const QString& level);
    void selectAllCheckBox();

private:
    Ui::PageAnalysis *ui;

    // 图表相关成员
    QChart *chart;
    QChartView *chartView;
    QLineSeries *series1;
    QLineSeries *series2;
    QLineSeries *series3;
    QLineSeries *series4;

    void setupTable();
    void fillTextBrowser();
    void initPrinterList();
    void refreshNumberCombos();
    void updateUIByPermission();


    QPair<QString, QString> getBatchTimeRange(const QString& batchNum);
    bool createReportTable();
    QString getUnitFromUnitTable(const QString& batchNum);
    bool saveToReportTable(const QString& batchNum, const QMap<QString, QMap<QString, QVariant>>& sensorData);
    void loadOperationData(const QString& batchNum);

    // 新增图表相关函数
    void setupChart();
    QVector<QPointF> getSensorData(const QString& sensorName, const QString& batchNum);
    QVector<QPointF> adaptiveSampling(const QVector<QPointF>& data, int maxPoints);

    void buildEmptyChart();
    void drawBatchChart(const QVector<QPointF>& s1,
                                    const QVector<QPointF>& s2,
                                    const QVector<QPointF>& s3,
                                    const QVector<QPointF>& s4,
                                    bool hasData,
                                    const QString &batch);

    QChart* buildPrintChart(QLineSeries* sourceSeries,const QString& title,const PrintContext& ctx);
    int printCharts(PrintContext& ctx,int xMargin,int startY);
    void printReportContent(PrintContext& ctx);
    int drawTable(PrintContext& ctx,QTableWidget *table,int x,int startY);
    int sumWidths(const QVector<int> &w, int idx);
    void retranslateUi();

    bool checkUnitConsistency(const QString &batchNum, const QString &currentUnit);
    void generateReport(const QString &batchNum);
    bool reportExists(const QString &batchNum);


};

#endif // PAGEANALYSIS_H
