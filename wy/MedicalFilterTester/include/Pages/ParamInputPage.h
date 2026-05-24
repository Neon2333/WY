#ifndef PARAMINPUTPAGE_H
#define PARAMINPUTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QEvent>
#include "DatabaseManager.h"

class ParamInputPage : public QWidget
{
    Q_OBJECT

public:
    explicit ParamInputPage(const QString &operatorName, QWidget *parent = nullptr);

    FilterParams getCurrentParams();

private slots:
    void onSaveClicked();
    void loadParams();

private:
    void setupUi();
    QLineEdit *createLineEdit(const QString &placeholder);

    QString m_operator;
    int m_currentFilterId = 0;
    QLineEdit *m_modelInput = nullptr;
    QLineEdit *m_areaInput = nullptr;
    QLineEdit *m_pressureInput = nullptr;
    QLineEdit *m_coaInput = nullptr;
    QLineEdit *m_durationInput = nullptr;
    QLineEdit *m_tankVolumeInput = nullptr;
    QPushButton *m_saveBtn = nullptr;
};

#endif
