#include "displaywindow.h"
#include "ui_displaywindow.h"
#include "math.h"
#include <systemdependfunction.h>
DisplayWindow::DisplayWindow(QSize displaySize,uint8_t*mouseInputReg,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DisplayWindow)
{
    ui->setupUi(this);
    ui->label->setMouseTracking(true);
    setMouseTracking(true);
    this->mouseInputReg = mouseInputReg;

    displayMap = QImage(displaySize,QImage::Format_RGB888);
    displayMap.fill(QColor(0,0,0));

    if(displaySize.width()<100 || displaySize.height()<100){
        double wScale = 100 / (double)displaySize.width();
        double hScale = 100 / (double)displaySize.height();
        displaySize*=fmax(wScale,hScale);
    }

    resize(displaySize);

    connect(&this->flashDisplayTimer,&QTimer::timeout,[=](){
        if(!isWritingVRAM){
            flashDisplay();
        }
    });

    titleText = QString::number(displaySize.width())+"x"+QString::number(displaySize.height())+"显示屏";
    setWindowTitle(titleText);
    flashDisplayTimer.start(1);
}

DisplayWindow::~DisplayWindow()
{
    delete ui;
}


void DisplayWindow::flashDisplay(){
    QPixmap tmp = QPixmap::fromImage(displayMap);
    this->ui->label->setPixmap(tmp.scaled(this->width(),this->height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void DisplayWindow::resizeEvent(QResizeEvent *event){
    QSize size = event->size();

    QSize mapSize = displayMap.size();

    double wScale = (double)size.width() / (double)mapSize.width();
    double hScale = (double)size.height() / (double)mapSize.height();

    ui->label->resize(mapSize*fmin(hScale,wScale));

    ui->label->move(this->width()/2 - ui->label->width()/2,this->height()/2 - ui->label->height()/2);

    flashDisplay();
}

void DisplayWindow::levelDisplayHandle(){
    mouseIsInDisplay = 0;
    setWindowTitle(titleText);
    mouseInputReg[8] = 0;
}

void DisplayWindow::leaveEvent(QEvent *event){
    mouseIsInDisplay = 1;
    mouseInputReg[8] = 0x00;
}

void DisplayWindow::enterEvent(QEvent *event){
    mouseIsInDisplay = 1;
    mouseInputReg[8] = 0xff;
}

void DisplayWindow::flashMousePos(QMouseEvent *event){
    double x = event->x()-ui->label->x();
    double y = event->y()-ui->label->y();

    double wScale = (double)ui->label->width() / (double)this->displayMap.width();
    double hScale = (double)ui->label->height() / (double)this->displayMap.height();

    double scale = fmin(hScale,wScale);

    x /= scale;
    y /= scale;

    short ix = x;
    short iy = y;
    WORD wx(ix);
    WORD wy(iy);
    mouseInputReg[0] = wx[0];
    mouseInputReg[1] = wx[1];

    mouseInputReg[2] = wy[0];
    mouseInputReg[3] = wy[1];
}
void DisplayWindow::mousePressEvent(QMouseEvent *event){
    if(event->button() ==Qt::LeftButton){
        mouseInputReg[4] = 0xff;
    }else if(event->button() ==Qt::RightButton){
        mouseInputReg[5] = 0xff;
    }else if(event->button() ==Qt::MiddleButton){
        mouseInputReg[6] = 0xff;
    }
    flashMousePos(event);
}

void DisplayWindow::mouseMoveEvent(QMouseEvent *event){
    if(!mouseIsInDisplay){
        return;
    }
    flashMousePos(event);
}

void DisplayWindow::mouseReleaseEvent(QMouseEvent *event){
    if(!mouseIsInDisplay){
        return;
    }


    if(event->button() ==Qt::LeftButton){
        mouseInputReg[4] = 0;
    }else if(event->button() ==Qt::RightButton){
        mouseInputReg[5] = 0;
    }else if(event->button() ==Qt::MiddleButton){
        mouseInputReg[6] = 0;
    }
    flashMousePos(event);
}


void DisplayWindow::wheelEvent(QWheelEvent *event){
    if(!mouseIsInDisplay){
        return;
    }
    if(event->angleDelta().y()>0){
        mouseInputReg[7]++;
    }else if(event->angleDelta().y()<0){
        mouseInputReg[7]--;
    }
}

void DisplayWindow::closeEvent(QCloseEvent *event){
    if(!isAllowClose){
        event->ignore();
    }
}
