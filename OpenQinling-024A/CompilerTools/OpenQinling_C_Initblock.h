#ifndef INITBLOCK_H
#define INITBLOCK_H

#include "OpenQinling_C_SrcObjectInfo.h"
#include <QList>
#include "OpenQinling_PSDL_MiddleNode.h"
#include "OpenQinling_DataStruct.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{


struct InitBlockByte{
    uint8_t byteData = 0x00;//如果不是指针标记,那就是不同字节数据,当前存储字节数据的值

    bool isPointerMark = 0;//是否是指针标记
    uint8_t pointerByte = 0;//如果是指针标记,是指针标记的第几个字节(从右到左)
    //.... [31-24]3 [23,16]2 [15-8]1 [7-0]0
    QString pointerMarkName;//如果是指针标记,标记名称
    int pointerMarkOffset = 0;//如果是指针标记,标记的上下偏移值
};

struct InitBlock : QVector<InitBlockByte>{

    uint cpuBit;//cpu字节数
    RamEndian ramEnd;//内存字节序

    //调转字节序
    QByteArray bytesReverse(QByteArray bytes);

    InitBlock(uint size,uint cpuBit,RamEndian ramEnd);


    //插入整型数据
    void insertData(uint beginAdd,int32_t data);
    void insertData(uint beginAdd,int16_t data);
    void insertData(uint beginAdd,int8_t data);
    void insertData(uint beginAdd,int64_t data);







    //插入无符号整型数据
    void insertData(uint beginAdd,uint32_t data);
    void insertData(uint beginAdd,uint16_t data);
    void insertData(uint beginAdd,uint8_t data);
    void insertData(uint beginAdd,uint64_t data);


    // 插入浮点型数据
    void insertData(uint beginAdd,double data);
    void insertData(uint beginAdd,float data);

    //插入二进制字符串
    void insertData(uint beginAdd,QByteArray data);

    //插入指针
    //bytes:插入指针的几个字节(例如int p;short = (short)&p)此时就是插入指针的末尾2字节,填2
    //markName:指针指向的数据标记(例如指向&p)此时就填p
    //markOffset:指针指向的数据标记的偏移地址,(例如 ((int)&p)-3),就填-3
    void insertData(uint beginAdd,QString markName,int markOffset,uint bytes = 0);

    //将Init块的数据转换为psdl的全局变量定义
    PsdlIR_Compiler::MiddleNode toPSDLVarDefineNode(QString varName,bool isStatic,bool isConst);


};
}

}
QT_END_NAMESPACE


#endif // INITBLOCK_H
