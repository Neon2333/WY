#include "mainwindow.h"
#include "QPushButton.h"
#include "QDebug.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(800,600);
    btn_difusionFlow = new QPushButton(this);
    qDebug()<<"123\n";
}

MainWindow::~MainWindow()
{
}
