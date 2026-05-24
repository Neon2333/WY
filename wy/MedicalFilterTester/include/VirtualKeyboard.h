#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFont>

class VirtualKeyboard : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualKeyboard(QWidget *parent = nullptr);
    void setTargetInput(QLineEdit *input);

signals:
    void keyPressed(const QString &key);
    void enterPressed();
    void closeRequested();

private:
    void createKey(const QString &text, QGridLayout *layout, int row, int col, int rowSpan = 1, int colSpan = 1);
    void updateKeyLabels();

    QLineEdit *m_targetInput = nullptr;
    bool m_isShifted = false;
    bool m_isNumberMode = false;

    QPushButton *m_shiftBtn = nullptr;
    QPushButton *m_spaceBtn = nullptr;
    QPushButton *m_enterBtn = nullptr;
    QPushButton *m_backspaceBtn = nullptr;
    QPushButton *m_123Btn = nullptr;
    QList<QPushButton*> m_letterKeys;

    QLabel *m_dragHandle = nullptr;
    bool m_dragging = false;
    QPoint m_dragPosition;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

#endif