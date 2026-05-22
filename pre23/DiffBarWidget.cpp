#include "DiffBarWidget.h"
#include <QPainter>
#include <QFontMetrics>

DiffBarWidget::DiffBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumHeight(300);  // 设置最小高度，确保有足够空间绘制（调整为合适值）
}

/*
 * QVector<DiffItem>: Qt的动态数组容器，存储DiffItem类型的元素
 * &values: 引用传递，避免拷贝整个vector
 */
void DiffBarWidget::setDiffValues(const QVector<DiffItem> &values)
{
    m_values = values;
    m_maxAbsValue = 0;
    for (auto &v : values)
        m_maxAbsValue = qMax(m_maxAbsValue, qAbs(v.value));
    if (m_maxAbsValue == 0) m_maxAbsValue = 1.0;  // 避免所有值为0时除零或不显示
    m_maxAbsValue *= 1.2;
    update();
}

void DiffBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int centerX = w / 2;

    int count = m_values.size();
    if (count == 0)
        return;

    // ===== 布局参数 =====
    const int topMargin = 8;
    const int bottomMargin = 30;
    int spacing = P.spacing;

    int usableHeight = h - topMargin - bottomMargin;

    // barHeight = 理想高度，但不超过最大高度
    int barHeight = (usableHeight - (count - 1) * spacing) / count;
    barHeight = qMin(barHeight, P.MaxbarHeight);   // 最大不超过

    // 可选：限制“最小高度”，防止条太细
    barHeight = qMax(barHeight, 26);

    // 字体
    QFont f = font();
    f.setPointSize(P.fontSize);
    p.setFont(f);
    QFontMetrics fm(f);

    int y = topMargin;

    for (const auto &item : m_values)
    {
        // 名称
        p.setPen(P.labelColor);
        p.drawText(10, y + barHeight - 4, item.name);

        // 比例
        double ratio = item.value / m_maxAbsValue;
        int len = int((w / 2 - 60) * qAbs(ratio));

        // 颜色
        QColor color = item.value >= 0 ? P.positiveColor : P.negativeColor;
        p.setBrush(color);
        p.setPen(Qt::NoPen);

        QRect barRect;
        if (item.value >= 0)
            barRect = QRect(centerX, y, len, barHeight);
        else
            barRect = QRect(centerX - len, y, len, barHeight);

        p.drawRoundedRect(barRect, P.cornerRadius, P.cornerRadius);

        // 数值
        p.setPen(P.valueColor);
        QString valStr = QString::number(item.value, 'f', 1);
        int textWidth = fm.horizontalAdvance(valStr);

        int valueX = (item.value >= 0)
                         ? centerX + len + 6
                         : centerX - len - textWidth - 6;

        p.drawText(valueX, y + barHeight - 4, valStr);

        y += barHeight + spacing;
    }
}
