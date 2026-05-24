#include "TestSelectionPage.h"
#include "DiffusionFlowTestPage.h"
#include "MessageDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TestSelectionPage::TestSelectionPage(const QString &operatorName, QWidget *parent)
    : QWidget(parent)
    , m_operator(operatorName)
    , m_currentTestPage(nullptr)
{
    setupUi();
}

void TestSelectionPage::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u9009\u62E9\u6D4B\u8BD5\u65B9\u6CD5"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1976D2; margin-bottom: 20px;");

    QWidget *buttonPanel = new QWidget;
    buttonPanel->setStyleSheet("background-color: white; border-radius: 10px; padding: 30px;");

    QHBoxLayout *btnRow1 = new QHBoxLayout;
    btnRow1->setSpacing(30);

    QPushButton *diffusionBtn = new QPushButton(QString::fromUtf8("\u6269\u6563\u6D41\u6D4B\u8BD5"));
    diffusionBtn->setFixedSize(200, 80);
    diffusionBtn->setCursor(Qt::PointingHandCursor);
    diffusionBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 10px; font-size: 16px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; }");
    connect(diffusionBtn, &QPushButton::clicked, this, &TestSelectionPage::onDiffusionFlowClicked);

    QPushButton *bubbleBtn = new QPushButton(QString::fromUtf8("\u6CE1\u70B9\u6D4B\u8BD5"));
    bubbleBtn->setFixedSize(200, 80);
    bubbleBtn->setCursor(Qt::PointingHandCursor);
    bubbleBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; border: none; border-radius: 10px; font-size: 16px; font-weight: bold; } QPushButton:hover { background-color: #F57C00; }");
    connect(bubbleBtn, &QPushButton::clicked, this, &TestSelectionPage::onBubblePointClicked);

    QPushButton *pressureBtn = new QPushButton(QString::fromUtf8("\u4FDD\u538B\u6D4B\u8BD5"));
    pressureBtn->setFixedSize(200, 80);
    pressureBtn->setCursor(Qt::PointingHandCursor);
    pressureBtn->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; border: none; border-radius: 10px; font-size: 16px; font-weight: bold; } QPushButton:hover { background-color: #7B1FA2; }");
    connect(pressureBtn, &QPushButton::clicked, this, &TestSelectionPage::onPressureHoldClicked);

    btnRow1->addWidget(diffusionBtn);
    btnRow1->addWidget(bubbleBtn);
    btnRow1->addWidget(pressureBtn);
    btnRow1->addStretch();

    buttonPanel->setLayout(btnRow1);

    m_testContainer = new QWidget;
    m_testContainer->setStyleSheet("background-color: #F5F5F5; border-radius: 10px; margin-top: 20px;");

    QVBoxLayout *containerLayout = new QVBoxLayout(m_testContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    m_testStackLayout = new QStackedLayout;
    containerLayout->addLayout(m_testStackLayout);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(buttonPanel);
    mainLayout->addWidget(m_testContainer, 1);

    setLayout(mainLayout);
}

void TestSelectionPage::onDiffusionFlowClicked()
{
    if (!m_currentTestPage || QString(m_currentTestPage->metaObject()->className()) != "DiffusionFlowTestPage") {
        if (m_currentTestPage) {
            m_testStackLayout->removeWidget(m_currentTestPage);
            delete m_currentTestPage;
        }
        m_currentTestPage = new DiffusionFlowTestPage(m_operator);
        m_testStackLayout->addWidget(m_currentTestPage);
    }
    m_testStackLayout->setCurrentWidget(m_currentTestPage);
}

void TestSelectionPage::onBubblePointClicked()
{
    MessageDialog::showMessage(this, QString::fromUtf8("\u6CE1\u70B9\u6D4B\u8BD5"), QString::fromUtf8("\u6CE1\u70B9\u6D4B\u8BD5\u529F\u80FD\u5F85\u5B9E\u73B0"), MessageDialog::Information);
}

void TestSelectionPage::onPressureHoldClicked()
{
    MessageDialog::showMessage(this, QString::fromUtf8("\u4FDD\u538B\u6D4B\u8BD5"), QString::fromUtf8("\u4FDD\u538B\u6D4B\u8BD5\u529F\u80FD\u5F85\u5B9E\u73B0"), MessageDialog::Information);
}