#ifndef FORMATTEXT_H
#define FORMATTEXT_H
#include <QStringList>
#include <QDebug>

//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name){
    //判断名称是否符合编译器自带的指令类型

    foreach(QChar c,name){
        //如果name中有字符非 _ 且非 - 且非 a-z A-Z 0-9的话就返回false
        if(c!=95 && c!=42 && !(c>=97 && c<=122) && !(c>=65 && c<=90) && !(c>=48 && c<=57)){
            return false;
        }
    }
    QChar c = name.at(0);
    //如果name的首字符非 a-z A-Z的话就返回false
    if(!(c>=65 && c<=90) && !(c>=97 && c<=122) && c!=95 && c!=42){
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




#endif // FORMATTEXT_H
