#include "VirtualKeyboard.h"
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFont>
#include <QEvent>
#include <QApplication>
#include <QMouseEvent>

VirtualKeyboard::VirtualKeyboard(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
    , m_dragging(false)
    , m_dragPosition(0, 0)
{
    setFixedSize(700, 300);
    setWindowTitle(QString::fromUtf8("虚拟键盘"));
    setStyleSheet("background-color: #ECEFF1; border: 2px solid #B0BEC5; border-radius: 10px;");
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    QLabel *dragHandle = new QLabel(QString::fromUtf8("═══════ 拖动此处移动键盘 ═══════"));
    dragHandle->setAlignment(Qt::AlignCenter);
    dragHandle->setStyleSheet("background-color: #607D8B; color: white; border-radius: 5px; padding: 5px; font-size: 12px;");
    dragHandle->setFixedHeight(30);
    dragHandle->installEventFilter(this);
    mainLayout->addWidget(dragHandle);

    QGridLayout *row1 = new QGridLayout;
    QGridLayout *row2 = new QGridLayout;
    QGridLayout *row3 = new QGridLayout;
    QGridLayout *row4 = new QGridLayout;

    row1->setSpacing(5);
    row2->setSpacing(5);
    row3->setSpacing(5);
    row4->setSpacing(5);

    QStringList row1LetterKeys = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
    QStringList row2LetterKeys = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
    QStringList row3LetterKeys = {"Z", "X", "C", "V", "B", "N", "M"};

    for (int i = 0; i < row1LetterKeys.size(); ++i) {
        createKey(row1LetterKeys[i], row1, 0, i);
    }

    for (int i = 0; i < row2LetterKeys.size(); ++i) {
        createKey(row2LetterKeys[i], row2, 0, i);
    }

    m_shiftBtn = new QPushButton("Shift");
    m_shiftBtn->setFixedSize(70, 50);
    m_shiftBtn->setFocusPolicy(Qt::NoFocus);
    m_shiftBtn->setStyleSheet("QPushButton { background-color: #90A4AE; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #78909C; }");
    row3->addWidget(m_shiftBtn, 0, 0);

    for (int i = 0; i < row3LetterKeys.size(); ++i) {
        createKey(row3LetterKeys[i], row3, 0, i + 1);
    }

    m_backspaceBtn = new QPushButton(QString::fromUtf8("回退"));
    m_backspaceBtn->setFixedSize(90, 50);
    m_backspaceBtn->setFocusPolicy(Qt::NoFocus);
    m_backspaceBtn->setStyleSheet("QPushButton { background-color: #90A4AE; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #78909C; }");
    row3->addWidget(m_backspaceBtn, 0, 8);

    m_123Btn = new QPushButton("ABC");
    m_123Btn->setFixedSize(60, 50);
    m_123Btn->setFocusPolicy(Qt::NoFocus);
    m_123Btn->setStyleSheet("QPushButton { background-color: #607D8B; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #455A64; }");
    row4->addWidget(m_123Btn, 0, 0);

    m_spaceBtn = new QPushButton(QString::fromUtf8("空格"));
    m_spaceBtn->setFixedHeight(50);
    m_spaceBtn->setFocusPolicy(Qt::NoFocus);
    m_spaceBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_spaceBtn->setStyleSheet("QPushButton { background-color: white; color: #212121; border: 2px solid #B0BEC5; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #ECEFF1; }");
    row4->addWidget(m_spaceBtn, 0, 1, 1, 6);

    m_enterBtn = new QPushButton(QString::fromUtf8("确定"));
    m_enterBtn->setFixedSize(100, 50);
    m_enterBtn->setFocusPolicy(Qt::NoFocus);
    m_enterBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #388E3C; }");
    row4->addWidget(m_enterBtn, 0, 7);

    QPushButton *closeBtn = new QPushButton(QString::fromUtf8("关闭"));
    closeBtn->setFixedSize(60, 50);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet("QPushButton { background-color: #F44336; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #D32F2F; }");
    row4->addWidget(closeBtn, 0, 8);

    mainLayout->addLayout(row1);
    mainLayout->addLayout(row2);
    mainLayout->addLayout(row3);
    mainLayout->addLayout(row4);

    setLayout(mainLayout);

    m_dragHandle = dragHandle;
    m_isNumberMode = true;
    updateKeyLabels();

    connect(m_shiftBtn, &QPushButton::clicked, this, [this]() {
        m_isShifted = !m_isShifted;
        updateKeyLabels();
    });

    connect(m_123Btn, &QPushButton::clicked, this, [this]() {
        m_isNumberMode = !m_isNumberMode;
        m_123Btn->setText(m_isNumberMode ? "ABC" : "123");
        updateKeyLabels();
    });

    connect(m_spaceBtn, &QPushButton::clicked, this, [this]() {
        if (m_targetInput) {
            m_targetInput->insert(" ");
        }
    });

    connect(m_enterBtn, &QPushButton::clicked, this, [this]() {
        emit enterPressed();
    });

    connect(m_backspaceBtn, &QPushButton::clicked, this, [this]() {
        if (m_targetInput) {
            m_targetInput->backspace();
        }
    });

    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        emit closeRequested();
        hide();
    });
}

void VirtualKeyboard::createKey(const QString &text, QGridLayout *layout, int row, int col, int rowSpan, int colSpan)
{
    QPushButton *btn = new QPushButton(text);
    btn->setFixedSize(55, 50);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setStyleSheet("QPushButton { background-color: white; color: #212121; border: 2px solid #B0BEC5; border-radius: 5px; font-size: 16px; font-weight: bold; } QPushButton:hover { background-color: #ECEFF1; } QPushButton:pressed { background-color: #CFD8DC; }");
    layout->addWidget(btn, row, col, rowSpan, colSpan);

    connect(btn, &QPushButton::clicked, this, [this, btn]() {
        if (m_targetInput) {
            QString charToInsert = btn->text();
            
            if (!m_isNumberMode && !m_isShifted) {
                charToInsert = charToInsert.toLower();
            }
            
            m_targetInput->insert(charToInsert);
            
            if (m_isShifted && !m_isNumberMode) {
                m_isShifted = false;
                updateKeyLabels();
            }
        }
    });

    m_letterKeys.append(btn);
}

void VirtualKeyboard::setTargetInput(QLineEdit *input)
{
    m_targetInput = input;
}

void VirtualKeyboard::updateKeyLabels()
{
    QStringList row1Keys, row2Keys, row3Keys;

    if (m_isNumberMode) {
        row1Keys = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
        row2Keys = {"-", "/", ":", ";", "(", ")", "$", "&", "@", "\""};
        row3Keys = {".", ",", "?", "!", "'"};
    } else {
        row1Keys = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
        row2Keys = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
        row3Keys = {"Z", "X", "C", "V", "B", "N", "M"};
    }

    int index = 0;
    for (const QString &key : row1Keys) {
        if (index < m_letterKeys.size()) {
            m_letterKeys[index]->setText(m_isShifted && !m_isNumberMode ? key : key.toLower());
            m_letterKeys[index]->setVisible(true);
            index++;
        }
    }

    for (const QString &key : row2Keys) {
        if (index < m_letterKeys.size()) {
            m_letterKeys[index]->setText(m_isShifted && !m_isNumberMode ? key : key.toLower());
            m_letterKeys[index]->setVisible(true);
            index++;
        }
    }

    for (const QString &key : row3Keys) {
        if (index < m_letterKeys.size()) {
            m_letterKeys[index]->setText(m_isShifted && !m_isNumberMode ? key : key.toLower());
            m_letterKeys[index]->setVisible(true);
            index++;
        }
    }

    for (int i = index; i < m_letterKeys.size(); ++i) {
        m_letterKeys[i]->setVisible(false);
    }

    if (m_isNumberMode) {
        m_shiftBtn->setVisible(false);
    } else {
        m_shiftBtn->setVisible(true);
        if (m_isShifted) {
            m_shiftBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; }");
        } else {
            m_shiftBtn->setStyleSheet("QPushButton { background-color: #90A4AE; color: white; border: none; border-radius: 5px; font-size: 14px; font-weight: bold; }");
        }
    }
}

bool VirtualKeyboard::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_dragHandle) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            m_dragging = true;
            m_dragPosition = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (m_dragging) {
                move(mouseEvent->globalPosition().toPoint() - m_dragPosition);
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void VirtualKeyboard::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit enterPressed();
    }
    QWidget::keyPressEvent(event);
}