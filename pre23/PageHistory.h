// PageHistory.h
#ifndef PAGEHISTORY_H
#define PAGEHISTORY_H

#include <QWidget>
#include <QDateTime>
#include <QLineEdit>

namespace Ui {
class PageHistory;
}

class PageHistory : public QWidget
{
    Q_OBJECT

public:
    explicit PageHistory(QWidget *parent = nullptr);
    ~PageHistory() override;

    void init();  // 初始化方法

private:
    Ui::PageHistory *ui;

    // 辅助方法
    void setupTable();
    QDateTime getDateTimeFromLineEdits(QLineEdit* year, QLineEdit* mon, QLineEdit* day,
                                       QLineEdit* hour, QLineEdit* min, QLineEdit* sec,
                                       bool* ok = nullptr) const;

    // 数据加载方法
    void loadRecentRows();
    void loadRange(const QDateTime &start, const QDateTime &end);

    QDateTime getStartTime(int index) const;

    void refreshUserCombos();
    void refreshNumberCombos();

    void retranslateUi();
    void updateUIByPermission();
    int fetchTotalHistoryCount() const;

private slots:
    // UI 槽函数
    void filterClicked();
    void exportPdfClicked();
    void filterByTime(int index);

    void filterByUser(const QString &uname);
    void filterByNumber(const QString &number);
    void onUserChanged(const QString& name, const QString& level);
    void on_toPdfByUser_clicked();
    void on_toPdfByNumber_clicked();
    void on_toPdfByTime_clicked();

    void on_clearData_clicked();
};

#endif // PAGEHISTORY_H
