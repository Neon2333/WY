/*
 * 可能使用了 EGLFS 或 LinuxFB 平台插件
 * 在这些平台下，Qt 默认只创建一个全屏窗口，不支持多个自由浮动窗口！
 * 换句话说：所有弹窗（QDialog/QMessageBox）都会“强制全屏”或者“锚定到左上角”，我们设置的位置会被忽略。
 *
*/

/*
 * ！！！！！根本性解决方案（适用于嵌入式 Qt）：
 * 不要使用浮动窗口（QDialog、QMessageBox）！
 * 而是： 直接在主界面中创建一个“半透明遮罩 + 中间提示框”作为“弹窗”效果
*/

#include "OverlayMessage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>

OverlayMessage::OverlayMessage(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("background-color: rgba(0, 0, 0, 120);");

    if (parent)
        setGeometry(parent->rect());

    QWidget *popup = new QWidget(this);
    popup->setFixedSize(300, 180);
    popup->setAttribute(Qt::WA_StyledBackground);
    popup->setStyleSheet("background-color: white; border-radius: 10px;");

    m_titleLabel = new QLabel(popup);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_textLabel = new QLabel(popup);
    m_textLabel->setWordWrap(true);
    m_textLabel->setStyleSheet("font-size: 14px;");
    m_textLabel->setAlignment(Qt::AlignCenter);

    m_okButton = new QPushButton(tr("确定"), popup);
    m_okButton->setFixedSize(100, 40);
    m_okButton->setStyleSheet(R"(
        QPushButton {
            font-weight: bold;
            font-size: 18px;
            background-color: #229590;
            color: white;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
    )");

    m_cancelButton = new QPushButton(tr("取消"), popup);
    m_cancelButton->setFixedSize(100, 40);
    m_cancelButton->setStyleSheet(R"(
        QPushButton {
            font-weight: bold;
            font-size: 18px;
            background-color: #229590;
            color: white;
            border: none;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #bbbbbb;
        }
    )");
    m_cancelButton->hide();  // 默认不显示

    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        this->close();
        emit closed();
    });

    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        this->close();
        emit cancelled();
    });

    QVBoxLayout *layout = new QVBoxLayout(popup);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_textLabel);
    layout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_okButton);
    btnLayout->addWidget(m_cancelButton);
    btnLayout->addStretch();

    layout->addLayout(btnLayout);

    popup->move((width() - popup->width()) / 2, (height() - popup->height()) / 2);
}

void OverlayMessage::showMessage(const QString &title, const QString &text)
{
    m_titleLabel->setText(title);
    m_textLabel->setText(text);
    show();
}

void OverlayMessage::setShowCancelButton(bool show)
{
    m_cancelButton->setVisible(show);
}
