// MainWindow.cpp
#include "MainWindow.h"
#include "LoginWidget.h"
#include "MainWidget.h"
#include "SessionManager.h"
#include <QDebug>
#include "AuditLogger.h"
#include <QFile>
// #include "frminput.h"
// #include "Keyboard\Keyboard.h"
#include "GlobalDefines.h"
#include <QMessageBox>
#include <QApplication>
#include "Keyboard\InputManager.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->showFullScreen();
    // resize(1280, 800);
    // 初始化输入法
    InputManager::Instance()->Init(this);

    // 显示登录界面
    showLogin();
}
void MainWindow::showLogin()
{
    // 销毁已有主界面
    if (m_mainWidget) {
        m_mainWidget->hide();
        m_mainWidget->deleteLater();
        m_mainWidget = nullptr;
    }

    // 创建登录界面（若未创建）
    if (!m_loginWidget) {
        m_loginWidget = new LoginWidget(this);
        // 连接 loginSuccess 信号到 onLoginSuccess 槽
        connect(m_loginWidget, &LoginWidget::loginSuccess,this, &MainWindow::onLoginSuccess);
    }

    // 将登录界面设为中央窗口并显示
    setCentralWidget(m_loginWidget);
    m_loginWidget->show();
}

void MainWindow::onLoginSuccess(const QString &username, const QString &level)
{
    // 1) 登录成功后，唯一一次写会话信息
    SessionManager::instance().setUser(username, level);
    qDebug() << "[MainWindow] 会话已设置：" << username << "/" << level;

    // 2) 销毁登录界面
    if (m_loginWidget) {
        m_loginWidget->hide();
        m_loginWidget->deleteLater();
        m_loginWidget = nullptr;
    }

    // 指定审计数据库 (与 PageTrail 使用同一个文件)
    AuditLogger::instance().setDatabasePath(trailDataBasePath);
    /* 日志----------------“登录主页”--------------  */
    AuditLogger::instance().log(tr("登录主页"));

    // 3) 创建并显示主界面
    m_mainWidget = new MainWidget(this);
    setCentralWidget(m_mainWidget);
    m_mainWidget->show();

    connect(m_mainWidget, &MainWidget::logoutRequested,this, &MainWindow::handleLogout);
}

void MainWindow::handleLogout()
{
    qDebug() << "开始处理退出...";

    QTimer::singleShot(100, this, [this]() {
        if (m_mainWidget) {
            qDebug() << "正在销毁主界面...";
            m_mainWidget->hide();
            m_mainWidget->deleteLater();
            m_mainWidget = nullptr;
        }
    });

    // 4. 显示登录界面
    qDebug() << "显示登录界面";
    showLogin();
}

// bool MainWindow::eventFilter(QObject *obj, QEvent *event)
// {
//     // 只关心 QPushButton 相关的触摸事件
//     if (QPushButton *button = qobject_cast<QPushButton*>(obj))
//     {
//         switch (event->type())
//         {
//         case QEvent::TouchBegin:
//         case QEvent::TouchUpdate:   // 按住滑动时保持按下视觉
//             button->setProperty("touchPressed", true);
//             button->style()->unpolish(button);
//             button->style()->polish(button);
//             button->update();
//             return true;   // 我们接管了这个事件

//         case QEvent::TouchEnd:
//         case QEvent::TouchCancel:
//             button->setProperty("touchPressed", false);
//             button->style()->unpolish(button);
//             button->style()->polish(button);
//             button->update();
//             return true;

//         default:
//             break;
//         }
//     }

//     // 其他所有事件（包括鼠标、键盘、其他控件）都交给下一个 filter 或原处理
//     return QMainWindow::eventFilter(obj, event);
// }


// // 让所有按钮自动接受触摸事件（不然有些按钮收不到 TouchEvent）
// void MainWindow::enableTouchForAllButtons(QWidget *parent)
// {
//     if (!parent) parent = this;

//     for (QObject *child : parent->children())
//     {
//         if (QWidget *widget = qobject_cast<QWidget*>(child))
//         {
//             if (qobject_cast<QPushButton*>(widget))
//             {
//                 widget->setAttribute(Qt::WA_AcceptTouchEvents, true);
//             }
//             enableTouchForAllButtons(widget);  // 递归
//         }
//     }
// }
