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
    uint8_t byteData = 0xCC;//如果不是指针标记,那就是不同字节数据,当前存储字节数据的值

    bool isPointerMark = 0;//是否是指针标记
    uint8_t pointerByte = 0;//如果是指针标记,是指针标记的第几个字节(从右到左)
    //.... [31-24]3 [23,16]2 [15-8]1 [7-0]0
    QString pointerMarkName;//如果是指针标记,标记名称
    int pointerMarkOffset = 0;//如果是指针标记,标记的上下偏移值
};

//void printInitBlocks(QList<InitBlock> & block);
//bool allocInitBlock(QList<InitBlock> &list,InitBlock initInfo);
struct InitBlock : QVector<InitBlockByte>{

    uint cpuBit;//cpu字节数
    RamEndian ramEnd;//内存字节序

    //调转字节序
    QByteArray bytesReverse(QByteArray bytes){
        //DWORD/WORD/QWORD转出的byteArray是大端字节序,如果当前cpu也是大端字节序的,那么无需调转,直接退出
        if(ramEnd == BIG_ENDIAN){
            return bytes;
        }
        //DWORD/WORD/QWORD转出的byteArray是大端字节序,如果当前cpu也是小端字节序的,就调转字节序
        QByteArray ret;
        for(int i = bytes.length()-1;i>=0;i++){
            ret.append(bytes[i]);
        }
        return ret;
    }


    InitBlock(uint size,uint cpuBit,RamEndian ramEnd){
        this->resize(size);
        this->cpuBit = cpuBit;
        this->ramEnd = ramEnd;
    }
    //插入整型数据
    void insertData(uint beginAdd,int32_t data){
        DWORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }
    }
    void insertData(uint beginAdd,int16_t data){
        WORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }

    }
    void insertData(uint beginAdd,int8_t data){
        InitBlockByte init;
        init.byteData = data;
        this->operator[](beginAdd) = init;
    }
    void insertData(uint beginAdd,int64_t data){
        QWORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }

    }
    //插入无符号整型数据
    void insertData(uint beginAdd,uint32_t data){
        DWORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }

    }
    void insertData(uint beginAdd,uint16_t data){
        WORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }

    }
    void insertData(uint beginAdd,uint8_t data){
        InitBlockByte init;
        init.byteData = data;
        this->operator[](beginAdd) = init;
    }
    void insertData(uint beginAdd,uint64_t data){
        QWORD ds(data);

        QByteArray tmp = ds.toByteArray();

        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }
    }
    // 插入浮点型数据
    void insertData(uint beginAdd,double data){
        QWORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }
    }
    void insertData(uint beginAdd,float data){
        DWORD ds(data);
        QByteArray bytes = bytesReverse(ds.toByteArray());

        for(uint i = 0;i<(uint)bytes.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = bytes[i];
            this->operator[](i+beginAdd) = init;
        }
    }
    //插入二进制字符串
    void insertData(uint beginAdd,QByteArray data){
        for(uint i = 0;i<(uint)data.length();i++){
            if(i+beginAdd >= (uint)this->length()){
                break;
            }
            InitBlockByte init;
            init.byteData = data[i];
            this->operator[](i+beginAdd) = init;
        }
    }

    //插入指针
    //bytes:插入指针的几个字节(例如int p;short = (short)&p)此时就是插入指针的末尾2字节,填2
    //markName:指针指向的数据标记(例如指向&p)此时就填p
    //markOffset:指针指向的数据标记的偏移地址,(例如 ((int)&p)-3),就填-3
    void insertData(uint beginAdd,QString markName,int markOffset,uint bytes = 0){
        uint cpuByte = cpuBit/8;
        if(bytes==0 || bytes>cpuByte)bytes = cpuByte;

        for(uint i = 0;i<bytes;i++){
            InitBlockByte tmp;
            tmp.isPointerMark = 1;
            if(ramEnd == BIG_ENDIAN){
                tmp.pointerByte = bytes-1-i;
            }else{
                tmp.pointerByte = i;
            }
            tmp.pointerMarkName = markName;
            tmp.pointerMarkOffset = markOffset;
            this->operator[](i+beginAdd) = tmp;
        }
    }

    //将Init块的数据转换为psdl的全局变量定义
    PsdlIR_Compiler::MiddleNode toPSDLVarDefineNode(QString varName,bool isStatic,bool isConst){

        PsdlIR_Compiler::MiddleNode varDef;
        varDef.nodeType = "ARRAY";
        varDef.args = QStringList(varName);



        PsdlIR_Compiler::MiddleNode_Att body;
        body.attName = "BODY";
        for(int i = 0;i<this->length();i++){
            PsdlIR_Compiler::MiddleNode unitNode;
            unitNode.nodeType = "VAR";
            if(this->operator[](i).isPointerMark){
                QString hlStr = "<";

                hlStr += QString::number((this->operator[](i).pointerByte+1) * 8 -1) + "-";
                hlStr += QString::number(this->operator[](i).pointerByte * 8) + ">";

                QString markStr = "["+this->operator[](i).pointerMarkName;
                int offset = this->operator[](i).pointerMarkOffset;
                if(offset >0){
                    markStr += "+"+QString::number(offset)+"D";
                }else if(offset<0){
                    markStr += QString::number(offset)+"D";
                }
                markStr+="]";
                markStr+= hlStr;


                unitNode.args = QStringList({
                            "ubyte",
                            markStr
                        });
            }else{
                QString numStr = QString::number(this->operator[](i).byteData,16).toUpper();
                while(numStr.length()<2){
                    numStr.prepend("0");
                }
                numStr = numStr.right(2)+"H";
                unitNode.args = QStringList({
                            "ubyte",
                            numStr
                        });
            }
            body.subNodes.append(unitNode);
        }
        varDef.atts.append(body);

        if(!isStatic){
            PsdlIR_Compiler::MiddleNode_Att tmp;
            tmp.attName = "EXPORT";
            varDef.atts.append(tmp);
        }

        if(isConst){
            PsdlIR_Compiler::MiddleNode_Att tmp;
            tmp.attName = "CONST";
            varDef.atts.append(tmp);
        }


        return varDef;
    }

};
}

}
QT_END_NAMESPACE


#endif // INITBLOCK_H
