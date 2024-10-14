#include "OpenQinling_PSDL_FormatText.h"
#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

//判断名称是否符合要求[只能为文字(任意语言的)/数字/_/*/@。且首字符不可是数字]
bool nameIsConform(QString name){
    foreach(QChar c,name){
        //如果name中有字符非 _ 且非 * 且非 文字 0-9的话就返回false
        if(!c.isNumber() && !c.isLetter() && c!='_' && c!='*' && c!='@'){
            return false;
        }
    }
    QChar c = name.at(0);
    //name首字符不可是数字
    if(c.isNumber()){
        return false;
    }
    return true;
}


//判断定义的变量数据类型是否符合要求
bool varTypeIsConform(QString typeName){
    const QStringList typeList={
        "BYTE","UBYTE",     //字节型变量
        "SHORT","USHORT",   //短整型变量
        "INT","UINT",       //整型变量
        "LONG","ULONG",     //长整型变量
        "FLOAT","DOUBLE",   //浮点型变量
        "POINTER"           //指针类型(指向数组/函数/变量的地址)
    };
    return typeList.contains(typeName);
}

//如果是指针类型变量,指针类型转换为无符号整型类型(具体根据不同的cpu位数来确定,32位cpu转为UINT)
QString pointerTypeConvert(QString typeName){
    if(typeName=="POINTER"){
        return "UINT";
    }
    return typeName;
}
//获取变量数据类型的字节数
uint getVarTypeSize(QString typeName){
    typeName = pointerTypeConvert(typeName);
    const QStringList byte1={
        "BYTE","UBYTE"
    };
    const QStringList byte2={
        "SHORT","USHORT"
    };
    const QStringList byte4={
        "INT","UINT","FLOAT"
    };
    const QStringList byte8={
        "LONG","ULONG","DOUBLE"
    };

    if(byte1.contains(typeName)){
        return 1;
    }else if(byte2.contains(typeName)){
        return 2;
    }else if(byte4.contains(typeName)){
        return 4;
    }else if(byte8.contains(typeName)){
        return 8;
    }

    return 0;
}
}


}
QT_END_NAMESPACE
