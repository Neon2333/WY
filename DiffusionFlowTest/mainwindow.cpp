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


    qout << Config::Instance().IsSaveRawFrame();
    qout << Config::Instance().IsSaveCmdFrame();
}

MainWindow::~MainWindow()
{
}
