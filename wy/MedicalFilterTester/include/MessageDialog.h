#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>

class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    enum DialogType {
        Information,
        Warning,
        Success,
        Error
    };

    static void showMessage(QWidget *parent, const QString &title, const QString &message, DialogType type = Information);
    static bool question(QWidget *parent, const QString &title, const QString &message);

private:
    explicit MessageDialog(QWidget *parent = nullptr);
    void setupUi(const QString &title, const QString &message, DialogType type);
    void paintEvent(QPaintEvent *event) override;

    QLabel *m_iconLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_okBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    bool m_result = false;

private slots:
    void onOkClicked();
    void onCancelClicked();
};

#endif