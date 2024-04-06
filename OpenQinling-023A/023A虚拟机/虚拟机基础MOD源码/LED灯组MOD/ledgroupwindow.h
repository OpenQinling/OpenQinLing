#ifndef LEDGROUPWINDOW_H
#define LEDGROUPWINDOW_H

#include <QWidget>
#include <QVector>
#include <QTimer>
namespace Ui {
class LedGroupWindow;
}

class LedGroupWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LedGroupWindow(int count,QColor ledColor,QWidget *parent = nullptr);
    ~LedGroupWindow();

    bool isAllowClose = 0;
    int count;
    QColor ledColor;

    const int ledSize = 50;//单个led灯的大小

    QVector<uint8_t> ledStates;//各个led的状态

    QPixmap drawLedPixmap(int index);//绘制一个LED显示的画面

    void flashDisplay();//刷新画面

    QTimer flashTimer;

    bool isWriting = 0;
private:
    void closeEvent(QCloseEvent *event) override;
    Ui::LedGroupWindow *ui;
};

#endif // LEDGROUPWINDOW_H
