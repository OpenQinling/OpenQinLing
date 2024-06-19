#include "OpenQinling_PSDL_ConstValueAnalysis.h"
#include "OpenQinling_PSDL_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

//获取一个数至少要多少位才可以存储
uint getNumDigit(ulong num){
    for(ulong i = 0;i<64;i++){
        ulong z = 1<<i;
        if(num<z){
            return i;
        }
    }
    return 64;
}

//解析基础常量定义格式
uint analysisStdConst(QString stdConstText,bool*isSuceess){
    uint arraySize;
    bool suceess;
    if(isSuceess!=NULL){
        *isSuceess = 0;
    }
    QString numText = stdConstText.left(stdConstText.length()-1);
    QString sys = stdConstText.right(1).toUpper();
    if(sys=='H'){
        arraySize = numText.toInt(&suceess,16);
    }else if(sys=='B'){
        arraySize = numText.toInt(&suceess,2);
    }else if(sys=='O'){
        arraySize = numText.toInt(&suceess,8);
    }else if(sys=='D'){
        arraySize = numText.toInt(&suceess,10);
    }else{
        return 0xffffffff;
    }
    if(!suceess){
        return 0xffffffff;
    }
    if(isSuceess!=NULL){
        *isSuceess = 1;
    }
    return arraySize;
}


//将中级语言定义常量的文本解析转换为汇编语言定义常量的文本
//同时返回出中级语言定义的常量数据类型/常量位数
//txt:中级语言常量定义文本,
//ptrList:当前源文件中存在的指针标记名(解析函数/全局变量地址的常量时要用)
//
//type:常量数据类型
//size:常量
QString analysisConstValue(QString txt,QStringList&ptrList,bool&suceess,QString &type,uint &size){
    QString asmText;
    suceess = 0;
    QString numText;
    QString sys;
    if(txt[0]=='<' && txt[txt.length()-1]=='>'){
        QStringList tmp = txt.mid(1,txt.length()-2).split(':');
        if(tmp.length()!=2)return asmText;
        tmp[0] = tmp[0].toUpper();
        if(varTypeIsConform(tmp[0])==0 || tmp[1].length()<2)return asmText;
        type = pointerTypeConvert(tmp[0]);

        numText = tmp[1].left(tmp[1].length()-1);
        sys = tmp[1].right(1).toUpper();
        uint typeDigit = getVarTypeSize(type)*8;
        if(sys=='H' && (type!="FLOAT"&&type!="DOUBLE")){
            ulong num = numText.toULong(&suceess,16);
            size = getNumDigit(num);
        }else if(sys=='B' && (type!="FLOAT"&&type!="DOUBLE")){
            ulong num = numText.toULong(&suceess,2);
            size = getNumDigit(num);
        }else if(sys=='O' && (type!="FLOAT"&&type!="DOUBLE")){
            ulong num = numText.toULong(&suceess,8);
            size = getNumDigit(num);
        }else if(sys=='D' && (type!="FLOAT"&&type!="DOUBLE")){
            long num = numText.toLong(&suceess,10);
            size = getNumDigit(num);
        }else if(sys=='F'){
            numText.toDouble(&suceess);
            if(type=="FLOAT"){
                size = 32;
            }else if(type=="DOUBLE"){
                size = 64;
            }else{
                suceess = 0;
                return asmText;
            }
        }else{
            suceess = 0;
            return asmText;
        }
        if(suceess==0)return asmText;
        suceess = 0;
        if(size>typeDigit)return asmText;

        asmText = numText+sys;
        suceess=1;
        typeDigit = typeDigit/8;
        return asmText;
    }else if(txt[0]=='[' && (txt[txt.length()-1]==']' || txt[txt.length()-1]=='>')){

        QStringList tmp;

        int8_t h = 31,l = 0;
        if(txt.contains('<')&&txt[txt.length()-1]=='>'){
            txt.remove(txt.length()-1,1);

            tmp = txt.split('<');
            if(tmp.length()!=2)return asmText;
            txt = tmp[0];

            QString spl = tmp[1];
            tmp = spl.split('-');
            if(tmp.length()!=2)return asmText;
            bool su1,su2;
            h = tmp[0].toUInt(&su1);
            l = tmp[1].toUInt(&su2);
            if(!(su2&&su2))return asmText;
        }





        bool isAdd;
        if(txt.contains('+')){
            tmp = txt.mid(1,txt.length()-2).split('+');
            isAdd = 1;
        }else{
            tmp = txt.mid(1,txt.length()-2).split('-');
            isAdd = 0;
        }
        if(tmp.length()>2 || tmp.length()==0)return asmText;

        if(!ptrList.contains(tmp[0]))return asmText;
        if(tmp.length()==1){
            asmText = "["+tmp[0]+"]";
            suceess=1;
            type = "UINT";
            size=32;
            return asmText;
        }
        if(tmp[1].length()<2)return asmText;


        numText = tmp[1].left(tmp[1].length()-1);

        sys = tmp[1].right(1).toUpper();
        uint typeDigit = 32;
        type = "UINT";
        if(sys=='H'){
            ulong num = numText.toULong(&suceess,16);
            size = getNumDigit(num);
        }else if(sys=='B'){
            ulong num = numText.toULong(&suceess,2);
            size = getNumDigit(num);
        }else if(sys=='O'){
            ulong num = numText.toULong(&suceess,8);
            size = getNumDigit(num);
        }else if(sys=='D'){
            long num = numText.toLong(&suceess,10);
            size = getNumDigit(num);
            if(num<0){
                suceess = 0;
                return asmText;
            }
        }else{
            suceess = 0;
            return asmText;
        }
        if(suceess==0)return asmText;
        suceess = 0;
        if(size>typeDigit)return asmText;


        asmText = "["+tmp[0]+(isAdd?"+":"-")+numText+sys+"]";

        if(h!=31 || l!=0){
            asmText.append("("+QString::number(h)+"-"+QString::number(l)+")");
        }

        suceess=1;
        size=h-l+1;
        return asmText;
    }else{

        if(txt.length()<2)return asmText;
        numText = txt.left(txt.length()-1);

        sys = txt.right(1).toUpper();

        if(sys=='H'){
            ulong num = numText.toULong(&suceess,16);
            size = getNumDigit(num);
            type ="UINT";
        }else if(sys=='B'){
            ulong num = numText.toULong(&suceess,2);
            size = getNumDigit(num);
            type ="UINT";
        }else if(sys=='O'){
            ulong num = numText.toULong(&suceess,8);
            size = getNumDigit(num);
            type ="UINT";
        }else if(sys=='D'){
            long num = numText.toLong(&suceess,10);
            size = getNumDigit(num);
            type ="INT";
        }else if(sys=='F'){
            numText.toDouble(&suceess);
            size = 32;
            type ="FLOAT";
        }else{
            suceess = 0;
            return asmText;
        }
        if(suceess==0)return asmText;

        suceess = 0;
        uint typeDigit = 32;
        if(size>typeDigit)return asmText;


        asmText = numText+sys;
        suceess=1;
        typeDigit = typeDigit/8;
        return asmText;
    }
    return asmText;
}

/*中级语言常量定义语法
 * 0:自动判断数据类型的数值型常量
 *  常量值
 *  (进制为b/h默认为uint,d默认为int,f默认为float)
 *
 * 1.设置数据类型的数值型常量
 *  <常量数据类型:常量值>
 *
 * 2.指针型常量:指向一个函数/全局变量首地址
 *  [函数/全局变量名 +/- 常量值]
 *
 *
 * 常量值的格式:
 *  实际的常量数值+常量进制
 *  b 2进制
 *  h 16进制
 *  d 10进制
 *  f 浮点数
 */

}

}
QT_END_NAMESPACE
