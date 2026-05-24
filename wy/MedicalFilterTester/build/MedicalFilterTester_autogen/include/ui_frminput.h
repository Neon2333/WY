/********************************************************************************
** Form generated from reading UI file 'frminput.ui'
**
** Created by: Qt User Interface Compiler version 6.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FRMINPUT_H
#define UI_FRMINPUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_frmInput
{
public:
    QFrame *frameMain;
    QGridLayout *gridLayout;
    QPushButton *btns;
    QPushButton *btnOther7;
    QPushButton *btna;
    QPushButton *btnu;
    QPushButton *btnc;
    QPushButton *btnOther2;
    QPushButton *btnh;
    QPushButton *btnq;
    QPushButton *btn9;
    QPushButton *btnOther4;
    QPushButton *btn7;
    QPushButton *btng;
    QPushButton *btnDelete;
    QPushButton *btn5;
    QPushButton *btnf;
    QPushButton *btnz;
    QPushButton *btnOther13;
    QPushButton *btnOther14;
    QPushButton *btn3;
    QPushButton *btni;
    QPushButton *btn1;
    QPushButton *btnk;
    QPushButton *btnv;
    QPushButton *btnx;
    QPushButton *btnOther3;
    QPushButton *btnn;
    QPushButton *btnSpace;
    QPushButton *btnOther10;
    QPushButton *btn2;
    QPushButton *btny;
    QPushButton *btnOther16;
    QPushButton *btno;
    QPushButton *btnOther12;
    QPushButton *btnd;
    QPushButton *btnType;
    QPushButton *btnOther1;
    QPushButton *btnl;
    QPushButton *btnOther19;
    QPushButton *btnOther18;
    QPushButton *btne;
    QPushButton *btnOther8;
    QPushButton *btnOther11;
    QPushButton *btnw;
    QPushButton *btnm;
    QPushButton *btnOther17;
    QPushButton *btnp;
    QPushButton *btnOther15;
    QPushButton *btn8;
    QSpacerItem *spacer;
    QPushButton *btnb;
    QPushButton *btnj;
    QPushButton *btn4;
    QPushButton *btnOther9;
    QPushButton *btn0;
    QPushButton *btn6;
    QPushButton *btnt;
    QPushButton *btnDot;
    QPushButton *btnr;
    QPushButton *btnOther20;
    QPushButton *btnOther5;
    QPushButton *btnOther21;
    QPushButton *btnClose;
    QLabel *lab_Title;
    QLabel *labPY;

    void setupUi(QWidget *frmInput)
    {
        if (frmInput->objectName().isEmpty())
            frmInput->setObjectName("frmInput");
        frmInput->resize(990, 310);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frmInput->sizePolicy().hasHeightForWidth());
        frmInput->setSizePolicy(sizePolicy);
        frmInput->setMaximumSize(QSize(1200, 400));
        frmInput->setStyleSheet(QString::fromUtf8("background-color: transparent;"));
        frameMain = new QFrame(frmInput);
        frameMain->setObjectName("frameMain");
        frameMain->setGeometry(QRect(0, 0, 990, 300));
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(frameMain->sizePolicy().hasHeightForWidth());
        frameMain->setSizePolicy(sizePolicy1);
        frameMain->setMinimumSize(QSize(990, 300));
        frameMain->setMaximumSize(QSize(990, 330));
        QFont font;
        font.setFamilies({QString::fromUtf8("Microsoft YaHei")});
        font.setPointSize(13);
        frameMain->setFont(font);
        frameMain->setStyleSheet(QString::fromUtf8("\n"
"\n"
"#frameMain {\n"
"    border-radius: 15px;        /* \345\234\206\350\247\222\345\215\212\345\276\204 */\n"
"	background-color: #1976D2;\n"
"	border: 1px solid #1565C0;\n"
"}\n"
"\n"
"QPushButton {\n"
"    color: white;\n"
"}\n"
"\n"
"/* \346\214\211\351\222\256\346\214\211\344\270\213\346\225\210\346\236\234 */\n"
"QPushButton:pressed {\n"
"    background-color: #1565C0;  /* \346\233\264\346\267\261\347\232\204\351\242\234\350\211\262 */\n"
"    padding-top: 7px;          /* \346\250\241\346\213\237\344\270\213\346\262\211\346\225\210\346\236\234 */\n"
"    padding-bottom: 5px;\n"
"    margin-top: 5px;           /* \344\277\235\346\214\201\346\200\273\351\253\230\345\272\246\344\270\215\345\217\230 */\n"
"}"));
        gridLayout = new QGridLayout(frameMain);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setHorizontalSpacing(1);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(6, 3, 6, 14);
        btns = new QPushButton(frameMain);
        btns->setObjectName("btns");
        sizePolicy1.setHeightForWidth(btns->sizePolicy().hasHeightForWidth());
        btns->setSizePolicy(sizePolicy1);
        btns->setFont(font);
        btns->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btns, 3, 1, 1, 1);

        btnOther7 = new QPushButton(frameMain);
        btnOther7->setObjectName("btnOther7");
        sizePolicy1.setHeightForWidth(btnOther7->sizePolicy().hasHeightForWidth());
        btnOther7->setSizePolicy(sizePolicy1);
        btnOther7->setFont(font);
        btnOther7->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther7, 1, 5, 1, 1);

        btna = new QPushButton(frameMain);
        btna->setObjectName("btna");
        sizePolicy1.setHeightForWidth(btna->sizePolicy().hasHeightForWidth());
        btna->setSizePolicy(sizePolicy1);
        btna->setFont(font);
        btna->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btna, 3, 0, 1, 1);

        btnu = new QPushButton(frameMain);
        btnu->setObjectName("btnu");
        sizePolicy1.setHeightForWidth(btnu->sizePolicy().hasHeightForWidth());
        btnu->setSizePolicy(sizePolicy1);
        btnu->setFont(font);
        btnu->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnu, 2, 6, 1, 1);

        btnc = new QPushButton(frameMain);
        btnc->setObjectName("btnc");
        sizePolicy1.setHeightForWidth(btnc->sizePolicy().hasHeightForWidth());
        btnc->setSizePolicy(sizePolicy1);
        btnc->setFont(font);
        btnc->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnc, 4, 2, 1, 1);

        btnOther2 = new QPushButton(frameMain);
        btnOther2->setObjectName("btnOther2");
        sizePolicy1.setHeightForWidth(btnOther2->sizePolicy().hasHeightForWidth());
        btnOther2->setSizePolicy(sizePolicy1);
        btnOther2->setFont(font);
        btnOther2->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther2, 1, 1, 1, 1);

        btnh = new QPushButton(frameMain);
        btnh->setObjectName("btnh");
        sizePolicy1.setHeightForWidth(btnh->sizePolicy().hasHeightForWidth());
        btnh->setSizePolicy(sizePolicy1);
        btnh->setFont(font);
        btnh->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnh, 3, 5, 1, 1);

        btnq = new QPushButton(frameMain);
        btnq->setObjectName("btnq");
        sizePolicy1.setHeightForWidth(btnq->sizePolicy().hasHeightForWidth());
        btnq->setSizePolicy(sizePolicy1);
        btnq->setFont(font);
        btnq->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnq, 2, 0, 1, 1);

        btn9 = new QPushButton(frameMain);
        btn9->setObjectName("btn9");
        sizePolicy1.setHeightForWidth(btn9->sizePolicy().hasHeightForWidth());
        btn9->setSizePolicy(sizePolicy1);
        btn9->setFont(font);
        btn9->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn9, 1, 17, 1, 1);

        btnOther4 = new QPushButton(frameMain);
        btnOther4->setObjectName("btnOther4");
        sizePolicy1.setHeightForWidth(btnOther4->sizePolicy().hasHeightForWidth());
        btnOther4->setSizePolicy(sizePolicy1);
        btnOther4->setFont(font);
        btnOther4->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther4, 1, 3, 1, 1);

        btn7 = new QPushButton(frameMain);
        btn7->setObjectName("btn7");
        sizePolicy1.setHeightForWidth(btn7->sizePolicy().hasHeightForWidth());
        btn7->setSizePolicy(sizePolicy1);
        btn7->setFont(font);
        btn7->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn7, 1, 15, 1, 1);

        btng = new QPushButton(frameMain);
        btng->setObjectName("btng");
        sizePolicy1.setHeightForWidth(btng->sizePolicy().hasHeightForWidth());
        btng->setSizePolicy(sizePolicy1);
        btng->setFont(font);
        btng->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btng, 3, 4, 1, 1);

        btnDelete = new QPushButton(frameMain);
        btnDelete->setObjectName("btnDelete");
        sizePolicy1.setHeightForWidth(btnDelete->sizePolicy().hasHeightForWidth());
        btnDelete->setSizePolicy(sizePolicy1);
        btnDelete->setFont(font);
        btnDelete->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnDelete, 1, 12, 1, 2);

        btn5 = new QPushButton(frameMain);
        btn5->setObjectName("btn5");
        sizePolicy1.setHeightForWidth(btn5->sizePolicy().hasHeightForWidth());
        btn5->setSizePolicy(sizePolicy1);
        btn5->setFont(font);
        btn5->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn5, 2, 16, 1, 1);

        btnf = new QPushButton(frameMain);
        btnf->setObjectName("btnf");
        sizePolicy1.setHeightForWidth(btnf->sizePolicy().hasHeightForWidth());
        btnf->setSizePolicy(sizePolicy1);
        btnf->setFont(font);
        btnf->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnf, 3, 3, 1, 1);

        btnz = new QPushButton(frameMain);
        btnz->setObjectName("btnz");
        sizePolicy1.setHeightForWidth(btnz->sizePolicy().hasHeightForWidth());
        btnz->setSizePolicy(sizePolicy1);
        btnz->setFont(font);
        btnz->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnz, 4, 0, 1, 1);

        btnOther13 = new QPushButton(frameMain);
        btnOther13->setObjectName("btnOther13");
        sizePolicy1.setHeightForWidth(btnOther13->sizePolicy().hasHeightForWidth());
        btnOther13->setSizePolicy(sizePolicy1);
        btnOther13->setFont(font);
        btnOther13->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther13, 2, 13, 1, 1);

        btnOther14 = new QPushButton(frameMain);
        btnOther14->setObjectName("btnOther14");
        sizePolicy1.setHeightForWidth(btnOther14->sizePolicy().hasHeightForWidth());
        btnOther14->setSizePolicy(sizePolicy1);
        btnOther14->setFont(font);
        btnOther14->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther14, 3, 10, 1, 1);

        btn3 = new QPushButton(frameMain);
        btn3->setObjectName("btn3");
        sizePolicy1.setHeightForWidth(btn3->sizePolicy().hasHeightForWidth());
        btn3->setSizePolicy(sizePolicy1);
        btn3->setFont(font);
        btn3->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn3, 3, 17, 1, 1);

        btni = new QPushButton(frameMain);
        btni->setObjectName("btni");
        sizePolicy1.setHeightForWidth(btni->sizePolicy().hasHeightForWidth());
        btni->setSizePolicy(sizePolicy1);
        btni->setFont(font);
        btni->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btni, 2, 7, 1, 1);

        btn1 = new QPushButton(frameMain);
        btn1->setObjectName("btn1");
        sizePolicy1.setHeightForWidth(btn1->sizePolicy().hasHeightForWidth());
        btn1->setSizePolicy(sizePolicy1);
        btn1->setFont(font);
        btn1->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn1, 3, 15, 1, 1);

        btnk = new QPushButton(frameMain);
        btnk->setObjectName("btnk");
        sizePolicy1.setHeightForWidth(btnk->sizePolicy().hasHeightForWidth());
        btnk->setSizePolicy(sizePolicy1);
        btnk->setFont(font);
        btnk->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnk, 3, 7, 1, 1);

        btnv = new QPushButton(frameMain);
        btnv->setObjectName("btnv");
        sizePolicy1.setHeightForWidth(btnv->sizePolicy().hasHeightForWidth());
        btnv->setSizePolicy(sizePolicy1);
        btnv->setFont(font);
        btnv->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnv, 4, 3, 1, 1);

        btnx = new QPushButton(frameMain);
        btnx->setObjectName("btnx");
        sizePolicy1.setHeightForWidth(btnx->sizePolicy().hasHeightForWidth());
        btnx->setSizePolicy(sizePolicy1);
        btnx->setFont(font);
        btnx->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnx, 4, 1, 1, 1);

        btnOther3 = new QPushButton(frameMain);
        btnOther3->setObjectName("btnOther3");
        sizePolicy1.setHeightForWidth(btnOther3->sizePolicy().hasHeightForWidth());
        btnOther3->setSizePolicy(sizePolicy1);
        btnOther3->setFont(font);
        btnOther3->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther3, 1, 2, 1, 1);

        btnn = new QPushButton(frameMain);
        btnn->setObjectName("btnn");
        sizePolicy1.setHeightForWidth(btnn->sizePolicy().hasHeightForWidth());
        btnn->setSizePolicy(sizePolicy1);
        btnn->setFont(font);
        btnn->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnn, 4, 5, 1, 1);

        btnSpace = new QPushButton(frameMain);
        btnSpace->setObjectName("btnSpace");
        sizePolicy1.setHeightForWidth(btnSpace->sizePolicy().hasHeightForWidth());
        btnSpace->setSizePolicy(sizePolicy1);
        btnSpace->setFont(font);
        btnSpace->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnSpace, 4, 10, 1, 2);

        btnOther10 = new QPushButton(frameMain);
        btnOther10->setObjectName("btnOther10");
        sizePolicy1.setHeightForWidth(btnOther10->sizePolicy().hasHeightForWidth());
        btnOther10->setSizePolicy(sizePolicy1);
        btnOther10->setFont(font);
        btnOther10->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther10, 2, 11, 1, 1);

        btn2 = new QPushButton(frameMain);
        btn2->setObjectName("btn2");
        sizePolicy1.setHeightForWidth(btn2->sizePolicy().hasHeightForWidth());
        btn2->setSizePolicy(sizePolicy1);
        btn2->setFont(font);
        btn2->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn2, 3, 16, 1, 1);

        btny = new QPushButton(frameMain);
        btny->setObjectName("btny");
        sizePolicy1.setHeightForWidth(btny->sizePolicy().hasHeightForWidth());
        btny->setSizePolicy(sizePolicy1);
        btny->setFont(font);
        btny->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btny, 2, 5, 1, 1);

        btnOther16 = new QPushButton(frameMain);
        btnOther16->setObjectName("btnOther16");
        sizePolicy1.setHeightForWidth(btnOther16->sizePolicy().hasHeightForWidth());
        btnOther16->setSizePolicy(sizePolicy1);
        btnOther16->setFont(font);
        btnOther16->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther16, 3, 13, 1, 1);

        btno = new QPushButton(frameMain);
        btno->setObjectName("btno");
        sizePolicy1.setHeightForWidth(btno->sizePolicy().hasHeightForWidth());
        btno->setSizePolicy(sizePolicy1);
        btno->setFont(font);
        btno->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btno, 2, 8, 1, 1);

        btnOther12 = new QPushButton(frameMain);
        btnOther12->setObjectName("btnOther12");
        sizePolicy1.setHeightForWidth(btnOther12->sizePolicy().hasHeightForWidth());
        btnOther12->setSizePolicy(sizePolicy1);
        btnOther12->setFont(font);
        btnOther12->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther12, 2, 12, 1, 1);

        btnd = new QPushButton(frameMain);
        btnd->setObjectName("btnd");
        sizePolicy1.setHeightForWidth(btnd->sizePolicy().hasHeightForWidth());
        btnd->setSizePolicy(sizePolicy1);
        btnd->setFont(font);
        btnd->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnd, 3, 2, 1, 1);

        btnType = new QPushButton(frameMain);
        btnType->setObjectName("btnType");
        sizePolicy1.setHeightForWidth(btnType->sizePolicy().hasHeightForWidth());
        btnType->setSizePolicy(sizePolicy1);
        btnType->setFont(font);
        btnType->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnType, 4, 12, 1, 2);

        btnOther1 = new QPushButton(frameMain);
        btnOther1->setObjectName("btnOther1");
        sizePolicy1.setHeightForWidth(btnOther1->sizePolicy().hasHeightForWidth());
        btnOther1->setSizePolicy(sizePolicy1);
        btnOther1->setFont(font);
        btnOther1->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther1, 1, 0, 1, 1);

        btnl = new QPushButton(frameMain);
        btnl->setObjectName("btnl");
        sizePolicy1.setHeightForWidth(btnl->sizePolicy().hasHeightForWidth());
        btnl->setSizePolicy(sizePolicy1);
        btnl->setFont(font);
        btnl->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnl, 3, 8, 1, 1);

        btnOther19 = new QPushButton(frameMain);
        btnOther19->setObjectName("btnOther19");
        sizePolicy1.setHeightForWidth(btnOther19->sizePolicy().hasHeightForWidth());
        btnOther19->setSizePolicy(sizePolicy1);
        btnOther19->setFont(font);
        btnOther19->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther19, 1, 7, 1, 1);

        btnOther18 = new QPushButton(frameMain);
        btnOther18->setObjectName("btnOther18");
        sizePolicy1.setHeightForWidth(btnOther18->sizePolicy().hasHeightForWidth());
        btnOther18->setSizePolicy(sizePolicy1);
        btnOther18->setFont(font);
        btnOther18->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther18, 4, 8, 1, 1);

        btne = new QPushButton(frameMain);
        btne->setObjectName("btne");
        sizePolicy1.setHeightForWidth(btne->sizePolicy().hasHeightForWidth());
        btne->setSizePolicy(sizePolicy1);
        btne->setFont(font);
        btne->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btne, 2, 2, 1, 1);

        btnOther8 = new QPushButton(frameMain);
        btnOther8->setObjectName("btnOther8");
        sizePolicy1.setHeightForWidth(btnOther8->sizePolicy().hasHeightForWidth());
        btnOther8->setSizePolicy(sizePolicy1);
        btnOther8->setFont(font);
        btnOther8->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther8, 1, 6, 1, 1);

        btnOther11 = new QPushButton(frameMain);
        btnOther11->setObjectName("btnOther11");
        sizePolicy1.setHeightForWidth(btnOther11->sizePolicy().hasHeightForWidth());
        btnOther11->setSizePolicy(sizePolicy1);
        btnOther11->setFont(font);
        btnOther11->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther11, 1, 11, 1, 1);

        btnw = new QPushButton(frameMain);
        btnw->setObjectName("btnw");
        sizePolicy1.setHeightForWidth(btnw->sizePolicy().hasHeightForWidth());
        btnw->setSizePolicy(sizePolicy1);
        btnw->setFont(font);
        btnw->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnw, 2, 1, 1, 1);

        btnm = new QPushButton(frameMain);
        btnm->setObjectName("btnm");
        sizePolicy1.setHeightForWidth(btnm->sizePolicy().hasHeightForWidth());
        btnm->setSizePolicy(sizePolicy1);
        btnm->setFont(font);
        btnm->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnm, 4, 6, 1, 1);

        btnOther17 = new QPushButton(frameMain);
        btnOther17->setObjectName("btnOther17");
        sizePolicy1.setHeightForWidth(btnOther17->sizePolicy().hasHeightForWidth());
        btnOther17->setSizePolicy(sizePolicy1);
        btnOther17->setFont(font);
        btnOther17->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther17, 4, 7, 1, 1);

        btnp = new QPushButton(frameMain);
        btnp->setObjectName("btnp");
        sizePolicy1.setHeightForWidth(btnp->sizePolicy().hasHeightForWidth());
        btnp->setSizePolicy(sizePolicy1);
        btnp->setFont(font);
        btnp->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnp, 2, 10, 1, 1);

        btnOther15 = new QPushButton(frameMain);
        btnOther15->setObjectName("btnOther15");
        sizePolicy1.setHeightForWidth(btnOther15->sizePolicy().hasHeightForWidth());
        btnOther15->setSizePolicy(sizePolicy1);
        btnOther15->setFont(font);
        btnOther15->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther15, 3, 12, 1, 1);

        btn8 = new QPushButton(frameMain);
        btn8->setObjectName("btn8");
        sizePolicy1.setHeightForWidth(btn8->sizePolicy().hasHeightForWidth());
        btn8->setSizePolicy(sizePolicy1);
        btn8->setFont(font);
        btn8->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn8, 1, 16, 1, 1);

        spacer = new QSpacerItem(10, 20, QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(spacer, 1, 14, 1, 1);

        btnb = new QPushButton(frameMain);
        btnb->setObjectName("btnb");
        sizePolicy1.setHeightForWidth(btnb->sizePolicy().hasHeightForWidth());
        btnb->setSizePolicy(sizePolicy1);
        btnb->setFont(font);
        btnb->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnb, 4, 4, 1, 1);

        btnj = new QPushButton(frameMain);
        btnj->setObjectName("btnj");
        sizePolicy1.setHeightForWidth(btnj->sizePolicy().hasHeightForWidth());
        btnj->setSizePolicy(sizePolicy1);
        btnj->setFont(font);
        btnj->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnj, 3, 6, 1, 1);

        btn4 = new QPushButton(frameMain);
        btn4->setObjectName("btn4");
        sizePolicy1.setHeightForWidth(btn4->sizePolicy().hasHeightForWidth());
        btn4->setSizePolicy(sizePolicy1);
        btn4->setFont(font);
        btn4->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn4, 2, 15, 1, 1);

        btnOther9 = new QPushButton(frameMain);
        btnOther9->setObjectName("btnOther9");
        sizePolicy1.setHeightForWidth(btnOther9->sizePolicy().hasHeightForWidth());
        btnOther9->setSizePolicy(sizePolicy1);
        btnOther9->setFont(font);
        btnOther9->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther9, 1, 10, 1, 1);

        btn0 = new QPushButton(frameMain);
        btn0->setObjectName("btn0");
        sizePolicy1.setHeightForWidth(btn0->sizePolicy().hasHeightForWidth());
        btn0->setSizePolicy(sizePolicy1);
        btn0->setFont(font);
        btn0->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn0, 4, 15, 1, 2);

        btn6 = new QPushButton(frameMain);
        btn6->setObjectName("btn6");
        sizePolicy1.setHeightForWidth(btn6->sizePolicy().hasHeightForWidth());
        btn6->setSizePolicy(sizePolicy1);
        btn6->setFont(font);
        btn6->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btn6, 2, 17, 1, 1);

        btnt = new QPushButton(frameMain);
        btnt->setObjectName("btnt");
        sizePolicy1.setHeightForWidth(btnt->sizePolicy().hasHeightForWidth());
        btnt->setSizePolicy(sizePolicy1);
        btnt->setFont(font);
        btnt->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnt, 2, 4, 1, 1);

        btnDot = new QPushButton(frameMain);
        btnDot->setObjectName("btnDot");
        sizePolicy1.setHeightForWidth(btnDot->sizePolicy().hasHeightForWidth());
        btnDot->setSizePolicy(sizePolicy1);
        btnDot->setFont(font);
        btnDot->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnDot, 4, 17, 1, 1);

        btnr = new QPushButton(frameMain);
        btnr->setObjectName("btnr");
        sizePolicy1.setHeightForWidth(btnr->sizePolicy().hasHeightForWidth());
        btnr->setSizePolicy(sizePolicy1);
        btnr->setFont(font);
        btnr->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnr, 2, 3, 1, 1);

        btnOther20 = new QPushButton(frameMain);
        btnOther20->setObjectName("btnOther20");
        sizePolicy1.setHeightForWidth(btnOther20->sizePolicy().hasHeightForWidth());
        btnOther20->setSizePolicy(sizePolicy1);
        btnOther20->setFont(font);
        btnOther20->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther20, 1, 8, 1, 1);

        btnOther5 = new QPushButton(frameMain);
        btnOther5->setObjectName("btnOther5");
        sizePolicy1.setHeightForWidth(btnOther5->sizePolicy().hasHeightForWidth());
        btnOther5->setSizePolicy(sizePolicy1);
        btnOther5->setFont(font);
        btnOther5->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther5, 1, 4, 1, 1);

        btnOther21 = new QPushButton(frameMain);
        btnOther21->setObjectName("btnOther21");
        sizePolicy1.setHeightForWidth(btnOther21->sizePolicy().hasHeightForWidth());
        btnOther21->setSizePolicy(sizePolicy1);
        btnOther21->setFont(font);
        btnOther21->setFocusPolicy(Qt::NoFocus);

        gridLayout->addWidget(btnOther21, 3, 11, 1, 1);

        btnClose = new QPushButton(frameMain);
        btnClose->setObjectName("btnClose");
        sizePolicy1.setHeightForWidth(btnClose->sizePolicy().hasHeightForWidth());
        btnClose->setSizePolicy(sizePolicy1);
        btnClose->setMaximumSize(QSize(150, 50));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Microsoft YaHei")});
        font1.setPointSize(10);
        font1.setBold(true);
        btnClose->setFont(font1);
        btnClose->setFocusPolicy(Qt::NoFocus);
        btnClose->setStyleSheet(QString::fromUtf8(""));

        gridLayout->addWidget(btnClose, 0, 16, 1, 2);

        lab_Title = new QLabel(frameMain);
        lab_Title->setObjectName("lab_Title");
        sizePolicy1.setHeightForWidth(lab_Title->sizePolicy().hasHeightForWidth());
        lab_Title->setSizePolicy(sizePolicy1);
        lab_Title->setMinimumSize(QSize(0, 25));
        lab_Title->setMaximumSize(QSize(100, 16777215));
        lab_Title->setFont(font);
        lab_Title->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));

        gridLayout->addWidget(lab_Title, 0, 0, 1, 2);

        labPY = new QLabel(frameMain);
        labPY->setObjectName("labPY");
        sizePolicy1.setHeightForWidth(labPY->sizePolicy().hasHeightForWidth());
        labPY->setSizePolicy(sizePolicy1);
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Microsoft YaHei")});
        font2.setPointSize(13);
        font2.setBold(true);
        labPY->setFont(font2);
        labPY->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));
        labPY->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(labPY, 0, 6, 1, 6);


        retranslateUi(frmInput);

        QMetaObject::connectSlotsByName(frmInput);
    } // setupUi

    void retranslateUi(QWidget *frmInput)
    {
        frmInput->setWindowTitle(QCoreApplication::translate("frmInput", "\350\276\223\345\205\245\346\263\225", nullptr));
        btns->setText(QCoreApplication::translate("frmInput", "s", nullptr));
        btnOther7->setText(QCoreApplication::translate("frmInput", "&&", nullptr));
        btna->setText(QCoreApplication::translate("frmInput", "a", nullptr));
        btnu->setText(QCoreApplication::translate("frmInput", "u", nullptr));
        btnc->setText(QCoreApplication::translate("frmInput", "c", nullptr));
        btnOther2->setText(QCoreApplication::translate("frmInput", "@", nullptr));
        btnh->setText(QCoreApplication::translate("frmInput", "h", nullptr));
        btnq->setText(QCoreApplication::translate("frmInput", "q", nullptr));
        btn9->setText(QCoreApplication::translate("frmInput", "9", nullptr));
        btnOther4->setText(QCoreApplication::translate("frmInput", "$", nullptr));
        btn7->setText(QCoreApplication::translate("frmInput", "7", nullptr));
        btng->setText(QCoreApplication::translate("frmInput", "g", nullptr));
        btnDelete->setText(QCoreApplication::translate("frmInput", "\342\206\220", nullptr));
        btn5->setText(QCoreApplication::translate("frmInput", "5", nullptr));
        btnf->setText(QCoreApplication::translate("frmInput", "f", nullptr));
        btnz->setText(QCoreApplication::translate("frmInput", "z", nullptr));
        btnOther13->setText(QCoreApplication::translate("frmInput", "|", nullptr));
        btnOther14->setText(QCoreApplication::translate("frmInput", ":", nullptr));
        btn3->setText(QCoreApplication::translate("frmInput", "3", nullptr));
        btni->setText(QCoreApplication::translate("frmInput", "i", nullptr));
        btn1->setText(QCoreApplication::translate("frmInput", "1", nullptr));
        btnk->setText(QCoreApplication::translate("frmInput", "k", nullptr));
        btnv->setText(QCoreApplication::translate("frmInput", "v", nullptr));
        btnx->setText(QCoreApplication::translate("frmInput", "x", nullptr));
        btnOther3->setText(QCoreApplication::translate("frmInput", "#", nullptr));
        btnn->setText(QCoreApplication::translate("frmInput", "n", nullptr));
        btnSpace->setText(QCoreApplication::translate("frmInput", "SPACE", nullptr));
        btnOther10->setText(QCoreApplication::translate("frmInput", "_", nullptr));
        btn2->setText(QCoreApplication::translate("frmInput", "2", nullptr));
        btny->setText(QCoreApplication::translate("frmInput", "y", nullptr));
        btnOther16->setText(QCoreApplication::translate("frmInput", "=", nullptr));
        btno->setText(QCoreApplication::translate("frmInput", "o", nullptr));
        btnOther12->setText(QCoreApplication::translate("frmInput", "/", nullptr));
        btnd->setText(QCoreApplication::translate("frmInput", "d", nullptr));
        btnType->setText(QCoreApplication::translate("frmInput", "\345\260\217\345\206\231Aa", nullptr));
        btnOther1->setText(QCoreApplication::translate("frmInput", "!", nullptr));
        btnl->setText(QCoreApplication::translate("frmInput", "l", nullptr));
        btnOther19->setText(QCoreApplication::translate("frmInput", "(", nullptr));
        btnOther18->setText(QCoreApplication::translate("frmInput", "\\", nullptr));
        btne->setText(QCoreApplication::translate("frmInput", "e", nullptr));
        btnOther8->setText(QCoreApplication::translate("frmInput", "*", nullptr));
        btnOther11->setText(QCoreApplication::translate("frmInput", "+", nullptr));
        btnw->setText(QCoreApplication::translate("frmInput", "w", nullptr));
        btnm->setText(QCoreApplication::translate("frmInput", "m", nullptr));
        btnOther17->setText(QCoreApplication::translate("frmInput", ",", nullptr));
        btnp->setText(QCoreApplication::translate("frmInput", "p", nullptr));
        btnOther15->setText(QCoreApplication::translate("frmInput", "?", nullptr));
        btn8->setText(QCoreApplication::translate("frmInput", "8", nullptr));
        btnb->setText(QCoreApplication::translate("frmInput", "b", nullptr));
        btnj->setText(QCoreApplication::translate("frmInput", "j", nullptr));
        btn4->setText(QCoreApplication::translate("frmInput", "4", nullptr));
        btnOther9->setText(QCoreApplication::translate("frmInput", "-", nullptr));
        btn0->setText(QCoreApplication::translate("frmInput", "0", nullptr));
        btn6->setText(QCoreApplication::translate("frmInput", "6", nullptr));
        btnt->setText(QCoreApplication::translate("frmInput", "t", nullptr));
        btnDot->setText(QCoreApplication::translate("frmInput", ".", nullptr));
        btnr->setText(QCoreApplication::translate("frmInput", "r", nullptr));
        btnOther20->setText(QCoreApplication::translate("frmInput", ")", nullptr));
        btnOther5->setText(QCoreApplication::translate("frmInput", "%", nullptr));
        btnOther21->setText(QCoreApplication::translate("frmInput", "\"", nullptr));
        btnClose->setText(QCoreApplication::translate("frmInput", "\345\205\263\351\227\255/CLOSE", nullptr));
        lab_Title->setText(QCoreApplication::translate("frmInput", " \345\260\217\345\206\231", nullptr));
        labPY->setText(QCoreApplication::translate("frmInput", "WOWA-\346\227\240\346\266\257", nullptr));
    } // retranslateUi

};

namespace Ui {
    class frmInput: public Ui_frmInput {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FRMINPUT_H
