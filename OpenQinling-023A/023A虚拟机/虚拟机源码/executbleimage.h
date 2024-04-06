#ifndef EXECUTBLEIMAGE_H
#define EXECUTBLEIMAGE_H


#include <QList>
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
class ExecutableMemBlock{
public:
    uint baseAdd = 0;//内存块的基地址
    uint len = 0;    //内存块的长度
    bool isZiData = 0;//是否为空数据块
    QByteArray data;//内存块中的数据
};

//可执行程序映像数据
class ExecutableImage:public QList<ExecutableMemBlock>{
public:
    //mode: 0只打印出各个内存块的基址/大小 1还打印出各内存块的二进制数据
    QString toString(bool mode=0);

    //从etb格式的文件数据中提取出可执行程序映像数据对象
    bool fromEtbFileData(QByteArray data);

    //转为etb格式的文件数据(OpenQinling-CPU编译工具链专用程序映像数据存储格式)
    QByteArray toEtbFileData();


private:
    //根据内存块地址从低到高排序
    void reorder();

};

#endif // EXECUTBLEIMAGE_H
