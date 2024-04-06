#include "configwindow.h"
#include "ui_configwindow.h"

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
    memSize = ui->spinBox_memSize->value();
}
