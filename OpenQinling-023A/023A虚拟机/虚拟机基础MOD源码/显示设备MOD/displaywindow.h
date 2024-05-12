#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QWidget>
#include <QResizeEvent>
#include <QTimer>
#include <QCursor>
#include <QDebug>
#include <datastruct.h>
namespace Ui {
class DisplayWindow;
}

class DisplayWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayWindow(QSize displaySize,uint8_t*mouseInputReg,QWidget *parent = nullptr);
    ~DisplayWindow();

    uint8_t*mouseInputReg;

    QTimer flashDisplayTimer;
    QString titleText;

    bool isWritingVRAM = 0;
    QImage displayMap;

    void flashDisplay();
    void resizeEvent(QResizeEvent *event) override;

    bool mouseIsInDisplay = 0;

    void levelDisplayHandle();


    void flashMousePos(QMouseEvent *event);




    void leaveEvent(QEvent *event) override;
    void enterEvent(QEvent *event) override;


    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    float hwRation = 1;//宽高比

    bool isAllowClose = 0;//是否允许关闭该窗口
private:
    Ui::DisplayWindow *ui;
};

#endif // DISPLAYWINDOW_H
