#ifndef WIDGET_H
#define WIDGET_H

#include <QTimer>
#include <QTime>
#include <QLCDNumber>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
private:
    QTimer *myTimer;
    QLCDNumber *myLCDNumber;

//private slots:
//    void showTime();  //顯示時間的函式
};

#endif 
