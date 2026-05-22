#include "OverlayLockWidget.h"
#include "DatabaseManager.h"
#include "SessionManager.h"
#include "AuditLogger.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "OverlayMessage.h"
#include <QEvent>

OverlayLockWidget::OverlayLockWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setStyleSheet("background-color: rgba(0, 0, 0, 120);");
    setGeometry(parent->rect());

    setupUI();
}

void OverlayLockWidget::setupUI()
{
    QWidget *panel = new QWidget(this);
    panel->setFixedSize(360, 280);
    panel->setStyleSheet("background-color: white; border-radius: 12px;");

    QLabel *titleLabel = new QLabel(tr("身份验证"), panel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 10px;background-color: #FAF211;");

    usernameEdit = new QLineEdit(panel);
    usernameEdit->setPlaceholderText(tr("用户名"));
    usernameEdit->setStyleSheet("font-size: 16px; padding: 6px;");
    usernameEdit->setMinimumHeight(54);

    passwordEdit = new QLineEdit(panel);
    passwordEdit->setPlaceholderText(tr("密码"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("font-size: 16px; padding: 6px;");
    passwordEdit->setMinimumHeight(54);

    okButton = new QPushButton(tr("解锁"), panel);
    okButton->setStyleSheet(R"(
        QPushButton {
            font-size: 18px;
            min-height: 42px;
            background-color: #229590;
            color: white;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
    )");

    connect(okButton, &QPushButton::clicked, this, &OverlayLockWidget::handleLogin);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->addWidget(titleLabel);
    layout->addSpacing(10);
    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addSpacing(20);
    layout->addWidget(okButton);

    // 居中 panel
    panel->move((width() - panel->width()) / 2, (height() - panel->height()) / 2);
}

bool OverlayLockWidget::event(QEvent *e) {
    // 所有用户操作都会触发活动检测
    switch (e->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::KeyPress:
    case QEvent::TouchBegin:
        // 通知主窗口有用户活动（防止在锁屏界面黑屏）
        emit activity();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void OverlayLockWidget::showPanel()
{
    this->show();
}

void OverlayLockWidget::handleLogin()
{
    const QString username = usernameEdit->text().trimmed();
    const QString password = passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        // 简单反馈，你可以用 showMessageOverlay(this, ...) 替代
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("用户名或密码不能为空"));
        passwordEdit->clear();
        return;
    }

    DatabaseManager &db = DatabaseManager::instance();
    if (!db.validateUser(username, password)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("用户名或密码错误"));
        passwordEdit->clear();
        return;
    }

    QString realUsername, userLevel;
    if (!db.getUserInfo(username, realUsername, userLevel)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("登录失败"), tr("获取用户信息失败"));
        return;
    }

    SessionManager::instance().setUser(realUsername, userLevel);
    AuditLogger::instance().log(tr("解锁成功 - 用户: %1(%2)").arg(realUsername, userLevel));

    emit loginSuccess(realUsername, userLevel);
    close();
}
