#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QStackedLayout>
#include <QString>
#include <QLabel>

class ParamInputPage;
class TestSelectionPage;
class HistoryPage;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &operatorName, QWidget *parent = nullptr);

signals:
    void logout();
    void exportRequested();

private slots:
    void onSettingsClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();

    QString m_operator;
    QLabel *m_operatorLabel = nullptr;
    QWidget *m_contentWidget = nullptr;
    QStackedLayout *m_contentLayout = nullptr;
};

#endif