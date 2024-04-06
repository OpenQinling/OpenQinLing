#ifndef INITROMWINDOW_H
#define INITROMWINDOW_H

#include <QWidget>
#include <executbleimage.h>
namespace Ui {
class InitRomWindow;
}

class InitRomWindow : public QWidget
{
    Q_OBJECT

public:
    explicit InitRomWindow(QWidget *parent = nullptr);
    ~InitRomWindow();

private:
    Ui::InitRomWindow *ui;
    int MainWidget_w;
    int MainWidget_h;
    int MainWidget_xOffset;
    int MainWidget_yOffset;
    int mouseLastX,mouseLastY;
    ExecutableImage exeImge;
    void showEvent(QShowEvent *event)override;
    void closeEvent(QCloseEvent *event)override;
    bool eventFilter(QObject*obj,QEvent*event)override;
    //窗口大小改变信号
    void resizeEvent(QResizeEvent*e)override;
    void moveInfoPage();
};

#endif // INITROMWINDOW_H
