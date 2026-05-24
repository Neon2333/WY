#include "ParamInputPage.h"
#include "MessageDialog.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QEvent>

ParamInputPage::ParamInputPage(const QString &operatorName, QWidget *parent)
    : QWidget(parent)
    , m_operator(operatorName)
{
    setupUi();
    loadParams();
}

void ParamInputPage::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u6DA1\u5FC3\u89C4\u683C\u53C2\u6570\u8F93\u5165"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1976D2; margin-bottom: 20px;");

    QWidget *formWidget = new QWidget;
    formWidget->setStyleSheet("background-color: white; border-radius: 10px; padding: 20px;");

    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->setVerticalSpacing(15);
    gridLayout->setHorizontalSpacing(20);

    m_modelInput = createLineEdit(QString::fromUtf8("\u6DA1\u5FC3\u578B\u53F7 (\u4F8B: HF-2540)"));
    m_areaInput = createLineEdit(QString::fromUtf8("\u6DA1\u5FC3\u6709\u6548\u9762\u79EF (cm\u00B2)"));
    m_pressureInput = createLineEdit(QString::fromUtf8("\u6D4B\u8BD5\u538B\u529B Pc (mbar)"));
    m_coaInput = createLineEdit(QString::fromUtf8("\u6269\u6563\u6D41\u9608\u503C COA (ml/min)"));
    m_durationInput = createLineEdit(QString::fromUtf8("\u6D4B\u8BD5\u65F6\u95F4 T (s)"));
    m_tankVolumeInput = createLineEdit(QString::fromUtf8("\u6C14\u7F50\u4F53\u79EF Vg (ml)"));

    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6DA1\u5FC3\u578B\u53F7:")), 0, 0);
    gridLayout->addWidget(m_modelInput, 0, 1);
    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6DA1\u5FC3\u6709\u6548\u9762\u79EF:")), 0, 2);
    gridLayout->addWidget(m_areaInput, 0, 3);

    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6D4B\u8BD5\u538B\u529B Pc:")), 1, 0);
    gridLayout->addWidget(m_pressureInput, 1, 1);
    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6269\u6563\u6D41\u9608\u503C COA:")), 1, 2);
    gridLayout->addWidget(m_coaInput, 1, 3);

    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6D4B\u8BD5\u65F6\u95F4 T:")), 2, 0);
    gridLayout->addWidget(m_durationInput, 2, 1);
    gridLayout->addWidget(new QLabel(QString::fromUtf8("\u6C14\u7F50\u4F53\u79EF Vg:")), 2, 2);
    gridLayout->addWidget(m_tankVolumeInput, 2, 3);

    formWidget->setLayout(gridLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    m_saveBtn = new QPushButton(QString::fromUtf8("\u4FDD\u5B58\u53C2\u6570"));
    m_saveBtn->setFixedSize(150, 45);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: bold; }");

    btnLayout->addWidget(m_saveBtn);

    QLabel *tipLabel = new QLabel(QString::fromUtf8("\u63D0\u793A: \u8BF7\u5148\u8F93\u5165\u6DA1\u5FC3\u89C4\u683C\u53C2\u6570\uFF0C\u70B9\u51FB\u201C\u4FDD\u5B58\u53C2\u6570\u201D\u4FDD\u5B58\u540E\u518D\u8FDB\u884C\u6D4B\u8BD5"));
    tipLabel->setStyleSheet("color: #757575; font-size: 12px; margin-top: 10px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(formWidget);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(tipLabel);
    mainLayout->addStretch();

    setLayout(mainLayout);

    connect(m_saveBtn, &QPushButton::clicked, this, &ParamInputPage::onSaveClicked);
}

QLineEdit *ParamInputPage::createLineEdit(const QString &placeholder)
{
    QLineEdit *edit = new QLineEdit;
    edit->setPlaceholderText(placeholder);
    edit->setFixedHeight(40);
    edit->setStyleSheet("QLineEdit { border: 2px solid #E0E0E0; border-radius: 5px; padding: 8px; font-size: 13px; background: white; color: #212121; } QLineEdit:focus { border-color: #2196F3; }");
    return edit;
}

void ParamInputPage::loadParams()
{
    m_modelInput->clear();
    m_areaInput->clear();
    m_pressureInput->clear();
    m_coaInput->clear();
    m_durationInput->clear();
    m_tankVolumeInput->clear();
    m_currentFilterId = 0;

    FilterParams params = DatabaseManager::instance().getLatestFilterParams();
    if (params.id > 0 && !params.model.isEmpty()) {
        m_modelInput->setText(params.model);
        m_areaInput->setText(QString::number(params.area));
        m_pressureInput->setText(QString::number(params.testPressure));
        m_coaInput->setText(QString::number(params.coaThreshold));
        m_durationInput->setText(QString::number(params.testDuration));
        m_tankVolumeInput->setText(QString::number(params.tankVolume));
        m_currentFilterId = params.id;
    }
}

void ParamInputPage::onSaveClicked()
{
    QString model = m_modelInput->text().trimmed();
    QString areaStr = m_areaInput->text().trimmed();
    QString pressureStr = m_pressureInput->text().trimmed();
    QString coaStr = m_coaInput->text().trimmed();
    QString durationStr = m_durationInput->text().trimmed();
    QString tankStr = m_tankVolumeInput->text().trimmed();

    if (model.isEmpty() || areaStr.isEmpty() || pressureStr.isEmpty() ||
        coaStr.isEmpty() || durationStr.isEmpty() || tankStr.isEmpty()) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u8F93\u5165\u9519\u8BEF"), QString::fromUtf8("\u8BF7\u586B\u5199\u6240\u6709\u53C2\u6570"), MessageDialog::Warning);
        return;
    }

    bool ok;
    double area = areaStr.toDouble(&ok);
    double pressure = pressureStr.toDouble(&ok);
    double coa = coaStr.toDouble(&ok);
    int duration = durationStr.toInt(&ok);
    double tank = tankStr.toDouble(&ok);

    if (!ok) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u8F93\u5165\u9519\u8BEF"), QString::fromUtf8("\u53C2\u6570\u683C\u5F0F\u9519\u8BEF\uFF0C\u8BF7\u68C0\u67E5"), MessageDialog::Warning);
        return;
    }

    FilterParams params;
    params.model = model;
    params.area = area;
    params.testPressure = pressure;
    params.coaThreshold = coa;
    params.testDuration = duration;
    params.tankVolume = tank;

    bool success;
    if (m_currentFilterId > 0) {
        params.id = m_currentFilterId;
        success = DatabaseManager::instance().updateFilterParams(params);
    } else {
        success = DatabaseManager::instance().saveFilterParams(params);
    }

    if (success) {
        MessageDialog::showMessage(this, QString::fromUtf8("\u4FDD\u5B58\u6210\u529F"), QString::fromUtf8("\u53C2\u6570\u5DF2\u4FDD\u5B58"), MessageDialog::Success);
        loadParams();
    } else {
        MessageDialog::showMessage(this, QString::fromUtf8("\u4FDD\u5B58\u5931\u8D25"), QString::fromUtf8("\u53C2\u6570\u4FDD\u5B58\u5931\u8D25\uFF0C\u8BF7\u91CD\u8BD5"), MessageDialog::Error);
    }
}

FilterParams ParamInputPage::getCurrentParams()
{
    FilterParams params;
    params.id = m_currentFilterId;
    params.model = m_modelInput->text().trimmed();
    params.area = m_areaInput->text().toDouble();
    params.testPressure = m_pressureInput->text().toDouble();
    params.coaThreshold = m_coaInput->text().toDouble();
    params.testDuration = m_durationInput->text().toInt();
    params.tankVolume = m_tankVolumeInput->text().toDouble();
    return params;
}
