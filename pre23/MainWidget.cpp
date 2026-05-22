//MainWidget.cpp
#include "MainWidget.h"
#include "ui_MainWidget.h"
#include "SessionManager.h"
#include "PageMain.h"
#include "PageChart.h"
#include "PageHistory.h"
#include "PageTrail.h"
#include "PageSystem.h"
#include "PageAnalysis.h"
#include "PageUser.h"
#include "DatabaseManager.h"
#include "IdleWatcher.h"        //超时监控
#include "AuditLogger.h"        //日志
#include "OverlayLockWidget.h"  //弹窗
#include "GlobalDefines.h"
#include "LanguageManager.h"
#include "OverlayMessage.h"
#include "SessionManager.h"
#include "PermissionManager.h"


#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
    , timer(new QTimer(this))
    , m_firstShow(true)
    , pageMain(nullptr)
    , pageChart(nullptr)
    , pageHistory(nullptr)
    , pageTrail(nullptr)
    , pageSystem(nullptr)
    , pageAnalysis(nullptr)
    , pageUser(nullptr)
{
    ui->setupUi(this);

    // 1) 实时更新时间显示
    updateTime();
    connect(timer, &QTimer::timeout, this, &MainWidget::updateTime);
    timer->start(1000);

    // 2) 显示当前用户信息
    ui->mainUserNameLabel->setText(SessionManager::instance().userName());
    ui->mainUserLevelLabel->setText(SessionManager::instance().userLevel());

    // 监听会话切换信号，自动更新界面
    connect(&SessionManager::instance(),&SessionManager::userChanged,this,
            [this](const QString &u, const QString &lvl) {
                ui->mainUserNameLabel->setText(u);
                ui->mainUserLevelLabel->setText(lvl);
            });

    // 3) 实例化所有页面（但不打开 DB）
    pageMain    = new PageMain(this);
    pageChart   = new PageChart(this);
    pageHistory = new PageHistory(this);
    pageTrail   = new PageTrail(this);
    pageSystem = new PageSystem(this);
    pageAnalysis = new PageAnalysis(this);
    pageUser = new PageUser(this);

    // 关键连接：转发 PageSystem 的信号
    ui->stackedWidget->insertWidget(0, pageMain);
    ui->stackedWidget->insertWidget(1, pageChart);
    ui->stackedWidget->insertWidget(2, pageHistory);
    ui->stackedWidget->insertWidget(3, pageTrail);
    ui->stackedWidget->insertWidget(4, pageAnalysis);
    ui->stackedWidget->insertWidget(5, pageUser);
    ui->stackedWidget->insertWidget(6, pageSystem);

    // 4) 导航按钮绑定
    connect(ui->mainBtn,  &QPushButton::clicked, this, &MainWidget::switchToPageMain);
    connect(ui->chartBtn, &QPushButton::clicked, this, &MainWidget::switchToPageChart);
    connect(ui->hisBtn,   &QPushButton::clicked, this, &MainWidget::switchToPageHis);
    connect(ui->trailBtn, &QPushButton::clicked, this, &MainWidget::switchToPageTrail);
    connect(ui->anaBtn, &QPushButton::clicked, this, &MainWidget::switchToPageAna);
    connect(ui->sysBtn,   &QPushButton::clicked, this, &MainWidget::switchToPageSys);
    connect(ui->userBtn,   &QPushButton::clicked, this, &MainWidget::switchToPageUser);

    // 空闲检测
    int lockTimeMs =SessionManager::instance().lockTime() * 1000 * 60;
    IdleWatcher *idleWatcher = new IdleWatcher(lockTimeMs, this);
    idleWatcher->startWatching();

    connect(idleWatcher, &IdleWatcher::idleTimeout, this, [=]() {
        AuditLogger::instance().log(tr("系统空闲，已锁定，请重新登录"));

        auto lockPanel = new OverlayLockWidget(this);
        connect(lockPanel, &OverlayLockWidget::loginSuccess, this, [=](const QString &name, const QString &level) {
            ui->mainUserNameLabel->setText(name);
            ui->mainUserLevelLabel->setText(level);
            idleWatcher->startWatching();       // 成功后重新开始闲置监测
            lockPanel->deleteLater();           // 自动销毁
        });
        // 超时弹窗出现后，**停止**监测，避免重复触发
        lockPanel->showPanel();
    });

    connect(idleWatcher, &IdleWatcher::remainingTimeChanged,
            this, [=](int sec) {
                int minutes = (sec + 59) / 60; // 向上取整，避免 0 分钟
                ui->leftTimeLabel->setText(QString::number(minutes));
            });

    connect(&SessionManager::instance(),&SessionManager::lockTimeChanged,this,[=](int minutes) {
                idleWatcher->setTimeout(minutes * 60 * 1000);
                idleWatcher->startWatching();   // 立刻重置计时
            });

    connect(ui->lockTimeBtn, &QPushButton::clicked,this, &MainWidget::applyLockTime);

    // 关机信号槽连接
    connect(ui->toolBtnShutDown, &QToolButton::clicked, this, &MainWidget::closeSystem);

    connect(pageMain, &PageMain::alarmMuteChanged, this, &MainWidget::onAlarmMuteChanged);

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &MainWidget::onUserChanged);
    // 根据权限统一设置UI状态
    updateUIByPermission();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &MainWidget::retranslateUi);
}

void MainWidget::retranslateUi()
{
    if(ui) ui->retranslateUi(this);
}

MainWidget::~MainWidget()
{
    // 关闭数据库
    DatabaseManager::instance().closeDatabase(DatabaseManager::HistoryDatabase);
    DatabaseManager::instance().closeDatabase(DatabaseManager::TrailDatabase);

    delete ui;
}

void MainWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_firstShow) {
        switchToPageMain();
        m_firstShow = false;
        //设置锁定时间
        int lockTime = SessionManager::instance().lockTime();
        ui->lockTimeSpinBox->setValue(lockTime);
    }
}

void MainWidget::updateTime()
{
    ui->timeLabel->setText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        );
}

void MainWidget::switchToPageMain()
{
    pageMain->init();
    ui->stackedWidget->setCurrentWidget(pageMain);
}

void MainWidget::switchToPageChart()
{
    pageChart->init();
    ui->stackedWidget->setCurrentWidget(pageChart);
}

void MainWidget::switchToPageHis()
{
    pageHistory->init();
    ui->stackedWidget->setCurrentWidget(pageHistory);
}

void MainWidget::switchToPageTrail()
{
    pageTrail->init();
    ui->stackedWidget->setCurrentWidget(pageTrail);
}

void MainWidget::switchToPageAna()
{
    pageAnalysis->init();
    ui->stackedWidget->setCurrentWidget(pageAnalysis);
}

void MainWidget::switchToPageSys()
{
    ui->stackedWidget->setCurrentWidget(pageSystem);
}

void MainWidget::switchToPageUser()
{
    ui->stackedWidget->setCurrentWidget(pageUser);
    pageUser->init();
}

// 关闭系统
void MainWidget::closeSystem()
{
    // 避免重复创建消息框
    if (m_overlayMsg) {
        qDebug() << "消息框已存在，不再创建";
        return;
    }

    // 创建消息框并设置父对象为顶层窗口
    QWidget* topLevel = this->window();
    m_overlayMsg = new OverlayMessage(topLevel);

    // 设置消息框属性
    m_overlayMsg->setShowCancelButton(true);

    // 连接消息框信号
    connect(m_overlayMsg, &OverlayMessage::closed, this, [this]() {
        qDebug() << "MainWidget: 用户确认关闭系统";
        AuditLogger::instance().log(tr("用户确认关闭系统"));

        m_overlayMsg = nullptr;     // 先置空指针
        emit logoutRequested();     // 触发退出信号与MainWindow::handleLogout链接
    });

    connect(m_overlayMsg, &OverlayMessage::cancelled, this, [this]() {
        qDebug() << "MainWidget: 用户取消关闭系统";
        AuditLogger::instance().log(tr("用户取消关闭系统"));

        // 安全清理
        if (m_overlayMsg) {
            m_overlayMsg->deleteLater();
            m_overlayMsg = nullptr;
        }
    });

    // 显示消息框
    m_overlayMsg->showMessage(tr("系统退出"),
                              tr("确定要退出系统并返回登录界面吗？\n退出后请关闭电源"));
}

// 改变用户重新应用权限
void MainWidget::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态
void MainWidget::updateUIByPermission()
{
    // SuperAdmin权限控制的UI元素（需要SuperAdmin及以上权限）
    QList<QWidget*> SuperAdminWidgets = {
        ui->lockTimeSpinBox,ui->labelLockTime,ui->lockTimeBtn
    };

    // 设置Admin权限控件
    for (auto widget : SuperAdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::SuperAdmin, widget);
    }
}

void MainWidget::onAlarmMuteChanged(bool muted)
{
    // 更新图标静音
    ui->alarmIconFrame->setProperty("muted", muted);
    ui->alarmIconFrame->setStyleSheet(ui->alarmIconFrame->styleSheet());
}

//修改锁定时间
void MainWidget::applyLockTime()
{
    // 关键：强制结束编辑
    ui->lockTimeSpinBox->clearFocus();

    int value = ui->lockTimeSpinBox->value();

    SessionManager::instance().setLockTime(value);

    int ms = value * 60 * 1000;
    auto idleWatcher = findChild<IdleWatcher *>();
    if (idleWatcher) {
        idleWatcher->setTimeout(ms);
        idleWatcher->resetRemainingTime();
        idleWatcher->startWatching();
    }

    AuditLogger::instance().log(
        tr("用户：%1 修改锁定时间为 %2 分钟")
            .arg(SessionManager::instance().userName())
            .arg(value)
        );
}
