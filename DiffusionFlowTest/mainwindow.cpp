#include "mainwindow.h"
#include <QPushButton>
#include "Common/utils.hpp"

#include "DB/sqlite3/sqlite3.h"
#include <iostream>
#include "Common/utils.hpp"
#include "Common/config.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(800,600);
    btn_difusionFlow = new QPushButton(this);
    qout << "123";


    // sqlite3* db = nullptr;
    // char* errMsg = nullptr;
    // int rc;

    // // 1. 打开数据库文件（如果不存在会自动创建）
    // rc = sqlite3_open(R"(DB/wy.db)", &db);
    // if (rc) 
    // {
    //     std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
    //     sqlite3_close(db);
    //     assert(false);
    // }
    // qout << "数据库打开成功";     

    // const char* createSql = "CREATE TABLE IF NOT EXISTS Users ("
    //                       "ID INTEGER PRIMARY KEY, "
    //                       "Name TEXT NOT NULL, "
    //                       "Age INTEGER NOT NULL);";
    // rc = sqlite3_exec(db, createSql, 0, 0, &errMsg);
    // if (rc != SQLITE_OK) {
    //     qout << "建表失败: " << errMsg;
    //     sqlite3_free(errMsg);
    // }

    // sqlite3_stmt* stmt;
    // const char* sql = "INSERT INTO Users (ID, Name, Age) VALUES (?, ?, ?);";
    // sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    // sqlite3_bind_int(stmt, 1, 3);
    // sqlite3_bind_text(stmt, 2, "Charlie", -1, SQLITE_STATIC);
    // sqlite3_bind_int(stmt, 3, 28);
    // sqlite3_step(stmt);
    // sqlite3_finalize(stmt);

    qout << config::cfg.IsSaveCmdFrame();
    qdebug << config::cfg.IsSaveRawFrame();
    qerr << config::cfg.IsSaveRawFrame();
}

MainWindow::~MainWindow()
{
}
