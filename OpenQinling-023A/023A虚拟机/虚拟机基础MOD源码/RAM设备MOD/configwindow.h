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

    uint32_t memSize = 512;//单位MB
private:
    Ui::ConfigWindow *ui;

    void closeEvent(QCloseEvent *) override;

};

#endif // CONFIGWINDOW_H
