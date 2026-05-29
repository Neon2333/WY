#include "mainwindow.h"
#include <QPushButton>
#include "Common/utils.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(800,600);
    btn_difusionFlow = new QPushButton(this);
    qout << "123\n";
}

MainWindow::~MainWindow()
{
}
