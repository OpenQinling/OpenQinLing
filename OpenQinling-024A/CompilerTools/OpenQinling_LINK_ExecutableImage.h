#ifndef OPENQINLING_LINK_EXECUTABLEIMAGE_H
#define OPENQINLING_LINK_EXECUTABLEIMAGE_H
#include <QList>
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{





class ExecutableMemBlock{
public:
    uint64_t baseAdd = 0;//内存块的基地址
    uint64_t len = 0;    //内存块的长度
    bool isZiData = 0;//是否为空数据块
    QByteArray data;//内存块中的数据
};

//可执行程序映像数据
class ExecutableImage:public QList<ExecutableMemBlock>{
public:
    //转为etb格式的文件数据(OpenQinling-CPU编译工具链专用程序映像数据存储格式)
    QByteArray toEtbFileData();
    //转为bin格式的文件数据(从0x00000000位置开始的二进制数据,空白区域填充0x00)
    QByteArray toBinFileData();
    //转为coe格式的文件数据(从0x00000000位置开始的二进制数据,空白区域填充0x00)
    QByteArray toCoeFileData(uint byteWidth = 8);

    //mode: 0只打印出各个内存块的基址/大小 1还打印出各内存块的二进制数据
    QString toString(bool mode=0);
    //从etb格式的文件数据中提取出可执行程序映像数据对象
    bool fromEtbFileData(QByteArray data);

private:
    void reorder();
};




}}
QT_END_NAMESPACE

#endif // OPENQINLING_LINK_EXECUTABLEIMAGE_H
