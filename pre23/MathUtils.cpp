#include "MathUtils.h"

// // 辅助函数：计算点到线段的距离
// double perpendicularDistance(const QPointF& pt, const QPointF& lineStart, const QPointF& lineEnd) {
//     // 计算线段的向量 (dx, dy)
//     double dx = lineEnd.x() - lineStart.x();
//     double dy = lineEnd.y() - lineStart.y();
//     // 如果线段起点和终点重合，则距离为0
//     if (dx == 0 && dy == 0) return 0;
//     // 计算点pt到线段所在直线的垂足在线段上的投影参数t
//     // t = 0 表示垂足是线段起点，t = 1 表示是终点，0<t<1表示在线段内部
//     double t = ((pt.x() - lineStart.x()) * dx + (pt.y() - lineStart.y()) * dy) / (dx * dx + dy * dy);
//     // 将t约束在[0, 1]范围内，确保投影点在线段上而不是直线上
//     t = qBound(0.0, t, 1.0);
//     // 根据投影参数t计算线段上的投影点坐标(projX, projY)
//     double projX = lineStart.x() + t * dx;
//     double projY = lineStart.y() + t * dy;
//     // 返回点pt与线段上投影点(projX, projY)之间的欧几里得距离
//     return qSqrt(qPow(pt.x() - projX, 2) + qPow(pt.y() - projY, 2));
// }

// // 递归Douglas-Peucker
// void douglasPeucker(const QVector<QPointF>& data, int start, int end, double epsilon, QVector<QPointF>& result) {
//     // 如果线段只有起点和终点两个点（或更少），则无需进一步处理
//     if (end - start < 2) return;

//     // 寻找线段data[start]到data[end]之间，距离该线段最远的那个点
//     double maxDist = 0;
//     int maxIdx = start + 1; // 初始化索引，循环从start+1开始
//     for (int i = start + 1; i < end; ++i) { // 遍历起点和终点之间的所有点
//         double dist = perpendicularDistance(data[i], data[start], data[end]); // 计算距离
//         if (dist > maxDist) { // 更新最大距离和对应的索引
//             maxDist = dist;
//             maxIdx = i;
//         }
//     }

//     // 如果找到的最远点距离大于设定的阈值epsilon，则进行递归处理
//     if (maxDist > epsilon) {
//         // 递归处理第一部分：从start到maxIdx
//         douglasPeucker(data, start, maxIdx, epsilon, result);
//         // 将最远点（关键点）添加到结果中
//         result.append(data[maxIdx]);  // 注意：避免重复添加
//         // 递归处理第二部分：从maxIdx到end
//         douglasPeucker(data, maxIdx, end, epsilon, result);
//     }
//     // 如果最远点距离小于等于epsilon，则说明这条线段足够平滑，可以由端点data[start]和data[end]代表，不需要再添加中间的点
// }


// /*
//  * 传入原始数据: 把你想要简化的那条曲线的所有原始点，放在一个 QVector<QPointF> 类型的变量里，作为第一个参数 points 传进去。
// 设置简化程度:
// epsRatio (比例系数): 一个 double 数值，比如 0.01 (1%) 或 0.05 (5%)。这个值越大，简化得越厉害（输出点越少）；越小则保留更多细节（输出点越多）。它决定了 EPSILON 与数据范围的比例。
// epsMin (最小阈值): 一个 double 数值，比如 0.1 或 1.0。这是一个 EPSILON 的下限，防止简化过度。即使你的数据范围很小，EPSILON 也不会小于这个值。
// 获取简化结果: 函数会返回一个新的 QVector<QPointF>，里面包含了简化后剩下的关键点。
// */


// /*举例
//  * range= 10
//  * epsRatio  = 0.02
//  * EPSILON = 10 * 0.02 = 0.2
//  * y变化小于0.2，被平滑
//  */

// // 封装：动态 EPSILON 简化曲线
// QVector<QPointF> simplifyCurve(const QVector<QPointF>& points, double epsRatio, double epsMin) {
//     // 如果输入点的数量少于3个，无法构成需要简化的曲线，则直接返回原点集
//     if (points.size() < 3)
//         return points; // 点太少不用简化

//     // 计算 y 值范围
//     // 初始化最小值和最大值为第一个点的y坐标
//     double minY = points.first().y();
//     double maxY = points.first().y();
//     // 遍历所有点，找到y坐标的最小值和最大值
//     for (const auto& pt : points) {
//         minY = qMin(minY, pt.y()); // 更新最小y值
//         maxY = qMax(maxY, pt.y()); // 更新最大y值
//     }

//     // 计算y方向的数值范围
//     double range = maxY - minY;
//     // 动态计算Epsilon阈值：取 (范围 * 比例系数) 和 最小阈值 中的较大者
//     double EPSILON = qMax(range * epsRatio, epsMin);

//     // 创建用于存储简化后结果的容器，并先加入第一个点
//     QVector<QPointF> simplified;
//     simplified.append(points.first());
//     // 调用递归函数对曲线进行简化
//     douglasPeucker(points, 0, points.size() - 1, EPSILON, simplified);
//     // 递归结束后，将最后一个点加入结果（因为递归函数内部没有添加最后一个点）
//     simplified.append(points.last());

//     // 返回简化后的点集
//     return simplified;
// }


#include <QVector>
#include <QPointF>
#include <QtMath>  // 用于 qSqrt, qPow, qMin, qMax, qBound 等

// 辅助函数：计算点到线段的距离（原代码，未改动）
double perpendicularDistance(const QPointF& pt, const QPointF& lineStart, const QPointF& lineEnd) {
    // 计算线段的向量 (dx, dy)
    double dx = lineEnd.x() - lineStart.x();
    double dy = lineEnd.y() - lineStart.y();
    // 如果线段起点和终点重合，则距离为0
    if (dx == 0 && dy == 0) return 0;
    // 计算点pt到线段所在直线的垂足在线段上的投影参数t
    // t = 0 表示垂足是线段起点，t = 1 表示是终点，0<t<1表示在线段内部
    double t = ((pt.x() - lineStart.x()) * dx + (pt.y() - lineStart.y()) * dy) / (dx * dx + dy * dy);
    // 将t约束在[0, 1]范围内，确保投影点在线段上而不是直线上
    t = qBound(0.0, t, 1.0);
    // 根据投影参数t计算线段上的投影点坐标(projX, projY)
    double projX = lineStart.x() + t * dx;
    double projY = lineStart.y() + t * dy;
    // 返回点pt与线段上投影点(projX, projY)之间的欧几里得距离
    return qSqrt(qPow(pt.x() - projX, 2) + qPow(pt.y() - projY, 2));
}

// 递归Douglas-Peucker（原代码，未改动）
void douglasPeucker(const QVector<QPointF>& data, int start, int end, double epsilon, QVector<QPointF>& result) {
    // 如果线段只有起点和终点两个点（或更少），则无需进一步处理
    if (end - start < 2) return;
    // 寻找线段data[start]到data[end]之间，距离该线段最远的那个点
    double maxDist = 0;
    int maxIdx = start + 1; // 初始化索引，循环从start+1开始
    for (int i = start + 1; i < end; ++i) { // 遍历起点和终点之间的所有点
        double dist = perpendicularDistance(data[i], data[start], data[end]); // 计算距离
        if (dist > maxDist) { // 更新最大距离和对应的索引
            maxDist = dist;
            maxIdx = i;
        }
    }
    // 如果找到的最远点距离大于设定的阈值epsilon，则进行递归处理
    if (maxDist > epsilon) {
        // 递归处理第一部分：从start到maxIdx
        douglasPeucker(data, start, maxIdx, epsilon, result);
        // 将最远点（关键点）添加到结果中
        result.append(data[maxIdx]);  // 注意：避免重复添加
        // 递归处理第二部分：从maxIdx到end
        douglasPeucker(data, maxIdx, end, epsilon, result);
    }
    // 如果最远点距离小于等于epsilon，则说明这条线段足够平滑，可以由端点data[start]和data[end]代表，不需要再添加中间的点
}

// 这个函数：滤过“变化很小的点”（相邻或从上一个保留点的 delta y < 阈值）
// 逻辑：从第一个点开始，遍历后续点，如果当前点 y 与上一个保留点的 y 差 >= minDelta，则保留；否则去除（相当于合并小变化为直线）
// 始终保留首尾点，确保曲线完整。即使尾部小变化，也包括最后一个点（避免曲线截断）
// minDelta 可以设为 EPSILON（e.g., 1），这样 <1 的变化被滤除
// 优点：处理高频小噪声（e.g., 幅度小但频率高的波动，会被合并）；累计变化大时会保留
// 示例：range=1000, epsRatio=0.001, minDelta=1，则相邻变化<1的点被去除
QVector<QPointF> filterSmallChanges(const QVector<QPointF>& points, double minDelta) {
    if (points.size() < 2) return points;  // 点太少，直接返回
    QVector<QPointF> filtered;
    filtered.append(points.first());  // 始终添加第一个点
    for (int i = 1; i < points.size() - 1; ++i) {  // 遍历到倒数第二个点（最后一个单独处理）
        double deltaY = qAbs(points[i].y() - filtered.last().y());  // 计算与上一个保留点的 y 差（绝对值）
        if (deltaY >= minDelta) {  // 如果变化 >= 阈值，则保留该点
            filtered.append(points[i]);
        }
        // 否则跳过，不添加（小变化被滤除，相当于从上一个保留点直线到下一个大变化点）
    }
    // 始终添加最后一个点（即使与上一个保留点的 delta < minDelta，也添加，以保持曲线末端）
    // 如果你想严格滤除尾部小变化，可以加判断：if (qAbs(points.last().y() - filtered.last().y()) >= minDelta)
    // 但推荐始终添加，以避免曲线意外截断
    if (!filtered.isEmpty() && points.last() != filtered.last()) {
        filtered.append(points.last());
    }
    return filtered;
}

// 封装：动态 EPSILON 简化曲线（修改版：添加预过滤步骤）
QVector<QPointF> simplifyCurve(const QVector<QPointF>& points, double epsRatio, double epsMin) {
    // 如果输入点的数量少于3个，无法构成需要简化的曲线，则直接返回原点集
    if (points.size() < 3)
        return points; // 点太少不用简化
    // 计算 y 值范围
    // 初始化最小值和最大值为第一个点的y坐标
    double minY = points.first().y();
    double maxY = points.first().y();
    // 遍历所有点，找到y坐标的最小值和最大值
    for (const auto& pt : points) {
        minY = qMin(minY, pt.y()); // 更新最小y值
        maxY = qMax(maxY, pt.y()); // 更新最大y值
    }
    // 计算y方向的数值范围
    double range = maxY - minY;
    // 动态计算Epsilon阈值：取 (范围 * 比例系数) 和 最小阈值 中的较大者
    double EPSILON = qMax(range * epsRatio, epsMin);
    // 先进行预过滤：使用 EPSILON 作为 minDelta，滤除小变化点（你的初衷）
    // 这步会在 DP 前去除小跳动，提高简化效果，尤其对小波动区
    QVector<QPointF> filteredPoints = filterSmallChanges(points, EPSILON);
    // 创建用于存储简化后结果的容器，并先加入第一个点
    QVector<QPointF> simplified;
    simplified.append(filteredPoints.first());
    // 调用递归函数对过滤后的曲线进行 DP 简化
    douglasPeucker(filteredPoints, 0, filteredPoints.size() - 1, EPSILON, simplified);
    // 递归结束后，将最后一个点加入结果（因为递归函数内部没有添加最后一个点）
    simplified.append(filteredPoints.last());
    // 返回简化后的点集
    return simplified;
}

