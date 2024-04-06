#include "deviceinfowindow.h"
#include "ui_deviceinfowindow.h"
#include "mainwindow.h"
#include <QGraphicsBlurEffect>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <toolfunction.h>
#include <QThread>
#include <systemdependfunction.h>
DeviceInfoWindow::DeviceInfoWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceInfoWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_TranslucentBackground);

    InfoPage_w = ui->InfoPage->width();
    InfoPage_h = ui->InfoPage->height();

    ui->titleTextarea->installEventFilter(this);
    ui->closePageButton->installEventFilter(this);


    flashCloseButton();


    connect(ui->useMemBegin,&QLineEdit::editingFinished,[=](){
        QString textBegin = ui->useMemBegin->text();
        bool isSuceess;
        uint startAdd = textBegin.toUInt(&isSuceess,16);
        uint endAdd = startAdd+(this->devMode->useMemLength-1);
        if(endAdd<(startAdd)){
            ui->useMemBegin->setText(ToolFunction::transHexString(this->devMode->beginAddress));
            return;
        }

        uint32_t useMemLength = devMode->useMemLength;
        bool tryAlloc = ToolFunction::judgeContainBlockSet(mainWindow->getAllFreeMemBlock(this->devIndex),{BlockInfo({startAdd,useMemLength})});
        if(!tryAlloc){
            ui->useMemBegin->setText(ToolFunction::transHexString(this->devMode->beginAddress));
            return;
        }

        while(textBegin.length()<8){
            textBegin.prepend("0");
        }
        this->devMode->beginAddress = startAdd;
        textBegin = textBegin.right(8);
        ui->useMemBegin->setText("0x"+textBegin);



        QString textEnd = QString::number(endAdd,16);
        while(textEnd.length()<8){
            textEnd.prepend("0");
        }
        textEnd = textEnd.right(8);
        ui->useMemEnd->setText("0x"+textEnd);
        isHaveChange = 1;
        flashShowIcon();
    });

    connect(ui->interNumTextArea,&QLineEdit::editingFinished,[=](){
        bool suceess;
        int32_t num = ui->interNumTextArea->text().toInt(&suceess);
        if(!suceess || num<=15 || num>255){
            num = -1;
            ui->interNumTextArea->setText("不使用中断功能");
        }else if(!mainWindow->getAllFreeInterNum(this->devIndex).contains(num)){
            num = this->devMode->useInterNum;
            QString txt = QString::number(num);
            while (txt.length()<3) {
                txt.prepend("0");
            }
            ui->interNumTextArea->setText(txt);
        }else{
            QString numText = QString::number(num);
            while(numText.length()<3){
                numText.prepend("0");
            }
            numText = numText.right(3);
            ui->interNumTextArea->setText(numText);
        }

        if(num!=this->devMode->useInterNum){
            this->devMode->useInterNum = num;
            isHaveChange = 1;
            flashShowIcon();
        }

    });

    connect(ui->ModifyDataButton,&QPushButton::clicked,[=](){
        devMode->ModifyData();
    });

    connect(ui->DeleteDevButton,&QPushButton::clicked,[=](){
        //调用MOD卸载回调
        if(this->devMode->BlackoutDevice!=NULL){
            this->devMode->BlackoutDevice();
        }
        if(this->devMode->UnloadDeviceMod!=NULL){
            this->devMode->UnloadDeviceMod();
        }


        //卸载DLL
        SystemDependFunction::UnloadDynamicLibrary(this->devMode->dllHandler);

        //删除MOD的文件夹
        QDir dir(QApplication::applicationDirPath()+"/Data/Devices/"+this->devMode->devFolderName);
        dir.removeRecursively();

        //删除内存中的虚拟设备对象
        mainWindow->allUseDeviceMode.removeAt(this->devIndex);

        isHaveChange = 1;
        close();
    });

}

void DeviceInfoWindow::flashShowIcon(){
    devMode->crateIconMap();

    float devW = (float)ui->deviceIcon->width() / (float)devMode->iconMap.width();
    float devH = (float)ui->deviceIcon->height() / (float)devMode->iconMap.height();

    float scale = fminf(devW,devH);

    QPixmap map = devMode->iconMap.scaled(devMode->iconMap.width()*scale,
                                        devMode->iconMap.height()*scale,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
    ui->deviceIcon->setPixmap(map);

    ui->DeviceTypeText->setText(devMode->devType);
    ui->deviceFunctionInfoText->setText(devMode->devFunText);
}

void DeviceInfoWindow::showDeviceInfo(int index){
    devIndex = index;
    devMode = &mainWindow->allUseDeviceMode[index];
    ui->titleTextarea->setText(devMode->devName);

    if(devMode->devType=="CPU"){
        ui->deviceAddress->close();
        ui->DeleteDevButton->close();
    }else{
        if(devMode->devName=="InnerFLASH"){
            ui->DeleteDevButton->close();
            ui->useMemBegin->setReadOnly(1);
        }else{
            ui->DeleteDevButton->show();
            ui->useMemBegin->setReadOnly(0);
        }

        ui->deviceAddress->show();

        ui->useMemBegin->setText(ToolFunction::transHexString(devMode->beginAddress));
        ui->useMemEnd->setText(ToolFunction::transHexString(devMode->beginAddress+devMode->useMemLength-1));
    }

    if(devMode->ModifyData==NULL){
        ui->ModifyDataButton->close();
    }else{
        ui->ModifyDataButton->show();
    }

    if(devMode->isSupportInterruptAsk){
        ui->InterNum->show();
        if(devMode->useInterNum<=15 || devMode->useInterNum>=256){
            ui->interNumTextArea->setText("不使用中断功能");
        }else{
            QString txt = QString::number(devMode->useInterNum);
            while(txt.length()<3){
                txt.prepend("0");
            }

            ui->interNumTextArea->setText(txt);
        }

    }else{
        ui->InterNum->close();
    }

    flashShowIcon();
    isHaveChange = 0;
    show();
}
void DeviceInfoWindow::flashCloseButton(int isHover){
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
DeviceInfoWindow::~DeviceInfoWindow()
{
    delete ui;
}

void DeviceInfoWindow::showEvent(QShowEvent *event){
    InfoPage_xOffset = 0;
    InfoPage_yOffset = 0;
    ui->InfoPage->move(this->width()/2 - InfoPage_w/2,this->height()/2 - InfoPage_h/2);
}
void DeviceInfoWindow::closeEvent(QCloseEvent *event){
    if(isHaveChange){
        mainWindow->vriCPU->restart();
        mainWindow->flashDevConfigWindow();
        mainWindow->flashShowMemData();
        mainWindow->flashShowRegValue();
    }
}

void DeviceInfoWindow::moveInfoPage(){
    if(InfoPage_xOffset+this->width()/2 - InfoPage_w/2 <0){
        InfoPage_xOffset = 0- (this->width()/2 - InfoPage_w/2);
    }else if(InfoPage_xOffset+this->width()/2 + InfoPage_w/2 > this->width()){
        InfoPage_xOffset = this->width() - (this->width()/2 + InfoPage_w/2);
    }
    if(InfoPage_yOffset+this->height()/2 - InfoPage_h/2 <0){
        InfoPage_yOffset = 0 - (this->height()/2 - InfoPage_h/2);
    }else if(InfoPage_yOffset+this->height()/2 + InfoPage_h/2 > this->height()){
        InfoPage_yOffset = this->height() - (this->height()/2 + InfoPage_h/2);
    }

    ui->InfoPage->move(this->width()/2 - InfoPage_w/2 + InfoPage_xOffset,
                       this->height()/2 - InfoPage_h/2 + InfoPage_yOffset);
}

void DeviceInfoWindow::resizeEvent(QResizeEvent*e){
    moveInfoPage();
}
//事件处理器
bool DeviceInfoWindow::eventFilter(QObject*obj,QEvent*event){
    if(event->type() == QEvent::Enter && obj==ui->closePageButton){
        flashCloseButton(1);
        return true;
    }else if((event->type() == QEvent::Leave)&& obj==ui->closePageButton){
        flashCloseButton();
        return true;
    }else if((event->type() == QEvent::MouseButtonPress)&& obj==ui->closePageButton){
        this->close();
        return true;
    }else if(event->type() == QEvent::MouseButtonPress && obj==ui->titleTextarea){
        mouseLastX = -1;
        mouseLastY = -1;
        return true;
    }else if((event->type() == QEvent::MouseMove)&& obj==ui->titleTextarea){
        QMouseEvent * mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseLastX==-1 || mouseLastY==-1){
            mouseLastX = mouseEvent->x();
            mouseLastY = mouseEvent->y();
            return true;
        }
        int offX = mouseEvent->x() - mouseLastX;
        int offY = mouseEvent->y() - mouseLastY;

        InfoPage_xOffset += offX;
        InfoPage_yOffset += offY;
        moveInfoPage();
        return true;
    }
    return false;
}
