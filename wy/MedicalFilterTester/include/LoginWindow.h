#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QEvent>
#include <QPaintEvent>
#include "DatabaseManager.h"

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    QString getCurrentUser() const;

signals:
    void loginSuccess(const QString &username);

private slots:
    void onLoginClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();

    QLineEdit *m_usernameInput = nullptr;
    QLineEdit *m_passwordInput = nullptr;
    QPushButton *m_loginBtn = nullptr;
    QString m_currentUser;
};

#endif
