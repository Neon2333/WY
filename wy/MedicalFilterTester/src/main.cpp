#include <QApplication>
#include "LoginWindow.h"
#include "MainWindow.h"
#include "DatabaseManager.h"
#include "MessageDialog.h"
#include "frminput.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(QString::fromUtf8("\u533B\u7597\u6DA1\u5FC3\u6C14\u5BC6\u6027\u6D4B\u8BD5\u7CFB\u7EDF"));
    app.setApplicationVersion("1.0.0");

    // 初始化虚拟键盘
    frmInput::Instance()->Init("bottom", "blue");

    if (!DatabaseManager::instance().initialize()) {
        MessageDialog::showMessage(nullptr, QString::fromUtf8("\u9519\u8BEF"), QString::fromUtf8("\u6570\u636E\u5E93\u521D\u59CB\u5316\u5931\u8D25\uFF0C\u7A0D\u540E\u91CD\u8BD5"), MessageDialog::Error);
        return -1;
    }

    LoginWindow loginWindow;
    loginWindow.show();

    MainWindow *mainWindow = nullptr;

    QObject::connect(&loginWindow, &LoginWindow::loginSuccess, [&](const QString &username) {
        mainWindow = new MainWindow(username);
        mainWindow->showMaximized();

        QObject::connect(mainWindow, &MainWindow::logout, [&]() {
            delete mainWindow;
            mainWindow = nullptr;
            loginWindow.show();
        });

        QObject::connect(mainWindow, &MainWindow::exportRequested, [&]() {
            if (mainWindow) {
                MessageDialog::showMessage(mainWindow, QString::fromUtf8("\u5BFC\u51FA\u62A5\u8868"), QString::fromUtf8("\u5BFC\u51FAExcel\u62A5\u8868\u529F\u80FD\u5F85\u5B9E\u73B0"), MessageDialog::Information);
            }
        });

        loginWindow.hide();
    });

    return app.exec();
}