#ifndef DEVICEMODE_H
#define DEVICEMODE_H

#include <QString>
#include <QPixmap>
#include <QPainter>
#include <vritualcpu.h>

#include <toolfunction.h>

class DeviceMode
{
public:
    DeviceMode();
    QPixmap iconMap;//设备图标图片
    QString devType;//设备类型名
    QString devName;//设备名


    QString devFunText;//设备功能作用文本


    int useInterNum = -1;//设备使用的中断号(>=256或<0就是不使用中断功能)
    int isSupportInterruptAsk = 0;//设备本身是否支持中断触发功能

    int beginAddress = 0;//设备占用的内存起始地址
    int useMemLength = 0;//设备占用的内存字节数

    QPoint point;//设备图标显示时的位置

    //设备回调函数
    //设备加载、卸载、信息获取回调
    function<void(void)>LoadDeviceMod = NULL;
    function<void(void)>UnloadDeviceMod = NULL;

    //设备启停回调
    function<void(void)>StartDevice = NULL;//设备启动回调(cpu启动时会回调)
    function<void(void)>StopDevice = NULL;//设备暂停回调(cpu被暂停时会回调)
    function<void(void)>BlackoutDevice = NULL;//设备断电回调(cpu重启/虚拟机应用退出/删除设备时会回调)
    function<void(void)>EnergizeDevice = NULL;//设备通电回调(cpu重启/虚拟机应用启动/第一次加载设备时会回调)

    //修改设备内部数据回调
    function<void(void)>ModifyData = NULL;

    //设备读写回调
    function<void(uint8_t data, uint32_t add)>WriteDevice = NULL;//向设备写入数据回调
    function<uint8_t(uint32_t add)>ReadDevice = NULL;//从设备读取数据回调
    function<uint8_t(void)>GetIsHaveInterruptAsk = NULL;//读取设备是否有中断请求的回调

    QString devFolderName;//设备实体存储的文件夹名称
    void* dllHandler;//设备DLL的句柄
    QString coreDllPath;//设备核心DLL在存储文件夹中的相对路径

    //生成iconMap
    void crateIconMap();

};

#endif // DEVICEMODE_H
