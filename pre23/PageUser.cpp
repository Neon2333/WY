#include "PageUser.h"
#include "ui_PageUser.h"
#include "DatabaseManager.h"
#include "AuditLogger.h"
#include "OverlayChangePasswordWidget.h"
#include "OverlayMessage.h"
#include "LanguageManager.h"
#include  "PermissionManager.h"
#include "SessionManager.h"

#include <QDebug>
#include <QInputDialog>
#include <QSettings>
#include <QScrollBar>
#include <QDateTime>
#include <QString>

#include <QDialog>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>

PageUser::PageUser(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageUser)
{
    ui->setupUi(this);
    // —— 用单例打开用户数据库 ——
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::UserDatabase)) {
        qDebug() << "数据库错误,无法打开用户数据库" << this;
    }

    refreshUserCombos();// 更新删除、修改、切换下拉框

    // 连接用户变化信号
    connect(&SessionManager::instance(), &SessionManager::userChanged,this, &PageUser::onUserChanged);
    // 根据权限统一设置UI状态
    updateUIByPermission();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,this, &PageUser::retranslateUi);
}

PageUser::~PageUser()
{
    delete ui;
}

void PageUser::retranslateUi()
{
    ui->retranslateUi(this);
}

void PageUser::init()
{
    // 1. 打开轨迹数据库
    if (!DatabaseManager::instance().openDatabase(DatabaseManager::UserDatabase)) {
        qCritical() << "[用户设置] 打开用户数据库失败";
        return;
    }
    loadUserRows();
    setupTable();
    QString username = SessionManager::instance().userName();
    ui->userCoBoxKeyWord->setCurrentText(username);
}

void PageUser::setupTable()
{
    QStringList headers = {tr("用户名"), tr("权限等级"), tr("创建天数"), tr("是否锁定")};
    ui->userTable->setColumnCount(headers.size());
    ui->userTable->setHorizontalHeaderLabels(headers);
    ui->userTable->setColumnWidth(0, 120);
    ui->userTable->setColumnWidth(1, 140);
    ui->userTable->setColumnWidth(2, 150);
    ui->userTable->horizontalHeader()->setStretchLastSection(true);
    ui->userTable->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width: 30px; }");
    ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void PageUser::loadUserRows()
{
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::UserDatabase);
    if (!db) {
        qCritical() << "[用户设置] 获取数据库连接失败";
        return;
    }

    const char* sql =
        "SELECT username, level, usertime, islocked "
        "FROM userinfo ORDER BY datetime(usertime);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qCritical() << "[用户设置] SQL 准备失败：" << sqlite3_errmsg(db);
        return;
    }

    ui->userTable->setRowCount(0);
    int row = 0;

    QDateTime now = QDateTime::currentDateTime();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ui->userTable->insertRow(row);

        // ===== 用户名 =====
        QString username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        auto* itemUser = new QTableWidgetItem(username);
        itemUser->setTextAlignment(Qt::AlignCenter);
        ui->userTable->setItem(row, 0, itemUser);

        // ===== 权限等级 =====
        QString level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        auto* itemLevel = new QTableWidgetItem(level);
        itemLevel->setTextAlignment(Qt::AlignCenter);
        ui->userTable->setItem(row, 1, itemLevel);

        // ===== 创建天数 =====
        const char* timeStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        QDateTime createTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");

        int days = createTime.isValid()
                       ? createTime.daysTo(now)
                       : 0;

        auto* itemDays = new QTableWidgetItem(QString::number(days));
        itemDays->setTextAlignment(Qt::AlignCenter);
        ui->userTable->setItem(row, 2, itemDays);

        // ===== 是否锁定 =====
        const char* lockVal =reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        QString lockText = (lockVal && lockVal[0] == 'Y')
                               ? tr("是")
                               : tr("否");

        auto* itemLock = new QTableWidgetItem(lockText);
        itemLock->setTextAlignment(Qt::AlignCenter);
        ui->userTable->setItem(row, 3, itemLock);

        row++;
    }

    sqlite3_finalize(stmt);
    qDebug() << "[用户设置] 加载了" << row << "条记录";
}

//下拉列表更新
void PageUser::refreshUserCombos()
{
    // 清空所有相关下拉框
    ui->userCoBoxKeyWord->clear();
    ui->userCoBoxSwitch->clear();
    ui->userCoBoxDel->clear();

    // 从 DatabaseManager 拿到用户库句柄
    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::UserDatabase);
    if (!db) return;

    const char *sql = "SELECT username FROM userinfo ORDER BY username;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    // 遍历所有用户，将用户名加入下拉框
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 0));
        QString uname = QString::fromUtf8(name);
        ui->userCoBoxKeyWord->addItem(uname);
        ui->userCoBoxSwitch->addItem(uname);
        ui->userCoBoxDel->addItem(uname);
    }

    sqlite3_finalize(stmt);

    // 设置“删除用户”下拉框默认选中当前登录用户
    const QString currentUser = SessionManager::instance().userName();
    int index = ui->userCoBoxKeyWord->findText(currentUser);
    if (index >= 0) {
        ui->userCoBoxKeyWord->setCurrentIndex(index);
    }
}

//添加用户
void PageUser::on_userAddBtn_clicked()
{
    // ===== 0. 当前登录用户 & 权限 =====
    QString currentUser = SessionManager::instance().userName();

    auto currentLevel =
        DatabaseManager::instance().getUserPermissionLevel(currentUser);

    // 普通用户禁止添加用户
    if (currentLevel < PermissionManager::Admin) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("权限不足"), tr("您无权添加用户"));
        return;
    }

    // ===== 1. 读取输入 =====
    QString username = ui->userAddUser->text().trimmed();
    QString password = ui->userAddKeyword->text().trimmed();
    QString confirmPwd = ui->userAddKeyword1->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || confirmPwd.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("用户名或密码为空！"));
        return;
    }

    if (password != confirmPwd) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("两次输入的密码不一致"));
        return;
    }

    // ===== 2. 密码复杂度 =====
    if (!DatabaseManager::instance().isValidPasswordComplexity(password)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(
            tr("密码不符合要求"),
            tr("密码长度需大于8位，并至少包含以下三种类型：\n"
               "大写字母、小写字母、数字、特殊字符")
            );
        return;
    }

    sqlite3* db =
        DatabaseManager::instance().getDatabaseHandle(DatabaseManager::UserDatabase);
    sqlite3_stmt* stmt = nullptr;

    // ===== 3. 检查用户名是否已存在 =====
    const char* checkSql =
        "SELECT COUNT(*) FROM userinfo WHERE username = ?;";

    sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1,
                      username.toUtf8().constData(),
                      -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW &&
        sqlite3_column_int(stmt, 0) > 0) {

        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("添加失败"), tr("用户名已存在！"));
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // ===== 4. 根据当前权限，生成“可选权限列表” =====
    QStringList levelOptions;

    if (currentLevel == PermissionManager::Admin) {
        levelOptions << "Standard" << "QA" << "Supervisor";
    } else if (currentLevel == PermissionManager::SuperAdmin) {
        levelOptions << "Standard" << "QA"
                     << "Supervisor" << "Admin" << "SuperAdmin";
    }

    bool ok = false;
    QString level;

    {
        QDialog dialog(this);
        dialog.setWindowTitle(tr("选择权限等级"));
        dialog.resize(420, 300);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QLabel *label = new QLabel(tr("请选择用户权限等级:"), &dialog);
        label->setStyleSheet("font-size:18px;");
        layout->addWidget(label);

        QButtonGroup *group = new QButtonGroup(&dialog);

        for (int i = 0; i < levelOptions.size(); ++i) {
            QRadioButton *radio = new QRadioButton(levelOptions[i], &dialog);
            radio->setStyleSheet("font-size:18px;");
            radio->setMinimumHeight(40);

            group->addButton(radio, i);
            layout->addWidget(radio);

            if (i == 0) {
                radio->setChecked(true);
            }
        }

        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                 Qt::Horizontal,
                                 &dialog);

        QPushButton *okBtn = buttonBox->button(QDialogButtonBox::Ok);
        QPushButton *cancelBtn = buttonBox->button(QDialogButtonBox::Cancel);

        okBtn->setFixedSize(120, 45);
        cancelBtn->setFixedSize(120, 45);

        okBtn->setStyleSheet("font-size:16px;");
        cancelBtn->setStyleSheet("font-size:16px;");

        layout->addWidget(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                         &dialog, &QDialog::accept);

        QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                         &dialog, &QDialog::reject);

        ok = (dialog.exec() == QDialog::Accepted);

        if (ok) {
            int id = group->checkedId();
            if (id >= 0 && id < levelOptions.size()) {
                level = levelOptions[id];
            }
        }
    }

    if (!ok || level.isEmpty()) {
        return;
    }

    if (!ok || level.isEmpty()) {
        return;
    }

    // ===== 5. 兜底权限校验（防止 UI 被绕）=====
    auto targetLevel =
        static_cast<PermissionManager::PermissionLevel>(
            PermissionManager::levelFromString(level)
            );

    if ((targetLevel >= currentLevel) && (currentLevel != PermissionManager::SuperAdmin)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(
            tr("权限错误"),
            tr("不能创建权限不低于自己的用户")
            );
        return;
    }

    // ===== 6. 插入数据库 =====
    QString usertime =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    const char* insertSql =
        "INSERT INTO userinfo "
        "(username,password,level,usertime,islocked,failed_attempts,firstlogin) "
        "VALUES(?,?,?,?,?,?,?);";

    sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, level.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, usertime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, "N", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, 0);
    sqlite3_bind_text(stmt, 7, "Y", -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("添加失败"), sqlite3_errmsg(db));
    } else {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("成功"), tr("用户添加成功！"));

        ui->userAddUser->clear();
        ui->userAddKeyword->clear();
        ui->userAddKeyword1->clear();

        refreshUserCombos();

        AuditLogger::instance().log(
            tr("用户 %1 添加用户 %2(%3)")
                .arg(currentUser)
                .arg(username)
                .arg(level)
            );
    }
    sqlite3_finalize(stmt);
}

//修改密码
void PageUser::on_userKeyWordBtn_clicked()
{
    // 1. 读取并 trim 用户输入
    QString username = ui->userCoBoxKeyWord->currentText();
    QString password   = ui->userChangeKeyword->text().trimmed();
    QString newPwd = ui->userChangeKeyword1->text().trimmed();
    QString confirmPwd = ui->userChangeKeyword2->text().trimmed();

    sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::UserDatabase);
    sqlite3_stmt *stmt = nullptr;

    // 2. 输入非空校验
    if (username.isEmpty() || password.isEmpty() || newPwd.isEmpty() || confirmPwd.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("输入错误"), tr("请选择用户并输入新密码！"));
        return;
    }


    // 3. 调用 DatabaseManager 验证用户名/密码
    if (!DatabaseManager::instance().validateUser(username, password)) {
        // 验证失败，增加失败计数
        int failedAttempts = 0;
        if (!DatabaseManager::instance().incrementFailedAttempts(username, failedAttempts)) {
            qDebug() << "无法更新失败尝试次数";
        }
        QString errorMessage = "用户名或密码错误";
        if (failedAttempts <= 5) {
            int maxAttempts = 5;
            int remainingAttempts = qMax(0, maxAttempts - failedAttempts);// 假设允许5次失败，第6次失败时锁定，但根据>5调整
            errorMessage += QString("，剩余 %1 次尝试机会").arg(remainingAttempts);
        }
        auto *msg = new OverlayMessage(this);
        msg->showMessage("登录失败", errorMessage);
        // 检查是否超过5次
        if (failedAttempts > 5) {
            if (!DatabaseManager::instance().lockAccount(username)) {
                qDebug() << "无法锁定账户";
            }
            auto *lockMsg = new OverlayMessage(this);
            lockMsg->showMessage("账户锁定", "登录失败超过5次，该账户已被锁定");

            AuditLogger::instance().log(QString("用户：%1 登录失败超过5次，已被锁定").arg(username));
        }
        return;
    }

    // 4.密码复杂度校验
    if (!DatabaseManager::instance().isValidPasswordComplexity(newPwd)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(
            tr("密码不符合要求"),
            tr("密码长度需大于8位，并至少包含以下三种类型：\n"
               "大写字母、小写字母、数字、特殊字符")
            );
        return;
    }

    //5.一致检验
    if (newPwd != confirmPwd) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("错误"), tr("两次输入的密码不一致"));
        return;
    }

    //6.权限边界校验
    QString currentUser = SessionManager::instance().userName();

    auto currentLevel =DatabaseManager::instance().getUserPermissionLevel(currentUser);

    auto targetLevel =DatabaseManager::instance().getUserPermissionLevel(username);

    // 不能修改权限 >= 自己 的用户
    if (currentLevel <= targetLevel && currentUser != username) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("权限不足"),tr("您无权修改该用户的密码"));

        AuditLogger::instance().log(
            QString("用户 %1 尝试修改 %2 的密码，被拒绝")
                .arg(currentUser,username)
            );
        return;
    }

    // 7.执行更新
    QString usertime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    const char *sql = "UPDATE userinfo SET password = ?, usertime = ? WHERE username = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, newPwd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, usertime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("修改失败"), QString::fromUtf8(sqlite3_errmsg(db)));
    } else {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("成功"), tr("密码已修改！"));
        ui->userChangeKeyword->clear();

        AuditLogger::instance().log(QString(tr("修改用户：%1 的密码")).arg(username));
    }
    sqlite3_finalize(stmt);
}

//切换用户
void PageUser::on_userSwitchBtn_clicked()
{
    // 1.2.获取要切换的用户名和密码
    QString username = ui->userCoBoxSwitch->currentText();
    QString pwd = ui->userSwitchKeyword->text().trimmed();
    if (username.isEmpty() || pwd.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage("提示", "用户名或密码不能为空");
        return;
    }

    // 4.检查账户是否锁定
    QString lockStatus;
    if (!DatabaseManager::instance().getLockStatus(username, lockStatus)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("切换失败"), tr("非法用户，请切换用户登录"));
        return;
    }
    if (lockStatus == "Y") {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("切换失败"), tr("该账户已锁定"));
        return;
    }
    // 5.调用 DatabaseManager 验证用户名/密码
    if (!DatabaseManager::instance().validateUser(username, pwd)) {
        // 验证失败，增加失败计数
        int failedAttempts = 0;
        if (!DatabaseManager::instance().incrementFailedAttempts(username, failedAttempts)) {
            qDebug() << "无法更新失败尝试次数";
        }
        QString errorMessage = tr("用户名或密码错误");
        if (failedAttempts <= 5) {
            int maxAttempts = 5;
            int remainingAttempts = qMax(0, maxAttempts - failedAttempts);// 假设允许5次失败，第6次失败时锁定，但根据>5调整
            errorMessage += QString(tr("，剩余 %1 次尝试机会")).arg(remainingAttempts);
        }
        auto *msg = new OverlayMessage(this);
        msg->showMessage("切换失败", errorMessage);
        // 检查是否超过5次
        if (failedAttempts > 5) {
            if (!DatabaseManager::instance().lockAccount(username)) {
                qDebug() << "无法锁定账户";
            }
            auto *lockMsg = new OverlayMessage(this);
            lockMsg->showMessage(tr("账户锁定"), tr("登录失败超过5次，该账户已被锁定"));
            AuditLogger::instance().log(QString(tr("用户：%1 登录失败超过5次，已被锁定")).arg(username));
        }
        return;
    }
    // 6.验证通过，重置失败尝试次数
    if (!DatabaseManager::instance().resetFailedAttempts(username)) {
        qDebug() << "无法重置失败尝试次数";
    }
    // 7.检查账户是否初次登录（原始密码验证通过后）
    QString loginStatus;
    if (!DatabaseManager::instance().firstLogin(username, loginStatus)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("切换失败"), tr("非法用户，请切换用户登录"));
        return;
    }
    // 初次登录：强制设置密码
    if (loginStatus == "Y") {
        auto *dlg = new OverlayChangePasswordWidget(
            username,
            OverlayChangePasswordWidget::Reason::FirstLogin,
            this);
        connect(dlg, &OverlayChangePasswordWidget::passwordChanged,
                this, [=]() {
                    QString realUsername, userLevel;
                    DatabaseManager::instance().getUserInfo(username, realUsername, userLevel);
                    SessionManager::instance().setUser(realUsername, userLevel);
                    auto* successMsg = new OverlayMessage(this);
                    successMsg->showMessage(tr("切换成功"), QString(tr("已切换到用户 \"%1\"，权限：%2"))
                                                                .arg(realUsername,userLevel));
                    AuditLogger::instance().log(QString(tr("切换到用户 \"%1\"，权限：%2"))
                                                    .arg(realUsername,userLevel));
                });
        dlg->show();
        return;
    }
    // 8.检查密码是否过期
    QDateTime createTime;
    if (DatabaseManager::instance().getUserCreateTime(username, createTime)) {
        const int days = createTime.daysTo(QDateTime::currentDateTime());
        if (days > 90) {
            auto *dlg = new OverlayChangePasswordWidget(username, OverlayChangePasswordWidget::Reason::PasswordExpired, this);
            connect(dlg, &OverlayChangePasswordWidget::passwordChanged,
                    this, [=]() {
                        QString realUsername, userLevel;
                        DatabaseManager::instance().getUserInfo(username, realUsername, userLevel);
                        SessionManager::instance().setUser(realUsername, userLevel);
                        auto* successMsg = new OverlayMessage(this);
                        successMsg->showMessage(tr("切换成功"), QString(tr("已切换到用户 \"%1\"，权限：%2"))
                                                                    .arg(realUsername)
                                                                    .arg(userLevel));
                        AuditLogger::instance().log(QString(tr("切换到用户 \"%1\"，权限：%2"))
                                                        .arg(realUsername)
                                                        .arg(userLevel));
                    });
            dlg->show();
            return;
        }
    }
    // 9.读取权限等级
    QString realUsername, userLevel;
    if (!DatabaseManager::instance().getUserInfo(username, realUsername, userLevel)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("切换失败"), tr("获取用户信息失败"));
        return;
    }

    // 设置 Session 并显示成功消息
    SessionManager::instance().setUser(realUsername, userLevel);
    auto* msg = new OverlayMessage(this);
    msg->showMessage(tr("切换成功"), QString(tr("已切换到用户 %1，权限：%2"))
                                         .arg(realUsername,userLevel));

    refreshUserCombos();
    AuditLogger::instance().log(QString(tr("切换到用户 %1，权限：%2"))
                                    .arg(realUsername,userLevel));
}

//unLock用户
void PageUser::on_userUnLockBtn_clicked()
{
    QString username = ui->userCoBoxDel->currentText().trimmed();
    if (username.isEmpty()) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("请选择用户！"));
        return;
    }

    QString lockStatus;
    if (!DatabaseManager::instance().getLockStatus(username, lockStatus)) {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("解锁失败"), tr("无法获取锁定状态"));
        return;
    }
    if (lockStatus != "Y") {
        auto *msg = new OverlayMessage(this);
        msg->showMessage(tr("提示"), tr("该账户未锁定，无需解锁"));
        return;
    }

    auto *confirm = new OverlayMessage(this);
    confirm->setShowCancelButton(true);
    confirm->showMessage(tr("确认解锁"),tr("确定要解锁用户 [%1] 吗？").arg(username));

    connect(confirm, &OverlayMessage::closed, this, [=]() {

        sqlite3* db = DatabaseManager::instance().getDatabaseHandle(DatabaseManager::UserDatabase);
        sqlite3_stmt *stmt = nullptr;
        const char *sql =
            "UPDATE userinfo "
            "SET islocked = 'N', failed_attempts = 0 "
            "WHERE username = ? AND islocked = 'Y';";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("解锁失败"),QString::fromUtf8(sqlite3_errmsg(db)));
            return;
        }

        sqlite3_bind_text(stmt, 1,username.toUtf8().constData(),-1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("解锁失败"),
                             QString::fromUtf8(sqlite3_errmsg(db)));
        } else {
            auto *msg = new OverlayMessage(this);
            msg->showMessage(tr("提示"),
                             tr("用户 %1 已解锁").arg(username));

            AuditLogger::instance().log(
                tr("解锁用户：%1").arg(username)
                );
            refreshUserCombos();
        }

        sqlite3_finalize(stmt);
    });
}

// 改变用户重新应用权限
void PageUser::onUserChanged(const QString &userName, const QString &userLevel)
{
    Q_UNUSED(userName);
    Q_UNUSED(userLevel);

    // 直接更新UI权限状态
    updateUIByPermission();
}

//不同权限下的ui状态
void PageUser::updateUIByPermission()
{
    // Admin权限控制的UI元素
    QList<QWidget*> AdminWidgets = {
        ui->userCoBoxKeyWord,
        ui->userAddBtn,ui->userAddKeyword,ui->userAddKeyword1,ui->userAddUser,
    };

    // 设置Admin权限控件
    for (auto widget : AdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::Admin, widget);
    }

    // 超级管理员权限控制的UI元素
    QList<QWidget*> superAdminWidgets = {
        ui->userCoBoxDel/*,ui->userDelBtn*/,ui->userUnLockBtn
    };

    // 设置超级管理员权限控件
    for (auto widget : superAdminWidgets) {
        PermissionManager::setUIByPermission(PermissionManager::SuperAdmin, widget);
    }

}
