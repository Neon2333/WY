QT += core gui serialport charts printsupport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TRANSLATIONS = Resources/i18n/pressureMonitor_zh_CN.ts Resources/i18n/pressureMonitor_en.ts

RC_FILE = Resources/appIcon.rc

SOURCES += \
    AuditLogger.cpp \
    ConfigSettings.cpp \
    ConnectionHandler.cpp \
    DatabaseManager.cpp \
    DateTime.cpp \
    DiffAlarmManager.cpp \
    DiffBarWidget.cpp \
    IdleWatcher.cpp \
    LanguageManager.cpp \
    LoginWidget.cpp \
    MainWidget.cpp \
    MainWindow.cpp \
    MathUtils.cpp \
    NetworkServer.cpp \
    OverlayChangePasswordWidget.cpp \
    OverlayLockWidget.cpp \
    OverlayMessage.cpp \
    PageAnalysis.cpp \
    PageChart.cpp \
    PageHistory.cpp \
    PageMain.cpp \
    PageSystem.cpp \
    PageTrail.cpp \
    PageUser.cpp \
    QRoundProgressBar.cpp \
    RemoteUserManager.cpp \
    SensorDataProvider.cpp \
    SerialReader.cpp \
    SessionManager.cpp \
    TimeRangeSliderBridge.cpp \
    ToPdf.cpp \
    TransformData.cpp \
    # Keyboard/frminput.cpp \
    main.cpp \
    sqlite/sqlite3.c\
    BackupManager.cpp \
    Keyboard/KeyButton.cpp \
    Keyboard/Keyboard.cpp \
    Keyboard/InputManager.cpp \
    Keyboard/NumberKeyboard.cpp \
    # touchchartview.cpp


HEADERS += \
    AuditLogger.h \
    ConfigSettings.h \
    ConnectionHandler.h \
    DatabaseManager.h \
    DateTime.h \
    DiffAlarmManager.h \
    DiffBarWidget.h \
    GlobalDefines.h \
    HistoryData.h \
    IdleWatcher.h \
    LanguageManager.h \
    LoginWidget.h \
    MainWidget.h \
    MainWindow.h \
    MathUtils.h \
    NetworkServer.h \
    OverlayChangePasswordWidget.h \
    OverlayLockWidget.h \
    OverlayMessage.h \
    PageAnalysis.h \
    PageChart.h \
    PageHistory.h \
    PageMain.h \
    PageSystem.h \
    PageTrail.h \
    PageUser.h \
    PermissionManager.h \
    QRoundProgressBar.h \
    RemoteUserManager.h \
    SensorData.h \
    SensorDataProvider.h \
    SensorPacket.h \
    SerialReader.h \
    SessionManager.h \
    TimeRangeSliderBridge.h \
    ToPdf.h \
    TrailData.h \
    TransformData.h \
    # Keyboard/frminput.h \
    messagebox.h \
    sqlite/sqlite3.h\
    BackupManager.h \
    Keyboard/AbstractKeyboard.h \
    Keyboard/KeyButton.h \
    Keyboard/Keyboard.h \
    Keyboard/InputManager.h \
    Keyboard/NumberKeyboard.h \
    # touchchartview.h

FORMS += \
    Keyboard.ui \
    LoginWidget.ui \
    MainWidget.ui \
    MainWindow.ui \
    PageAnalysis.ui \
    PageChart.ui \
    PageHistory.ui \
    PageMain.ui \
    PageSystem.ui \
    PageTrail.ui \
    PageUser.ui \
    frminput.ui

RESOURCES += \
    Resources/icon.qrc \
    Resources/translations.qrc \
    Resources/Image.qrc


# 可启用下列宏
# 启用汉字库
# ENABLED_CHINESE_LIB

# 启用词组汉字库
# ENABLED_CHINESE_PHRASE_LIB

# 启用谷歌汉字库(推荐使用)
# ENABLED_GOOGLE_CHINESE_LIB

# 启用文泉驿字体库
# ENABLED_WQY_FONT

DEFINES += ENABLED_GOOGLE_CHINESE_LIB

contains(DEFINES, ENABLED_CHINESE_LIB) {
    RESOURCES += Resources/ChineseLib.qrc
}

contains(DEFINES, ENABLED_CHINESE_PHRASE_LIB) {
    RESOURCES += Resources/ChinesePhraseLib.qrc
}

contains(DEFINES, ENABLED_GOOGLE_CHINESE_LIB) {
    RESOURCES += Resources/GoogleChineseLib.qrc
}

contains(DEFINES, ENABLED_WQY_FONT) {
    RESOURCES += Resources/Font.qrc
}
