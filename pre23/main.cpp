#include "MainWindow.h"
#include <QApplication>
#include <QScreen>

#include "LanguageManager.h"
#include "NetworkServer.h"
#include <QSettings>
#include "GlobalDefines.h"

int main(int argc, char *argv[])
{

    QApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents);
    QApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);

    QApplication a(argc, argv);

    // 加载上次使用的语言
    LanguageManager::instance().loadLanguage(LanguageManager::instance().currentLanguage());

    QSettings settings("PageMain.ini", QSettings::IniFormat);
    quint16 port = settings.value("Port", 6020).toUInt();

    NetworkServer server;
    server.startServer(port);   // 启动监听端口 6000

    MainWindow w;
    w.show();
    return a.exec();
}


// #ifdef Q_OS_WIN
// #include <windows.h>
// #endif

// #ifdef Q_OS_WIN
// // 隐藏任务栏
// void hideTaskbar() {
//     HWND taskBar = FindWindow(L"Shell_TrayWnd", NULL);
//     if (taskBar) {
//         ShowWindow(taskBar, SW_HIDE);
//     }
// }

// // 显示任务栏
// void showTaskbar() {
//     HWND taskBar = FindWindow(L"Shell_TrayWnd", NULL);
//     if (taskBar) {
//         ShowWindow(taskBar, SW_SHOW);
//     }
// }
// #endif

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);

//     // 加载上次使用的语言
//     LanguageManager::instance().loadLanguage(LanguageManager::instance().currentLanguage());

//     // 读取端口配置
//     QSettings settings(systemConfig, QSettings::IniFormat);
//     quint16 port = settings.value("Port", 6020).toUInt();

//     // 启动服务器
//     NetworkServer server;
//     server.startServer(port);

//     // 创建主窗口
//     MainWindow w;

//     // 保留普通窗口风格，确保下拉框正常渲染
//     w.setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);

//     // 最大化窗口到屏幕大小（不覆盖任务栏 z-order）
//     w.showMaximized();
//     // w.resize(1280,850);

// #ifdef Q_OS_WIN
//     // 隐藏任务栏，让窗口看起来像全屏
//     hideTaskbar();
// #endif

//     // 当应用退出时恢复任务栏
//     QObject::connect(&a, &QApplication::aboutToQuit, [](){
// #ifdef Q_OS_WIN
//         showTaskbar();
// #endif
//     });

//     return a.exec();
// }

/*
 * 模块化设计：这些文件采用分层架构。
 * NetworkServer 负责网络监听，但端口/IP 是运行时配置的（可能在主程序如 main.cpp 中调用 startServer(端口号) 时指定）。
 * ConnectionHandler 只处理连接后的逻辑，
 * RemoteUserManager 只管用户数据。这种分离使得代码更易复用和测试。
 * 避免硬编码：硬编码 IP/端口（如直接写 listen("192.168.1.1", 8080)）会限制部署。
 * 例如，在本地开发用 127.0.0.1:8080，在生产环境可能需要公网 IP 和不同端口。
 * 使用动态参数或默认值（如 AnyIPv4）更灵活，通常从外部配置文件（如 JSON/INI 文件）、环境变量或用户输入中加载。
 */
