#ifndef ANALYSISMEMORYMALLOCINFO_H
#define ANALYSISMEMORYMALLOCINFO_H
#include <QBuffer>
#include <QDataStream>

//内存分配信息:[用于记录在编译完成后，物理内存的利用情况]
/*从编译出的.bin文件中会解析出QList<MemoryMallocInfo>，如果自行开发编译器，应当遍历这些MemoryMallocInfo对象,
  并根据对象中的内存块分配信息，将其中的二进制数据写入指定内存设备的指定地址上*/
class MemoryMallocInfo{
public:
    uint baseAddress;//起始地址
    uint endAddress;//结束地址
    int isUsage=0;//是否被使用[0为空，1为被常量区/代码区使用了，2为被栈区使用了]
    QString name;//如果被使用了，使用该内存的内存块名称
    QByteArray bin;//如果被使用了，该内存中存储的二进制数据
    int device =0;//当前内存所属的物理设备。如果是冯诺依曼架构就是0，
                  //如果是哈佛架构,指令内存为1，数据内存为2
    int devID = 0;//设备号[一个程序数据可能分配在多个物理存储器上，该参数记录当前分配的物理存储器编号]

    QString toString(){
        QString type;
        if(isUsage==0){
            type = "\033[36mFree\033[0m";
        }else if(isUsage==1){
            type = name;
        }else if(isUsage==2){
            type = "\033[35mStack\033[0m";
        }

        QString dev = "BLOCK";
        if(device==1){
            dev = "CODE";
        }else if(device==2){
            dev = "DATA";
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

        double size = endAddress-baseAddress+1;
        int unit = 0;//长度的单位(0B 1KB 2MB 3GB)
        while(size>=1024){
            size /= 1024;
            unit++;
        }

        QString sizeText = QString::number(size);
        if(sizeText.contains(".")){
            //如果sizeText存在小数，小数位数限制在2位
            int i = 0;
            while(sizeText.at(sizeText.length()-1-i)!='.'){
                i++;
            }

            while(i>2){
                i--;
                sizeText.remove(sizeText.length()-1,1);
            }
        }


        QString uintText;
        if(unit==0){
            uintText = "B";
        }else if(unit==1){
            uintText = "KB";
        }else if(unit==2){
            uintText = "MB";
        }else if(unit==2){
            uintText = "GB";
        }

        return dev+QString::number(devID)+":\t-area["+base+"-"+end+"]\t-type["+type+"]\t-size["+sizeText+uintText+"]";
    }
};
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
        stream>>memInfo.devID;
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