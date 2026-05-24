#ifndef DIFFUSIONFLOWTESTPAGE_H
#define DIFFUSIONFLOWTESTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QThread>
#include <atomic>
#include "DatabaseManager.h"
#include "VirtualKeyboard.h"
#include "RoundedProgressBar.h"

class TestWorker : public QObject
{
    Q_OBJECT

public:
    TestWorker(class DiffusionFlowTestPage *page, const FilterParams &params);

public slots:
    void run();

signals:
    void progressUpdated(int value, const QString &stage);
    void dataUpdated(double p1, double p2, double vs, double q);
    void resultReady(bool qualified, const QString &resultText, const QString &resultStyle);
    void finished();

private:
    class DiffusionFlowTestPage *m_page;
    FilterParams m_params;
};

class DiffusionFlowTestPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiffusionFlowTestPage(const QString &operatorName, QWidget *parent = nullptr);

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onStopClicked();
    void onProgressUpdated(int value, const QString &stage);
    void onDataUpdated(double p1, double p2, double vs, double q);
    void onResultReady(bool qualified, const QString &resultText, const QString &resultStyle);
    void onTestFinished();

private:
    void setupUi();
    void runTest();
    void finishTest();
    void pauseTest();
    void resumeTest();
    void stopTest();
    void waitForResume();
    void updateProgress(int value, const QString &stage);
    void updateDataLabels(double p1, double p2, double vs, double q);
    void updateResult(bool qualified, const QString &resultText, const QString &resultStyle);

    QString m_operator;
    QLabel *m_p1Label = nullptr;
    QLabel *m_p2Label = nullptr;
    QLabel *m_vsLabel = nullptr;
    QLabel *m_qLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_resultLabel = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_pauseBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    RoundedProgressBar *m_progressBar = nullptr;
    VirtualKeyboard *m_keyboard = nullptr;
    QThread *m_testThread = nullptr;
    std::atomic<bool> m_isTestRunning{false};
    std::atomic<bool> m_isPaused{false};
    std::atomic<bool> m_shouldStop{false};

    friend class TestWorker;
};

#endif