#include "mainwindow.h"
#include <QPushButton>

#include <iostream>
#include "Common/utils.hpp"
#include "Common/config.hpp"
#include "DB/sqliteConnection.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(800,600);
    btn_difusionFlow = new QPushButton(this);


    qout << Config::Instance().IsSaveRawFrame();
    qout << Config::Instance().IsSaveCmdFrame();



    auto db = SqliteConnection();
    auto rs1 = db.Execute("CREATE TABLE IF NOT EXISTS Users (ID INTEGER PRIMARY KEY, Name TEXT, Age INTEGER)");
    auto rs2 = db.Execute("INSERT INTO Users (Name, Age) VALUES ('Alice', 25), ('Bob', 30)");
    auto rs = db.QueryDb("SELECT ID, Name, Age FROM Users WHERE Age > ?", 18);
    if (!rs)
    {
        qout << "query failed";
        return;
    } 
    while (rs->Next()) 
    {
        int id = rs->GetInt("ID");
        QString name = rs->GetString("Name");
        int age = rs->GetInt("Age");

        qout << "User: " << id << ", " << name << ", " << age;
    }
}

MainWindow::~MainWindow()
{
}
