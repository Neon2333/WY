#include "DiffusionFlowTestPage.h"
#include "MessageDialog.h"
#include "VirtualKeyboard.h"
#include "DatabaseManager.h"
#include "RoundedProgressBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include <QRandomGenerator>
#include <QApplication>
#include <cmath>

DiffusionFlowTestPage::DiffusionFlowTestPage(const QString &operatorName, QWidget *parent)
    : QWidget(parent)
    , m_operator(operatorName)
    , m_keyboard(new VirtualKeyboard(this))
    , m_testThread(nullptr)
    , m_isTestRunning(false)
    , m_isPaused(false)
    , m_shouldStop(false)
{
    setupUi();
    m_keyboard->hide();
}

void DiffusionFlowTestPage::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u6269\u6563\u6D41\u6D4B\u8BD5"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1976D2; margin-bottom: 20px;");

    QWidget *controlPanel = new QWidget;
    controlPanel->setStyleSheet("background-color: white; border-radius: 10px; padding: 20px;");

    QHBoxLayout *controlLayout = new QHBoxLayout;

    m_startBtn = new QPushButton(QString::fromUtf8("\u5F00\u59CB\u6D4B\u8BD5"));
    m_startBtn->setFixedSize(150, 75);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setFocusPolicy(Qt::NoFocus);
    m_startBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: bold; outline: none; } QPushButton:hover { background-color: #388E3C; } QPushButton:disabled { background-color: #BDBDBD; }");

    m_pauseBtn = new QPushButton(QString::fromUtf8("\u6682\u505C"));
    m_pauseBtn->setFixedSize(150, 75);
    m_pauseBtn->setCursor(Qt::PointingHandCursor);
    m_pauseBtn->setEnabled(false);
    m_pauseBtn->setFocusPolicy(Qt::NoFocus);
    m_pauseBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: bold; outline: none; } QPushButton:disabled { background-color: #BDBDBD; }");

    m_stopBtn = new QPushButton(QString::fromUtf8("\u505C\u6B62"));
    m_stopBtn->setFixedSize(150, 75);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setFocusPolicy(Qt::NoFocus);
    m_stopBtn->setStyleSheet("QPushButton { background-color: #F44336; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: bold; outline: none; } QPushButton:disabled { background-color: #BDBDBD; }");

    controlLayout->addWidget(m_startBtn);
    controlLayout->addWidget(m_pauseBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addStretch();

    controlPanel->setLayout(controlLayout);

    m_progressBar = new RoundedProgressBar;

    m_statusLabel = new QLabel(QString::fromUtf8("\u5C1A\u672A\u5F00\u59CB\u6D4B\u8BD5"));
    m_statusLabel->setFixedSize(200, 45);
    m_statusLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_statusLabel->setStyleSheet("QLabel { font-size: 14px; color: #212121; padding: 0px; background-color: #FFF8E1; border-radius: 22px; border: none; margin: 0px; min-height: 45px; max-height: 45px; }");
    m_statusLabel->setWordWrap(false);

    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(15);
    progressLayout->addWidget(m_progressBar, 1, Qt::AlignVCenter);
    progressLayout->addWidget(m_statusLabel, 0, Qt::AlignVCenter);

    QWidget *dataPanel = new QWidget;
    dataPanel->setStyleSheet("background-color: white; border-radius: 10px; padding: 20px;");

    QGridLayout *dataLayout = new QGridLayout;
    dataLayout->setVerticalSpacing(15);
    dataLayout->setHorizontalSpacing(30);

    m_p1Label = new QLabel("P1 = -- mbar");
    m_p1Label->setStyleSheet("font-size: 16px; color: #212121; font-weight: bold;");

    m_p2Label = new QLabel("P2 = -- mbar");
    m_p2Label->setStyleSheet("font-size: 16px; color: #212121; font-weight: bold;");

    m_vsLabel = new QLabel(QString::fromUtf8("Vs = -- ml"));
    m_vsLabel->setStyleSheet("font-size: 16px; color: #212121; font-weight: bold;");

    m_qLabel = new QLabel(QString::fromUtf8("Q = -- ml/min"));
    m_qLabel->setStyleSheet("font-size: 16px; color: #212121; font-weight: bold;");

    dataLayout->addWidget(m_p1Label, 0, 0);
    dataLayout->addWidget(m_p2Label, 0, 1);
    dataLayout->addWidget(m_vsLabel, 1, 0);
    dataLayout->addWidget(m_qLabel, 1, 1);

    dataPanel->setLayout(dataLayout);

    m_resultLabel = new QLabel(QString::fromUtf8("\u6D4B\u8BD5\u7ED3\u679C = --"));
    m_resultLabel->setStyleSheet("font-size: 18px; color: #212121; font-weight: bold; padding: 15px; background-color: #F5F5F5; border-radius: 8px;");
    m_resultLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(controlPanel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(progressLayout);
    mainLayout->addWidget(dataPanel);
    mainLayout->addWidget(m_resultLabel);
    mainLayout->addStretch();

    setLayout(mainLayout);

    connect(m_startBtn, &QPushButton::clicked, this, &DiffusionFlowTestPage::onStartClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &DiffusionFlowTestPage::onPauseClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &DiffusionFlowTestPage::onStopClicked);
}

void DiffusionFlowTestPage::onStartClicked()
{
    FilterParams params = DatabaseManager::instance().getLatestFilterParams();
    if (params.id <= 0) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u63D0\u793A"), QString::fromUtf8("\u8BF7\u5148\u8F93\u5165\u6DA4\u5FC3\u89C4\u683C\u53C2\u6570"), MessageDialog::Warning);
        return;
    }

    if (m_isTestRunning) return;

    m_isTestRunning = true;
    m_isPaused = false;
    m_shouldStop = false;
    m_startBtn->setEnabled(false);
    m_pauseBtn->setEnabled(true);
    m_stopBtn->setEnabled(true);
    m_pauseBtn->setText(QString::fromUtf8("\u6682\u505C"));

    m_testThread = new QThread();
    
    TestWorker *worker = new TestWorker(this, params);
    worker->moveToThread(m_testThread);

    connect(m_testThread, &QThread::started, worker, &TestWorker::run);
    connect(m_testThread, &QThread::finished, m_testThread, &QThread::deleteLater);
    connect(m_testThread, &QThread::finished, worker, &TestWorker::deleteLater);
    
    connect(worker, &TestWorker::progressUpdated, this, &DiffusionFlowTestPage::onProgressUpdated);
    connect(worker, &TestWorker::dataUpdated, this, &DiffusionFlowTestPage::onDataUpdated);
    connect(worker, &TestWorker::resultReady, this, &DiffusionFlowTestPage::onResultReady);
    connect(worker, &TestWorker::finished, this, &DiffusionFlowTestPage::onTestFinished);

    m_testThread->start();
}

void DiffusionFlowTestPage::onPauseClicked()
{
    if (!m_isTestRunning) return;

    m_isPaused = !m_isPaused;
    QString btnText = m_isPaused ? QString::fromUtf8("\u7EE7\u7EED") : QString::fromUtf8("\u6682\u505C");
    m_pauseBtn->setText(btnText);
}

void DiffusionFlowTestPage::onStopClicked()
{
    m_shouldStop = true;
    m_isPaused = false;
    m_pauseBtn->setText(QString::fromUtf8("\u6682\u505C"));
    stopTest();
    finishTest();
}

void DiffusionFlowTestPage::onProgressUpdated(int value, const QString &stage)
{
    m_progressBar->setValue(value);
    m_statusLabel->setText(stage);
}

void DiffusionFlowTestPage::onDataUpdated(double p1, double p2, double vs, double q)
{
    if (p1 > 0) m_p1Label->setText("P1 = " + QString::number(p1, 'f', 1) + " mbar");
    if (p2 > 0) m_p2Label->setText("P2 = " + QString::number(p2, 'f', 1) + " mbar");
    if (vs > 0) m_vsLabel->setText(QString::fromUtf8("Vs = ") + QString::number(vs, 'f', 1) + " ml");
    if (q > 0) m_qLabel->setText(QString::fromUtf8("Q = ") + QString::number(q, 'f', 4) + " ml/min");
}

void DiffusionFlowTestPage::onResultReady(bool qualified, const QString &resultText, const QString &resultStyle)
{
    m_resultLabel->setText(QString::fromUtf8("\u6D4B\u8BD5\u7ED3\u679C = ") + resultText);
    m_resultLabel->setStyleSheet(QString::fromUtf8("font-size: 18px; font-weight: bold; padding: 15px; background-color: #F5F5F5; border-radius: 8px; %1").arg(resultStyle));
    m_resultLabel->setAlignment(Qt::AlignCenter);

    m_pauseBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
    m_pauseBtn->setText(QString::fromUtf8("\u6682\u505C"));
}

void DiffusionFlowTestPage::onTestFinished()
{
    m_isTestRunning = false;
    m_startBtn->setEnabled(true);
    m_pauseBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
}

void DiffusionFlowTestPage::finishTest()
{
    m_isTestRunning = false;
    m_isPaused = false;
    m_shouldStop = false;
    m_startBtn->setEnabled(true);
    m_pauseBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
    m_pauseBtn->setText(QString::fromUtf8("\u6682\u505C"));
    m_statusLabel->setText(QString::fromUtf8("\u5C1A\u672A\u5F00\u59CB\u6D4B\u8BD5"));
    m_progressBar->setValue(0);
}

void DiffusionFlowTestPage::pauseTest()
{
}

void DiffusionFlowTestPage::resumeTest()
{
}

void DiffusionFlowTestPage::stopTest()
{
    if (m_testThread && m_testThread->isRunning()) {
        m_testThread->requestInterruption();
        m_testThread->quit();
        m_testThread->wait(1000);
    }
}

void DiffusionFlowTestPage::waitForResume()
{
    while (m_isPaused && !m_shouldStop) {
        QThread::msleep(100);
    }
}

TestWorker::TestWorker(DiffusionFlowTestPage *page, const FilterParams &params)
    : QObject(nullptr)
    , m_page(page)
    , m_params(params)
{
}

void TestWorker::run()
{
    double Pc = m_params.testPressure;
    double Vs = 0;
    double COA = m_params.coaThreshold;
    int testDuration = m_params.testDuration;

    QRandomGenerator *rng = QRandomGenerator::global();

    emit progressUpdated(0, QString::fromUtf8("\u6C14\u6E90\u68C0\u6D4B"));
    for (int i = 0; i < 15; ++i) {
        QThread::msleep(100);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
    }

    emit progressUpdated(10, QString::fromUtf8("\u6C14\u8DEF\u5BC6\u95ED\u68C0\u6D4B"));
    for (int i = 0; i < 15; ++i) {
        QThread::msleep(100);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
    }

    emit progressUpdated(20, QString::fromUtf8("\u8BA1\u7B97\u4F53\u79EFVs"));
    double Vg = m_params.tankVolume;
    double P10 = 0, P20 = 0, P11 = 0, P21 = 0;

    for (int i = 0; i < 30; ++i) {
        QThread::msleep(100);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
    }

    P10 = Pc - 200 + rng->bounded(10);
    P20 = 5 + rng->bounded(5);
    P11 = P10 + 50 + rng->bounded(10);
    P21 = P20 + 30 + rng->bounded(10);

    Vs = Vg * (P11 - P10) / (P21 - P20);
    emit dataUpdated(0, 0, Vs, 0);

    emit progressUpdated(30, QString::fromUtf8("\u4FDD\u538B\u6D4B\u8BD5"));
    double P12 = Pc + 2000 + rng->bounded(50);
    double P23 = 0;

    for (int i = 0; i < 20; ++i) {
        QThread::msleep(100);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
    }

    P23 = P12 - 100 - rng->bounded(30);
    emit dataUpdated(P12, P23, Vs, 0);

    emit progressUpdated(40, QString::fromUtf8("\u8FDB\u884C\u4FDD\u538B\u6D4B\u8BD5"));

    double C = 0.5;
    int waitTime = static_cast<int>(-Vs / C * log((P12 - Pc) / (P12 - P23)));

    for (int i = 0; i < waitTime / 100; ++i) {
        QThread::msleep(100);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
    }

    emit progressUpdated(60, QString::fromUtf8("\u8BA1\u7B97\u6269\u6563\u6D41Q"));

    double P24 = P23 + 50 + rng->bounded(20);
    double Q = ((P24 - P23) * Vs) / (testDuration * 1013.25);

    emit dataUpdated(P12, P24, Vs, Q);

    int totalSteps = testDuration / 1000;
    for (int i = 0; i < totalSteps; ++i) {
        QThread::msleep(1000);
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }
        while (m_page->m_isPaused) {
            QThread::msleep(100);
            if (m_page->m_shouldStop) break;
        }
        if (m_page->m_shouldStop) {
            emit finished();
            return;
        }

        double decayFactor = exp(-static_cast<double>(i + 1) / totalSteps);
        Q = Q * (0.7 + 0.3 * decayFactor) + rng->bounded(0.1);

        int progress = 60 + (i + 1) * 35 / totalSteps;
        emit progressUpdated(progress, QString::fromUtf8("\u6D4B\u8BD5\u4E2D: %1/%2").arg(i + 1).arg(totalSteps));
        emit dataUpdated(P12, P24, Vs, Q);
    }

    emit progressUpdated(95, QString::fromUtf8("\u68C0\u6D4B\u7ED3\u679C"));

    QString resultText;
    QString resultStyle;
    bool isQualified = Q < COA;

    if (isQualified) {
        resultText = QString::fromUtf8("\u5408\u683C (Q=%1 < COA=%2)").arg(QString::number(Q, 'f', 4)).arg(QString::number(COA, 'f', 4));
        resultStyle = "color: #4CAF50;";
    } else {
        resultText = QString::fromUtf8("\u4E0D\u5408\u683C (Q=%1 >= COA=%2)").arg(QString::number(Q, 'f', 4)).arg(QString::number(COA, 'f', 4));
        resultStyle = "color: #F44336;";
    }

    emit resultReady(isQualified, resultText, resultStyle);

    TestResult result;
    result.filterParamId = m_params.id;
    result.testMethod = QString::fromUtf8("\u6269\u6563\u6D41");
    result.testData = QString::fromUtf8("{\"Pc\":%1,\"Vs\":%2,\"Q\":%3,\"COA\":%4,\"P12\":%5,\"P24\":%6}").arg(Pc).arg(Vs).arg(Q).arg(COA).arg(P12).arg(P24);
    result.result = isQualified ? QString::fromUtf8("\u5408\u683C") : QString::fromUtf8("\u4E0D\u5408\u683C");
    result.operatorName = m_page->m_operator;

    DatabaseManager::instance().saveTestResult(result);

    emit progressUpdated(100, QString::fromUtf8("\u6D4B\u8BD5\u5B8C\u6210"));

    QThread::msleep(500);

    emit finished();
}
