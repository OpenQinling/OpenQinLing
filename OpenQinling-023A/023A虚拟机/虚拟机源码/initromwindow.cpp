#include "initromwindow.h"
#include "ui_initromwindow.h"
#include <QMouseEvent>
#include <QFileDialog>
#include <QDebug>
#include <mainwindow.h>
InitRomWindow::InitRomWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InitRomWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_TranslucentBackground);
    MainWidget_w = ui->MainWidget->width();
    MainWidget_h = ui->MainWidget->height();
    ui->titleTextarea->installEventFilter(this);

    connect(ui->selectFileButton,&QPushButton::clicked,[=](){
        QJsonObject obj = settingJsonDoc.object();
        if(!obj.contains("SelectInitRomFile_defaultPath") || obj.value("SelectInitRomFile_defaultPath").type()!=QJsonValue::String){
            obj.insert("SelectInitRomFile_defaultPath",QApplication::applicationDirPath());
        }

        QString path = QFileDialog::getOpenFileName(this,"选择初始化文件",obj.value("SelectInitRomFile_defaultPath").toString(),"*.*");
        if(!path.isEmpty()){
            QFileInfo info(path);
            obj.insert("SelectInitRomFile_defaultPath",info.absolutePath());
            settingJsonDoc.setObject(obj);
            ui->selectEdit->setText(path);
        }
    });


    connect(ui->selectEdit,&QLineEdit::textChanged,[=](const QString&path){
        if(path==0){
            ui->stateInfo->setText("<未选择文件>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#ff0000;"
                                         "}");
            return;
        }

        bool isSuceess = 0;
        //解析选中的初始化文件信息
        QFile file(path);
        if(file.open(QFile::ReadOnly)){
            isSuceess = exeImge.fromEtbFileData(file.readAll());
        }

        if(isSuceess){
            ui->stateInfo->setText("<有效文件>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#009900;"
                                         "}");
        }else{
            ui->stateInfo->setText("<无效文件>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#ff0000;"
                                         "}");
        }
    });

    connect(ui->cancelButton,&QPushButton::clicked,[=](){
        ui->selectEdit->setText("");
        this->close();
    });

    connect(ui->okButton,&QPushButton::clicked,[=](){
        if(ui->stateInfo->text()=="<有效文件>"){
            for(int i = 0;i<exeImge.length();i++){
                if(!exeImge[i].isZiData){
                    for(uint j = 0;j<exeImge[i].len;j++){
                        mainWindow->vriCPU->WriteDevice(exeImge[i].data[j],j+exeImge[i].baseAdd);
                    }
                }
            }
            ui->selectEdit->setText("");
            mainWindow->vriCPU->restart();
            mainWindow->flashShowMemData();
            mainWindow->flashShowRegValue();
            this->close();
        }
    });
}

InitRomWindow::~InitRomWindow()
{
    delete ui;
}

void InitRomWindow::moveInfoPage(){
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
void InitRomWindow::showEvent(QShowEvent *event){
    MainWidget_xOffset = 0;
    MainWidget_yOffset = 0;
    ui->MainWidget->move(this->width()/2 - MainWidget_w/2,this->height()/2 - MainWidget_h/2);
}
void InitRomWindow::closeEvent(QCloseEvent *event){

}
void InitRomWindow::resizeEvent(QResizeEvent*e){
    moveInfoPage();
}
//事件处理器
bool InitRomWindow::eventFilter(QObject*obj,QEvent*event){
    if(event->type() == QEvent::MouseButtonPress && obj==ui->titleTextarea){
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
