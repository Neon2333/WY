#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QLineEdit>
#include <QTextEdit>
#include "Keyboard.h"

using namespace AeaQt;


class InputManager : public QObject
{
    Q_OBJECT

public:
    static InputManager* Instance();

    void Init(QWidget *parent);

private slots:
    void focusChanged(QWidget *old, QWidget *now);

    void onKeyboardInput(int code, QString text);
private:
    explicit InputManager(QObject *parent = nullptr);

    void showKeyboard(QWidget* now);

    AeaQt::Keyboard *keyboard;
    QWidget *mainWidget;
    QWidget *currentInput;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

};

#endif
