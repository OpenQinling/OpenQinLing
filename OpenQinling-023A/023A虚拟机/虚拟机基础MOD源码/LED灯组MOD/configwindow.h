#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QDialog>

namespace Ui {
class ConfigWindow;
}

class ConfigWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);
    ~ConfigWindow();


    int r,g,b;//led颜色
    int count;//led组中的led数量

private:
    void closeEvent(QCloseEvent *) override;
    Ui::ConfigWindow *ui;
};

#endif // CONFIGWINDOW_H
