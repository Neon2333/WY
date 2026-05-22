#ifndef OVERLAYCHANGEPASSWORDWIDGET_H
#define OVERLAYCHANGEPASSWORDWIDGET_H

#include <QWidget>

class QLineEdit;
class QPushButton;

class OverlayChangePasswordWidget : public QWidget
{
    Q_OBJECT
public:
    enum class Reason {
        FirstLogin,
        PasswordExpired
    };

    explicit OverlayChangePasswordWidget(const QString &username,
                                         Reason reason,
                                         QWidget *parent = nullptr);


signals:
    // 密码修改成功后通知外部（attemptLogin 用）
    void passwordChanged();

protected:
    bool event(QEvent *e) override;

private slots:
    void handleChangePassword();

private:
    void setupUI();

private:
    QString     m_username;
    QLineEdit  *passwordEdit;
    QLineEdit  *confirmEdit;
    QPushButton *okButton;

    Reason m_reason;


};

#endif // OVERLAYCHANGEPASSWORDWIDGET_H
