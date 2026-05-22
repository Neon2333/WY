// MainWidget.h
#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTranslator>
#include <QPixmap>
#include <QLabel>
#include "OverlayMessage.h"

namespace Ui { class MainWidget; }

// 前向声明各个子页面类
class PageMain;
class PageChart;
class PageHistory;
class PageTrail;
class PageSystem;
class PageAnalysis;
class PageUser;

/**
 * @brief The MainWidget class
 * 主界面，负责：
 *  - 实时更新时间显示
 *  - 根据 SessionManager 中的用户信息在页面上展示用户名/权限
 *  - 管理 QStackedWidget 内各个子页面的切换
 */
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

    void retranslateUi();  // 添加刷新方法

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void updateTime();          // 刷新顶部时间
    void switchToPageMain();    // 切换到主页
    void switchToPageChart();   // 切换到图表页
    void switchToPageHis();     // 切换到历史页
    void switchToPageTrail();   // 切换到审计追踪页
    void switchToPageSys();     // 切换到系统设置页
    void switchToPageAna();     // 切换到分析页
    void switchToPageUser();     // 切换到分析页
    void onUserChanged(const QString &userName, const QString &userLevel);

    void onAlarmMuteChanged(bool muted);
public slots:
    void closeSystem();

    void applyLockTime();
private:
    Ui::MainWidget *ui;
    QTimer         *timer;

    bool m_firstShow;          // 用于首次 showEvent 切到主页

    PageMain   *pageMain;
    PageChart  *pageChart;
    PageHistory *pageHistory;
    PageTrail  *pageTrail;
    PageSystem *pageSystem;
    PageAnalysis *pageAnalysis;
    PageUser *pageUser;
    OverlayMessage *m_overlayMsg = nullptr;  // 使用自定义名称

    void updateUIByPermission();
    void setupImage();  // 设置图片

signals:
    void logoutRequested();

};

#endif // MAINWIDGET_H
