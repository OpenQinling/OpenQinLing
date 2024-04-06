#ifndef ASMC_TYPEDEF_H
#define ASMC_TYPEDEF_H
#include <QStringList>
#include <QMap>
#define DataMemBlock true;
#define CodeMemBlock false;

#include <BinaryObject_Linrary.h>

typedef ulong baseAddress;

class BlockInfo{
public:
    ulong baseAdress;
    ulong sizeBytes;
};

//链接脚本中解析出的数据
class LinkSprict{
public:
    //能够存放内存块的区域
    QList<BlockInfo> memSpace;
    //要检查的内存块
    QList<QString> checkData;
    QList<QString> checkCode;
    //固定基地址的内存块
    QMap<QString,baseAddress> fixed;

    QString toString(){
        QString memSpaceStr = "MemSpace:";
        foreach(BlockInfo info,memSpace){
            QString temp = "["+QString::number(info.baseAdress,10)+","+QString::number(info.sizeBytes+info.baseAdress-1)+"] ";
            memSpaceStr.append(temp);
        }
        QString checkDataStr = "CheckDataBlock:"+checkData.join(",");
        QString checkCodeStr = "CheckCodeBlock:"+checkCode.join(",");

        QString fixedStr = "FixedBlock:";
        foreach(QString key,fixed.keys()){
            QString temp = "["+key+","+QString::number(fixed.value(key),10)+"] ";
            fixedStr.append(temp);
        }
        return memSpaceStr+"\r\n"+checkDataStr+"\r\n"+checkCodeStr+"\r\n"+fixedStr;
    }
};
#endif // ASMC_TYPEDEF_H
