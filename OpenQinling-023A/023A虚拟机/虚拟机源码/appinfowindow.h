#ifndef APPINFOWINDOW_H
#define APPINFOWINDOW_H

#include <QWidget>

namespace Ui {
class AppInfoWindow;
}

class AppInfoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AppInfoWindow(QWidget *parent = nullptr);
    ~AppInfoWindow();


private:
    Ui::AppInfoWindow *ui;
    void moveInfoPage();
    void showEvent(QShowEvent *event)override;
    void resizeEvent(QResizeEvent*e)override;
    bool eventFilter(QObject*obj,QEvent*event)override;
    void flashCloseButton(int isHover = 0);

    int MainWidget_w,MainWidget_h;
    int mouseLastX,mouseLastY;
    int MainWidget_xOffset,MainWidget_yOffset;
};

#endif // APPINFOWINDOW_H
