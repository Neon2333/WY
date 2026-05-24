#include "MainWindow.h"
#include "ParamInputPage.h"
#include "TestSelectionPage.h"
#include "HistoryPage.h"
#include "MessageDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QApplication>

MainWindow::MainWindow(const QString &operatorName, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
    , m_operator(operatorName)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
}

void MainWindow::setupUi()
{
    this->setWindowTitle(QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5\u7CFB\u7EDF"));
    this->setMinimumSize(1024, 768);
    this->setStyleSheet("background-color: #F5F5F5;");

    QWidget *titleBar = new QWidget;
    titleBar->setFixedHeight(50);
    titleBar->setStyleSheet("background-color: #1565C0; border-top-left-radius: 15px; border-top-right-radius: 15px;");

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5\u7CFB\u7EDF"));
    titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    m_operatorLabel = new QLabel(QString::fromUtf8("\u64CD\u4F5C\u4EBA: %1").arg(m_operator));
    m_operatorLabel->setStyleSheet("color: white; font-size: 14px; margin-right: 20px;");

    QPushButton *closeBtn = new QPushButton(QString::fromUtf8("\u5173\u95ED\u8F6F\u4EF6"));
    closeBtn->setFixedSize(100, 35);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { background-color: #D32F2F; color: white; border: none; border-radius: 5px; font-size: 13px; font-weight: bold; } QPushButton:hover { background-color: #B71C1C; }");

    titleLayout->addWidget(m_operatorLabel);
    titleLayout->addWidget(closeBtn);

    titleBar->setLayout(titleLayout);

    QWidget *navWidget = new QWidget;
    navWidget->setFixedWidth(200);
    navWidget->setStyleSheet("background-color: #1976D2;");

    QVBoxLayout *navLayout = new QVBoxLayout;
    navLayout->setContentsMargins(0, 20, 0, 20);
    navLayout->setSpacing(10);

    QLabel *menuLabel = new QLabel(QString::fromUtf8("\u83DC\u5355"));
    menuLabel->setStyleSheet("color: #BBDEFB; font-size: 12px; padding: 5px 20px;");
    menuLabel->setAlignment(Qt::AlignLeft);

    QPushButton *paramBtn = new QPushButton(QString::fromUtf8("\u6DA1\u5FC3\u89C4\u683C\u53C2\u6570\u8F93\u5165"));
    paramBtn->setCursor(Qt::PointingHandCursor);
    paramBtn->setStyleSheet("QPushButton { background-color: transparent; color: white; border: none; text-align: left; padding: 15px 20px; font-size: 14px; } QPushButton:hover { background-color: #1565C0; }");

    QPushButton *testBtn = new QPushButton(QString::fromUtf8("\u8FDB\u884C\u6D4B\u8BD5"));
    testBtn->setCursor(Qt::PointingHandCursor);
    testBtn->setStyleSheet("QPushButton { background-color: transparent; color: white; border: none; text-align: left; padding: 15px 20px; font-size: 14px; } QPushButton:hover { background-color: #1565C0; }");

    QPushButton *historyBtn = new QPushButton(QString::fromUtf8("\u6D4B\u8BD5\u5386\u53F2\u8BB0\u5F55"));
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setStyleSheet("QPushButton { background-color: transparent; color: white; border: none; text-align: left; padding: 15px 20px; font-size: 14px; } QPushButton:hover { background-color: #1565C0; }");

    QPushButton *exportBtn = new QPushButton(QString::fromUtf8("\u5BFC\u51FAExcel\u62A5\u8868"));
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet("QPushButton { background-color: transparent; color: white; border: none; text-align: left; padding: 15px 20px; font-size: 14px; } QPushButton:hover { background-color: #1565C0; }");

    QPushButton *settingsBtn = new QPushButton(QString::fromUtf8("\u7CFB\u7EDF\u8BBE\u7F6E"));
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setStyleSheet("QPushButton { background-color: transparent; color: white; border: none; text-align: left; padding: 15px 20px; font-size: 14px; } QPushButton:hover { background-color: #1565C0; }");

    navLayout->addWidget(menuLabel);
    navLayout->addWidget(paramBtn);
    navLayout->addWidget(testBtn);
    navLayout->addWidget(historyBtn);
    navLayout->addWidget(exportBtn);
    navLayout->addStretch();
    navLayout->addWidget(settingsBtn);

    navWidget->setLayout(navLayout);

    m_contentWidget = new QWidget;
    m_contentWidget->setStyleSheet("background-color: #F5F5F5; border-bottom-left-radius: 15px;");

    m_contentLayout = new QStackedLayout;
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    ParamInputPage *paramPage = new ParamInputPage(m_operator);
    TestSelectionPage *testPage = new TestSelectionPage(m_operator);
    HistoryPage *historyPage = new HistoryPage();

    m_contentLayout->addWidget(paramPage);
    m_contentLayout->addWidget(testPage);
    m_contentLayout->addWidget(historyPage);

    m_contentWidget->setLayout(m_contentLayout);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(navWidget);
    contentLayout->addWidget(m_contentWidget, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addLayout(contentLayout);

    setLayout(mainLayout);

    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        if (MessageDialog::question(this, QString::fromUtf8("\u63D0\u793A"), QString::fromUtf8("\u786E\u5B9A\u8981\u5173\u95ED\u8F6F\u4EF6\u5417\uFF1F"))) {
            QApplication::quit();
        }
    });

    connect(paramBtn, &QPushButton::clicked, this, [this]() {
        m_contentLayout->setCurrentIndex(0);
    });

    connect(testBtn, &QPushButton::clicked, this, [this]() {
        m_contentLayout->setCurrentIndex(1);
    });

    connect(historyBtn, &QPushButton::clicked, this, [this]() {
        m_contentLayout->setCurrentIndex(2);
    });

    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        emit exportRequested();
    });

    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    m_contentLayout->setCurrentIndex(0);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), 15, 15);
    painter.fillPath(path, QColor(245, 245, 245));
}

void MainWindow::onSettingsClicked()
{
    MessageDialog::showMessage(this, QString::fromUtf8("\u7CFB\u7EDF\u8BBE\u7F6E"), QString::fromUtf8("\u7CFB\u7EDF\u8BBE\u7F6E\u529F\u80FD\u5F85\u5B9E\u73B0"), MessageDialog::Information);
}