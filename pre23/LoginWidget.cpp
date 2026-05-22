// LoginWidget.cpp
#include "LoginWidget.h"
#include "ui_LoginWidget.h"
#include "DatabaseManager.h"

#include "OverlayMessage.h"
#include "OverlayChangePasswordWidget.h"
#include "AuditLogger.h"
#include "GlobalDefines.h"
#include "LanguageManager.h"

#include <QDebug>
#include <QDateTime>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    // 将登录按钮点击和 attemptLogin() 绑定
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWidget::attemptLogin);
    connect(ui->exitTheProgram, &QPushButton::clicked,this, &LoginWidget::exitTheProgram);


    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &LoginWidget::retranslateUi);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::retranslateUi()
{
    if(ui) ui->retranslateUi(this);
}

void LoginWidget::attemptLogin()
{
    // 1. 读取并 trim 用户输入
    const QString username = ui->usernameInput->text().trimmed();
    const QString password = ui->passwordInput->text().trimmed();
    // 2. 输入非空校验
    if (username.isEmpty() || password.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("用户名或密码不能为空"));
        return;
    }
    // 3. 先检查用户是否存在，不存在则显示通用错误消息
    if (!DatabaseManager::instance().userExists(username)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("该用户不存在"));
        return;
    }

    // 4. 检查账户是否锁定
    QString lockStatus;
    if (!DatabaseManager::instance().getLockStatus(username, lockStatus)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("非法用户，请切换用户登录"));
        return;
    }
    if (lockStatus == "Y") {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("该账户已锁定"));
        return;
    }
    // 5. 调用 DatabaseManager 验证用户名/密码
    if (!DatabaseManager::instance().validateUser(username, password)) {
        // 验证失败，增加失败计数
        int failedAttempts = 0;
        if (!DatabaseManager::instance().incrementFailedAttempts(username, failedAttempts)) {
            qDebug() << "无法更新失败尝试次数";
        }
        QString errorMessage = tr("用户名或密码错误");
        if (failedAttempts <= 5) {
            int maxAttempts = 5;
            int remainingAttempts = qMax(0, maxAttempts - failedAttempts);// 假设允许5次失败，第6次失败时锁定，但根据>5调整
            errorMessage += QString(tr("，剩余 %1 次尝试机会")).arg(remainingAttempts);
        }
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), errorMessage);
        // 检查是否超过5次
        if (failedAttempts > 5) {
            if (!DatabaseManager::instance().lockAccount(username)) {
                qDebug() << "无法锁定账户";
            }
            auto *lockMsg = new OverlayMessage(this);
            lockMsg->showMessage(tr("账户锁定"), tr("登录失败超过5次，该账户已被锁定"));
            AuditLogger::instance().log(QString(tr("用户：%1 登录失败超过5次，已被锁定")).arg(username));
        }
        return;
    }
    // 6. 验证通过，重置失败尝试次数
    if (!DatabaseManager::instance().resetFailedAttempts(username)) {
        qDebug() << "无法重置失败尝试次数";
    }

    // 7. 检查账户是否初次登录（原始密码验证通过后）
    QString loginStatus;
    if (!DatabaseManager::instance().firstLogin(username, loginStatus)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("非法用户，请切换用户登录"));
        return;
    }

    // 初次登录：强制设置密码
    if (loginStatus == "Y") {

        auto *dlg = new OverlayChangePasswordWidget(username,OverlayChangePasswordWidget::Reason::FirstLogin,this);

        connect(dlg, &OverlayChangePasswordWidget::passwordChanged,
                this, [=]() {
                    QString realUsername, userLevel;
                    DatabaseManager::instance().getUserInfo(username, realUsername, userLevel);
                    emit loginSuccess(realUsername, userLevel);
                });

        dlg->show();
        return;
    }

    // 8.检查密码是否过期
    QDateTime createTime;
    if (DatabaseManager::instance().getUserCreateTime(username, createTime)) {

        const int days =createTime.daysTo(QDateTime::currentDateTime());

        if (days > 90) {

            auto *dlg = new OverlayChangePasswordWidget(username,OverlayChangePasswordWidget::Reason::PasswordExpired,this);

            connect(dlg, &OverlayChangePasswordWidget::passwordChanged,
                    this, [=]() {
                        QString realUsername, userLevel;
                        DatabaseManager::instance().getUserInfo(username, realUsername, userLevel);
                        emit loginSuccess(realUsername, userLevel);
                    });

            dlg->show();
            return;
        }
    }


    // 9. 读取权限等级
    QString realUsername, userLevel;
    if (!DatabaseManager::instance().getUserInfo(username, realUsername, userLevel)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("获取用户信息失败"));
        return;
    }

    // 10. 发出登录成功信号，携带用户名和权限等级
    emit loginSuccess(realUsername, userLevel);
}

void LoginWidget::exitTheProgram()
{
    auto *msg = new OverlayMessage(this);

    msg->setShowCancelButton(true);

    msg->showMessage("退出程序Exit program","确定要退出程序吗？\n Exit the program?");

    QObject::connect(msg, &OverlayMessage::closed, msg, [=](){
        QCoreApplication::quit();
    });
}
