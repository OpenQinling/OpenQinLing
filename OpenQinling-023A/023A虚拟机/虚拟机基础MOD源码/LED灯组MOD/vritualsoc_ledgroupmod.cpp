#include <QWidget>
#include <QDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <datastruct.h>
#include <ledgroupwindow.h>
#include <QRgb>
#define C_EXPORT __declspec(dllexport)
#define CPP_EXPORT extern "C" __declspec(dllexport)

typedef enum {
    RW_BYTE,
    RW_WORD,
    RW_DWORD
}RwDatas;
typedef struct{
    const char* typeName;//虚拟设备类型名
    const char* functionInfo;//虚拟设备功能描述文本
    uint32_t memLength;//虚拟设备占用的内存空间
    uint8_t isHaveInterruptAsk;//该设备是否有中断请求的功能
    uint8_t isSupportModifyData;//该设备是否支持修改内部数据的功能(内部数据即无法被内存寻址直接访问的寄存器的数据,对于磁盘型设备应当开启该功能)

    //设备加载、卸载、信息获取回调
    void (*LoadDeviceMod)(void);
    void (*UnloadDeviceMod)(void);

    //修改内部数据的回调
    void (*ModifyDataHandle)(void);//应当由MOD开发者在该函数数中提供Dialog类型的GUI窗口,用于修改内部数据

    //设备启停回调
    void (*StartDeviceHandle)(void);//设备启动回调(cpu启动时会回调)
    void (*StopDeviceHandle)(void);//设备暂停回调(cpu被暂停时会回调)
    void (*BlackoutDeviceHandle)(void);//设备断电回调(cpu重启/虚拟机应用退出/删除设备时会回调)
    void (*EnergizeDeviceHandle)(void);//设备通电回调(cpu重启/虚拟机应用启动/第一次加载设备时会回调)

    //设备读写回调
    void (*WriteDeviceHandle)(uint8_t data, uint32_t add);//向设备写入数据回调
    uint8_t(*ReadDeviceHandle)(uint32_t add);//从设备读取数据回调
    uint8_t(*GetIsHaveInterruptAskHandle)(void);//读取设备是否有中断请求的回调
}ModeInfo;


static void WriteDeviceHandle(uint8_t data, uint32_t add);
static uint8_t ReadDeviceHandle(uint32_t add);
static void StartDeviceHandle(void);
static void StopDeviceHandle(void);
static void BlackoutDeviceHandle(void);
static void EnergizeDeviceHandle(void);
static uint8_t GetIsHaveInterruptAskHandle(void);
static void LoadDeviceMod(void);
static void UnloadDeviceMod(void);
static const char* DevTypeName = "LED";
static char* DevFunInfo = NULL;
static uint32_t DevMemLength;
static uint8_t isHaveInterruptAsk = 0;

#define IS_SUPPORT_CONFIG 1 //是否支持MOD配置功能

#include <configwindow.h>

QString packUnzipPath;
QString coreDllPath;

#if IS_SUPPORT_CONFIG
    //配置MOD接口:(packUnzipPath_utf8:当前MOD包解压缩的地址,utf8格式)
    CPP_EXPORT void ConfigModule(const char*packUnzipPath_utf8,const char*coreDllPath_utf8){
        packUnzipPath = packUnzipPath_utf8;
        coreDllPath = coreDllPath_utf8;
        ConfigWindow configWindow;
        configWindow.exec();

        QFile file(packUnzipPath+"/setting.cof");
        if(file.open(QFile::WriteOnly)){
            file.resize(0);
            QDataStream stream(&file);
            stream<<configWindow.r;
            stream<<configWindow.g;
            stream<<configWindow.b;
            stream<<configWindow.count;
            file.close();
        }else{
            QMessageBox::critical(NULL,"错误","配置文件[setting.cof]无法保存");
        }
    }
#endif


//显示屏的配置信息
QColor ledColor;
int ledCount;

//获取MOD信息接口:(packUnzipPath_utf8:当前MOD包解压缩的地址,utf8格式)
CPP_EXPORT ModeInfo GetModeRegisterTable(const char*packUnzipPath_utf8,const char*coreDllPath_utf8){
    packUnzipPath = packUnzipPath_utf8;
    coreDllPath = coreDllPath_utf8;

    QFile file(packUnzipPath+"/setting.cof");
    if(file.open(QFile::ReadOnly)){
        if(file.size()==16){
            QDataStream stream(&file);
            int r,g,b;

            stream>>r;
            stream>>g;
            stream>>b;
            stream>>ledCount;

            ledColor = QColor(r,g,b);
        }
        file.close();
    }


    DevMemLength = ledCount;
    QString devFunInfoText =
              QString::number(ledCount)+"个color(r="+
              QString::number(ledColor.red())+",g="+
              QString::number(ledColor.green())+",b="+
              QString::number(ledColor.blue())+")色的LED组成的LED组,"
              "占用"+QString::number(DevMemLength)+"字节内存,每个字节对应控制每个led灯,写入数值越大,对应led越亮";

    static QByteArray textBytes = devFunInfoText.toUtf8();
    DevFunInfo = textBytes.data();



    //设备模块信息注册表
    ModeInfo RegisterDevTable;
    //////////////////设备属性信息//////////////////////
    RegisterDevTable.typeName = DevTypeName;//设备类型名称(必要)
    RegisterDevTable.functionInfo = DevFunInfo;//设备类型功能描述(非必要,可为NULL)
    RegisterDevTable.memLength = DevMemLength;//设备占用的内存空间字节数(必要)
    RegisterDevTable.isHaveInterruptAsk = isHaveInterruptAsk;//是否有中断请求的功能

    ///////////////设备功能的回调函数////////////////////
    ///全部都为非必要,如果无需回调处理的直接传入NULL即可///

    RegisterDevTable.LoadDeviceMod = LoadDeviceMod;//加载MOD时会回调
    RegisterDevTable.UnloadDeviceMod = UnloadDeviceMod;//卸载MOD时会回调

    RegisterDevTable.StartDeviceHandle = StartDeviceHandle;//设备启动运行时会回调
    RegisterDevTable.StopDeviceHandle = StopDeviceHandle;//设备暂停运行时会回调
    RegisterDevTable.BlackoutDeviceHandle = BlackoutDeviceHandle;//设备通电时会回调
    RegisterDevTable.EnergizeDeviceHandle = EnergizeDeviceHandle;//设备断电时会回调

    RegisterDevTable.isSupportModifyData = 0;
    RegisterDevTable.ModifyDataHandle = 0;

    //设备内存读写/中断请求回调
    RegisterDevTable.WriteDeviceHandle = WriteDeviceHandle;//CPU向设备写数据时回调
    RegisterDevTable.ReadDeviceHandle = ReadDeviceHandle;//CPU从设备读数据时回调
    RegisterDevTable.GetIsHaveInterruptAskHandle = GetIsHaveInterruptAskHandle;//CPU判断设备是否有中断请求的回调
    return RegisterDevTable;
}

//获取uint中[m:n]位置的数据
static uint32_t getBitsData(uint32_t d,int m,int n){
            if(m>31 || n>31 || m<0 || n<0 || m<n){
                return d;
            }
            uint32_t mask = ((1u <<(m-n+1))-1)<<n;
            return (d & mask)>>n;
        }


uint8_t *mem;
LedGroupWindow * ledgroupWindow;
/*
 *功能:CPU向当前设备写入数据时回调
 *参数-data: 写入的1字节数据
 *参数-add:  写入当前设备的寄存器地址(相对于当前设备挂载的内存总线首地址的偏移地址)
 */
static void WriteDeviceHandle(uint8_t data, uint32_t add) {
    ledgroupWindow->isWriting = 1;
    mem[add] = data;
    ledgroupWindow->ledStates[add] = data;
    ledgroupWindow->isWriting = 0;
}

/*
 *功能:CPU从当前设备写读取据时回调
 *参数-add: 写入当前设备的寄存器地址(相对于当前设备挂载的内存总线首地址的偏移地址)
 *返回值:   返回读取的1字节数据
 */
static uint8_t ReadDeviceHandle(uint32_t add) {
    return mem[add];
}

/*功能:启动SOC的运行时回调*/
static void StartDeviceHandle(void) {

}

/*功能:暂停SOC的运行时回调*/
static void StopDeviceHandle(void) {

}



int windowX = -1,windowY = -1;
int windowW = -1,windowH = -1;
/*功能:SOC断电时回调*/
static void BlackoutDeviceHandle(void) {
    free(mem);
    ledgroupWindow->isAllowClose = 1;
    ledgroupWindow->close();
    windowX = ledgroupWindow->x();
    windowY = ledgroupWindow->y();
    windowW = ledgroupWindow->width();
    windowH = ledgroupWindow->height();
    delete ledgroupWindow;
}

/*功能:SOC上电时回调*/
static void EnergizeDeviceHandle(void) {

    mem = new uint8_t[DevMemLength];
    for(uint i = 0;i<DevMemLength;i++){
        mem[i] = 0x00;
    }

    ledgroupWindow = new LedGroupWindow(ledCount,ledColor);
    ledgroupWindow->setWindowFlag(Qt::WindowCloseButtonHint,false);

    if(windowX!=-1 && windowY!=-1){
        ledgroupWindow->move(windowX,windowY);
    }
    if(windowW!=-1 && windowH!=-1){
        ledgroupWindow->resize(windowW,windowH);
    }
    ledgroupWindow->show();
}

/*
 *功能:CPU读取设备是否有中断请求时回调(如果<模块信息注册表>中设定了当前设备无中断请求功能,则CPU不会回调该函数)
 *返回值:[0无请求,1有请求]
*/
static uint8_t GetIsHaveInterruptAskHandle(void) {
    return 0;
}

/*功能:加载MOD时回调*/
static void LoadDeviceMod(void) {
}

/*功能:卸载MOD时回调*/
static void UnloadDeviceMod(void) {
}
