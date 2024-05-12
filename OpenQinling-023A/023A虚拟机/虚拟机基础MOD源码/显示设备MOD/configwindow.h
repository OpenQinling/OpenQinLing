#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QDialog>

namespace Ui {
class ConfigWindow;
}

enum ColorSystem{
    RGB888,//彩屏rgb888
    RGB565,//彩屏rgb565
    Monoch//黑白屏
};
class ConfigWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);
    ~ConfigWindow();

    ColorSystem colorSystem = RGB888;

    int width_px = 50;
    int height_px = 50;


private:
    void closeEvent(QCloseEvent *) override;

    Ui::ConfigWindow *ui;
};

#endif // CONFIGWINDOW_H
