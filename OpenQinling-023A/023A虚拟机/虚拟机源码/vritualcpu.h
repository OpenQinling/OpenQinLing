#ifndef VRITUALCPU_H
#define VRITUALCPU_H
#include <QThread>
#include <toolfunction.h>
#include <datasturct.h>
#include <QDebug>
#include <QElapsedTimer>
using namespace std;

class VritualCPU
{
public:

    VritualCPU(function<void(void)>StartDevice,
               function<void(void)>StopDevice,
               function<void(void)>BlackoutDevice,
               function<void(void)>EnergizeDevice,
               function<void(uint8_t data,uint32_t add)>WriteDevice,
               function<uint8_t(uint32_t add)>ReadDevice,
               function<uint8_t(uint8_t &num)>GetIsHaveInterruptAsk,
               function<void(void)>cpuStopCallBack);

    ~VritualCPU();


    //运行cpu(step传入运行的步数,-1则为持续运行)
    void run(int step = -1);


    //重启cpu(重启后cpu会处于暂停中)
    void restart();


    //强制暂停运行中的cpu
    void stop();


    //获取当前是否正在运行,还有多少步要运行(-1为持续运行)
    int getIsRuning(){
        if(runStep>0 || runStep==-1){
            return runStep;
        }
        return 0;
    }

    function<void(void)>cpuStopCallBack = [=](){};

    function<void(void)>StartDevice = [=](){};//运行设备
    function<void(void)>StopDevice = [=](){};//暂停设备
    function<void(void)>BlackoutDevice = [=](){};//设备断电回调
    function<void(void)>EnergizeDevice = [=](){};//设备通电回调
    function<void(uint8_t data,uint32_t add)>WriteDevice = [=](uint8_t data,uint32_t add){};//写入数据
    function<uint8_t(uint32_t add)>ReadDevice = [=](uint32_t add)->uint8_t{return 0xffffffff;};//读取数据
    function<uint8_t(uint8_t &num)>GetIsHaveInterruptAsk = [=](uint8_t &num)->uint8_t{return 0;};//获取当前是否有中断请求
    enum RegIndex{
        r1,
        r2,
        r3,
        r4,
        r5,
        r6,
        r7,
        r8,
        flag,
        pc,
        tpc,
        ipc,
        sp,
        tlb,
        sys
    };
    //cpu寄存器数据
    uint32_t regGroup[15];
private:
    QThread * cpuRunThread;
    int runStep = -2;
    bool isDeleteCpuObj = 0;

    uint32_t ReadDeviceFun(uint32_t add,RwDatas size);

    void WriteDeviceFun(uint32_t data,uint32_t add,RwDatas size);


    //获取内存页信息
    uint32_t getVirtualPageInfo(uint32_t virAdd,bool &isHavePage,bool &pageIsInStorge,bool &isReadOnlyPage);


    //cpu运行一步
    void runOwnStep();

    //读写寄存器数据
    void setRegData(uint32_t d,int regIndex);
    uint32_t getRegData(int regIndex);

};

#endif // VRITUALCPU_H
