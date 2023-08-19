#ifndef GENERATEBIN_H
#define GENERATEBIN_H
#include <QList>
#include <QIODevice>
#include <QBuffer>
#include <QByteArray>


class MemoryMallocInfo{
public:
    uint baseAddress;//当前内存块的起始地址
    uint endAddress;//当前内存块的结束地址
    int isUsage=0;//当前内存块的是否被使用[0为空，1为被常量区/代码区使用了，2为被栈区使用了]
    //(对于0或2来说，这个内存块就没必要下载了,如果是1的话就根据下面的信息进行下载)

    QString name;//如果被使用了，使用该内存的内存块名称
    QByteArray bin;//如果被使用了，该内存中存储的二进制数据
    int device =0;//当前内存所属的物理设备。如果是冯诺依曼架构就是0，
                  //如果是哈佛架构,指令内存为1，数据内存为2

    //用于debug时打印输出MemoryMallocInfo的大致信息
    QString toString(){
        QString type;
        if(isUsage==0){
            type = "Free";
        }else if(isUsage==1){
            type = name;
        }else if(isUsage==2){
            type = "Stack";
        }

        QString dev;
        if(device==1){
            dev = ":CODE";
        }else if(device==2){
            dev = ":DATA";
        }
        QString base = QString::number(baseAddress,16);
        while (base.length()<8) {
            base.prepend("0");
        }
        base.prepend("0x");
        QString end = QString::number(endAddress,16);
        while (end.length()<8) {
            end.prepend("0");
        }
        end.prepend("0x");
        return "<"+base+"-"+end+">"+type+dev;
    }
};
/*
 * MemoryMallocInfo中就存储了一个被编译出的内存块的完整信息。
 * 对于一个汇编程序来说，会有多个内存块，然后会将这些内存块的数据全部整合进.bin文件中
 * 所以从.bin中会解析出QList<MemoryMallocInfo>
 *
 * 如果要自行开发程序下载器，可以在下载器中先从.bin文件中解析出QList<MemoryMallocInfo>
 * 然后依次遍历每个MemoryMallocInfo，根据其描述的所属存储的设备、数据起始地址来将其下载入物理内存设备的指定位置
 */


//从.bin文件中分离出MemoryMallocInfo
QList<MemoryMallocInfo> deconBin(QByteArray bin){
    QList<MemoryMallocInfo> list;
    QBuffer buffer(&bin);
    buffer.open(QIODevice::ReadOnly);
    QDataStream stream(&buffer);

    int length;
    stream>>length;//取出bin中内存块的数量
    for(int i = 0;i<length;i++){
        MemoryMallocInfo memInfo;
        stream>>memInfo.isUsage;
        stream>>memInfo.device;
        stream>>memInfo.baseAddress;
        stream>>memInfo.endAddress;
        if(memInfo.isUsage==1){
            int dataLength;
            stream>>dataLength;
            qint8 *tmp = new qint8[dataLength];
            for(int j = 0;j<dataLength;j++){
                uchar c;
                stream>>c;
                tmp[j] = c;
            }
            memInfo.name = QByteArray((const char*)tmp,dataLength);
            delete [] tmp;
            stream>>dataLength;
            tmp = new qint8[dataLength];
            for(int j = 0;j<dataLength;j++){
                uchar c;
                stream>>c;
                tmp[j] = c;
            }
            memInfo.bin = QByteArray((const char*)tmp,dataLength);
            delete [] tmp;
        }
        list.append(memInfo);
    }

    return list;
}



#endif // GENERATEBIN_H
