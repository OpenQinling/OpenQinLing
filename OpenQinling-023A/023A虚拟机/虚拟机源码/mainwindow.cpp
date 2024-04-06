#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QResizeEvent>
#include <QDebug>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QGraphicsDropShadowEffect>
#include <QFile>
#include <QFileInfo>
#include <systemdependfunction.h>
#pragma execution_character_set("utf-8")
#define NumSysSetButtonHandle(reg,bit) connect(ui->NumSysSetButton_##reg,&QPushButton::clicked,this,[=](){ \
    QPushButton &thisButton = *ui->NumSysSetButton_##reg; \
    Reg_NumSysType &type = this->reg##_type; \
    QString bitText = #bit; \
    switch (type) { \
    case NumSys_UH:{ \
        type = NumSys_UD; \
        thisButton.setText("u"+bitText+"-D"); \
        break; \
    } \
    case NumSys_UD:{ \
        type = NumSys_INT; \
        thisButton.setText("i"+bitText); \
        break; \
    } \
    case NumSys_INT:{ \
        type = NumSys_F; \
        thisButton.setText("f"+bitText); \
        break; \
    } \
    case NumSys_F:{ \
        type = NumSys_UH; \
        thisButton.setText("u"+bitText+"H"); \
        break; \
    } \
    case NumSys_UB:return;\
    } \
    flashShowRegValue(); \
});

#define SelectMergeRegHandle(LH) connect(ui->Marge##LH##Set_ComboBox,&QComboBox::currentTextChanged,[=](QString regName){ \
    Reg & reg = LH##Reg; \
    if(regName=="R1"){ \
        reg = R1; \
    }else if(regName=="R2"){ \
        reg = R2; \
    }else if(regName=="R3"){ \
        reg = R3; \
    }else if(regName=="R4"){ \
        reg = R4; \
    }else if(regName=="R5"){ \
        reg = R5; \
    }else if(regName=="R6"){ \
        reg = R6; \
    }else if(regName=="R7"){ \
        reg = R7; \
    }else if(regName=="R8"){ \
        reg = R8; \
    } \
    flashShowRegValue(); \
});

MainWindow *mainWindow;
void MainWindow::initGUIStyle(){
    ui->setupUi(this);
    window_w = this->width();
    window_h = this->height();
    ui->AddDevPrompt->close();
    ui->AddDev_Button->installEventFilter(this);
    ui->StepCPU_button->installEventFilter(this);
    ui->ShowOperPrompt_Button->installEventFilter(this);
    ui->SetShowRAMStartPlace_TextInput->setText("00000000");
    ui->SetShowRAMEndPlace_TextInput->setText("00000200");
    flashShowMemData();
    ShowRAMdataTextArea_w = ui->ShowRAMdataTextArea->width();
    ShowRAMdataTextArea_h = ui->ShowRAMdataTextArea->height();
    widget_18_h = ui->widget_18->height();
    widget_18_w = ui->widget_18->width();
    widget_19_h = ui->widget_19->height();
    widget_19_w = ui->widget_19->width();
    widget_20_h = ui->widget_20->height();
    widget_20_w = ui->widget_20->width();
    widget_22_h = ui->widget_22->height();
    widget_22_w = ui->widget_22->width();

    InitROMbutton_h = ui->InitROMbutton->height();
    InitROMbutton_w = ui->InitROMbutton->width();
    InitROMbutton_x = ui->InitROMbutton->x();
    InitROMbutton_y = ui->InitROMbutton->y();
    SocSetingPage_h = ui->SocSetingPage->height();
    SocSetingPage_w = ui->SocSetingPage->width();

    label_41_h = ui->label_41->height();
    label_41_w = ui->label_41->width();

    AddDev_Button_x = ui->AddDev_Button->x();
    AddDev_Button_y = ui->AddDev_Button->y();

    AddDevPrompt_x = ui->AddDevPrompt->x();
    AddDevPrompt_y = ui->AddDevPrompt->y();

    ui->OperPrompt->close();
    ui->StepCtrlPrompt->close();
    ui->StepCPU_button->installEventFilter(this);

#define installRegTriggerPromptShowEvent(reg) ui->RegPrompt_##reg->close(); \
    ui->DataEdit_##reg->installEventFilter(this);
    installRegTriggerPromptShowEvent(r1);
    installRegTriggerPromptShowEvent(r2);
    installRegTriggerPromptShowEvent(r3);
    installRegTriggerPromptShowEvent(r4);
    installRegTriggerPromptShowEvent(r5);
    installRegTriggerPromptShowEvent(r6);
    installRegTriggerPromptShowEvent(r7);
    installRegTriggerPromptShowEvent(r8);
    installRegTriggerPromptShowEvent(sp);
    installRegTriggerPromptShowEvent(flag);
    installRegTriggerPromptShowEvent(ipc);
    installRegTriggerPromptShowEvent(tpc);
    installRegTriggerPromptShowEvent(pc);
    installRegTriggerPromptShowEvent(sys);
    installRegTriggerPromptShowEvent(tlb);

    socDevConfigPage = new SocDeviceConfig(ui->SocSetingPage,&this->allUseDeviceMode);
    socDevConfigPage->move(3,3);
    socDevConfigPage->resize(SocSetingPage_w-6,SocSetingPage_h-6);
    socDevConfigPage->setStyleSheet("QWidget{"
                                    "background-color:#ff000000;"
                                    "}"
                                    );
    socDevConfigPage->show();
    deviceInfoWindow = new DeviceInfoWindow(this);
    deviceInfoWindow->installEventFilter(this);
    deviceInfoWindow->close();

    initRomWindow = new InitRomWindow(this);
    initRomWindow->installEventFilter(this);
    initRomWindow->close();

    appInfoWindow = new AppInfoWindow(this);
    appInfoWindow->installEventFilter(this);
    appInfoWindow->close();


    appendDeviceWindow = new AppendDeviceWindow(this);
    appendDeviceWindow->installEventFilter(this);
    appendDeviceWindow->close();

    //修改寄存器数值显示模式的响应处理
    NumSysSetButtonHandle(r1,32);
    NumSysSetButtonHandle(r2,32);
    NumSysSetButtonHandle(r3,32);
    NumSysSetButtonHandle(r4,32);
    NumSysSetButtonHandle(r5,32);
    NumSysSetButtonHandle(r6,32);
    NumSysSetButtonHandle(r7,32);
    NumSysSetButtonHandle(r8,32);
    NumSysSetButtonHandle(tpc,32);
    NumSysSetButtonHandle(ipc,32);
    NumSysSetButtonHandle(pc,32);
    NumSysSetButtonHandle(sp,32);
    NumSysSetButtonHandle(flag,32);
    NumSysSetButtonHandle(sys,32);
    NumSysSetButtonHandle(tlb,32);
    NumSysSetButtonHandle(merge,64);

    //寄存器合并显示选择合并的寄存器的响应处理
    SelectMergeRegHandle(High);
    SelectMergeRegHandle(Low);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    mainWindow = this;

    checkCpuIsStopTimer = new QTimer(this);
    checkCpuIsStopTimer->start(1);

    qDebug()<<"创建虚拟CPU模块...";

    vriCPU = new VritualCPU(
    [=](){
        //cpu启动处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            if(allUseDeviceMode[i].StartDevice!=0){
                allUseDeviceMode[i].StartDevice();
            }
        }
    },[=](){
        //cpu暂停处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            if(allUseDeviceMode[i].StopDevice!=0){
                allUseDeviceMode[i].StopDevice();
            }
        }
    },[=](){
        //cpu断电处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            if(allUseDeviceMode[i].BlackoutDevice!=0){
                allUseDeviceMode[i].BlackoutDevice();
            }
        }
    },[=](){
        //cpu上电处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            if(allUseDeviceMode[i].EnergizeDevice!=0){
                allUseDeviceMode[i].EnergizeDevice();
            }
        }
    },[=](uint8_t data,uint32_t add){
        //cpu写内存回调处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            uint32_t beginAdd = allUseDeviceMode[i].beginAddress;
            uint32_t endAdd = allUseDeviceMode[i].beginAddress + allUseDeviceMode[i].useMemLength -1;
            if(allUseDeviceMode[i].WriteDevice!=0 && beginAdd<=add && endAdd>=add){
                return allUseDeviceMode[i].WriteDevice(data,add-allUseDeviceMode[i].beginAddress);
            }
        }
    },[=](uint32_t add)->uint8_t{

        //cpu读内存回调处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            uint32_t beginAdd = allUseDeviceMode[i].beginAddress;
            uint32_t endAdd = allUseDeviceMode[i].beginAddress + allUseDeviceMode[i].useMemLength -1;
            if(allUseDeviceMode[i].ReadDevice!=0 && beginAdd<=add && endAdd>=add){
                return allUseDeviceMode[i].ReadDevice(add-allUseDeviceMode[i].beginAddress);
            }
        }
        return 0;
    },[=](uint8_t&num)->uint8_t{
        //cpu判断是否有外部中断请求的回调处理
        for(int i = 0;i<allUseDeviceMode.length();i++){
            if(allUseDeviceMode[i].GetIsHaveInterruptAsk!=0){
                bool isAsk = allUseDeviceMode[i].GetIsHaveInterruptAsk();
                if(isAsk){
                    num = allUseDeviceMode[i].useInterNum;
                    return 1;
                }
            }
        }
        num = 0;
        return 0;
    },[=](){
        isStop = 1;
    });

    DeviceMode cpuMode;
    cpuMode.devType = "CPU";
    cpuMode.devName = "CORE";
    cpuMode.devFunText = "虚拟SOC的中央控制器,使用开源秦岭(OpenQinling)-023A指令集。"
                         "指令集为32位RISC架构,支持浮点运算/保护模式/软硬中断/虚拟内存在内的功能。"
                         "详细指令集信息查看帮助文档";
    cpuMode.isSupportInterruptAsk = 0;
    cpuMode.crateIconMap();
    this->allUseDeviceMode.append(cpuMode);

    qDebug()<<"创建虚拟内置FLASH模块...";

    DeviceMode innerFlash;
    innerFlash.devType = "ROM";
    innerFlash.devName = "InnerFLASH";
    innerFlash.beginAddress = 0;
    innerFlash.useMemLength = 65536;
    innerFlash.devFunText = "虚拟SOC内置64KB-ROM。\r\n其中0-1023区域为CPU的中断向量表,分别存储0-255号中断处理函数的地址;\r\n"
                            "1024-65531为Bootload程序存储区(该处默认全部都为0x00,可自行向此处写入Bootload程序的二进制数据),1024处为CPU上电后起始执行位置;\r\n"
                            "65532-65535保存tlb缺页、访问非法内存页时内存页的虚拟内存起始地址。\r\n"
                            "该区域虚拟机关机数据不丢失(虚拟机程序启动即为开机,关闭程序即为关机)"
                            ",用于存储BootLoad程序。\r\n"
                            "可通过点击[初始化ROM数据]的按钮,进入初始化界面后通过.etb(二进制内存段数据描述)文件进行初始化。"
                            "etb文件可由OpenQinling编译工具链中的程序链接器或.hex转换器生成";
    innerFlash.crateIconMap();
    innerFlash.WriteDevice = [=](uint8_t data,uint32_t add){
        innerFlashData[add] = data;
    };
    innerFlash.ReadDevice = [=](uint32_t add)->uint8_t{
        return innerFlashData[add];
    };
    innerFlash.isSupportInterruptAsk = 0;
    this->allUseDeviceMode.append(innerFlash);

    qDebug()<<"加载配置的第三方虚拟设备...";

    //加载外部的自定义虚拟设备
    QJsonObject rootNode = settingJsonDoc.object();
    QStringList allDevFolderName;
    if(rootNode.value("DeviceList").isArray()){
        QJsonArray deviceListNode = rootNode.value("DeviceList").toArray();
        for(int i = 0;i<deviceListNode.size();i++){
            if(!deviceListNode[i].isObject())continue;

            DeviceMode devMode;

            QJsonObject devNode = deviceListNode[i].toObject();

            if(devNode.value("devFolderName").isString()){
                devMode.devFolderName = devNode.value("devFolderName").toString();
            }else{
                continue;
            }

            if(devNode.value("devName").isString()){
                devMode.devName = devNode.value("devName").toString();
            }else{
                continue;
            }
            if(devNode.value("beginAddress").isString()){
                bool suceess;
                devMode.beginAddress = devNode.value("beginAddress").toString().toUInt(&suceess,16);
                if(!suceess){
                    continue;
                }
            }else{
                continue;
            }

            if(devNode.value("useInterNum").isDouble()){
                devMode.useInterNum = devNode.value("useInterNum").toDouble();
            }else{
                continue;
            }

            if(devNode.value("coreDllPath").isString()){
                devMode.coreDllPath = devNode.value("coreDllPath").toString();
            }else{
                continue;
            }


            devMode.dllHandler = SystemDependFunction::LoadDynamicLibrary(QApplication::applicationDirPath()+
                                                                      "/Data/Devices/"+
                                                                      devMode.devFolderName+"/"+
                                                                      devMode.coreDllPath);

            if(devMode.dllHandler==0){
                continue;
            }


            ModeInfo(*getDllInfoFun)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8) = (ModeInfo(*)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8))SystemDependFunction::GetDynamicLibraryFunction(devMode.dllHandler,"GetModeRegisterTable");
            if(getDllInfoFun==0){
                SystemDependFunction::UnloadDynamicLibrary(devMode.dllHandler);
                continue;
            }

            if(QDir(QApplication::applicationDirPath()+
                    "/Data/Devices/"+
                    devMode.devFolderName).exists()){
                allDevFolderName += devMode.devFolderName;
            }

            QString unzipPath = QApplication::applicationDirPath()+"/Data/Devices/"+devMode.devFolderName;
            ModeInfo modeInfo = getDllInfoFun(unzipPath.toUtf8().data(),devMode.coreDllPath.toUtf8().data());

            devMode.devType = modeInfo.typeName;
            devMode.devFunText = modeInfo.functionInfo;
            devMode.useMemLength = modeInfo.memLength;

            devMode.LoadDeviceMod = modeInfo.LoadDeviceMod;
            devMode.UnloadDeviceMod = modeInfo.UnloadDeviceMod;

            devMode.EnergizeDevice = modeInfo.EnergizeDeviceHandle;
            devMode.BlackoutDevice = modeInfo.BlackoutDeviceHandle;
            devMode.StopDevice = modeInfo.StopDeviceHandle;
            devMode.StartDevice = modeInfo.StartDeviceHandle;

            devMode.ReadDevice = modeInfo.ReadDeviceHandle;
            devMode.WriteDevice = modeInfo.WriteDeviceHandle;
            devMode.GetIsHaveInterruptAsk = modeInfo.GetIsHaveInterruptAskHandle;
            devMode.isSupportInterruptAsk = modeInfo.isHaveInterruptAsk;

            if(modeInfo.isSupportModifyData){
                devMode.ModifyData = modeInfo.ModifyDataHandle;
            }else{
                devMode.ModifyData = NULL;
            }
            devMode.crateIconMap();
            allUseDeviceMode.append(devMode);
        }
    }

    //删除无用的设备的存放文件夹
    QDir devicesDir = QApplication::applicationDirPath()+"/Data/Devices";
    QFileInfoList allDevDir = devicesDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    foreach(QFileInfo devDir,allDevDir){
        if(devDir.isFile()){
            devicesDir.remove(devDir.fileName());
        }else{
            if(!allDevFolderName.contains(devDir.baseName())){
                QDir(devDir.absoluteFilePath()).removeRecursively();
            }
        }
    }


    qDebug()<<"启动GUI界面中...";
    initGUIStyle();
    flashShowRegValue();

    lastShowRAMStartPlaceText = ui->SetShowRAMStartPlace_TextInput->text();
    lastShowRAMEndPlaceText = ui->SetShowRAMStartPlace_TextInput->text();
    //内存数据展示位置设置
    connect(ui->SetShowRAMStartPlace_TextInput,&QLineEdit::editingFinished,[=](){
        QString textBegin = ui->SetShowRAMStartPlace_TextInput->text();
        QString textEnd = ui->SetShowRAMEndPlace_TextInput->text();
        bool isSuceess;
        uint endAdd = textEnd.toUInt(&isSuceess,16);
        uint startAdd = textBegin.toUInt(&isSuceess,16);
        if(isSuceess==0){
            ui->SetShowRAMStartPlace_TextInput->setText(lastShowRAMStartPlaceText);
        }
        while(textBegin.length()<8){
            textBegin.prepend("0");
        }
        textBegin = textBegin.right(8);
        ui->SetShowRAMStartPlace_TextInput->setText(textBegin);

        endAdd = startAdd+512;
        if(endAdd<(startAdd)){
            endAdd = 0xffffffff;
        }

        textEnd = QString::number(endAdd,16);
        while(textEnd.length()<8){
            textEnd.prepend("0");
        }
        textEnd = textEnd.right(8);
        ui->SetShowRAMEndPlace_TextInput->setText(textEnd);

        flashShowMemData();
    });
    connect(ui->SetShowRAMEndPlace_TextInput,&QLineEdit::editingFinished,[=](){
        QString textEnd = ui->SetShowRAMEndPlace_TextInput->text();
        QString textBegin = ui->SetShowRAMStartPlace_TextInput->text();
        bool isSuceess;
        uint startAdd = textBegin.toUInt(&isSuceess,16);
        uint endAdd = textEnd.toUInt(&isSuceess,16);
        if(isSuceess==0 || endAdd<startAdd){
            ui->SetShowRAMEndPlace_TextInput->setText(lastShowRAMEndPlaceText);
        }
        if(endAdd-startAdd > 4095){
            endAdd = startAdd+4095;
        }
        textEnd = QString::number(endAdd,16);
        while(textEnd.length()<8){
            textEnd.prepend("0");
        }
        textEnd = textEnd.right(8);
        ui->SetShowRAMEndPlace_TextInput->setText(textEnd);
        flashShowMemData();
    });

    //cpu启停控制
    connect(ui->StartCPU_button,&QPushButton::clicked,[=](){

        //启动持续运行
        if(!vriCPU->getIsRuning()){
            vriCPU->run(-1);
            CPU_StartCallBack();
        }

    });
    connect(ui->RestartCPU_button,&QPushButton::clicked,[=](){
        //重启
        vriCPU->restart();
        flashShowMemData();
        flashShowRegValue();
    });
    connect(ui->StopCPU_button,&QPushButton::clicked,[=](){
        //暂停运行
        if(vriCPU->getIsRuning()){
            vriCPU->stop();
        }
    });
    connect(ui->StepCPU_button,&QPushButton::clicked,[=](){
        //运行n条指令
        if(!vriCPU->getIsRuning()){
            vriCPU->run(ui->StepNumSet_SpinBox->value());
            CPU_StartCallBack();
        }

    });
    connect(checkCpuIsStopTimer,&QTimer::timeout,[=](){
        if(isStop){
            //cpu停止运行的处理
            isStop = 0;
            CPU_StopCallBack();
        }
    });

    //初始化rom
    connect(ui->InitROMbutton,&QPushButton::clicked,[=](){
        if(!vriCPU->getIsRuning()){
            initRomWindow->show();
        }
    });

    connect(ui->AddDev_Button,&QPushButton::clicked,[=](){
        appendDevice();
    });

    connect(ui->OpenAppInfoButton,&QPushButton::clicked,[=](){
         appInfoWindow->show();
    });
    setWindowTitle("OpenQinling-VritualSoc");
}

void MainWindow::appendDevice(QString addPckPath){
    if(!vriCPU->getIsRuning()){
        appendDeviceWindow->show();
        if(!addPckPath.isEmpty()){
            appendDeviceWindow->setSelectPckPath(addPckPath);
        }
    }
}

void MainWindow::CPU_StopCallBack(){
    flashShowMemData();
    flashShowRegValue();

    ui->CPU_StateShow->setText("暂停中");
    ui->CPU_StateShow->setStyleSheet("QPushButton{"
                                     "background-color:#ff0000;"
                                     "border:5px outset #ff0000;"
                                     "border-radius:13px;"
                                     "color:rgb(255,255,255);"
                                     "}");
    socDevConfigPage->stop();
}
void MainWindow::CPU_StartCallBack(){
    flashShowMemData();
    flashShowRegValue();
    ui->CPU_StateShow->setText("运行中");
    ui->CPU_StateShow->setStyleSheet("QPushButton{"
                                     "background-color:#00cc00;"
                                     "border:5px outset #009900;"
                                     "border-radius:13px;"
                                     "color:rgb(255,255,255);"
                                     "}");
    socDevConfigPage->start();

}


//显示设备信息界面
void MainWindow::showDevieInfoPage(int index){
    if(vriCPU->getIsRuning())return;
    if(index<0 || index>=allUseDeviceMode.length())return;
    deviceInfoWindow->showDeviceInfo(index);
}

//设置一个寄存器显示的数据
void MainWindow::setRegValue(Reg reg,DWORD data){
    switch (reg) {
    case R1:{r1_data = data;return;}
    case R2:{r2_data = data;return;}
    case R3:{r3_data = data;return;}
    case R4:{r4_data = data;return;}
    case R5:{r5_data = data;return;}
    case R6:{r6_data = data;return;}
    case R7:{r7_data = data;return;}
    case R8:{r8_data = data;return;}
    case FLAG:{flag_data = data;return;}
    case TPC:{tpc_data = data;return;}
    case IPC:{ipc_data = data;return;}
    case SP:{sp_data = data;return;}
    case PC:{pc_data = data;return;}
    case SYS:{sys_data = data;return;}
    case TLB:{tlb_data = data;return;}
    default:return;
    }
}
//设置一个寄存器显示的数值方式
void MainWindow::setRegSys(Reg reg,Reg_NumSysType sys){
    switch (reg) {
    case R1:{r1_type = sys;return;}
    case R2:{r2_type = sys;return;}
    case R3:{r3_type = sys;return;}
    case R4:{r4_type = sys;return;}
    case R5:{r5_type = sys;return;}
    case R6:{r6_type = sys;return;}
    case R7:{r7_type = sys;return;}
    case R8:{r8_type = sys;return;}
    case FLAG:{flag_type = sys;return;}
    case TPC:{tpc_type = sys;return;}
    case IPC:{ipc_type = sys;return;}
    case SP:{sp_type = sys;return;}
    case PC:{pc_type = sys;return;}
    case SYS:{sys_type = sys;return;}
    case TLB:{tlb_type = sys;return;}
    case Merge:{merge_type = sys;return;}
    default:return;
    }
}

//获取寄存器数据
DWORD MainWindow::getRegValue(Reg reg){
    switch (reg) {
    case R1:{return r1_data;}
    case R2:{return r2_data;}
    case R3:{return r3_data;}
    case R4:{return r4_data;}
    case R5:{return r5_data;}
    case R6:{return r6_data;}
    case R7:{return r7_data;}
    case R8:{return r8_data;}
    case FLAG:{return flag_data;}
    case TPC:{return tpc_data;}
    case IPC:{return ipc_data;}
    case SP:{return sp_data;}
    case PC:{return pc_data;}
    case SYS:{return sys_data;}
    case TLB:{return tlb_data;}
    default:return 0;
    }
}


//将寄存器数据转为文本
QString MainWindow::regValueToString(DWORD value,Reg_NumSysType type){
    QString txt;
    switch (type) {
    case NumSys_UH:{
        txt = QString::number(value.toUInt(),16);
        while(txt.length()<8){
            txt.prepend("0");
        }
        txt.prepend("0x");
        return txt;
    }
    case NumSys_UD:{
        txt = QString::number(value.toUInt(),10);
        return txt;
    }
    case NumSys_INT:{
        txt = QString::number(value.toInt(),10);
        return txt;
    }
    case NumSys_F:{
        txt = QString::number(value.toFloat());
        return txt;
    }
    case NumSys_UB:{
        txt = QString::number(value.toUInt(),2);
        while(txt.length()<32){
            txt.prepend("0");
        }

        QString tmp;
        for(int i = 0;i<32;i++){
            tmp.append(txt[i]);
            if((i+1)%4==0 && i!=31){
                tmp.append("|");
            }
        }

        return tmp;
    }
    }
    return QString();
}
QString MainWindow::mergeRegValueToString(DWORD valueH,DWORD valueL,Reg_NumSysType type){
    QWORD value(valueH,valueL);
    QString txt;
    switch (type) {
    case NumSys_UH:{
        txt = QString::number(value.toULong(),16);
        while(txt.length()<16){
            txt.prepend("0");
        }
        txt.prepend("0x");
        return txt;
    }
    case NumSys_UD:{
        txt = QString::number(value.toULong(),10);
        return txt;
    }
    case NumSys_INT:{
        txt = QString::number(value.toLong(),10);
        return txt;
    }
    case NumSys_F:{
        txt = QString::number(value.toDouble());
        return txt;
    }
    }
    return QString();
}

//刷新内存展示的值
void MainWindow::flashShowMemData(){
    QString textBegin = ui->SetShowRAMStartPlace_TextInput->text();
    QString textEnd = ui->SetShowRAMEndPlace_TextInput->text();
    bool isSuceess;
    uint endAdd = textEnd.toUInt(&isSuceess,16);
    uint startAdd = textBegin.toUInt(&isSuceess,16);

    if(vriCPU->getIsRuning()){
        QString txt;
        int len = endAdd - startAdd +1;
        for(int i = 0;i<len;i++){
            txt+="XX ";
        }
        ui->ShowRAMdataTextArea->setText(txt);
    }else{
        QString txt;

        for(uint64_t i = startAdd;i<=endAdd;i++){
            QString tmp;
            uint8_t d  = vriCPU->ReadDevice(i);

            tmp = QString::number(d,16);
            while(tmp.length()<2){
                tmp.prepend("0");
            }
            tmp = tmp.right(2);
            txt += tmp+" ";
        }
        ui->ShowRAMdataTextArea->setText(txt);
    }
}

//刷新寄存器数值的显示
void MainWindow::flashShowRegValue(){
    if(vriCPU->getIsRuning()){
        QString invalidText = "- - - - - - - - - - - - - ";
        ui->DataEdit_r1->setText(invalidText);
        ui->DataEdit_r2->setText(invalidText);
        ui->DataEdit_r3->setText(invalidText);
        ui->DataEdit_r4->setText(invalidText);
        ui->DataEdit_r5->setText(invalidText);
        ui->DataEdit_r6->setText(invalidText);
        ui->DataEdit_r7->setText(invalidText);
        ui->DataEdit_r8->setText(invalidText);
        ui->DataEdit_flag->setText(invalidText);
        ui->DataEdit_pc->setText(invalidText);
        ui->DataEdit_ipc->setText(invalidText);
        ui->DataEdit_tpc->setText(invalidText);
        ui->DataEdit_sp->setText(invalidText);
        ui->DataEdit_sys->setText(invalidText);
        ui->DataEdit_tlb->setText(invalidText);
        ui->DataEdit_Merge->setText(invalidText);

        return;
    }
    r1_data = vriCPU->regGroup[VritualCPU::r1];
    r2_data = vriCPU->regGroup[VritualCPU::r2];
    r3_data = vriCPU->regGroup[VritualCPU::r3];
    r4_data = vriCPU->regGroup[VritualCPU::r4];
    r5_data = vriCPU->regGroup[VritualCPU::r5];
    r6_data = vriCPU->regGroup[VritualCPU::r6];
    r7_data = vriCPU->regGroup[VritualCPU::r7];
    r8_data = vriCPU->regGroup[VritualCPU::r8];
    sp_data = vriCPU->regGroup[VritualCPU::sp];
    flag_data = vriCPU->regGroup[VritualCPU::flag];
    pc_data = vriCPU->regGroup[VritualCPU::pc];
    tpc_data = vriCPU->regGroup[VritualCPU::tpc];
    ipc_data = vriCPU->regGroup[VritualCPU::ipc];
    sys_data = vriCPU->regGroup[VritualCPU::sys];
    tlb_data = vriCPU->regGroup[VritualCPU::tlb];


    ui->DataEdit_r1->setText(regValueToString(r1_data,r1_type));
    ui->DataEdit_r2->setText(regValueToString(r2_data,r2_type));
    ui->DataEdit_r3->setText(regValueToString(r3_data,r3_type));
    ui->DataEdit_r4->setText(regValueToString(r4_data,r4_type));
    ui->DataEdit_r5->setText(regValueToString(r5_data,r5_type));
    ui->DataEdit_r6->setText(regValueToString(r6_data,r6_type));
    ui->DataEdit_r7->setText(regValueToString(r7_data,r7_type));
    ui->DataEdit_r8->setText(regValueToString(r8_data,r8_type));
    ui->DataEdit_flag->setText(regValueToString(flag_data,flag_type));
    ui->DataEdit_pc->setText(regValueToString(pc_data,pc_type));
    ui->DataEdit_ipc->setText(regValueToString(ipc_data,ipc_type));
    ui->DataEdit_tpc->setText(regValueToString(tpc_data,tpc_type));
    ui->DataEdit_sp->setText(regValueToString(sp_data,sp_type));
    ui->DataEdit_sys->setText(regValueToString(sys_data,sys_type));
    ui->DataEdit_tlb->setText(regValueToString(tlb_data,tlb_type));

    ui->RegPrompt_r1->setText(regValueToString(r1_data,NumSys_UB));
    ui->RegPrompt_r2->setText(regValueToString(r2_data,NumSys_UB));
    ui->RegPrompt_r3->setText(regValueToString(r3_data,NumSys_UB));
    ui->RegPrompt_r4->setText(regValueToString(r4_data,NumSys_UB));
    ui->RegPrompt_r5->setText(regValueToString(r5_data,NumSys_UB));
    ui->RegPrompt_r6->setText(regValueToString(r6_data,NumSys_UB));
    ui->RegPrompt_r7->setText(regValueToString(r7_data,NumSys_UB));
    ui->RegPrompt_r8->setText(regValueToString(r8_data,NumSys_UB));
    ui->RegPrompt_flag->setText(regValueToString(flag_data,NumSys_UB));
    ui->RegPrompt_pc->setText(regValueToString(pc_data,NumSys_UB));
    ui->RegPrompt_ipc->setText(regValueToString(ipc_data,NumSys_UB));
    ui->RegPrompt_tpc->setText(regValueToString(tpc_data,NumSys_UB));
    ui->RegPrompt_sp->setText(regValueToString(sp_data,NumSys_UB));
    ui->RegPrompt_sys->setText(regValueToString(sys_data,NumSys_UB));
    ui->RegPrompt_tlb->setText(regValueToString(tlb_data,NumSys_UB));

    DWORD h = getRegValue(HighReg);
    DWORD l = getRegValue(LowReg);
    ui->DataEdit_Merge->setText(mergeRegValueToString(h,l,merge_type));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//事件处理器
bool MainWindow::eventFilter(QObject*obj,QEvent*event){
    if(event->type() == QEvent::Enter && obj==ui->AddDev_Button){
        ui->AddDevPrompt->show();
        return true;
    }else if((event->type() == QEvent::Leave)&& obj==ui->AddDev_Button){
        ui->AddDevPrompt->close();
        return true;
    }

    if(event->type() == QEvent::Enter && obj==ui->StepCPU_button){
        ui->StepCtrlPrompt->show();
        return true;
    }else if((event->type() == QEvent::Leave)&& obj==ui->StepCPU_button){
        ui->StepCtrlPrompt->close();
        return true;
    }
    if(event->type() == QEvent::Enter && obj==ui->ShowOperPrompt_Button){
        ui->OperPrompt->show();
        return true;
    }else if((event->type() == QEvent::Leave)&& obj==ui->ShowOperPrompt_Button){
        ui->OperPrompt->close();
        return true;
    }

#define DataEditShowPromptHandle(reg) if(event->type() == QEvent::Enter && obj==ui->DataEdit_##reg){\
    ui->RegPrompt_##reg->show();\
    return true;\
}else if((event->type() == QEvent::Leave)&& obj==ui->DataEdit_##reg){\
    ui->RegPrompt_##reg->close();\
    return true;\
}


    if(!vriCPU->getIsRuning()){
        DataEditShowPromptHandle(r1);
        DataEditShowPromptHandle(r2);
        DataEditShowPromptHandle(r3);
        DataEditShowPromptHandle(r4);
        DataEditShowPromptHandle(r5);
        DataEditShowPromptHandle(r6);
        DataEditShowPromptHandle(r7);
        DataEditShowPromptHandle(r8);
        DataEditShowPromptHandle(pc);
        DataEditShowPromptHandle(tpc);
        DataEditShowPromptHandle(ipc);
        DataEditShowPromptHandle(sp);
        DataEditShowPromptHandle(flag);
        DataEditShowPromptHandle(tlb);
        DataEditShowPromptHandle(sys);
    }

    return false;
}
//窗口大小变更
void MainWindow::resizeEvent(QResizeEvent*e){
    ui->InitROMbutton->resize(e->size().width()-window_w+InitROMbutton_w,ui->InitROMbutton->height());
    ui->InitROMbutton->move(InitROMbutton_x,e->size().height()-window_h+InitROMbutton_y);
    ui->widget_22->resize(e->size().width()-window_w+widget_22_w,e->size().height()-window_h+widget_22_h);
    ui->SocSetingPage->resize(e->size().width()-window_w+SocSetingPage_w,e->size().height()-window_h+SocSetingPage_h);
    socDevConfigPage->resize(e->size().width()-window_w+SocSetingPage_w-6,e->size().height()-window_h+SocSetingPage_h-6);

    ui->widget_20->resize(widget_20_w,e->size().height()-window_h+widget_20_h);

    ui->widget_19->resize(widget_19_w,e->size().height()-window_h+widget_19_h);
    ui->widget_18->resize(widget_18_w,e->size().height()-window_h+widget_18_h);
    ui->label_41->resize(e->size().width()-window_w+label_41_w,label_41_h);
    ui->ShowRAMdataTextArea->resize(ShowRAMdataTextArea_w,e->size().height()-window_h+ShowRAMdataTextArea_h);

    ui->InitROMbutton->move(InitROMbutton_x,e->size().height()-window_h+InitROMbutton_y);

    ui->AddDev_Button->move(e->size().width()-window_w+AddDev_Button_x,AddDev_Button_y);
    ui->AddDevPrompt->move(e->size().width()-window_w+AddDevPrompt_x,AddDevPrompt_y);

    deviceInfoWindow->resize(e->size());
    initRomWindow->resize(e->size());
    appendDeviceWindow->resize(e->size());
    appInfoWindow->resize(e->size());
}


void MainWindow::closeEvent(QCloseEvent *event){
    exitHandle();
}

//获取所有没有被占用的外部中断号(index不搜索的设备)
QList<int> MainWindow::getAllFreeInterNum(int index){
    QList<int> allUseNum;
    for(int i = 16;i<=255;i++){
        allUseNum.append(i);
    }
    for(int i = 0;i<allUseDeviceMode.length();i++){
        if(index==i){
            continue;
        }
        if(allUseDeviceMode[i].useInterNum==-1)continue;
        if(!allUseNum.contains(allUseDeviceMode[i].useInterNum))continue;
        allUseNum.removeOne(allUseDeviceMode[i].useInterNum);
    }
    return allUseNum;
}

//获取所有没有被占用的内存区域
QList<BlockInfo> MainWindow::getAllFreeMemBlock(int index){
    QList<BlockInfo> freeblock;
    BlockInfo block;
    block.baseAdress = 0;
    block.sizeBytes = 0x100000000;
    freeblock.append(block);

    for(int i = 0;i<allUseDeviceMode.length();i++){
        if(allUseDeviceMode[i].devType!="CPU" && i!=index){
            BlockInfo thisDevBlock;
            thisDevBlock.baseAdress = allUseDeviceMode[i].beginAddress;
            thisDevBlock.sizeBytes = allUseDeviceMode[i].useMemLength;
            freeblock = ToolFunction::removeOccupiedMemBlock(freeblock,thisDevBlock);
        }
    }
    return freeblock;
}

//判断一个设备名称是否已存在
bool MainWindow::judgeIsHaveDev(QString name){
    for(int i = 0;i<allUseDeviceMode.length();i++){
        if(allUseDeviceMode[i].devName == name){
            return 1;
        }
    }
    return 0;
}

//新增一个设备时的默认名称
QString MainWindow::newDevDefaultName(){
    int index = 0;
    QString name;
    do{
        name = "NewDev"+QString::number(index);
        index++;
    }while(judgeIsHaveDev(name));
    return name;
}

//刷新硬件配置界面
void MainWindow::flashDevConfigWindow(){
    bool isFlash;
    if(vriCPU->getIsRuning()){
        isFlash = socDevConfigPage->start();
    }else{
        isFlash = socDevConfigPage->stop();
    }
    if(isFlash==0){
        socDevConfigPage->drawDisplayPixmap();
        socDevConfigPage->flashDisplay();
    }
}
