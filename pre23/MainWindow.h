// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


// #include "frminput.h"

class LoginWidget;
class MainWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);



private slots:
    /// 显示登录界面
    void showLogin();
    /**
     * @brief onLoginSuccess
     * 接收 loginSuccess(username, level) 信号后执行：
     * 1) 设置全局会话 2) 显示主界面
     */
    void onLoginSuccess(const QString &username, const QString &level);


private:
    LoginWidget  *m_loginWidget  = nullptr;
    MainWidget   *m_mainWidget   = nullptr;

    // void showEvent(QShowEvent *event);

    void handleLogout();


protected:
    // bool eventFilter(QObject *obj, QEvent *event) override;
    // void enableTouchForAllButtons(QWidget *parent = nullptr);
};

#endif // MAINWINDOW_H
