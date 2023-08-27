#ifndef ANALYSISMEMORYMALLOCINFO_H
#define ANALYSISMEMORYMALLOCINFO_H
#include <QByteArray>
#include <QString>
#include <QBuffer>
#include <QDataStream>
//内存分配信息:[用于记录在编译完成后，物理内存的利用情况]
class MemoryMallocInfo{
public:
    uint baseAddress;//起始地址
    uint endAddress;//结束地址
    int isUsage=0;//是否被使用[0为空，1为被常量区/代码区使用了，2为被栈区使用了]
    QString name;//如果被使用了，使用该内存的内存块名称
    QByteArray bin;//如果被使用了，该内存中存储的二进制数据
    int device =0;//当前内存所属的物理设备。如果是冯诺依曼架构就是0，
                  //如果是哈佛架构,指令内存为1，数据内存为2

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
//把MemoryMallocInfo整合成.bin文件的二进制文件结构
QByteArray inteMemory(QList<MemoryMallocInfo> mem){
    QByteArray list;
    QBuffer buffer(&list);
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream<<mem.length();
    foreach(MemoryMallocInfo info,mem){
        stream<<info.isUsage;
        stream<<info.device;
        stream<<info.baseAddress;
        stream<<info.endAddress;
        if(info.isUsage==1){
            stream<<info.name.toUtf8().length();
            foreach(uchar c,info.name.toUtf8()){
                 stream<<c;
            }
            stream<<info.bin.length();
            foreach(uchar c,info.bin){
                 stream<<c;
            }
        }
    }
    return list;
}
//从.bin文件中分离出MemoryMallocInfo
QList<MemoryMallocInfo> deconMemory(QByteArray bin){
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
#endif // ANALYSISMEMORYMALLOCINFO_H