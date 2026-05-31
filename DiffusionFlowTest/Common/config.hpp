#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include <memory>
#include "utils.hpp"


class Config
{
public:
    static Config& Instance()
    {
        static Config instance;
        return instance;
    }
    

    //获取软件根目录
    QString GetRootDir() const { return rootDir_; }


    //配置文件路径
    QString GetConfigPath() const { return configPath_; } 
    void SetConfigPath(const QString& parentPath, const QString& fileName = "config.ini")
    {
        if(parentPath.isEmpty())
            return;
        if(IsDirExists(parentPath)) //父目录不存在则创建
            qout << parentPath << "exists..";
        else
            qout << parentPath << "not exists.., mkdir it..";

        configPath_ = parentPath + "/" + fileName;
        setting_ = std::make_unique<QSettings>(configPath_, QSettings::IniFormat);  //从新路径加载配置

        //从新配置文件读取参数
        configPath_ = setting_->value("Global/configPath", configPath_).toString();
        logPath_ = setting_->value("Global/logPath", logPath_).toString();
        dbPath_ = setting_->value("Global/dbPath", dbPath_).toString();
        isSaveRawFrame_ = setting_->value("Global/isSaveRawFrame", isSaveRawFrame_).toString();      
        isSaveCmdFrame_ = setting_->value("Global/isSaveCmdFrame", isSaveCmdFrame_).toString();
    }    


    //日志文件路径
    QString GetLogPath() const { return logPath_; }
    void SetLogPath(const QString& parentPath, const QString& fileName = "log.txt")
    {
        if(parentPath.isEmpty())
            return;
        if(IsDirExists(parentPath))
            qout << parentPath << "exists..";
        else
            qout << parentPath << "not exists.., mkdir it..";

        logPath_ = parentPath + "/" + fileName;
        SetValue("Global", "logPath", logPath_);
    }


    //数据库路径
    QString GetDbPath() const { return dbPath_; }
    void SetDbPath(const QString& parentPath, const QString& fileName = "wy.db")
    {
        if(parentPath.isEmpty())
            return;
        if(IsDirExists(parentPath))
            qout << parentPath << "exists..";
        else
            qout << parentPath << "not exists.., mkdir it..";

        dbPath_ = parentPath + "/" + fileName;
        SetValue("Global", "dbPath", dbPath_);
    }


    // 配置是否保存原始帧
    void SetSaveRawFrame(bool isSave)
    {
        isSaveRawFrame_ = isSave ? "true" : "false";
        SetValue("Global", "isSaveRawFrame", isSaveRawFrame_);
    }
    bool IsSaveRawFrame() const
    {
        return isSaveRawFrame_ == "true";
    }

    // 配置是否保存命令帧
    void SetSaveCmdFrame(bool isSave)
    {
        isSaveCmdFrame_ = isSave ? "true" : "false";
        SetValue("Global", "isSaveCmdFrame", isSaveCmdFrame_);
    }
    bool IsSaveCmdFrame() const
    {
        return isSaveCmdFrame_ == "true";
    }

private:
    Config()
    {
        #if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
            setting_->setIniCodec(QTextCodec::codecForName("UTF-8"));
        #endif

        setting_ = std::make_unique<QSettings>(configPath_, QSettings::IniFormat);

        SetConfigPath(GetRootDir());
        SetLogPath(GetRootDir() + "Log");
        SetDbPath(GetRootDir() + "DB");
        SetSaveRawFrame(true); 
        SetSaveCmdFrame(true);
    }

    void SetValue(const QString& group, const QString& key, const QString& value)
    {
        setting_->beginGroup(group);    //创建文件
        setting_->setValue(group + "/" + key, value);
        setting_->endGroup();
    }

    std::unique_ptr<QSettings> setting_;

    //路径
    QString rootDir_ = QCoreApplication::applicationDirPath() + "/";
    QString configPath_ = rootDir_ + "config.ini";
    QString logPath_ = rootDir_ + "Log/log.txt";
    QString dbPath_ = rootDir_ + "DB/wy.db";
    //配置
    QString isSaveRawFrame_{"false"};
    QString isSaveCmdFrame_{"false"};
};


#endif // CONFIG_HPP
