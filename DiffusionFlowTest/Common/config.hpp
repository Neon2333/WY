#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include <memory>


class Config
{
public:
    Config()
    {
        #if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
            setting_->setIniCodec(QTextCodec::codecForName("UTF-8"));
        #endif

        QVariant isSaveRawFrameTmp = setting_->value("Global/isSaveRawFrame");
        isSaveRawFrame = isSaveRawFrameTmp.toString();

        QVariant isSaveCmdFrameTmp = setting_->value("Global/isSaveCmdFrame");
        isSaveCmdFrame = isSaveCmdFrameTmp.toString();
    }
    ~Config() = default;

    // 修改配置文件路径
    void ModifyConfigPath(const QString& path)
    {
        setting_ = std::make_unique<QSettings>(path, QSettings::IniFormat);
    }    

    void ModifyLogPath(const QString& path)
    {
        logPath = rootDir + "/" + path;
    }

    // 配置是否保存原始帧
    void SetSaveRawFrame(bool isSave)
    {
        isSaveRawFrame = isSave ? "true" : "false";
        setting_->setValue("Global/isSaveRawFrame", isSaveRawFrame);
    }
    bool IsSaveRawFrame() const
    {
        return isSaveRawFrame == "true";
    }

    // 配置是否保存命令帧
    void SetSaveCmdFrame(bool isSave)
    {
        isSaveCmdFrame = isSave ? "true" : "false";
        setting_->setValue("Global/isSaveCmdFrame", isSaveCmdFrame);
    }
    bool IsSaveCmdFrame() const
    {
        return isSaveCmdFrame == "true";
    }

public:
    inline static QString rootDir = QCoreApplication::applicationDirPath() + "/";
    inline static QString configPath = rootDir + "config.ini";
    inline static QString logPath = rootDir + "Log/" + "log.txt";

    inline static QString isSaveRawFrame;
    inline static QString isSaveCmdFrame;

private:
    std::unique_ptr<QSettings> setting_ = std::make_unique<QSettings>(configPath, QSettings::IniFormat);
};

namespace config
{
    Config cfg;
};

#endif // CONFIG_HPP
