#ifndef PAGEUSER_H
#define PAGEUSER_H

#include <QWidget>

namespace Ui {
class PageUser;
}

class PageUser : public QWidget
{
    Q_OBJECT

public:
    explicit PageUser(QWidget *parent = nullptr);
    ~PageUser();

    void init();
private:
    Ui::PageUser *ui;

    void saveUserSettings();
    void loadUserSettings();


    void refreshUserCombos();           // 更新三个下拉框的用户列表
    void AdminCanControlState(bool enabled);
    void SuperAdminCanControlState(bool enabled);

    void setupTable();
    void loadUserRows();

    void retranslateUi();
    void updateUIByPermission();

private slots:

    void on_userAddBtn_clicked();        // 新增用户
    void on_userKeyWordBtn_clicked();     // 修改密码
    void on_userSwitchBtn_clicked();     // 切换用户
    // void on_userDelBtn_clicked();        // 删除用户
    void on_userUnLockBtn_clicked();

    void onUserChanged(const QString &userName, const QString &userLevel);

};


#endif // PAGEUSER_H
