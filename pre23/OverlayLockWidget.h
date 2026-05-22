#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class OverlayLockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayLockWidget(QWidget *parent = nullptr);
    void showPanel();  // 显示认证界面

signals:
    void loginSuccess(const QString &username, const QString &userLevel);
    void activity();  // 每次检测到用户活动时发射

private slots:
    void handleLogin();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *okButton;

    void setupUI();

protected:
    bool event(QEvent *e) override;

};
