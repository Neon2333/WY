#include "LanguageManager.h"
#include <QApplication>
#include <QDebug>

LanguageManager& LanguageManager::instance()
{
    static LanguageManager lm;
    return lm;
}

LanguageManager::LanguageManager()
    : settings("MyCompany", "PressureMonitor")
{
}

QString LanguageManager::currentLanguage()
{
    return settings.value("language", "default").toString();
}

void LanguageManager::loadLanguage(const QString &lang)
{
    qApp->removeTranslator(&translator);

    // 如果切回默认中文，则不加载任何 qm
    if (lang == "default")
    {
        settings.setValue("language", "default");
        emit languageChanged();
        return;
    }

    // 加载英文翻译
    QString qmPath = QString(":/i18n/pressureMonitor_%1.qm").arg(lang);

    if (translator.load(qmPath))
    {
        qApp->installTranslator(&translator);
        settings.setValue("language", lang);
        emit languageChanged();
    }
    else
    {
        qDebug() << "Failed to load:" << qmPath;
    }
}
