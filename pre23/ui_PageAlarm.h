/********************************************************************************
** Form generated from reading UI file 'PageAlarm.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PAGEALARM_H
#define UI_PAGEALARM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PageAlarm
{
public:

    void setupUi(QWidget *PageAlarm)
    {
        if (PageAlarm->objectName().isEmpty())
            PageAlarm->setObjectName("PageAlarm");
        PageAlarm->resize(1280, 800);

        retranslateUi(PageAlarm);

        QMetaObject::connectSlotsByName(PageAlarm);
    } // setupUi

    void retranslateUi(QWidget *PageAlarm)
    {
        PageAlarm->setWindowTitle(QCoreApplication::translate("PageAlarm", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PageAlarm: public Ui_PageAlarm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PAGEALARM_H
