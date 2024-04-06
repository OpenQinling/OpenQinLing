#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QEvent>
#include <QObject>
#include <datasturct.h>
#include <socdeviceconfig.h>
#include <deviceinfowindow.h>
#include <devicemode.h>
#include <vritualcpu.h>
#include <initromwindow.h>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <appenddevicewindow.h>
#include <appinfowindow.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum Reg_NumSysType{
    NumSys_UH,
    NumSys_UD,
    NumSys_UB,
    NumSys_INT,
    NumSys_F
};

enum Reg{
    Merge,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    FLAG,
    TPC,
    IPC,
    SP,
    PC,
    SYS,
    TLB,
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initGUIStyle();

    //设置一个寄存器显示的数据
    void setRegValue(Reg reg,DWORD data);
    //设置一个寄存器显示的数值方式
    void setRegSys(Reg reg,Reg_NumSysType sys);
    //显示设备详细信息界面
    void showDevieInfoPage(int deviceIndex);


    //当前所有外设模块
    QList<DeviceMode> allUseDeviceMode;

    //获取所有没有被占用的外部中断号(index不搜索的设备)
    QList<int> getAllFreeInterNum(int index = -1);

    //获取所有没有被占用的内存区域
    QList<BlockInfo> getAllFreeMemBlock(int index = -1);

    //判断一个设备名称是否已存在
    bool judgeIsHaveDev(QString name);

    //新增一个设备时的默认名称
    QString newDevDefaultName();



    void CPU_StopCallBack();
    void CPU_StartCallBack();
    //事件处理定时器
    QTimer *checkCpuIsStopTimer;
    bool isStop = 0;

    //事件处理器
    bool eventFilter(QObject*obj,QEvent*event)override;
    //窗口大小改变信号
    void resizeEvent(QResizeEvent*e)override;

    void closeEvent(QCloseEvent *event)override;

    SocDeviceConfig * socDevConfigPage;
    AppInfoWindow * appInfoWindow;
    //寄存器数据信息
    DWORD r1_data,r2_data,
          r3_data,r4_data,
          r5_data,r6_data,
          r7_data,r8_data,
          sp_data,pc_data,
          tpc_data,ipc_data,
          sys_data,tlb_data,
          flag_data;
    //寄存器数据在gui界面显示时的方式类型
    Reg_NumSysType r1_type = NumSys_UH,r2_type = NumSys_UH,
                    r3_type = NumSys_UH,r4_type = NumSys_UH,
                    r5_type = NumSys_UH,r6_type = NumSys_UH,
                    r7_type = NumSys_UH,r8_type = NumSys_UH,
                    sp_type = NumSys_UH,pc_type = NumSys_UH,
                    tpc_type = NumSys_UH,ipc_type = NumSys_UH,
                    sys_type = NumSys_UH,tlb_type = NumSys_UH,
                    flag_type = NumSys_UH,merge_type = NumSys_UH;
    //Merge合并的寄存器
    Reg HighReg = R1,LowReg = R1;

    DWORD getRegValue(Reg reg);

    //虚拟cpu
    VritualCPU * vriCPU;

    //将寄存器数据转为文本
    static QString regValueToString(DWORD value,Reg_NumSysType type);
    static QString mergeRegValueToString(DWORD valueH,DWORD valueL,Reg_NumSysType type);


    //刷新显示寄存器数值
    void flashShowRegValue();
    //刷新内存展示的值
    void flashShowMemData();
    //刷新硬件配置界面
    void flashDevConfigWindow();
    //增加一个设备
    void appendDevice(QString addPckPath = "");


    DeviceInfoWindow *deviceInfoWindow;
    InitRomWindow * initRomWindow;
    AppendDeviceWindow * appendDeviceWindow;
    int window_w,window_h;
    int ShowRAMdataTextArea_w,ShowRAMdataTextArea_h;
    int widget_18_h,widget_18_w;
    int widget_19_h,widget_19_w;
    int widget_20_h,widget_20_w;
    int widget_22_h,widget_22_w;
    int deviceInfoWindow_h,deviceInfoWindow_w;
    int InitROMbutton_h,InitROMbutton_w;
    int InitROMbutton_x,InitROMbutton_y;
    int SocSetingPage_h,SocSetingPage_w;
    int label_41_h,label_41_w;
    int AddDev_Button_x,AddDev_Button_y;
    int AddDevPrompt_x,AddDevPrompt_y;
    Ui::MainWindow *ui;

    QString lastShowRAMStartPlaceText;
    QString lastShowRAMEndPlaceText;




};

void exitHandle();
extern MainWindow *mainWindow;
extern QJsonDocument settingJsonDoc;
extern QByteArray innerFlashData;
#endif // MAINWINDOW_H
