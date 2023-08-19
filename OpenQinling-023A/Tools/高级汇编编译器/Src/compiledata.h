#ifndef COMPILEDATA_H
#define COMPILEDATA_H
#pragma execution_character_set("utf-8")
#include <QByteArray>
#include <QChar>
#include <QStringList>
#include "CompileErrorHandle.h"
#include <QDebug>
//字符串转义字符替换/字符串内数据取出[如果字符串外有字符，返回出错]
QByteArray replaceString(QByteArray txt){
    int isInString = 0;//当前遍历的字符是否在字符串中
    QByteArray strData;
    for(int i = 0;i<txt.length();i++){
        QChar c = txt.at(i);
        int inString = 0;

        if(c=='\"'){
            if(isInString){
                if(txt.at(i-1)!='\\'){
                    isInString = 0;
                }
            }else{
                if(isInString){
                    isInString = 0;
                }else{
                    inString = 1;
                }
            }
        }
        if(isInString){
            strData.append(txt.at(i));
        }

        if(inString){
            isInString = 1;
        }
    }

    QByteArray seplaceStr;//替换转义字符后的字符串
    for(int i=0;i<strData.length();i++){
        if(strData.at(i)=='\\'){
            if(i!=strData.length()-1){
                if(strData.at(i+1)=='a'){
                    seplaceStr.append('\a');
                    i++;
                }else if(strData.at(i+1)=='b'){
                    seplaceStr.append('\b');
                    i++;
                }else if(strData.at(i+1)=='f'){
                    seplaceStr.append('\f');
                    i++;
                }else if(strData.at(i+1)=='n'){
                    seplaceStr.append('\n');
                    i++;
                }else if(strData.at(i+1)=='r'){
                    seplaceStr.append('\r');
                    i++;
                }else if(strData.at(i+1)=='t'){
                    seplaceStr.append('\t');
                    i++;
                }else if(strData.at(i+1)=='v'){
                    seplaceStr.append('\v');
                    i++;
                }else if(strData.at(i+1)=='\\'){
                    seplaceStr.append('\\');
                    i++;
                }else if(strData.at(i+1)=='?'){
                    seplaceStr.append('\?');
                    i++;
                }else if(strData.at(i+1)=='\''){
                    seplaceStr.append('\'');
                    i++;
                }else if(strData.at(i+1)=='\"'){
                    seplaceStr.append('\"');
                    i++;
                }else if(strData.at(i+1)=='0'){
                    seplaceStr.append('\0');
                    i++;
                }
            }
        }else{
            seplaceStr.append(strData.at(i));
        }
    }
    return seplaceStr;
}
//编译有符号整形
unsigned long long compileUInt(QString order,uint*isSuccess){
    if(order.length()==0){
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }


    unsigned long long bin = 0;
    bool com_isSuccess = 0;

    if(order.at(order.length()-1)>=48 && order.at(order.length()-1)<=57){
        bin = order.toULongLong(&com_isSuccess,10);
    }else if(order.length()>=2){
        QString numSys = order.right(1);
        QString num = order.left(order.length()-1);
        if(numSys=="B"||numSys=="b"){
            bin = num.toULongLong(&com_isSuccess,2);
        }else if(numSys=="O"||numSys=="o"){
            bin = num.toULongLong(&com_isSuccess,8);
        }else if(numSys=="D"||numSys=="d"){
            bin = num.toULongLong(&com_isSuccess,10);
        }else if(numSys=="H"||numSys=="h"){
            bin = num.toULongLong(&com_isSuccess,16);
        }else{
            if(isSuccess!=NULL){
                *isSuccess = 0;
            }
            return 0;
        }
    }else{
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }
    if(com_isSuccess==0 && isSuccess!=NULL){
        *isSuccess = 0;
    }
    else if(isSuccess!=NULL){
        *isSuccess = 1;
    }
    return bin;
}

//编译无符号整形
long long compileInt(QString order,uint*isSuccess){
    if(order.length()==0){
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }

    long long bin = 0;
    bool com_isSuccess = 0;

    if(order.at(order.length()-1)>=48 && order.at(order.length()-1)<=57){
        bin = order.toLongLong(&com_isSuccess,10);
    }else if(order.length()>=2){
        QString numSys = order.right(1);
        QString num = order.left(order.length()-1);
        if(numSys=="B"||numSys=="b"){
            bin = num.toLongLong(&com_isSuccess,2);
        }else if(numSys=="O"||numSys=="o"){
            bin = num.toLongLong(&com_isSuccess,8);
        }else if(numSys=="D"||numSys=="d"){
            bin = num.toLongLong(&com_isSuccess,10);

        }else if(numSys=="H"||numSys=="h"){
            bin = num.toLongLong(&com_isSuccess,16);
        }else{
            if(isSuccess!=NULL){
                *isSuccess = 0;
            }
            return 0;
        }
    }else{
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }
    if(com_isSuccess==0 && isSuccess!=NULL){
        *isSuccess = 0;
    }
    else if(isSuccess!=NULL){
        *isSuccess = 1;
    }
    return bin;
}

//数据编译器[需先规范化代码格式，否则编译会出错]
QByteArray compileData(QStringList asm_data,uint* isSuccess){
    QByteArray bin;
    uint index=0;

    foreach(QString order_txt,asm_data){
        QStringList orderArgs = order_txt.split(":");
        if(orderArgs.length()!=2){
            if(isSuccess!=NULL){
                *isSuccess=0;
            }
            return compileError(index,order_txt,"format error");
        }
        QString order = orderArgs.at(0);

        if(order=="BYTE"||order=="byte"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-127 || tmp>127){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                char data = (char)tmp;
                bin.append(data);
            }
        }else if(order=="UBYTE"||order=="ubyte"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>255){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                uchar data = (char)tmp;
                bin.append(data);
            }
        }else if(order=="SHORT"||order=="short"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-32767 || tmp>32767){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                short data = (short)tmp;
                char*p = (char*)&data;
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="USHORT"||order=="ushort"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>65535){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                ushort data = (ushort)tmp;
                char*p = (char*)&data;
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="INT"||order=="int"){

            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-2147483647 || tmp>2147483647){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                int data = (int)tmp;
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="UINT"||order=="uint"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>4294967296){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                uint data = (uint)tmp;
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="LONG"||order=="long"){

            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                long long data = (long long)tmp;
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="ULONG"||order=="ulong"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                unsigned long long data = tmp;
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="FLOAT"||order=="float"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                bool cmpSuccess = 0;
                float data = thisData.toFloat(&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="DOUBLE"||order=="double"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                bool cmpSuccess = 0;
                double data = thisData.toDouble(&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="STR(UTF8)"||order=="str(utf8)"||order=="STR"||order=="str"){
            //已utf8格式编译字符串[str默认为utf8格式]
            bin.append(replaceString(orderArgs.at(1).toUtf8()));
        }else{
            if(isSuccess!=NULL){
                *isSuccess=0;
            }
            return compileError(index,order_txt,"thers is no such order");
        }
    }
    if(isSuccess!=NULL){
        *isSuccess=1;
    }
    return bin;
}

#endif // COMPILEDATA_H
