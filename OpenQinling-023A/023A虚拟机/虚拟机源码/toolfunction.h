#ifndef TOOLFUNCTION_H
#define TOOLFUNCTION_H

#include <QString>
#include <stdint.h>
#include <QPixmap>
#include <QDebug>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QtGui/private/qzipreader_p.h>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonObject>
#include <QJsonValue>
using namespace std;
enum RwDatas{
    RW_BYTE,
    RW_WORD,
    RW_DWORD
};
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


struct BlockInfo{
    unsigned long long baseAdress;
    unsigned long long sizeBytes;
};

namespace ToolFunction
{
    //使用多线程执行一个任务,所有线程都执行完该任务后函数退出(threadCount=0,默认使用cpu物理核心数相等的线程数)(stackSize=0 单个任务的堆栈大小,默认系统分配)
    void multithThreadExec(function<void(int threadIndex)>taskFun,int threadCount = 0,uint32_t stackSize = 0);

    //解压缩包到临时目录中
    bool unzipPackage(QString zipPath,QString&unzipPath,QString &coreLibPath);

    //生成虚拟设备的图标
    QPixmap crateIconMap(QString devType,QString devName,uint32_t beginAddress,uint32_t useMemLength,int useInterNum);

    //生成16进制文本
    QString transHexString(uint32_t hexNum);

    //获取uint中[m:n]位置的数据
    uint32_t getBitsData(uint32_t d,int m,int n);



    bool getDevModInfo(QString packPath,QString coreLibPath,QString &typeName,QString &funInfo,uint32_t &memLen,bool &isUseInter);

    //获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
    QList<bool> getBlockOverlap(QList<BlockInfo> memSpace);

    //整合重叠的区域,并从小到大排序
    QList<BlockInfo> integrationOverlapPlace(QList<BlockInfo> memSpace);

    //判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
    bool judgeContainBlockSet(QList<BlockInfo> memSpaceA,
                              QList<BlockInfo> memSpaceB);

    //在块集A上减去块a
    QList<BlockInfo> removeOccupiedMemBlock(QList<BlockInfo> memSpaceA,
                                            BlockInfo mema);

    //在块集A上找到能放下块a的位置
    BlockInfo searchFreeMemarySpace(QList<BlockInfo> memSpaceA,uint mema_size,bool *isScueess=NULL);

    //复制目录以及目录下所有文件
    bool copyDir(const QString &srcPath, const QString &dstPath);

    //获取2个路径的公共路径
    QString comPath(QString aPath,QString bPath);

};

#endif // TOOLFUNCTION_H
