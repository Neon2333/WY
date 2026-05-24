#ifndef ROUNDEDPROGRESSBAR_H
#define ROUNDEDPROGRESSBAR_H

#include <QProgressBar>
#include <QPaintEvent>

class RoundedProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    explicit RoundedProgressBar(QWidget *parent = nullptr);

    void setValue(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif