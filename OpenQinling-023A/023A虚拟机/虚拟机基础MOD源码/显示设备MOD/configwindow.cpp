#include "configwindow.h"
#include "ui_configwindow.h"

void test(int &a){

}

ConfigWindow::ConfigWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton,&QPushButton::clicked,[=](){
        this->close();
    });


}

ConfigWindow::~ConfigWindow()
{
    delete ui;
}

void ConfigWindow::closeEvent(QCloseEvent *){
    if(ui->comboBox_colorSys->currentText()=="彩屏RGB888"){
        this->colorSystem = RGB888;
    }else if(ui->comboBox_colorSys->currentText()=="彩屏RGB565"){
        this->colorSystem = RGB565;
    }else if(ui->comboBox_colorSys->currentText()=="黑白屏"){
        this->colorSystem = Monoch;
    }
    this->width_px = ui->spinBox_w->value();
    this->height_px = ui->spinBox_h->value();
}
