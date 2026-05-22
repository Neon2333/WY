#ifndef QROUNDPROGRESSBAR_H
#define QROUNDPROGRESSBAR_H

#include <QWidget>
#include <QColor>
#include <QString>

class QRoundProgressBar : public QWidget {
    Q_OBJECT

public:
    enum BarStyle {
        StyleDonut,
        StylePie,
        StyleLine,
        StyleExpand
    };

    explicit QRoundProgressBar(QWidget *parent = nullptr);

    void setRange(double min, double max);
    void setValue(double value);
    void setBarStyle(BarStyle style);
    void setFormat(const QString &format);
    void setDecimals(int count);
    void setDataPenWidth(int width);
    void setOutlinePenWidth(int width);
    void setStartAngle(double angle);
    void setForeground(const QColor &color);

    double bar_currentValue() const;// 自定义的QRoundProgressBar的接口

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(150, 150); }

private:
    double m_min, m_max, m_value;
    double m_startAngle;
    int m_outlinePenWidth;
    int m_dataPenWidth;
    int m_decimals;
    QString m_format;
    QColor m_foreground;
    BarStyle m_style;

    QString valueToText(double value) const;
    void drawBackground(QPainter &painter, const QRectF &rect);
    void drawValue(QPainter &painter, const QRectF &rect, double percent);
    void drawInnerBackground(QPainter &painter, const QRectF &rect);
};

#endif // QROUNDPROGRESSBAR_H
