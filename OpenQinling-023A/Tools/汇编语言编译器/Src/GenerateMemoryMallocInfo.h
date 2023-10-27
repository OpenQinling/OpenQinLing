#ifndef GENERATEMEMORYMALLOCINFO_H
#define GENERATEMEMORYMALLOCINFO_H
#pragma execution_character_set("utf-8")
#include "AnalysisMemoryBlock.h"
#include <QDir>
#define memArch_HD 0
#define memArch_FN 1

//物理内存设备类
class PRAM_Device{
public:
    int devID = 0;//设备号
    int device = 0;//设备类型(0为冯诺依曼架构内存 1哈佛架构指令内存 2哈佛架构数据内存)

    QList<MemoryMallocInfo> mallocInfo;//内存分配信息(从小到大排序)


    QString toString(){
        QString s;
        foreach(MemoryMallocInfo info,mallocInfo){
            s.append(info.toString()+"\r\n");
        }
        return "----------------------\r\n"+s;
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
        stream<<info.devID;
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


//从MemoryMallocInfo表中解析出PRAM_Device表
QList<PRAM_Device> analysiPRAM(QList<MemoryMallocInfo>info){
    QList<PRAM_Device> pram;

    foreach(MemoryMallocInfo tinfo,info){

        //当前内存分配信息所属的物理内存块是否已存在
        bool pramExist = 0;
        int i = 0;//如果已经存在，所属的物理内存块索引号
        for(;i<pram.length();i++){
            if(tinfo.devID==pram[i].devID && tinfo.device==pram[i].device){
                pramExist = 1;
                break;
            }
        }

        if(pramExist){
            //存在
            pram[i].mallocInfo.append(tinfo);
        }else{
            //不存在
            PRAM_Device dev;
            dev.devID = tinfo.devID;
            dev.device = tinfo.device;

            dev.mallocInfo.append(tinfo);
            pram.append(dev);
        }
    }
    return pram;
}

//生成coe文本文件
bool generateCoeFile(QString coeFolderPath,QList<MemoryMallocInfo> info){
    if(info.length()==0){
        qDebug()<<"Error:not have memory malloc info!";
        return false;
    }

    QDir path(coeFolderPath);

    //如果生成的coe文件位置已存在文件夹，则询问用户是否要删除
    if(path.exists()){
        cout<<"\""<<coeFolderPath.toUtf8().data()<<"\"already exists,Do you want to overwrite[Y/N]:";

        string reply;

        cin>>reply;


       if(reply=="y"||reply=="yes"||reply=="Y"||reply=="YES"){
            if(!path.removeRecursively()){
                qDebug()<<"Error:The dir don't have operation permissions!";
                return false;
            }
       }else{
            return false;
       }
    }


    //创建出存放coe的文件夹
    if(!path.mkpath(coeFolderPath)){
        qDebug()<<"Error:The dir don't have operation permissions!";
        return false;
    }

    //判断分配的内存是哈佛架构还是冯诺依曼架构
    bool memArch = info[0].device==0;

    //如果是哈佛架构,生成code/data文件夹
    if(memArch==memArch_HD){
        path.mkdir("DATA");
        path.mkdir("CODE");
    }

    //获取物理设备列表
    QList<PRAM_Device> pramList = analysiPRAM(info);

    foreach(PRAM_Device pram,pramList){
        //给每个物理设备生成对应的.coe文件
        QString coe_txt = "memory_initialization_radix=16;\r\nmemory_initialization_vector=\r\n";


        for(int i = 0;i<pram.mallocInfo.length();i++){

            QString mallocInfoCoe;

            if(pram.mallocInfo[i].isUsage==1){
                //被占用的内存块
                mallocInfoCoe = compileCOEtxt(pram.mallocInfo[i].bin);
            }else{
                //空闲内存块/栈内存块
                QByteArray arr(pram.mallocInfo[i].endAddress-pram.mallocInfo[i].baseAddress+1,0);
                mallocInfoCoe = compileCOEtxt(arr);
            }





            if(i==pram.mallocInfo.length()-1){
                mallocInfoCoe.append(";");
            }else{
                mallocInfoCoe.append("\r\n");
            }
            coe_txt.append(mallocInfoCoe);
        }


        if(pram.device==0){
            QFile file(coeFolderPath+"\\COE"+QString::number(pram.devID)+".coe");
            file.open(QFile::WriteOnly);
            file.write(coe_txt.toUtf8());
            file.close();
        }else if(pram.device==1){
            QFile file(coeFolderPath+"\\CODE\\COE"+QString::number(pram.devID)+".coe");
            file.open(QFile::WriteOnly);
            file.write(coe_txt.toUtf8());
            file.close();
        }else if(pram.device==2){
            QFile file(coeFolderPath+"\\DATA\\COE"+QString::number(pram.devID)+".coe");
            file.open(QFile::WriteOnly);
            file.write(coe_txt.toUtf8());
            file.close();
        }

    }
    return true;
}


#endif // GENERATEMEMORYMALLOCINFO_H
