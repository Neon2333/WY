#include "QRoundProgressBar.h"
#include <QPainter>
#include <QtMath>

QRoundProgressBar::QRoundProgressBar(QWidget *parent)
    : QWidget(parent),
    m_min(0),
    m_max(100),
    m_value(25),
    m_startAngle(90),
    m_outlinePenWidth(2),
    m_dataPenWidth(10),
    m_decimals(1),
    m_format("%p%"),
    m_foreground(Qt::blue),
    m_style(StyleDonut)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void QRoundProgressBar::setRange(double min, double max) {
    m_min = min;
    m_max = max;
    update();
}

void QRoundProgressBar::setValue(double value) {
    m_value = value;
    update();
}

void QRoundProgressBar::setBarStyle(BarStyle style) {
    m_style = style;
    update();
}

void QRoundProgressBar::setFormat(const QString &format) {
    m_format = format;
    update();
}

void QRoundProgressBar::setDecimals(int count) {
    m_decimals = count;
    update();
}

void QRoundProgressBar::setDataPenWidth(int width) {
    m_dataPenWidth = width;
    update();
}

void QRoundProgressBar::setOutlinePenWidth(int width) {
    m_outlinePenWidth = width;
    update();
}

void QRoundProgressBar::setStartAngle(double angle) {
    m_startAngle = angle;
    update();
}

void QRoundProgressBar::setForeground(const QColor &color) {
    m_foreground = color;
    update();
}

QString QRoundProgressBar::valueToText(double value) const {
    QString text = m_format;
    // text.replace("%v", QString::number(value, 'f', m_decimals));

    // 修改这里：强制显示整数，无小数
    text.replace("%v", QString::number(static_cast<int>(value))); // 强制转为整数

    double percent = (value - m_min) / (m_max - m_min) * 100.0;
    text.replace("%p", QString::number(percent, 'f', m_decimals));
    return text;
}

void QRoundProgressBar::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // QRectF rect = QRectF(m_outlinePenWidth,
    //                      m_outlinePenWidth,
    //                      width() - 2 * m_outlinePenWidth,
    //                      height() - 2 * m_outlinePenWidth);
    QRectF rect = QRectF(0, 0, width(), height());

    drawBackground(painter, rect);

    double percent = (m_value - m_min) / (m_max - m_min);
    drawValue(painter, rect, percent);

    drawInnerBackground(painter, rect);

    // draw text
    painter.setPen(Qt::black);
    painter.drawText(rect, Qt::AlignCenter, valueToText(m_value));

}

void QRoundProgressBar::drawBackground(QPainter &painter, const QRectF &rect) {
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(240, 240, 240));
    painter.drawEllipse(rect);
    painter.restore();
}

void QRoundProgressBar::drawValue(QPainter &painter, const QRectF &rect, double percent) {
    painter.save();

    QPen pen(m_foreground);
    pen.setCapStyle(Qt::FlatCap);
    pen.setWidth(m_dataPenWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    double arcLength = 360.0 * percent;
    QRectF arcRect = rect.adjusted(m_dataPenWidth / 2.0,
                                   m_dataPenWidth / 2.0,
                                   -m_dataPenWidth / 2.0,
                                   -m_dataPenWidth / 2.0);

    if (m_style == StyleLine || m_style == StyleDonut || m_style == StylePie) {
        painter.drawArc(arcRect, (90 - m_startAngle) * 16, -arcLength * 16);
    }

    painter.restore();
}

void QRoundProgressBar::drawInnerBackground(QPainter &painter, const QRectF &rect) {
    if (m_style == StyleDonut) {
        double innerRadius = rect.width() / 2 - m_dataPenWidth;
        QRectF innerRect(rect.center().x() - innerRadius,
                         rect.center().y() - innerRadius,
                         innerRadius * 2,
                         innerRadius * 2);
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().window());
        painter.drawEllipse(innerRect);
        painter.restore();
    }
}

double QRoundProgressBar::bar_currentValue() const
{
    return m_value;
}
