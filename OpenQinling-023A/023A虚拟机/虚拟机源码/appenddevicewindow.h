#ifndef APPENDDEVICEWINDOW_H
#define APPENDDEVICEWINDOW_H

#include <QWidget>
#include <devicemode.h>
#include <QtGui/private/qzipreader_p.h>
namespace Ui {
class AppendDeviceWindow;
}



class AppendDeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AppendDeviceWindow(QWidget *parent = nullptr);
    ~AppendDeviceWindow();

    void setSelectPckPath(QString path);
private:
    int MainWidget_w,MainWidget_h;
    int mouseLastX,mouseLastY;
    int MainWidget_xOffset,MainWidget_yOffset;
    void moveInfoPage();
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent*e);
    bool eventFilter(QObject*obj,QEvent*event);
    Ui::AppendDeviceWindow *ui;

    QString devName;//分配的设备名
    struct{
        bool isValid = 0;//是否有效
        uint32_t allocMemAddress;//分配的内存地址
        int allocInterNum;//分配的中断号(-1为未分配)

        QString typeName;
        QString funInfo;
        uint32_t menLength;
        bool isUseInter;
        QString coreLibPath;//核心lib位置
        QString unzipPath;//解包后临时存储位置
    }addModeInfo;

    bool packageModeInfo(QString zipPath);
    void flashShowIcon();


};

#endif // APPENDDEVICEWINDOW_H
