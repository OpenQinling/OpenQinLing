#include <QDebug>
#include <QTimer>
#define C_EXPORT __declspec(dllexport)
#define CPP_EXPORT extern "C" __declspec(dllexport)
#include <datastruct.h>
#include <QThread>
#include <QElapsedTimer>
#include <math.h>
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
static const char* DevTypeName = "Timer";
static const char* DevFunInfo = "毫秒级定时器设备,占用字6节内存寻址空间。"
                                "其中[0][1]储存当前的定时计数,"
                                "[2][3]存储定时总时间,"
                                "[4]是否触发了中断(CPU中断响应后应向该处写入0x00来关闭中断),"
                                "[5]是否启动定时器定时";
static uint32_t DevMemLength = 6;
static uint8_t isHaveInterruptAsk = 1;

#define IS_SUPPORT_CONFIG 0 //是否支持MOD配置功能

QString packUnzipPath;
QString coreDllPath;

#if IS_SUPPORT_CONFIG
    //配置MOD接口:(packUnzipPath_utf8:当前MOD包解压缩的地址,utf8格式)
    CPP_EXPORT void ConfigModule(const char*packUnzipPath_utf8,const char*coreDllPath_utf8){

    }
#endif

//获取MOD信息接口:(packUnzipPath_utf8:当前MOD包解压缩的地址,utf8格式)
CPP_EXPORT ModeInfo GetModeRegisterTable(const char*packUnzipPath_utf8,const char*coreDllPath_utf8){
    packUnzipPath = packUnzipPath_utf8;
    coreDllPath = coreDllPath_utf8;

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


uint8_t * memData;
/*
 *功能:CPU向当前设备写入数据时回调
 *参数-data: 写入的1字节数据
 *参数-add:  写入当前设备的寄存器地址(相对于当前设备挂载的内存总线首地址的偏移地址)
 */
static void WriteDeviceHandle(uint8_t data, uint32_t add) {
    memData[add] = data;
}

/*
 *功能:CPU从当前设备写读取据时回调
 *参数-add: 写入当前设备的寄存器地址(相对于当前设备挂载的内存总线首地址的偏移地址)
 *返回值:   返回读取的1字节数据
 */
static uint8_t ReadDeviceHandle(uint32_t add) {
    return  memData[add];
}

bool isTimerStart = 0;
bool isTimerEnergize = 0;
QThread *timerThread = NULL;
/*功能:启动SOC的运行时回调*/
static void StartDeviceHandle(void) {
    isTimerStart = 1;
}

/*功能:暂停SOC的运行时回调*/
static void StopDeviceHandle(void) {
    isTimerStart = 0;
}



/*功能:SOC断电时回调*/
static void BlackoutDeviceHandle(void) {
    isTimerEnergize = 0;
    while(!timerThread->isFinished());
    delete timerThread;
    timerThread = NULL;
    delete [] memData;
}




/*功能:SOC上电时回调*/
static void EnergizeDeviceHandle(void) {
    memData = new uint8_t[DevMemLength];
    for(uint32_t i = 0;i<DevMemLength;i++){
        memData[i] = 0x00;
    }
    isTimerEnergize = 1;
    timerThread = QThread::create([=](){
        QElapsedTimer timer;
        timer.start();
        while(isTimerEnergize){
            uint64_t s = timer.elapsed();
            QThread::msleep(1);
            if(!memData[5] && !isTimerStart)continue;
            uint64_t count = WORD(memData[0],memData[1]).toUShort();
            uint64_t total = WORD(memData[2],memData[3]).toUShort();

            uint64_t e = timer.elapsed()-s;
            count+=fmax(e,1);//防止=0

            if(count>=total){
                memData[4] = 0xff;
                uint64_t diff = count-total;
                WORD wc((uint16_t)diff);
                memData[0] = wc[0];
                memData[1] = wc[1];
            }else{
                WORD wc((uint16_t)count);
                memData[0] = wc[0];
                memData[1] = wc[1];
            }
        }
    });
    timerThread->start(QThread::TimeCriticalPriority);
}

/*
 *功能:CPU读取设备是否有中断请求时回调(如果<模块信息注册表>中设定了当前设备无中断请求功能,则CPU不会回调该函数)
 *返回值:[0无请求,1有请求]
*/
static uint8_t GetIsHaveInterruptAskHandle(void) {
    return memData[4];
}

/*功能:加载MOD时回调*/
static void LoadDeviceMod(void) {
}

/*功能:卸载MOD时回调*/
static void UnloadDeviceMod(void) {
}
