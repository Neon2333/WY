#ifndef TOUCHCHARTVIEW_H
#define TOUCHCHARTVIEW_H

#include <QtCharts/QChartView>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QDateTimeAxis>

QT_CHARTS_USE_NAMESPACE

    class TouchChartView : public QChartView
{
    Q_OBJECT
public:
    explicit TouchChartView(QChart *chart, QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    bool handleGesture(QGestureEvent *event);
    void handlePinch(QPinchGesture *pinch);
    void handlePan(QPanGesture *pan);

};

#endif
