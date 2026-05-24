#include "frminput.h"
#include "ui_frminput.h"
#include <QScreen>
#include <QGuiApplication>
#include <QPointer>
#include <QEvent>
#include <QKeyEvent>

// 单例声明
frmInput *frmInput::_instance = 0;

///////////////////////////////////////////////////////////////////////////////
// 构造/析构函数
///////////////////////////////////////////////////////////////////////////////

frmInput::frmInput(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmInput)
{
    ui->setupUi(this);
    this->InitProperty();
    this->InitForm();
    this->ChangeStyle();
    m_forceShow = false;
}

frmInput::~frmInput()
{
    delete ui;
}


// 初始化相关函数

void frmInput::Init(QString position, QString style)
{
    this->currentPosition = position;
    this->currentStyle = style;
    this->ChangeStyle();
}

void frmInput::InitForm()
{
    // 触摸屏优化设置
    this->setAttribute(Qt::WA_AcceptTouchEvents, true);

    // 设置窗口透明背景
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setStyleSheet("background: transparent;");

    // 新增：记录键盘高度（因为你 Designer 里大小固定）
    keyboardHeight = this->height();           // 或 ui->frameMain->height() 如果 frame 是主容器
    preferredOffset = 5;                       // 输入框下方留一点空隙

    Qt::WindowFlags flags = Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus|Qt::SubWindow;;

    this->setWindowFlags(flags);

    // 不激活窗口显示（避免点击键盘时窗口被激活导致输入控件失焦）
    this->setAttribute(Qt::WA_ShowWithoutActivating, true);

    // 窗口及主要容器不接受焦点（防止抢占）
    this->setFocusPolicy(Qt::NoFocus);
    ui->frameMain->setFocusPolicy(Qt::NoFocus);

    isFirst = true;

    // 初始化控件指针
    currentWidget = 0;
    currentLineEdit = 0;
    currentTextEdit = 0;
    currentPlain = 0;
    currentBrowser = 0;
    currentEditType = "";

    currentPosition = "";
    currentStyle = "";
    // btnFontSize = 10;
    // labFontSize = 6;


    currentType = "min";
    changeType(currentType);

    // 连接所有按钮的点击信号并禁止按钮获得焦点
    QList<QPushButton *> btn = this->findChildren<QPushButton *>();
    foreach (QPushButton * b, btn) {
        connect(b, SIGNAL(clicked()), this, SLOT(btn_clicked()));
        b->setFocusPolicy(Qt::NoFocus); // 重要：避免按钮获取焦点
    }

    // 绑定全局改变焦点信号槽
    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            this, SLOT(focusChanged(QWidget *, QWidget *)));

    // 安装事件过滤器
    qApp->installEventFilter(this);
}

void frmInput::InitProperty()
{
    // 属性设置保持不变（原样保留）
    ui->btnOther1->setProperty("btnOther", true);
    ui->btnOther2->setProperty("btnOther", true);
    ui->btnOther3->setProperty("btnOther", true);
    ui->btnOther4->setProperty("btnOther", true);
    ui->btnOther5->setProperty("btnOther", true);
    ui->btnOther7->setProperty("btnOther", true);
    ui->btnOther8->setProperty("btnOther", true);
    ui->btnOther9->setProperty("btnOther", true);
    ui->btnOther10->setProperty("btnOther", true);
    ui->btnOther11->setProperty("btnOther", true);
    ui->btnOther12->setProperty("btnOther", true);
    ui->btnOther13->setProperty("btnOther", true);
    ui->btnOther14->setProperty("btnOther", true);
    ui->btnOther15->setProperty("btnOther", true);
    ui->btnOther16->setProperty("btnOther", true);
    ui->btnOther17->setProperty("btnOther", true);
    ui->btnOther18->setProperty("btnOther", true);
    ui->btnOther19->setProperty("btnOther", true);
    ui->btnOther20->setProperty("btnOther", true);
    ui->btnOther21->setProperty("btnOther", true);

    ui->btnDot->setProperty("btnOther", true);
    ui->btnSpace->setProperty("btnOther", true);
    ui->btnDelete->setProperty("btnOther", true);

    ui->btn0->setProperty("btnNum", true);
    ui->btn1->setProperty("btnNum", true);
    ui->btn2->setProperty("btnNum", true);
    ui->btn3->setProperty("btnNum", true);
    ui->btn4->setProperty("btnNum", true);
    ui->btn5->setProperty("btnNum", true);
    ui->btn6->setProperty("btnNum", true);
    ui->btn7->setProperty("btnNum", true);
    ui->btn8->setProperty("btnNum", true);
    ui->btn9->setProperty("btnNum", true);

    ui->btna->setProperty("btnLetter", true);
    ui->btnb->setProperty("btnLetter", true);
    ui->btnc->setProperty("btnLetter", true);
    ui->btnd->setProperty("btnLetter", true);
    ui->btne->setProperty("btnLetter", true);
    ui->btnf->setProperty("btnLetter", true);
    ui->btng->setProperty("btnLetter", true);
    ui->btnh->setProperty("btnLetter", true);
    ui->btni->setProperty("btnLetter", true);
    ui->btnj->setProperty("btnLetter", true);
    ui->btnk->setProperty("btnLetter", true);
    ui->btnl->setProperty("btnLetter", true);
    ui->btnm->setProperty("btnLetter", true);
    ui->btnn->setProperty("btnLetter", true);
    ui->btno->setProperty("btnLetter", true);
    ui->btnp->setProperty("btnLetter", true);
    ui->btnq->setProperty("btnLetter", true);
    ui->btnr->setProperty("btnLetter", true);
    ui->btns->setProperty("btnLetter", true);
    ui->btnt->setProperty("btnLetter", true);
    ui->btnu->setProperty("btnLetter", true);
    ui->btnv->setProperty("btnLetter", true);
    ui->btnw->setProperty("btnLetter", true);
    ui->btnx->setProperty("btnLetter", true);
    ui->btny->setProperty("btnLetter", true);
    ui->btnz->setProperty("btnLetter", true);
}

///////////////////////////////////////////////////////////////////////////////
// 事件处理和焦点管理
///////////////////////////////////////////////////////////////////////////////

void frmInput::ShowPanel()
{
    // 显示时也不激活窗口（保持输入控件焦点）
    this->setVisible(true);
    this->raise();

}

bool frmInput::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::MouseButtonPress)
        return QWidget::eventFilter(obj, event);

    QWidget *w = qobject_cast<QWidget *>(obj);
    if (!w)
        return QWidget::eventFilter(obj, event);

    // 1点击发生在输入法自身（键盘区域） → 不处理，不隐藏
    if (this->isAncestorOf(w) || w == this) {
        return QWidget::eventFilter(obj, event);
    }

    // 2noinput 属性的控件 → 忽略
    if (w->property("noinput").toBool()) {
        return QWidget::eventFilter(obj, event);
    }

    // 3输入控件 → 显示键盘
    if (w->inherits("QLineEdit")) {

        ShowPanel();
        return QWidget::eventFilter(obj, event);
    }else {
        this->setVisible(false);
    }

    //4 其它地方 → 隐藏键盘
    this->setVisible(false);
    return QWidget::eventFilter(obj, event);
}

bool frmInput::checkPress(QPushButton *btn)
{
    if (!btn) return false;

    // 只有属于输入法键盘的合法按钮才继续处理
    bool num_ok = btn->property("btnNum").toBool();
    bool other_ok = btn->property("btnOther").toBool();
    bool letter_ok = btn->property("btnLetter").toBool();

    return (num_ok || other_ok || letter_ok);
}

void frmInput::reClicked()
{
    if (isPress && btnPress) {
        timerPress->setInterval(30);
        btnPress->click();
    }
}

void frmInput::focusChanged(QWidget *oldWidget, QWidget *nowWidget)
{
    Q_UNUSED(oldWidget);

    // 如果新焦点在键盘自身内部 → 不要隐藏，也不要移动（用户在点键盘按钮）
    if (nowWidget && this->isAncestorOf(nowWidget)) {
        return;
    }

    if (nowWidget && !this->isAncestorOf(nowWidget)) {
        // 忽略 noinput 属性控件
        if (nowWidget->property("noinput").toBool()) {
            if (!m_forceShow) {
                this->setVisible(false);
            }
            return;
        }

        isFirst = false;

        // 只处理 QLineEdit（你可以以后扩展到 QTextEdit 等）
        if (nowWidget->inherits("QLineEdit")) {
            currentLineEdit = qobject_cast<QLineEdit*>(nowWidget);
            currentEditType = "QLineEdit";

            // ======== 新增：自动定位到输入框下方 ========
            if (currentLineEdit) {
                // 1. 获取输入框在屏幕上的全局位置和大小
                QRect inputRect = currentLineEdit->rect();                      // 控件本地矩形
                QPoint bottomLeftGlobal = currentLineEdit->mapToGlobal(QPoint(0, inputRect.bottom() + preferredOffset));

                // 2. 计算键盘建议位置（默认放在输入框左下角对齐）
                int x = bottomLeftGlobal.x();
                int y = bottomLeftGlobal.y();

                // 3. 防止键盘超出屏幕底部 → 如果放不下，就放上面
                QScreen *screen = QGuiApplication::screenAt(currentLineEdit->mapToGlobal(QPoint(0,0)));
                if (!screen) screen = QGuiApplication::primaryScreen();

                QRect screenRect = screen->availableGeometry();  // 可用区域（避开任务栏）

                if (y + keyboardHeight > screenRect.bottom()) {
                    // 放上方：输入框顶部 - 键盘高度 - 一点间隙
                    y = currentLineEdit->mapToGlobal(QPoint(0, 0)).y() - keyboardHeight - preferredOffset;
                }

                // 4. 防止超出屏幕左/右边界（可选居中或靠左）
                if (x + this->width() > screenRect.right()) {
                    x = screenRect.right() - this->width();
                }
                if (x < screenRect.left()) {
                    x = screenRect.left();
                }

                // 5. 移动键盘到计算位置
                this->move(x, y);
            }

            ShowPanel();  // 显示键盘
        }
        else {
            // 非输入框 → 隐藏
            currentLineEdit = nullptr;
            currentEditType = "";
            m_forceShow = false;
            this->setVisible(false);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
// 其他辅助函数
///////////////////////////////////////////////////////////////////////////////

void frmInput::setForceShow(bool force)
{
    m_forceShow = force;
    if (!force) {
        this->setVisible(false);
    }
}


///////////////////////////////////////////////////////////////////////////////
// 按钮点击处理和文本输入
///////////////////////////////////////////////////////////////////////////////

void frmInput::changeType(QString type)
{
    if (type == "max") {
        changeLetter(true);
        ui->btnType->setText("大写Aa");
        ui->lab_Title->setText("大写AA");
    } else {
        changeLetter(false);
        ui->btnType->setText("小写Aa");
        ui->lab_Title->setText("小写aa");
    }
}

void frmInput::changeLetter(bool isUpper)
{
    QList<QPushButton *> btn = this->findChildren<QPushButton *>();
    foreach (QPushButton * b, btn) {
        if (b->property("btnLetter").toBool()) {
            b->setText(isUpper ? b->text().toUpper() : b->text().toLower());
        }
    }
}

void frmInput::btn_clicked()
{
    if (currentEditType == "") {
        // 没有目标输入控件，直接忽略（或隐藏）
        return;
    }

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString objectName = btn->objectName();

    // 在处理按钮点击之前，**尽量把焦点交还给当前输入控件**
    //（由于窗口设置为不接受焦点，这里 setFocus 不会把焦点给键盘窗口）
    if (currentEditType == "QLineEdit" && currentLineEdit) {
        currentLineEdit->setFocus(Qt::MouseFocusReason);
    } else if (currentEditType == "QTextEdit" && currentTextEdit) {
        currentTextEdit->setFocus(Qt::MouseFocusReason);
    } else if (currentEditType == "QPlainTextEdit" && currentPlain) {
        currentPlain->setFocus(Qt::MouseFocusReason);
    } else if (currentEditType == "QTextBrowser" && currentBrowser) {
        currentBrowser->setFocus(Qt::MouseFocusReason);
    } else if (currentEditType == "QWidget" && currentWidget) {
        currentWidget->setFocus(Qt::MouseFocusReason);
    }

    if (objectName == "btnType") {
        currentType = (currentType == "min") ? "max" : "min";
        changeType(currentType);
    } else if (objectName == "btnDelete") {
        deleteValue();
    } else if (objectName == "btnClose") {
        // 简单隐藏输入法
        this->setVisible(false);
    } else if (objectName == "btnSpace") {
        insertValue(" ");
    } else {
        QString value = btn->text();
        insertValue(value);
    }
}

void frmInput::insertValue(QString value)
{
    // 插入前确认当前焦点控件；如果焦点不在目标控件上，尝试再次设置焦点
    QWidget *focusWidget = qApp->focusWidget();
    if (focusWidget && focusWidget != currentLineEdit) {
        // 如果焦点改变到别处，则尝试恢复到之前记录的输入控件
        if (currentLineEdit && !currentLineEdit->hasFocus()) currentLineEdit->setFocus(Qt::MouseFocusReason);
        if (currentTextEdit && !currentTextEdit->hasFocus()) currentTextEdit->setFocus(Qt::MouseFocusReason);
        if (currentPlain && !currentPlain->hasFocus()) currentPlain->setFocus(Qt::MouseFocusReason);
        if (currentBrowser && !currentBrowser->hasFocus()) currentBrowser->setFocus(Qt::MouseFocusReason);
    }

    if (currentEditType == "QLineEdit" && currentLineEdit) {
        currentLineEdit->insert(value);
    } else if (currentEditType == "QTextEdit" && currentTextEdit) {
        currentTextEdit->insertPlainText(value);
    } else if (currentEditType == "QPlainTextEdit" && currentPlain) {
        currentPlain->insertPlainText(value);
    } else if (currentEditType == "QTextBrowser" && currentBrowser) {
        currentBrowser->insertPlainText(value);
    } else if (currentEditType == "QWidget" && currentWidget) {
        // 发送按键事件到控件（用于spinbox等）
        QKeyEvent keyPress(QEvent::KeyPress, 0, Qt::NoModifier, QString(value));
        QApplication::sendEvent(currentWidget, &keyPress);
    }
}

void frmInput::deleteValue()
{
    if (currentEditType == "QLineEdit" && currentLineEdit) {
        currentLineEdit->backspace();
    } else if (currentEditType == "QTextEdit" && currentTextEdit) {
        QTextCursor cursor = currentTextEdit->textCursor();
        if(cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.deletePreviousChar();
        }
    } else if (currentEditType == "QPlainTextEdit" && currentPlain) {
        QTextCursor cursor = currentPlain->textCursor();
        if(cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.deletePreviousChar();
        }
    } else if (currentEditType == "QTextBrowser" && currentBrowser) {
        QTextCursor cursor = currentBrowser->textCursor();
        if(cursor.hasSelection()) {
            cursor.removeSelectedText();
        } else {
            cursor.deletePreviousChar();
        }
    } else if (currentEditType == "QWidget" && currentWidget) {
        // 发送 Backspace（向后删除）
        QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier, QString());
        QApplication::sendEvent(currentWidget, &keyPress);
    }
}


///////////////////////////////////////////////////////////////////////////////
// 样式和字体设置
///////////////////////////////////////////////////////////////////////////////

void frmInput::ChangeStyle()
{
    if (currentStyle == "blue") {
        changeStyle("#DEF0FE", "#C0DEF6", "#C0DCF2", "#FFFFFF");
    } else if (currentStyle == "dev") {
        changeStyle("#C0D3EB", "#BCCFE7", "#B4C2D7", "#324C6C");
    } else if (currentStyle == "gray") {
        changeStyle("#E4E4E4", "#A2A2A2", "#A9A9A9", "#000000");
    } else if (currentStyle == "lightgray") {
        changeStyle("#EEEEEE", "#E5E5E5", "#D4D0C8", "#6F6F6F");
    } else if (currentStyle == "darkgray") {
        changeStyle("#D8D9DE", "#C8C8D0", "#A9ACB5", "#5D5C6C");
    } else if (currentStyle == "black") {
        changeStyle("#4D4D4D", "#292929", "#D9D9D9", "#CACAD0");
    } else if (currentStyle == "brown") {
        changeStyle("#667481", "#566373", "#C2CCD8", "#E7ECF0");
    } else if (currentStyle == "silvery") {
        changeStyle("#E1E4E6", "#CCD3D9", "#B2B6B9", "#000000");
    }
}

// void frmInput::ChangeFont()
// {
//     QFont btnFont(this->font().family(), btnFontSize);
//     QFont labFont(this->font().family(), labFontSize);
//     QList<QPushButton *> btns = ui->frameMain->findChildren<QPushButton *>();
//     foreach (QPushButton * btn, btns) {
//         btn->setFont(btnFont);
//     }
//     ui->btnClose->setFont(labFont);
// }

void frmInput::ChangeFont()
{
    // 直接指定字体家族为“微软雅黑”，避免依赖窗口本身的字体
    QFont btnFont("Microsoft YaHei", btnFontSize);   // 或 "微软雅黑"（两者等价，推荐用英文名更跨平台）
    QFont labFont("Microsoft YaHei", labFontSize);

    // 应用到所有按钮
    QList<QPushButton *> btns = ui->frameMain->findChildren<QPushButton *>();
    foreach (QPushButton *btn, btns) {
        btn->setFont(btnFont);
    }

    // 关闭按钮用 labFont
    ui->btnClose->setFont(labFont);
}

void frmInput::changeStyle(QString topColor, QString bottomColor, QString borderColor, QString textColor)
{
    QStringList qss;
    qss.append(QString("QWidget#widget_title{background:qlineargradient(spread:pad,x1:0,y1:0,x2:0,y2:1,stop:0 %1,stop:1 %2);}")
                   .arg(topColor).arg(bottomColor));
    qss.append("QPushButton{padding:10px;border-radius:5px;min-height:40px;}"); // 触屏优化：增大点击区域
    qss.append(QString("QPushButton:hover{background:qlineargradient(spread:pad,x1:0,y1:0,x2:0,y2:1,stop:0 %1,stop:1 %2);}")
                   .arg(topColor).arg(bottomColor));
    qss.append(QString("QLabel,QPushButton{color:%1;font-size:14px;}").arg(textColor)); // 增大字体
    qss.append(QString("QPushButton#btnClose{padding:5px;}"));
    qss.append(QString("QPushButton{border:2px solid %1;background:rgba(0,0,0,0);}")
                   .arg(borderColor));
    qss.append(QString("QLineEdit{border:2px solid %1;border-radius:5px;padding:5px;background:none;selection-background-color:%2;selection-color:%3;min-height:40px;}")
                   .arg(borderColor).arg(bottomColor).arg(topColor));
    this->setStyleSheet(qss.join(""));
}

