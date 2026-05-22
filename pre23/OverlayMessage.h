// OverlayMessage.h

#ifndef OVERLAYMESSAGE_H
#define OVERLAYMESSAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class OverlayMessage : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayMessage(QWidget *parent = nullptr);
    void showMessage(const QString &title, const QString &text);
    void setShowCancelButton(bool show);  // 可选的取消按钮控制接口

signals:
    void closed();
    void cancelled();  // 新增的取消信号

private:
    QLabel *m_titleLabel;
    QLabel *m_textLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // OVERLAYMESSAGE_H
