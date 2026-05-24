#include "RoundedProgressBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>

RoundedProgressBar::RoundedProgressBar(QWidget *parent)
    : QProgressBar(parent)
{
    setFixedHeight(45);
    setTextVisible(true);
    setOrientation(Qt::Horizontal);
}

void RoundedProgressBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int borderRadius = 22;
    int borderWidth = 3;
    QRect rect = this->rect();
    
    QPainterPath borderPath;
    borderPath.addRoundedRect(rect, borderRadius, borderRadius);
    painter.fillPath(borderPath, QColor("#E0E0E0"));
    
    QRect contentRect = rect.adjusted(borderWidth, borderWidth, -borderWidth, -borderWidth);
    QPainterPath contentPath;
    int contentRadius = borderRadius - borderWidth;
    contentPath.addRoundedRect(contentRect, contentRadius, contentRadius);
    painter.fillPath(contentPath, QColor("#F5F5F5"));
    
    double progress = (double)value() / maximum();
    if (progress > 0) {
        int chunkWidth = (int)(contentRect.width() * qMin(progress, 1.0));
        QRect chunkRect(contentRect.left(), contentRect.top(), chunkWidth, contentRect.height());
        
        QLinearGradient gradient(0, 0, contentRect.width(), 0);
        gradient.setColorAt(0, QColor("#2196F3"));
        gradient.setColorAt(1, QColor("#4CAF50"));
        
        QPainterPath chunkPath;
        if (progress >= 1.0) {
            chunkPath.addRoundedRect(chunkRect, contentRadius, contentRadius);
        } else {
            int r = contentRadius;
            QPainterPath fullPath;
            fullPath.addRoundedRect(chunkRect, contentRadius, contentRadius);
            
            QPainterPath rightClip;
            rightClip.addRect(chunkRect.left() + chunkWidth, chunkRect.top(), 
                             chunkRect.width() - chunkWidth + 1, chunkRect.height());
            
            chunkPath = fullPath.subtracted(rightClip);
        }
        
        painter.fillPath(chunkPath, QBrush(gradient));
    }
    
    int displayValue = qMax(0, value());
    QString percentText = QString("%1%").arg(displayValue);
    painter.setFont(QFont("Arial", 15, QFont::Bold));
    painter.setPen(QColor("#333333"));
    painter.drawText(contentRect, Qt::AlignCenter, percentText);
}

void RoundedProgressBar::setValue(int value)
{
    QProgressBar::setValue(value);
    update();
}
