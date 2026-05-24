#include "MessageDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>

MessageDialog::MessageDialog(QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowModality(Qt::ApplicationModal);
    setModal(true);
}

void MessageDialog::setupUi(const QString &title, const QString &message, DialogType type)
{
    this->setFixedSize(400, 220);
    this->setStyleSheet("background-color: transparent;");

    QWidget *contentWidget = new QWidget(this);
    contentWidget->setGeometry(5, 5, 390, 210);

    QColor bgColor = Qt::white;
    if (type == Success) bgColor = QColor(232, 245, 233);
    else if (type == Warning) bgColor = QColor(255, 243, 224);
    else if (type == Error) bgColor = QColor(255, 235, 238);

    contentWidget->setStyleSheet(QString("background-color: %1; border-radius: 15px;").arg(bgColor.name()));

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 3);
    contentWidget->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 顶部标题和图标区域
    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(12);

    QLabel *iconLabel = new QLabel(contentWidget);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(40, 40);

    QString iconStyle;
    QString iconText;
    if (type == Information) {
        iconStyle = "background-color: #2196F3; border-radius: 20px;";
        iconText = "ℹ";
    } else if (type == Warning) {
        iconStyle = "background-color: #FF9800; border-radius: 20px;";
        iconText = "⚠";
    } else if (type == Success) {
        iconStyle = "background-color: #4CAF50; border-radius: 20px;";
        iconText = "✓";
    } else if (type == Error) {
        iconStyle = "background-color: #F44336; border-radius: 20px;";
        iconText = "✕";
    }
    iconLabel->setStyleSheet(QString("color: white; font-size: 20px; font-weight: bold; %1").arg(iconStyle));
    iconLabel->setText(iconText);

    QLabel *titleLabel = new QLabel(title, contentWidget);
    titleLabel->setStyleSheet("font-size: 19px; font-weight: bold; color: #212121; background: transparent;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    // 消息文本区域
    QLabel *msgLabel = new QLabel(message, contentWidget);
    msgLabel->setStyleSheet("font-size: 16px; color: #424242; background: transparent;");
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);

    // 按钮区域
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(15);

    m_okBtn = new QPushButton(QString::fromUtf8("\u786E\u5B9A"), contentWidget);
    m_okBtn->setFixedSize(120, 40);
    m_okBtn->setCursor(Qt::PointingHandCursor);
    m_okBtn->setDefault(true);
    m_okBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #388E3C; }");

    m_cancelBtn = new QPushButton(QString::fromUtf8("\u53D6\u6D88"), contentWidget);
    m_cancelBtn->setFixedSize(120, 40);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet("QPushButton { background-color: #F44336; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #D32F2F; }");

    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(headerLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(msgLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    m_messageLabel = msgLabel;

    connect(m_okBtn, &QPushButton::clicked, this, &MessageDialog::onOkClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MessageDialog::onCancelClicked);

    m_okBtn->setFocus();
}

void MessageDialog::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(5, 5, -5, -5), 18, 18);
    painter.fillPath(path, QColor(0, 0, 0, 50));
}

void MessageDialog::showMessage(QWidget *parent, const QString &title, const QString &message, DialogType type)
{
    Q_UNUSED(parent);
    MessageDialog *dlg = new MessageDialog();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setupUi(title, message, type);
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    dlg->move(screenGeometry.center() - dlg->rect().center());
    dlg->m_cancelBtn->hide();
    dlg->open();
}

bool MessageDialog::question(QWidget *parent, const QString &title, const QString &message)
{
    Q_UNUSED(parent);
    MessageDialog *dlg = new MessageDialog();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setupUi(title, message, Warning);
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    dlg->move(screenGeometry.center() - dlg->rect().center());
    dlg->exec();
    return dlg->m_result;
}

void MessageDialog::onOkClicked()
{
    m_result = true;
    accept();
    close();
}

void MessageDialog::onCancelClicked()
{
    m_result = false;
    reject();
    close();
}