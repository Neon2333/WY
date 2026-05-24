#ifndef HISTORY_PAGE_H
#define HISTORY_PAGE_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>

class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPage(QWidget *parent = nullptr);
    ~HistoryPage();

private slots:
    void loadHistory();
    void onDeleteClicked();
    void onExportClicked();

private:
    void setupUi();

    struct Ui_HistoryPage {
        QTableWidget *tableWidget = nullptr;
    } *m_ui;
};

#endif