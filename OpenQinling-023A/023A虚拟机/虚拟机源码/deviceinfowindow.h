#ifndef DEVICEINFOWINDOW_H
#define DEVICEINFOWINDOW_H

#include <QWidget>
#include <QTimer>
#include <devicemode.h>
namespace Ui {
class DeviceInfoWindow;
}

class DeviceInfoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceInfoWindow(QWidget *parent = nullptr);
    ~DeviceInfoWindow();

    //显示一个设备的信息
    void showDeviceInfo(int index);

private:
    int InfoPage_xOffset,InfoPage_yOffset,InfoPage_w,InfoPage_h;

    int mouseLastX,mouseLastY;
    Ui::DeviceInfoWindow *ui;
    void showEvent(QShowEvent *event)override;
    void closeEvent(QCloseEvent *event)override;
    //窗口大小改变信号
    void resizeEvent(QResizeEvent*e)override;
    bool eventFilter(QObject*obj,QEvent*event)override;

    void moveInfoPage();

    void flashCloseButton(int isHover = 0);

    int devIndex;
    DeviceMode *devMode;

    bool isHaveChange = 0;//设备信息是否有过改变
    void flashShowIcon();
};

#endif // DEVICEINFOWINDOW_H
