#ifndef FRMINPUT_H
#define FRMINPUT_H

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QTimer>
#include <QMutexLocker>

namespace Ui
{
class frmInput;
}

class frmInput : public QWidget
{
    Q_OBJECT

public:
    explicit frmInput(QWidget *parent = 0);
    ~frmInput();

    // 单例模式
    static frmInput *Instance() {
        if (!_instance) {
            static QMutex mutex;
            QMutexLocker locker(&mutex);
            if (!_instance) {
                _instance = new frmInput;
            }
        }
        return _instance;
    }

    // 初始化面板状态
    void Init(QString position, QString style);
    // 公共函数用于外部控制显示/隐藏
    void setForceShow(bool force);

protected:
    // 事件过滤器
    bool eventFilter(QObject *obj, QEvent *event);


private slots:
    // 焦点改变事件槽函数处理
    void focusChanged(QWidget *oldWidget, QWidget *nowWidget);
    // 输入法面板按键处理
    void btn_clicked();
    // 改变输入法面板样式
    void changeStyle(QString topColor, QString bottomColor,
                     QString borderColor, QString textColor);
    // 定时器处理退格键
    void reClicked();

private:
    Ui::frmInput *ui;
    static frmInput *_instance;     // 实例对象

    bool m_forceShow;               // 强制显示标志

    int deskWidth;                  // 桌面宽度
    int deskHeight;                 // 桌面高度
    int frmWidth;                   // 窗体宽度
    int frmHeight;                  // 窗体高度

    QPoint mousePoint;              // 鼠标拖动自定义标题栏时的坐标
    bool mousePressed;              // 鼠标是否按下

    bool isPress;                   // 是否长按退格键
    QPushButton *btnPress;          // 长按按钮
    QTimer *timerPress;             // 退格键定时器
    bool checkPress(QPushButton *btn);              // 校验当前长按的按钮

    bool isFirst;                   // 是否首次加载
    void InitForm();                // 初始化窗体数据
    void InitProperty();            // 初始化属性
    void ChangeStyle();             // 改变样式
    void ChangeFont();              // 改变字体大小
    void ShowPanel();               // 显示输入法面板

    QWidget *currentWidget;         // 当前焦点的对象
    QLineEdit *currentLineEdit;     // 当前焦点的单行文本框
    QTextEdit *currentTextEdit;     // 当前焦点的多行文本框
    QPlainTextEdit *currentPlain;   // 当前焦点的富文本框
    QTextBrowser *currentBrowser;   // 当前焦点的文本浏览框

    QString currentEditType;        // 当前焦点控件的类型
    QString currentPosition;        // 当前输入法面板位置类型
    QString currentStyle;           // 当前输入法面板样式
    int btnFontSize;                // 当前输入法面板按钮字体大小
    int labFontSize;                // 当前输入法面板标签字体大小
    void insertValue(QString value); // 插入值到当前焦点控件
    void deleteValue();             // 删除当前焦点控件的一个字符

    QString currentType;            // 当前输入法类型（min小写/max大写）
    void changeType(QString type);  // 改变输入法类型
    void changeLetter(bool isUpper); // 改变字母大小写

private:
    int keyboardHeight;         // 键盘自身高度（固定）
    int preferredOffset;        // 输入框下方偏移距离（比如 5~10 像素）
};

#endif // FRMINPUT_H

