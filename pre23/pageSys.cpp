#include "syssetting.h"
#include "build\Qt_6_5_3_mingw_64-Debug\ui_pageSys.h"

SysSetting::SysSetting(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::SysSetting)
{
    ui->setupUi(this);
    //下方跳转按钮
    // connect(ui->mainBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("main"); });
    // connect(ui->chartBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("chart"); });
    // connect(ui->hisBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("his"); });
    // connect(ui->trailBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("trail"); });
    // connect(ui->alarmBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("alarm"); });
    // connect(ui->sysBtn, &QPushButton::clicked, this, [=]() { emit requestSwitchTo("sys"); });
}

SysSetting::~SysSetting()
{
    delete ui;
}
