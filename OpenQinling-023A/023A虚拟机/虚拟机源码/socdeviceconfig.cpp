#include "socdeviceconfig.h"
#include "mainwindow.h"
#include <QElapsedTimer>
SocDeviceConfig::SocDeviceConfig(QWidget *parent,QList<DeviceMode> *devPtr) : QLabel(parent)
{
    devModeList = devPtr;
    drawDisplayPixmap();
    flashDisplay();
}

void SocDeviceConfig::mouseDoubleClickEvent(QMouseEvent *event){
    double x = event->x()-this->width()/2;
    double y = event->y()-this->height()/2;
    x/=scale;
    y/=scale;
    x = displayX+x;
    y = displayY+y;
    x += displayPixmap.width()/2;
    y += displayPixmap.height()/2;
    QList<DeviceMode> &allDeviceIcon = *devModeList;
    for(int i = 0;i<allDeviceIcon.length();i++){
        if(allDeviceIcon[i].point.x()<=x &&
           x<=allDeviceIcon[i].point.x()+allDeviceIcon[i].iconMap.width()&&
           allDeviceIcon[i].point.y()<=y &&
           y<=allDeviceIcon[i].point.y()+allDeviceIcon[i].iconMap.height()){
            mainWindow->showDevieInfoPage(i);
        }
    }
}

SocDeviceConfig::~SocDeviceConfig(){

}


//画一个等边三角形
void SocDeviceConfig::drawTriangle(QPainter &pter,int x,int y,int size,TriangleDir dir){
    int d1X,d1Y;
    int d2X,d2Y;
    int d3X,d3Y;

    if(dir==Up){
        d1X = x;
        d1Y = y-size;

        d2Y = y+size;
        d3Y = d2Y;

        d2X = x-size;
        d3X = x+size;

    }else if(dir==Down){
        d1X = x;
        d1Y = y+size;

        d2Y = y-size;
        d3Y = d2Y;

        d2X = x-size;
        d3X = x+size;
    }else if(dir==Left){
        d1X = x-size;
        d1Y = y;

        d2X = x+size;
        d3X = d2X;

        d2Y = y-size;
        d3Y = y+size;

    }else if(dir==Right){
        d1X = x+size;
        d1Y = y;

        d2X = x-size;
        d3X = d2X;

        d2Y = y-size;
        d3Y = y+size;
    }

    QPoint ps[3] = {{d1X,d1Y},{d2X,d2Y},{d3X,d3Y}};
    pter.drawPolygon(ps,3);
}

//绘制一条导线
QPoint SocDeviceConfig::drawCircuitLine(QPoint startPoint,int length,int angle,QPainter&painter,int lastAnale){
    QPoint endPoint;
    int offset = 30;

    if(angle==0){
        endPoint.setX(startPoint.x() + length);
        endPoint.setY(startPoint.y());
    }else if(angle==90){
        endPoint.setX(startPoint.x());
        endPoint.setY(startPoint.y()-length);
    }else if(angle==180){
        endPoint.setX(startPoint.x() - length);
        endPoint.setY(startPoint.y());
    }else if(angle==270){
        endPoint.setX(startPoint.x());
        endPoint.setY(startPoint.y()+length);
    }else if(angle==45){
        endPoint.setX(startPoint.x()+length);
        endPoint.setY(startPoint.y()-length);
    }else if(angle==135){
        endPoint.setX(startPoint.x()-length);
        endPoint.setY(startPoint.y()-length);
    }else if(angle==225){
        endPoint.setX(startPoint.x()-length);
        endPoint.setY(startPoint.y()+length);
    }else if(angle==315){
        endPoint.setX(startPoint.x()+length);
        endPoint.setY(startPoint.y()+length);
    }

    if(lastAnale==90 && angle==135){
        startPoint.setX(startPoint.x()-offset*0.5);
        startPoint.setY(startPoint.y()-offset);
    }else if(lastAnale==135 && angle==180){
        startPoint.setX(startPoint.x()-offset);
        startPoint.setY(startPoint.y()-offset*0.5);
        endPoint.setY(endPoint.y()-offset*0.5);
    }else if(lastAnale==135 && angle==90){
        startPoint.setX(startPoint.x()-offset*0.5);
        startPoint.setY(startPoint.y()-offset);
        endPoint.setX(endPoint.x()-offset*0.5);
    }else if(lastAnale==180 && angle==135){
        startPoint.setX(startPoint.x()-offset);
        startPoint.setY(startPoint.y()-offset*0.5);
    }

    if(lastAnale==90 && angle==45){
        startPoint.setX(startPoint.x()+offset*0.5);
        startPoint.setY(startPoint.y()-offset);
    }else if(lastAnale==45 && angle==0){
        startPoint.setX(startPoint.x()+offset);
        startPoint.setY(startPoint.y()-offset*0.5);
        endPoint.setY(endPoint.y()-offset*0.5);
    }else if(lastAnale==45 && angle==90){
        startPoint.setX(startPoint.x()+offset*0.5);
        startPoint.setY(startPoint.y()-offset);
        endPoint.setX(endPoint.x()+offset*0.5);
    }else if(lastAnale==0 && angle==45){
        startPoint.setX(startPoint.x()+offset);
        startPoint.setY(startPoint.y()-offset*0.5);
    }

    if(lastAnale==270 && angle==225){
        startPoint.setX(startPoint.x()-offset*0.5);
        startPoint.setY(startPoint.y()+offset);
    }else if(lastAnale==225 && angle==180){
        startPoint.setX(startPoint.x()-offset);
        startPoint.setY(startPoint.y()+offset*0.5);
        endPoint.setY(endPoint.y()+offset*0.5);
    }else if(lastAnale==225 && angle==270){
        startPoint.setX(startPoint.x()-offset*0.5);
        startPoint.setY(startPoint.y()+offset);
        endPoint.setX(endPoint.x()-offset*0.5);
    }else if(lastAnale==180 && angle==225){
        startPoint.setX(startPoint.x()-offset);
        startPoint.setY(startPoint.y()+offset*0.5);
    }

    if(lastAnale==270 && angle==315){
        startPoint.setX(startPoint.x()+offset*0.5);
        startPoint.setY(startPoint.y()+offset);
    }else if(lastAnale==315 && angle==0){
        startPoint.setX(startPoint.x()+offset);
        startPoint.setY(startPoint.y()+offset*0.5);
        endPoint.setY(endPoint.y()+offset*0.5);
    }else if(lastAnale==315 && angle==270){
        startPoint.setX(startPoint.x()+offset*0.5);
        startPoint.setY(startPoint.y()+offset);
        endPoint.setX(endPoint.x()+offset*0.5);
    }else if(lastAnale==0 && angle==315){
        startPoint.setX(startPoint.x()+offset);
        startPoint.setY(startPoint.y()+offset*0.5);
    }

    painter.drawLine(startPoint,endPoint);
    return endPoint;
}

//绘制显示的图像
void SocDeviceConfig::drawDisplayPixmap(){

    QList<DeviceMode> &allDev = *devModeList;
    int groupCount = (allDev.length()-2)/4;
    int remCount = (allDev.length()-2)%4;
    int leftUpCount = groupCount;//左上方
    int leftDownCount = groupCount;//左下方
    int rightUpCount = groupCount;//右上方
    int rightDownCount = groupCount;//右下方

    int h = 2000+(groupCount+(remCount>0?1:0))*780;
    int w = 4000;
    minX = -w/2;
    minY = -h/2;
    maxX = w/2;
    maxY = h/2;
    displayPixmap = QPixmap(w,h);
    displayPixmap.fill(QColor(0,0,0,0));

    QPainter painter(&displayPixmap);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    //painter.setBrush(QColor(0,100,90,255));
    //painter.drawRect(0,0,w,h);

    QPen pen(QColor(180,181,179,255));

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);


    //isRunning = 1;
    //绘制电路
    QColor circuitColor;
    if(isRunning){
        circuitColor = QColor(255,0,0);
    }else{
        circuitColor = QColor(0,0,50);
    }
    pen = QPen(circuitColor);
    pen.setWidth(60);
    painter.setPen(pen);
    painter.setBrush(circuitColor);

    int cpuX = w/2-allDev[0].iconMap.width()/2;
    int cpuY = h/2-allDev[0].iconMap.height()/2;

    //绘制cpu到内部rom的电路
    painter.drawLine(cpuX+250,
                     cpuY,
                     cpuX+250,
                     cpuY - allDev[0].iconMap.height()+100);

    drawTriangle(painter,cpuX+250,cpuY,70,Up);


    //获取电路的四个方向需要画多少条分支线路
    QList<QPoint> allDeviceDrawPoint;

    if(remCount>0){
        leftUpCount++;
        remCount--;
    }
    if(remCount>0){
        leftDownCount++;
        remCount--;
    }
    if(remCount>0){
        rightUpCount++;
        remCount--;
    }

    QList<QPoint> tmpEndPoint;
    //为各方向绘制干线
    if(leftUpCount>0 || leftDownCount>0){
        tmpEndPoint.append(drawCircuitLine(QPoint(cpuX,cpuY+250),180,180,painter));
    }
    if(leftUpCount>0){
        tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,135,painter,180));

        for(int i = 0;i<leftUpCount;i++){

            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],i==0 ? 100 : 400,90,painter,i==0 ? 135 : -1));
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,135,painter,90));
            allDeviceDrawPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],180,180,painter,135));
            tmpEndPoint.removeLast();
        }
        for(int i = 0;i<leftUpCount;i++){
            tmpEndPoint.removeLast();
        }

        tmpEndPoint.removeLast();
    }

    if(leftDownCount>0){
        tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,225,painter,180));
        for(int i = 0;i<leftDownCount;i++){
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],i==0 ? 100 : 400,270,painter,i==0 ? 225 : -1));
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,225,painter,270));
            allDeviceDrawPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],180,180,painter,225));
            tmpEndPoint.removeLast();
        }
        for(int i = 0;i<leftDownCount;i++){
            tmpEndPoint.removeLast();
        }

        tmpEndPoint.removeLast();
    }
    if(leftUpCount>0 || leftDownCount>0){
        tmpEndPoint.removeLast();
    }
    if(rightUpCount>0 || rightDownCount>0){
        tmpEndPoint.append(drawCircuitLine(QPoint(cpuX+500,cpuY+250),180,0,painter));
    }
    if(rightUpCount>0){
        tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,45,painter,0));
        for(int i = 0;i<rightUpCount;i++){
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],i==0 ? 100 : 400,90,painter,i==0 ? 45 : -1));
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,45,painter,90));
            allDeviceDrawPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],180,0,painter,45));
            tmpEndPoint.removeLast();
        }
        for(int i = 0;i<rightUpCount;i++){
            tmpEndPoint.removeLast();
        }

        tmpEndPoint.removeLast();
    }
    if(rightDownCount>0){
        tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,315,painter,0));
        for(int i = 0;i<rightDownCount;i++){
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],i==0 ? 100 : 400,270,painter,i==0 ? 315 : -1));
            tmpEndPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],130,315,painter,270));
            allDeviceDrawPoint.append(drawCircuitLine(tmpEndPoint[tmpEndPoint.length()-1],180,0,painter,315));
            tmpEndPoint.removeLast();
        }
        for(int i = 0;i<rightDownCount;i++){
            tmpEndPoint.removeLast();
        }

        tmpEndPoint.removeLast();
    }
    if(rightUpCount>0 || rightDownCount>0){
        tmpEndPoint.removeLast();
    }

    //绘制外部设备
    for(int i = 0;i<allDev.length()-2;i++){
        DeviceMode & icon = allDev[i+2];
        allDeviceDrawPoint[i].setY(allDeviceDrawPoint[i].y()-icon.iconMap.height()/2);
        if(i<leftDownCount+leftUpCount){
            allDeviceDrawPoint[i].setX(allDeviceDrawPoint[i].x()-icon.iconMap.width());
        }

        icon.point = allDeviceDrawPoint[i];
        painter.drawPixmap(icon.point,
                            icon.iconMap);

    }

    //绘制CPU
    painter.drawPixmap(cpuX,
                       cpuY,allDev[0].iconMap);
    allDev[0].point = QPoint(cpuX,cpuY);
    //绘制内置ROM
    painter.drawPixmap(cpuX+100,
                       cpuY - allDev[0].iconMap.height()-100,
                        allDev[1].iconMap);
    allDev[1].point = QPoint(cpuX+100,
                             cpuY - allDev[0].iconMap.height()-100);

    painter.end();
}

//刷新显示的内容
void SocDeviceConfig::flashDisplay(){
    double displayW = this->width()/scale;
    double displayH = this->height()/scale;
    double x = displayX-(displayW/2)+(displayPixmap.width()/2);
    double y = displayY-(displayH/2)+(displayPixmap.height()/2);

    double diffX = 0;
    double diffY = 0;
    if(x<0){
        diffX = 0-x;
        x = 0;
    }
    if(y<0){
        diffY = 0-y;
        y = 0;
    }

    if(displayW+x>displayPixmap.width()){
        displayW = displayPixmap.width()-x;
    }
    if(displayH+y>displayPixmap.height()){
        displayH = displayPixmap.height()-y;
    }

    QPixmap background(this->width(),this->height());
    QColor(0,100,90,255);
    background.fill(QColor(220,255,220,255));


    QPainter painter(&background);
    if(displayW>0 && displayH>0){

        int threadNum = QThread::idealThreadCount();
        double segH = displayH/threadNum;
        QPixmap* allPixMap = new QPixmap[threadNum];


        ToolFunction::multithThreadExec([=](int index){
            //裁切要显示的部位
            allPixMap[index] = displayPixmap.copy(x,y+(ceil(segH)*index),displayW,ceil(segH));
            //缩放要显示的部位
            allPixMap[index] = allPixMap[index].scaled(displayW*scale,ceil(ceil(segH)*scale),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        },threadNum);

        int hoff = 0;
        for(int i = 0 ;i<threadNum;i++){
            painter.drawPixmap(diffX*scale,(diffY*scale)+hoff,allPixMap[i]);
            hoff+=allPixMap[i].height();
        }


        delete [] allPixMap;
    }



    this->setPixmap(background);
}

//设置为运行状态
bool SocDeviceConfig::start(){
    if(!isRunning){
        isRunning = 1;
        drawDisplayPixmap();
        flashDisplay();
        return 1;
    }
    return 0;
}

//设置为关闭状态
bool SocDeviceConfig::stop(){
    if(isRunning){
        isRunning = 0;
        drawDisplayPixmap();
        flashDisplay();
        return 1;
    }
    return 0;
}

//鼠标按下的信号
void SocDeviceConfig::mousePressEvent(QMouseEvent *event){
    lastMouseX = event->x();
    lastMouseY = event->y();
}

//鼠标移动信号[移动视角]
void SocDeviceConfig::mouseMoveEvent(QMouseEvent *event){
    if(lastMouseX==-1 || lastMouseY == -1){
        lastMouseX = event->x();
        lastMouseY = event->y();
    }else{
        int xDiff = event->x()-lastMouseX;
        int yDiff = event->y()-lastMouseY;

        displayX += xDiff/scale;
        displayY += yDiff/scale;

        lastMouseX = event->x();
        lastMouseY = event->y();
    }
    flashDisplay();
}

//鼠标滚轮信号处理[缩放渲染大小]
void SocDeviceConfig::wheelEvent(QWheelEvent *event){
    //滚动向前, 放大显示
    //滚动向后,缩小显示
    if(event->angleDelta().y()>0){
        scale*=1.05;
    }else if(event->angleDelta().y()<0){
        scale*=0.95;
    }
    if(scale<0.6){
        scale = 0.6;
    }else if(scale>1){
        scale = 1;
    }
    flashDisplay();
}

//视窗缩放事件
void SocDeviceConfig::resizeEvent(QResizeEvent*e){
    lastMouseX = -1;
    lastMouseY = -1;
    flashDisplay();
}
