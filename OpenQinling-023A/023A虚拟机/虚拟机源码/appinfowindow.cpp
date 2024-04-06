#include "appinfowindow.h"
#include "ui_appinfowindow.h"
#include <QDesktopServices>
#include <QMouseEvent>
#include <QPainter>
AppInfoWindow::AppInfoWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppInfoWindow)
{
    ui->setupUi(this);

    MainWidget_w = ui->MainWidget->width();
    MainWidget_h = ui->MainWidget->height();
    ui->titleTextarea->installEventFilter(this);
    ui->closePageButton->installEventFilter(this);


    flashCloseButton();

    connect(ui->pushButton,&QPushButton::clicked,[=](){
        QDesktopServices::openUrl(ui->lineEdit->text());
    });
}

AppInfoWindow::~AppInfoWindow()
{
    delete ui;
}
void AppInfoWindow::moveInfoPage(){
    if(MainWidget_xOffset+this->width()/2 - MainWidget_w/2 <0){
        MainWidget_xOffset = 0- (this->width()/2 - MainWidget_w/2);
    }else if(MainWidget_xOffset+this->width()/2 + MainWidget_w/2 > this->width()){
        MainWidget_xOffset = this->width() - (this->width()/2 +MainWidget_w/2);
    }
    if(MainWidget_yOffset+this->height()/2 - MainWidget_h/2 <0){
        MainWidget_yOffset = 0 - (this->height()/2 - MainWidget_h/2);
    }else if(MainWidget_yOffset+this->height()/2 + MainWidget_h/2 > this->height()){
        MainWidget_yOffset = this->height() - (this->height()/2 + MainWidget_h/2);
    }

    ui->MainWidget->move(this->width()/2 - MainWidget_w/2 + MainWidget_xOffset,
                       this->height()/2 - MainWidget_h/2 + MainWidget_yOffset);
}
void AppInfoWindow::showEvent(QShowEvent *event){

    MainWidget_xOffset = 0;
    MainWidget_yOffset = 0;
    ui->MainWidget->move(this->width()/2 - MainWidget_w/2,this->height()/2 - MainWidget_h/2);
    flashCloseButton();
}
void AppInfoWindow::resizeEvent(QResizeEvent*e){
    moveInfoPage();
}

void AppInfoWindow::flashCloseButton(int isHover){
    QPixmap pixmap(ui->closePageButton->size());

    int w = ui->closePageButton->width()-10;
    int h = ui->closePageButton->height()-10;
    pixmap.fill(QColor(0,0,0,0));

    QColor color;
    if(!isHover){
        color = QColor(255,255,255,255);
    }else{
        color = QColor(100,100,100,255);
    }

    QPen pen(color);
    pen.setWidth(7);
    QPainter p(&pixmap);
    p.setPen(pen);
    p.drawLine(10,10,w,h);
    p.drawLine(w,10,10,h);
    p.end();

    ui->closePageButton->setPixmap(pixmap);

}
//事件处理器
bool AppInfoWindow::eventFilter(QObject*obj,QEvent*event){
    if(event->type() == QEvent::Enter && obj==ui->closePageButton){
        flashCloseButton(1);
        return true;
    }else if((event->type() == QEvent::Leave)&& obj==ui->closePageButton){
        flashCloseButton();
        return true;
    }else if((event->type() == QEvent::MouseButtonPress)&& obj==ui->closePageButton){
        this->close();
        return true;
    }if(event->type() == QEvent::MouseButtonPress && obj==ui->titleTextarea){
            mouseLastX = -1;
            mouseLastY = -1;
            return true;
    }if((event->type() == QEvent::MouseMove)&& obj==ui->titleTextarea){
        QMouseEvent * mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseLastX==-1 || mouseLastY==-1){
            mouseLastX = mouseEvent->x();
            mouseLastY = mouseEvent->y();
            return true;
        }
        int offX = mouseEvent->x() - mouseLastX;
        int offY = mouseEvent->y() - mouseLastY;

        MainWidget_xOffset += offX;
        MainWidget_yOffset += offY;
        moveInfoPage();
        return true;
    }
    return false;
}
