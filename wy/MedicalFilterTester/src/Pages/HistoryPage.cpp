#include "HistoryPage.h"
#include "MessageDialog.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

struct Ui_HistoryPage {
    QTableWidget *tableWidget = nullptr;
};

HistoryPage::HistoryPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui_HistoryPage)
{
    setupUi();
    loadHistory();
}

HistoryPage::~HistoryPage()
{
    delete m_ui;
}

void HistoryPage::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u6D4B\u8BD5\u5386\u53F2\u8BB0\u5F55"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1976D2; margin-bottom: 20px;");

    m_ui->tableWidget = new QTableWidget();
    m_ui->tableWidget->setColumnCount(6);
    m_ui->tableWidget->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("\u6DA1\u5FC3\u578B\u53F7")
        << QString::fromUtf8("\u6D4B\u8BD5\u65B9\u6CD5")
        << QString::fromUtf8("\u6D4B\u8BD5\u7ED3\u679C")
        << QString::fromUtf8("\u64CD\u4F5C\u4EBA")
        << QString::fromUtf8("\u6D4B\u8BD5\u65F6\u95F4")
        << QString::fromUtf8("\u8BE6\u7EC6\u6570\u636E"));

    m_ui->tableWidget->setStyleSheet("QTableWidget { border: none; background-color: white; border-radius: 10px; } QTableWidget::item { padding: 10px; } QHeaderView::section { background-color: #E3F2FD; color: #1976D2; font-weight: bold; padding: 10px; border: none; }");
    m_ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QHBoxLayout *btnLayout = new QHBoxLayout;

    QPushButton *refreshBtn = new QPushButton(QString::fromUtf8("\u5237\u65B0"));
    refreshBtn->setFixedSize(100, 40);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 8px; font-size: 14px; } QPushButton:hover { background-color: #1976D2; }");

    QPushButton *deleteBtn = new QPushButton(QString::fromUtf8("\u5220\u9664"));
    deleteBtn->setFixedSize(100, 40);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet("QPushButton { background-color: #F44336; color: white; border: none; border-radius: 8px; font-size: 14px; } QPushButton:hover { background-color: #D32F2F; }");

    QPushButton *exportBtn = new QPushButton(QString::fromUtf8("\u5BFC\u51FA\u62A5\u8868"));
    exportBtn->setFixedSize(120, 40);
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 14px; } QPushButton:hover { background-color: #388E3C; }");

    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(deleteBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(exportBtn);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_ui->tableWidget, 1);
    mainLayout->addLayout(btnLayout);

    setLayout(mainLayout);

    connect(refreshBtn, &QPushButton::clicked, this, &HistoryPage::loadHistory);
    connect(deleteBtn, &QPushButton::clicked, this, &HistoryPage::onDeleteClicked);
    connect(exportBtn, &QPushButton::clicked, this, &HistoryPage::onExportClicked);
}

void HistoryPage::loadHistory()
{
    QList<TestResult> results = DatabaseManager::instance().getAllTestResults();
    QList<FilterParams> allParams = DatabaseManager::instance().getAllFilterParams();

    m_ui->tableWidget->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        const TestResult &result = results[i];

        QString filterModel;
        for (const FilterParams &p : allParams) {
            if (p.id == result.filterParamId) {
                filterModel = p.model;
                break;
            }
        }

        m_ui->tableWidget->setItem(i, 0, new QTableWidgetItem(filterModel));
        m_ui->tableWidget->setItem(i, 1, new QTableWidgetItem(result.testMethod));
        m_ui->tableWidget->setItem(i, 2, new QTableWidgetItem(result.result));
        m_ui->tableWidget->setItem(i, 3, new QTableWidgetItem(result.operatorName));
        m_ui->tableWidget->setItem(i, 4, new QTableWidgetItem(result.testAt));
        m_ui->tableWidget->setItem(i, 5, new QTableWidgetItem(result.testData));
    }

    m_ui->tableWidget->resizeColumnsToContents();
}

void HistoryPage::onDeleteClicked()
{
    QTableWidget *tableWidget = m_ui->tableWidget;
    int currentRow = tableWidget->currentRow();

    if (currentRow < 0) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u5220\u9664\u5931\u8D25"), QString::fromUtf8("\u8BF7\u5148\u9009\u62E9\u4E00\u6761\u8BB0\u5F55"), MessageDialog::Warning);
        return;
    }

    QList<TestResult> results = DatabaseManager::instance().getAllTestResults();
    if (currentRow < results.size()) {
        int id = results[currentRow].id;
        if (DatabaseManager::instance().deleteTestResult(id)) {
            MessageDialog::showMessage(this, QString::fromUtf8("\u5220\u9664\u6210\u529F"), QString::fromUtf8("\u8BB0\u5F55\u5DF2\u5220\u9664"), MessageDialog::Success);
            loadHistory();
        } else {
            MessageDialog::showMessage(this, QString::fromUtf8("\u5220\u9664\u5931\u8D25"), QString::fromUtf8("\u5220\u9664\u5931\u8D25\uFF0C\u8BF7\u91CD\u8BD5"), MessageDialog::Error);
        }
    }
}

void HistoryPage::onExportClicked()
{
    MessageDialog::showMessage(this, QString::fromUtf8("\u5BFC\u51FA\u62A5\u8868"), QString::fromUtf8("\u5BFC\u51FAExcel\u62A5\u8868\u529F\u80FD\u5F85\u5B9E\u73B0"), MessageDialog::Information);
}