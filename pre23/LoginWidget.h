// LoginWidget.h
#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include "DatabaseManager.h"

namespace Ui {
class LoginWidget;
}

/**
 * @brief The LoginWidget class
 * 负责界面上的用户名/密码输入和验证，验证成功后
 * 发出 loginSuccess(const QString&, const QString&) 信号
 * 将用户名和权限等级传递给上层（MainWindow）。
 */
class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

    void retranslateUi();  // 添加刷新方法

signals:
    /**
     * @brief loginSuccess
     * 登录验证通过后发出。
     * @param username 验证后的用户名
     * @param level    用户权限等级
     */
    void loginSuccess(const QString &username, const QString &level);

private slots:
    /**
     * @brief attemptLogin
     * 响应“登录”按钮点击，进行用户名/密码校验，
     * 成功后查询出权限等级并发出 loginSuccess。
     */
    void attemptLogin();
    void exitTheProgram();

private:
    Ui::LoginWidget *ui;

    // 如果用单例 DatabaseManager，就这样持有它
    DatabaseManager &m_dbManager = DatabaseManager::instance();

};

#endif // LOGINWIDGET_H

