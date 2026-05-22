#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <QPointF>
#include <QVector>
#include <QtMath>
#include <QtGlobal>

// 计算点到线段的垂直距离
double perpendicularDistance(const QPointF& pt, const QPointF& lineStart, const QPointF& lineEnd);

// Douglas-Peucker 算法
void douglasPeucker(const QVector<QPointF>& data, int start, int end, double epsilon, QVector<QPointF>& result);

// 封装：带动态 EPSILON 的曲线简化
QVector<QPointF> simplifyCurve(const QVector<QPointF>& points, double epsRatio = 0.02, double epsMin = 0.1);



void douglasPeuckerLocal(const QVector<QPointF>& data, int start, int end, double epsilon, QVector<QPointF>& result);

QVector<QPointF> simplifyCurveLocal(const QVector<QPointF>& points, double epsRatio = 0.02, double epsMin = 0.1);

#endif // MATHUTILS_H
