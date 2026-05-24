#ifndef TESTSELECTIONPAGE_H
#define TESTSELECTIONPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QStackedLayout>
#include <QString>

class DiffusionFlowTestPage;

class TestSelectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit TestSelectionPage(const QString &operatorName, QWidget *parent = nullptr);

private slots:
    void onDiffusionFlowClicked();
    void onBubblePointClicked();
    void onPressureHoldClicked();

private:
    void setupUi();

    QString m_operator;
    QWidget *m_testContainer = nullptr;
    QStackedLayout *m_testStackLayout = nullptr;
    QWidget *m_currentTestPage = nullptr;
};

#endif