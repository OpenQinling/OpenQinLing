#include "ledgroupwindow.h"
#include "ui_ledgroupwindow.h"
#include <QCloseEvent>
#include <QPainter>
LedGroupWindow::LedGroupWindow(int count,QColor ledColor,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LedGroupWindow)
{
    ui->setupUi(this);
    this->count = count;
    this->ledColor = ledColor;

    int ledDrawSize = ledSize*2;
    int w = count>8 ? ledDrawSize*8 : ledDrawSize*count;
    int h = count>8 ? ledDrawSize*2 : ledDrawSize;

    this->resize(w,h);
    this->setMinimumSize(w,h);
    this->setMaximumSize(w,h);
    ui->label->resize(w,h);

    ledStates.resize(count);
    for(int i = 0;i<ledStates.length();i++){
        ledStates[i] = 0;//初始状态全部为关闭
    }
    flashDisplay();

    connect(&flashTimer,&QTimer::timeout,[=](){
        if(!isWriting){
            flashDisplay();
        }
    });
    flashTimer.start(16);
}

LedGroupWindow::~LedGroupWindow()
{
    flashTimer.stop();
    delete ui;
}

void LedGroupWindow::closeEvent(QCloseEvent *event){
    if(!isAllowClose){
        event->ignore();
    }
}

QPixmap LedGroupWindow::drawLedPixmap(int index){
    int ledDrawSize = ledSize*2;
    QPixmap background(ledDrawSize,ledDrawSize);
    background.fill(QColor(255,255,255));
    QPainter painter(&background);
    if(ledStates[index]>0){
        int r = (float)ledColor.red() * (float)ledStates[index] / 255.0f;
        int g = (float)ledColor.green() * (float)ledStates[index] / 255.0f;
        int b = (float)ledColor.blue() * (float)ledStates[index] / 255.0f;
        painter.setBrush(QColor(r,g,b));
        QPen pen(QColor(0,0,0));
        pen.setWidth(ledSize/10);
        painter.setPen(pen);
        painter.drawEllipse(ledSize/2,ledSize/2,ledSize,ledSize);
    }else{
        painter.setBrush(QColor(0,0,0));
        QPen pen(QColor(0,0,0));
        pen.setWidth(0);
        painter.setPen(pen);
        painter.drawEllipse(ledSize/2,ledSize/2,ledSize,ledSize);
    }
    painter.end();
    return background;
}

void LedGroupWindow::flashDisplay(){
    QPixmap background(ui->label->width(),ui->label->height());
    QPainter painter(&background);
    int ledDrawSize = ledSize*2;
    for(int i = 0;i<count;i++){
        QPixmap led = drawLedPixmap(i);
        int wC = (i%8);//当前led是一行中的第几个
        int hC = i/8;//当前led是第几行
        painter.drawPixmap(wC*ledDrawSize,hC*ledDrawSize,led);
    }
    painter.end();
    ui->label->setPixmap(background);
}
