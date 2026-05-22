#pragma once
#include <QObject>
#include <QTranslator>
#include <QSettings>

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    static LanguageManager& instance();

    QString currentLanguage();
    void loadLanguage(const QString& lang);

signals:
    void languageChanged();  // 通知所有窗口刷新语言

private:
    LanguageManager();
    QTranslator translator;
    QSettings settings;
};
