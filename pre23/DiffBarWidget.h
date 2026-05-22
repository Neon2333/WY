#ifndef DIFFBARWIDGET_H
#define DIFFBARWIDGET_H

#include <QWidget>
#include "GlobalDefines.h"

struct DiffBarParams
{
    int MaxbarHeight = 48;
    int barHeight = 26;
    int spacing = 10;
    int cornerRadius = 5;
    int fontSize = 10;

    QColor positiveColor = QColor(161, 27, 214);   // 正值颜色
    QColor negativeColor = QColor(25, 120, 230);   // 负值颜色

    QColor labelColor   = QColor(0, 0, 0);         // 名称颜色
    QColor valueColor   = QColor(0, 0, 0);         // 数值颜色
};

class DiffBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DiffBarWidget(QWidget *parent = nullptr);

    void setDiffValues(const QVector<DiffItem> &values);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<DiffItem> m_values;

    // 所有可调参数都放这，一个地方改，全局生效
    DiffBarParams P;

    double m_maxAbsValue = 1.0;
};

#endif // DIFFBARWIDGET_H
