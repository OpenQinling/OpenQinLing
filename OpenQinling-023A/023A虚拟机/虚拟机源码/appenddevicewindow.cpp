#include "appenddevicewindow.h"
#include "ui_appenddevicewindow.h"
#include <QMouseEvent>
#include <mainwindow.h>
#include <QFileDialog>
#include <toolfunction.h>
#include <systemdependfunction.h>
//解MOD包
bool AppendDeviceWindow::packageModeInfo(QString zipPath){
    if(!ToolFunction::unzipPackage(zipPath,this->addModeInfo.unzipPath,this->addModeInfo.coreLibPath)){
        return 0;
    }

    return ToolFunction::getDevModInfo(this->addModeInfo.unzipPath,
                                       this->addModeInfo.coreLibPath,
                                       this->addModeInfo.typeName,
                                       this->addModeInfo.funInfo,
                                       this->addModeInfo.menLength,
                                       this->addModeInfo.isUseInter);
}

AppendDeviceWindow::AppendDeviceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppendDeviceWindow)
{
    ui->setupUi(this);
    MainWidget_w = ui->MainWidget->width();
    MainWidget_h = ui->MainWidget->height();
    ui->titleTextarea->installEventFilter(this);

    connect(ui->selectFileButton,&QPushButton::clicked,[=](){
        QJsonObject obj = settingJsonDoc.object();
        if(!obj.contains("SelectModPack_defaultPath") || obj.value("SelectModPack_defaultPath").type()!=QJsonValue::String){
            obj.insert("SelectModPack_defaultPath",QApplication::applicationDirPath()+"/Data/Library");
        }
        QString path = QFileDialog::getOpenFileName(this,"选择MOD包",obj.value("SelectModPack_defaultPath").toString(),"*.pck");
        if(!path.isEmpty()){
            QFileInfo info(path);
            obj.insert("SelectModPack_defaultPath",info.absolutePath());
            settingJsonDoc.setObject(obj);

            if(ui->selectEdit->text()==path){
                emit ui->selectEdit->textChanged(path);
            }else{
                ui->selectEdit->setText(path);
            }
        }
    });


    connect(ui->selectEdit,&QLineEdit::textChanged,[=](const QString&path){

        if(path.isEmpty()){
            ui->stateInfo->setText("<未选择文件>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#ff0000;"
                                         "}");
            ui->DeviceTypeText->setText("无效MOD");
            ui->deviceFunctionInfoText->setText("无");
            ui->DevConfigPage->close();
            this->addModeInfo.isValid = 0;

            return;
        }

        bool isSuceess = packageModeInfo(path);
        ui->deviceIcon->clear();
        ui->DeviceNameEdit->clear();

        if(isSuceess){

            ui->DevConfigPage->show();
            ui->stateInfo->setText("<有效包>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#009900;"
                                         "}");
            ui->DeviceTypeText->setText(this->addModeInfo.typeName);
            ui->deviceFunctionInfoText->setText(this->addModeInfo.funInfo);

            if(this->addModeInfo.isUseInter){
                ui->InterNum->show();
            }else{
                ui->InterNum->close();
            }

            QList<BlockInfo> allFreeBlock = mainWindow->getAllFreeMemBlock();
            bool suceess;
            BlockInfo allocBlock = ToolFunction::searchFreeMemarySpace(allFreeBlock,this->addModeInfo.menLength,&suceess);

            uint32_t beginAdd = allocBlock.baseAdress;
            uint32_t endAdd = allocBlock.baseAdress + allocBlock.sizeBytes - 1;


            if(suceess){
                ui->useMemBegin->setReadOnly(0);
                this->addModeInfo.allocMemAddress = beginAdd;
                ui->useMemBegin->setText(ToolFunction::transHexString(beginAdd));
                ui->useMemEnd->setText(ToolFunction::transHexString(endAdd));
                ui->MemAllocFailProm->close();
                ui->deviceAddress->show();
                ui->interNumTextArea->show();
                ui->interNumTextArea->setText("不使用中断功能");
                this->addModeInfo.allocInterNum = -1;
                this->devName = mainWindow->newDevDefaultName();
                ui->DeviceNameEdit->setText(this->devName);
                this->addModeInfo.isValid = 1;
                ui->deviceIcon->show();
                flashShowIcon();
            }else{
                ui->deviceIcon->close();
                ui->deviceAddress->close();
                ui->MemAllocFailProm->show();
                ui->interNumTextArea->close();
                this->addModeInfo.isValid = 0;
            }
        }else{
            ui->stateInfo->setText("<无效包>");
            ui->stateInfo->setStyleSheet("QWidget{"
                                         "color:#ff0000;"
                                         "}");
            ui->DeviceTypeText->setText("无效MOD");
            ui->deviceFunctionInfoText->setText("无");
            ui->DevConfigPage->close();
            this->addModeInfo.isValid = 0;
        }
    });

    connect(ui->useMemBegin,&QLineEdit::editingFinished,[=](){
        QString textBegin = ui->useMemBegin->text();
        bool isSuceess;
        uint startAdd = textBegin.toUInt(&isSuceess,16);
        uint endAdd = startAdd+(this->addModeInfo.menLength-1);
        if(endAdd<(startAdd)){
            ui->useMemBegin->setText(ToolFunction::transHexString(this->addModeInfo.allocMemAddress));
            return;
        }

        bool tryAlloc = ToolFunction::judgeContainBlockSet(mainWindow->getAllFreeMemBlock(),{BlockInfo({startAdd,this->addModeInfo.menLength})});
        if(!tryAlloc){
            ui->useMemBegin->setText(ToolFunction::transHexString(this->addModeInfo.allocMemAddress));
            return;
        }

        while(textBegin.length()<8){
            textBegin.prepend("0");
        }
        this->addModeInfo.allocMemAddress = startAdd;
        textBegin = textBegin.right(8);
        ui->useMemBegin->setText("0x"+textBegin);



        QString textEnd = QString::number(endAdd,16);
        while(textEnd.length()<8){
            textEnd.prepend("0");
        }
        textEnd = textEnd.right(8);
        ui->useMemEnd->setText("0x"+textEnd);
        flashShowIcon();
    });

    connect(ui->DeviceNameEdit,&QLineEdit::editingFinished,[=](){

        if(mainWindow->judgeIsHaveDev(ui->DeviceNameEdit->text())){
            ui->DeviceNameEdit->setText(this->devName);
        }else{
            this->devName = ui->DeviceNameEdit->text();
            flashShowIcon();
        }
    });


    connect(ui->interNumTextArea,&QLineEdit::editingFinished,[=](){
        bool suceess;
        uint32_t num = ui->interNumTextArea->text().toInt(&suceess);

        if(!suceess || num<=15 || num>255){
            ui->interNumTextArea->setText("不使用中断功能");
            this->addModeInfo.allocInterNum = -1;
        }else if(!mainWindow->getAllFreeInterNum().contains(num)){
            ui->interNumTextArea->setText("该中断号已被占用");
            this->addModeInfo.allocInterNum = -1;
        }else{
            QString numText = QString::number(num);
            while(numText.length()<3){
                numText.prepend("0");
            }

            numText = numText.right(3);

            ui->interNumTextArea->setText(numText);
            this->addModeInfo.allocInterNum = num;
        }

        flashShowIcon();
    });

    connect(ui->cancelButton,&QPushButton::clicked,[=](){
        this->close();
    });

    connect(ui->okButton,&QPushButton::clicked,[=](){
        if(this->addModeInfo.isValid){
            //将新增的设备保存起来
            DeviceMode dev;

            //基础属性
            dev.devName = this->devName;
            dev.devFunText = this->addModeInfo.funInfo;
            dev.devType = this->addModeInfo.typeName;
            dev.beginAddress = this->addModeInfo.allocMemAddress;
            dev.useMemLength = this->addModeInfo.menLength;
            dev.useInterNum = this->addModeInfo.allocInterNum;
            dev.crateIconMap();

            //将解出的MOD包从Tmp目录复制入Devices目录下
            QDir devDir = QApplication::applicationDirPath()+"/Data/Devices";
            QString storageFolderName;//存储的文件夹名称
            int index = 0;
            do {
                storageFolderName = "Device"+QString::number(index);
                index++;
            }while(devDir.exists(storageFolderName));

            QString unzipPath = QApplication::applicationDirPath()+"/Data/Devices/"+storageFolderName;
            if(!ToolFunction::copyDir(this->addModeInfo.unzipPath,
                                      unzipPath)){
                this->close();
                return;
            }

            dev.devFolderName = storageFolderName;
            dev.coreDllPath = this->addModeInfo.coreLibPath;

            dev.dllHandler = SystemDependFunction::LoadDynamicLibrary(unzipPath+"/"+dev.coreDllPath);

            if(dev.dllHandler==0){
                this->close();
                return;
            }

            ModeInfo(*getDllInfoFun)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8) = (ModeInfo(*)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8))SystemDependFunction::GetDynamicLibraryFunction(dev.dllHandler,"GetModeRegisterTable");
            if(getDllInfoFun==0){
                SystemDependFunction::UnloadDynamicLibrary(dev.dllHandler);
                this->close();
                return;
            }

            ModeInfo modeInfo = getDllInfoFun(unzipPath.toUtf8().data(),dev.coreDllPath.toUtf8().data());

            dev.LoadDeviceMod = modeInfo.LoadDeviceMod;
            dev.UnloadDeviceMod = modeInfo.UnloadDeviceMod;

            dev.EnergizeDevice = modeInfo.EnergizeDeviceHandle;
            dev.BlackoutDevice = modeInfo.BlackoutDeviceHandle;
            dev.StopDevice = modeInfo.StopDeviceHandle;
            dev.StartDevice = modeInfo.StartDeviceHandle;

            dev.ReadDevice = modeInfo.ReadDeviceHandle;
            dev.WriteDevice = modeInfo.WriteDeviceHandle;

            if(modeInfo.isSupportModifyData){
                dev.ModifyData = modeInfo.ModifyDataHandle;
            }else{
                dev.ModifyData = NULL;
            }

            dev.GetIsHaveInterruptAsk = modeInfo.GetIsHaveInterruptAskHandle;
            dev.isSupportInterruptAsk = modeInfo.isHaveInterruptAsk;
            if(dev.LoadDeviceMod !=NULL){
                dev.LoadDeviceMod();//调用模块加载处理
            }
            if(dev.EnergizeDevice != NULL){
                dev.EnergizeDevice();//调用模块上电处理
            }
            mainWindow->vriCPU->restart();
            mainWindow->allUseDeviceMode.append(dev);
            mainWindow->flashDevConfigWindow();
            mainWindow->flashShowMemData();
            mainWindow->flashShowRegValue();
            this->close();
        }
    });

}

void AppendDeviceWindow::setSelectPckPath(QString path){
    ui->selectEdit->setText(path);
}

AppendDeviceWindow::~AppendDeviceWindow()
{
    delete ui;
}

void AppendDeviceWindow::flashShowIcon(){
    QPixmap icon = ToolFunction::crateIconMap(
                                    this->addModeInfo.typeName,
                                    this->devName,
                                    this->addModeInfo.allocMemAddress,
                                    this->addModeInfo.menLength,
                                    this->addModeInfo.allocInterNum);

    float devW = (float)ui->deviceIcon->width() / (float)icon.width();
    float devH = (float)ui->deviceIcon->height() / (float)icon.height();

    float scale = fminf(devW,devH);
    icon = icon.scaled(icon.width()*scale,
                        icon.height()*scale,
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
    ui->deviceIcon->setPixmap(icon);
}


void AppendDeviceWindow::moveInfoPage(){
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
void AppendDeviceWindow::showEvent(QShowEvent *event){
    MainWidget_xOffset = 0;
    MainWidget_yOffset = 0;
    ui->DevConfigPage->close();
    ui->MainWidget->move(this->width()/2 - MainWidget_w/2,this->height()/2 - MainWidget_h/2);

    ui->selectEdit->setText("");
}
void AppendDeviceWindow::closeEvent(QCloseEvent *event){

}
void AppendDeviceWindow::resizeEvent(QResizeEvent*e){
    moveInfoPage();
}
//事件处理器
bool AppendDeviceWindow::eventFilter(QObject*obj,QEvent*event){
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
