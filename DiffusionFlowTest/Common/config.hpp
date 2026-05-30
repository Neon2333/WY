#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include <memory>


class Config
{
public:
    static Config& Instance()
    {
        static Config instance;
        return instance;
    }
    
    // 修改配置文件路径，将原有配置加载到新的路径
    void ModifyConfigPath(const QString& path)
    {
        configPath_ = path;
        setting_ = std::make_unique<QSettings>(configPath_, QSettings::IniFormat);

        isSaveRawFrame_ = setting_->value("Global/isSaveRawFrame").toString();
        isSaveCmdFrame_ = setting_->value("Global/isSaveCmdFrame").toString();
    }    

    //修改日志文件路径
    void ModifyLogPath(const QString& path)
    {
        logPath_ = path;
    }

    // 配置是否保存原始帧
    void SetSaveRawFrame(bool isSave)
    {
        isSaveRawFrame_ = isSave ? "true" : "false";
        setting_->beginGroup("Global");
        setting_->setValue("Global/isSaveRawFrame", isSaveRawFrame_);
        setting_->endGroup();
    }
    bool IsSaveRawFrame() const
    {
        return isSaveRawFrame_ == "true";
    }

    // 配置是否保存命令帧
    void SetSaveCmdFrame(bool isSave)
    {
        isSaveCmdFrame_ = isSave ? "true" : "false";
        setting_->beginGroup("Global");
        setting_->setValue("Global/isSaveCmdFrame", isSaveCmdFrame_);
        setting_->endGroup();
    }
    bool IsSaveCmdFrame() const
    {
        return isSaveCmdFrame_ == "true";
    }

public:
    QString rootDir_ = QCoreApplication::applicationDirPath() + "/";
    QString configPath_ = rootDir_ + "config.ini";
    QString logPath_ = rootDir_ + "Log/" + "log.txt";

private:
    Config()
    {
        #if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
            setting_->setIniCodec(QTextCodec::codecForName("UTF-8"));
        #endif

        setting_ = std::make_unique<QSettings>(configPath_, QSettings::IniFormat);

        SetSaveRawFrame(true);  //创建文件
        SetSaveCmdFrame(true);
    }

    std::unique_ptr<QSettings> setting_;
    QString isSaveRawFrame_{"false"};
    QString isSaveCmdFrame_{"false"};
};


#endif // CONFIG_HPP
