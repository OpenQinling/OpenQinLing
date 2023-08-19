#ifndef GENERATEMEMORYMALLOCINFO_H
#define GENERATEMEMORYMALLOCINFO_H
#pragma execution_character_set("utf-8")
#include "AnalysisMemoryBlock.h"

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

//将MemoryMallocInfo输出为tb测试文本
QStringList generateTbText(QList<MemoryMallocInfo> info){
    QString data_tb;
    QString code_tb;
    bool hd_or_fn=0;//0为冯诺依曼，1为哈佛
    for(int i = 0;i<info.length();i++){
        if(i==0){
            if(info[i].device==0){
                hd_or_fn = 0;
            }else{
                hd_or_fn = 1;
            }
        }

        if(!info[i].isUsage){
            continue;
        }

        QString tmp;
        for(int j = 0;j<info[i].bin.length();j++){
            QString num = QString::number((uchar)info[i].bin[j],16);
            while(num.length()<2){
                num.prepend("0");
            }
            num = "\tinitial data["+QString::number(j+info[i].baseAddress)+"]=8'h"+num+";\r\n";
            tmp +=num;
        }

        if(info[i].device==1){
            code_tb += tmp;
        }else{
            data_tb += tmp;
        }
    }

    if(hd_or_fn){
        return {data_tb,code_tb};
    }else{
        return {data_tb};
    }
}


#endif // GENERATEMEMORYMALLOCINFO_H
