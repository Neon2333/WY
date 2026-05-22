#include "OverlayChangePasswordWidget.h"
#include "DatabaseManager.h"
#include "OverlayMessage.h"
#include "AuditLogger.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>
#include <QEvent>
#include <QKeyEvent>

OverlayChangePasswordWidget::OverlayChangePasswordWidget(
    const QString &username,
    Reason reason,
    QWidget *parent)
    : QWidget(parent),
    m_username(username),
    m_reason(reason)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setStyleSheet("background-color: rgba(0, 0, 0, 120);");
    setGeometry(parent->rect());

    setupUI();
}

void OverlayChangePasswordWidget::setupUI()
{
    QWidget *panel = new QWidget(this);
    panel->setFixedSize(380, 300);
    panel->setStyleSheet("background-color: white; border-radius: 12px;");

    QString titleText;
    QString tipText;

    switch (m_reason) {
        case Reason::FirstLogin:
            titleText = tr("初次登录");
            tipText = tr("该账户为首次登录系统\n请设置登录密码后继续使用");
            break;

        case Reason::PasswordExpired:
            titleText = tr("密码已过期");
            tipText = tr("该账户已超过 90 天未修改密码\n请设置新密码后继续登录");
            break;
    }

    QLabel *titleLabel = new QLabel(titleText, panel);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold; padding: 10px;");

    QLabel *tipLabel = new QLabel(tipText, panel);
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setStyleSheet(
        "font-size: 14px; color: #555;");

    passwordEdit = new QLineEdit(panel);
    passwordEdit->setPlaceholderText(tr("新密码"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(36);
    passwordEdit->setStyleSheet(
        "font-size: 16px; padding: 6px;");

    confirmEdit = new QLineEdit(panel);
    confirmEdit->setPlaceholderText(tr("确认新密码"));
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setMinimumHeight(36);
    confirmEdit->setStyleSheet(
        "font-size: 16px; padding: 6px;");

    okButton = new QPushButton(tr("确定修改"), panel);
    okButton->setMinimumHeight(42);
    okButton->setStyleSheet(R"(
        QPushButton {
            font-size: 18px;
            background-color: #e67e22;
            color: white;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #d35400;
        }
    )");

    connect(okButton, &QPushButton::clicked,
            this, &OverlayChangePasswordWidget::handleChangePassword);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->addWidget(titleLabel);
    layout->addWidget(tipLabel);
    layout->addSpacing(12);
    layout->addWidget(passwordEdit);
    layout->addWidget(confirmEdit);
    layout->addSpacing(20);
    layout->addWidget(okButton);

    panel->move((width() - panel->width()) / 2,
                (height() - panel->height()) / 2);
}




bool OverlayChangePasswordWidget::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        auto *ke = dynamic_cast<QKeyEvent *>(e);
        if (ke && ke->key() == Qt::Key_Escape)
            return true;
    }
    return QWidget::event(e);
}


void OverlayChangePasswordWidget::handleChangePassword()
{
    const QString newPwd = passwordEdit->text().trimmed();
    const QString confirmPwd = confirmEdit->text().trimmed();

    if (newPwd.isEmpty() || confirmPwd.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请输入并确认新密码"));
        return;
    }

    if (newPwd != confirmPwd) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("两次输入的密码不一致"));
        confirmEdit->clear();
        return;
    }

    // 复用已有的复杂度校验
    if (!DatabaseManager::instance().isValidPasswordComplexity(newPwd)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(
            tr("密码不符合要求"),
            tr("密码长度需大于8位，并至少包含三种类型：\n"
               "大写字母、小写字母、数字、特殊字符"));
        return;
    }

    if (!DatabaseManager::instance().updatePasswd_Time_FirstLogin(m_username,newPwd,QDateTime::currentDateTime())) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("失败"), tr("密码更新失败，请联系管理员"));
        return;
    }

    AuditLogger::instance().log(
        QString("用户 %1 因密码过期修改密码").arg(m_username));

    emit passwordChanged();
    close();
}
