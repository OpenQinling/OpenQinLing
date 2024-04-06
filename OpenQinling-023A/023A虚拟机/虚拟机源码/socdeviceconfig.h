#ifndef SOCDEVICECONFIG_H
#define SOCDEVICECONFIG_H

#include <QWidget>
#include <QtDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QLabel>
#include <devicemode.h>
#include <QList>
#include <math.h>
//设备类型
enum DeviceType{
    Device_CPU,//中央处理器
    Device_ROM,//断电不丢存储器
    Device_RAM,//断电丢失存储器
    Device_Display,//显示器
    Device_Sound,//音响
    Device_Other//其它类型
};
class SocDeviceConfig : public QLabel
{
    Q_OBJECT
public:
    SocDeviceConfig(QWidget *parent,QList<DeviceMode> *devPtr);

    ~SocDeviceConfig();




    QPixmap displayPixmap;//显示的图像

    bool isRunning = 0;//渲染的图像是否是运行中的样式

    double scale = 0.75;//缩放倍数[数值越大,视野越小,显示的图像越大;数值越小,视野越大,显示的图像越小]
    double displayX = 0,displayY = 0;//显示的位置
    double minX,minY,maxX,maxY;

    QList<QPoint> allDeviceIconPoint;//所有设备的图标位置
    QList<DeviceMode> *devModeList;//设备模块列表指针



    enum TriangleDir{
        Up,
        Down,
        Left,
        Right
    };

    //画一个等边三角形
    void drawTriangle(QPainter &pter,int x,int y,int size,TriangleDir dir);

    //绘制一条导线
    QPoint drawCircuitLine(QPoint startPoint,int length,int angle,QPainter&painter,int lastAnale=-1);

    //绘制显示的图像
    void drawDisplayPixmap();

    //刷新显示的内容
    void flashDisplay();

    //设置为运行状态
    bool start();

    //设置为关闭状态
    bool stop();

signals:
private:
    int lastMouseX = -1,lastMouseY = -1;

    //鼠标按下的信号
    void mousePressEvent(QMouseEvent *event) override;


    //鼠标移动信号[移动视角]
    void mouseMoveEvent(QMouseEvent *event) override;


    //鼠标双击信号处理[查看外设及CPU的详细信息,以及配置信息]
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    //鼠标滚轮信号处理[缩放渲染大小]
    void wheelEvent(QWheelEvent *event) override;

    //视窗缩放事件
    void resizeEvent(QResizeEvent*e)override;


};

#endif // SOCDEVICECONFIG_H
