#ifndef REMOTEUSERMANAGER_H
#define REMOTEUSERMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>

class RemoteUserManager : public QObject
{
    Q_OBJECT
public:
    static RemoteUserManager* instance();

    QString login(const QString &user, const QString &pwd);
    bool validateToken(const QString &token);
    QString userFromToken(const QString &token);
    void logout(const QString &token);

private:
    explicit RemoteUserManager(QObject *parent = nullptr);
    QString generateToken() const;

private:
    struct SessionInfo {
        QString username;
        QDateTime expireTime;
    };

    QMap<QString, SessionInfo> m_sessions;
    const int TOKEN_EXPIRE_SEC = 3600;
};

#endif // REMOTEUSERMANAGER_H
