#ifndef PAGETRAIL_H
#define PAGETRAIL_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDateTime>

namespace Ui {
class PageTrail;
}

class PageTrail : public QWidget
{
    Q_OBJECT

public:
    explicit PageTrail(QWidget *parent = nullptr);
    ~PageTrail();

    void init();                                    // 初始化表格结构

protected:

private slots:
    void filterClicked();         // 筛选按钮点击
    void exportPdfClicked();      // 导出 PDF

    void filterByTime(int index);
    void filterByUser(const QString &uname);
    void filterByNumber(const QString &number);

    void onUserChanged(const QString& name, const QString& level);

    void on_toPdfByUser_clicked();
    void on_toPdfByNumber_clicked();
    void on_toPdfByTime_clicked();

    void on_clearData_clicked();
private:
    Ui::PageTrail *ui;
    QString currentDbPath;
    void retranslateUi();
    // 辅助函数
    QDateTime getDateTimeFromLineEdits(
        QLineEdit* year, QLineEdit* mon, QLineEdit* day,
        QLineEdit* hour, QLineEdit* min, QLineEdit* sec,
        bool* ok) const;

    // 其他功能方法
    void setupTable();                    // 设置表格结构

    QDateTime getStartTime(int index) const;

    void loadRecentRows();               // 加载最近数据
    void loadRange(const QDateTime &start, const QDateTime &end); // 加载时间范围

    void refreshUserCombos();
    void refreshNumberCombos();
    int fetchTotalTrailCount() const;
    void updateUIByPermission();
};

#endif // PAGETRAIL_H
