#include "touchchartview.h"

TouchChartView::TouchChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::PanGesture);
}

bool TouchChartView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        bool handled = handleGesture(static_cast<QGestureEvent *>(event));
        if (handled)
            return true;
    }
    return QChartView::event(event);
}

bool TouchChartView::handleGesture(QGestureEvent *event)
{
    if (QGesture *g = event->gesture(Qt::PinchGesture)) {
        handlePinch(static_cast<QPinchGesture *>(g));
        return true;
    }
    if (QGesture *g = event->gesture(Qt::PanGesture)) {
        handlePan(static_cast<QPanGesture *>(g));
        return true;
    }
    return false;
}
// ====================== 双指缩放======================
void TouchChartView::handlePinch(QPinchGesture *pinch)
{
    if (!(pinch->changeFlags() & QPinchGesture::ScaleFactorChanged))
        return;

    auto axes = chart()->axes(Qt::Horizontal);
    if (axes.isEmpty()) return;
    auto *axis = qobject_cast<QDateTimeAxis*>(axes.first());
    if (!axis) return;

    // 当前范围（每次都用最新，避免累积错误）
    qint64 currentMin = axis->min().toMSecsSinceEpoch();
    qint64 currentMax = axis->max().toMSecsSinceEpoch();
    qint64 currentSpan = currentMax - currentMin;
    if (currentSpan <= 0) return;

    // 使用 incremental scaleFactor（关键！防止多次调用导致过度缩放）
    qreal scaleFactor = pinch->scaleFactor();

    // 死区：手指轻微抖动 < 5% 就忽略
    if (qAbs(scaleFactor - 1.0) < 0.05)
        return;

    // 可选：加一点阻尼让缩放更“柔和”（推荐 0.6~0.8）
    // scaleFactor = 1.0 + (scaleFactor - 1.0) * 0.7;

    qint64 newSpan = static_cast<qint64>(currentSpan / scaleFactor);

    // 最小范围保护
    const qint64 minAllowedSpan = 5000;   // 根据你的数据调整
    if (newSpan < minAllowedSpan)
        newSpan = minAllowedSpan;

    // === 关键：任意位置缩放都以双指中心为锚点 ===
    QPointF widgetCenter = pinch->centerPoint();           // widget 坐标
    QPointF sceneCenter  = mapToScene(widgetCenter.toPoint()); // 转 scene 坐标（必须！）

    // 如果手指在 plotArea 外面，fallback 到图表中心
    if (!chart()->plotArea().contains(sceneCenter)) {
        sceneCenter = chart()->plotArea().center();
    }

    QPointF valuePoint = chart()->mapToValue(sceneCenter);
    qint64 centerX = static_cast<qint64>(valuePoint.x());

    qint64 newMin = centerX - newSpan / 2;
    qint64 newMax = centerX + newSpan / 2;

    axis->setRange(QDateTime::fromMSecsSinceEpoch(newMin),
                   QDateTime::fromMSecsSinceEpoch(newMax));
}

// ====================== 单指左右拖动 ======================
void TouchChartView::handlePan(QPanGesture *pan)
{
    // 只在拖动过程中处理（GestureUpdated）
    if (pan->state() != Qt::GestureUpdated)
        return;

    QPointF delta = pan->delta();

    // 只做水平拖动（左右），忽略垂直
    // 如果方向反了，把 -delta.x() 改成 +delta.x() 即可
    chart()->scroll(-delta.x(), 0.0);
}
