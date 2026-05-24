#include "LoginWindow.h"
#include "MessageDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
}

void LoginWindow::setupUi()
{
    this->setFixedSize(400, 500);
    this->setWindowTitle(QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5\u7CFB\u7EDF"));
    this->setStyleSheet("background-color: transparent;");

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setGeometry(10, 10, 380, 480);
    mainWidget->setStyleSheet("background-color: white; border-radius: 15px;");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 5);
    mainWidget->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(30, 40, 30, 30);
    mainLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5"));
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1976D2;");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel(QString::fromUtf8("\u8BBE\u5907\u767B\u5F55"));
    subtitleLabel->setStyleSheet("font-size: 14px; color: #757575; margin-bottom: 20px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    m_usernameInput = new QLineEdit();
    m_usernameInput->setPlaceholderText(QString::fromUtf8("\u8BF7\u8F93\u5165\u7528\u6237\u540D"));
    m_usernameInput->setFixedHeight(45);
    m_usernameInput->setStyleSheet("QLineEdit { border: 2px solid #E0E0E0; border-radius: 8px; padding: 10px; font-size: 14px; background: #F5F5F5; color: #212121; selection-background-color: #2196F3; } QLineEdit:focus { border-color: #2196F3; }");

    m_passwordInput = new QLineEdit();
    m_passwordInput->setPlaceholderText(QString::fromUtf8("\u8BF7\u8F93\u5165\u5BC6\u7801"));
    m_passwordInput->setFixedHeight(45);
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setStyleSheet("QLineEdit { border: 2px solid #E0E0E0; border-radius: 8px; padding: 10px; font-size: 14px; background: #F5F5F5; color: #212121; selection-background-color: #2196F3; } QLineEdit:focus { border-color: #2196F3; }");

    m_loginBtn = new QPushButton(QString::fromUtf8("\u767B\u5F55"));
    m_loginBtn->setFixedHeight(45);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; } QPushButton:pressed { background-color: #1565C0; }");
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);

    QPushButton *testLoginBtn = new QPushButton(QString::fromUtf8("\u4F7F\u7528\u6D4B\u8BD5\u8D26\u53F7\u767B\u5F55"));
    testLoginBtn->setFixedHeight(40);
    testLoginBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 13px; font-weight: bold; } QPushButton:hover { background-color: #388E3C; }");
    connect(testLoginBtn, &QPushButton::clicked, this, [this]() {
        m_usernameInput->setText("admin");
        m_passwordInput->setText("admin123");
        onLoginClicked();
    });

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_usernameInput);
    mainLayout->addWidget(m_passwordInput);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_loginBtn);
    mainLayout->addWidget(testLoginBtn);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void LoginWindow::onLoginClicked()
{
    QString username = m_usernameInput->text().trimmed();
    QString password = m_passwordInput->text();

    qDebug() << "Debug login - Username:" << username << "length:" << username.length();
    qDebug() << "Debug login - Password:" << password << "length:" << password.length();

    if (username.isEmpty() || password.isEmpty()) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u767B\u5F55\u5931\u8D25"), QString::fromUtf8("\u7528\u6237\u540D\u548C\u5BC6\u7801\u4E0D\u80FD\u4E3A\u7A7A"), MessageDialog::Warning);
        return;
    }

    if (DatabaseManager::instance().verifyUser(username, password)) {
        m_currentUser = username;
        m_usernameInput->clear();
        m_passwordInput->clear();
        emit loginSuccess(username);
    } else {
        MessageDialog::showMessage(this, QString::fromUtf8("\u767B\u5F55\u5931\u8D25"),
            QString::fromUtf8("\u7528\u6237\u540D\u6216\u5BC6\u7801\u9519\u8BEF\n\n\u5F53\u524D\u8F93\u5165: %1 / %2").arg(username).arg(password), MessageDialog::Error);
    }
}

QString LoginWindow::getCurrentUser() const
{
    return m_currentUser;
}

void LoginWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(10, 10, -10, -10), 18, 18);
    painter.fillPath(path, QColor(0, 0, 0, 30));
}
