#include "OpenQinling_C_JudgestrtoolFunction.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

bool judgeIsKeyword(QString txt){
    static const QStringList allKeyword ={
        //数据类型关键字
        "int","short","long","char","float","double","unsigned","signed","struct","union","void",
        //定义关键字
        "const","extern","static","enum","typedef","auto",
        //流程关键字
        "if","else","while","for","goto","continue","break","return","switch","case","default","do",
        //变量类型字节数获取
        "sizeof",
        //函数可变参数首地址获取(用法: void* p = __valist__)
        "__valist__",

        //如果一个函数开头有该属性,那么这个函数就是汇编内嵌函数
        "__asm__",

        //函数/变量的扩展属性(用法: __attribute__(扩展属性) void fun();)
        //这里面填的属性不是C语言前段编译器负责处理的,是后端处理器编译一个变量、函数时所需的属性信息，在前端编译器不会进行检测
        "__attribute__",
        /*示例:(PSDL后端支持的扩展属性)
         * __attribute__(interrupt) //系统内核的中断响应处理函数
         * __attribute__(drivercall)//驱动程序通过软中断调用系统内核功能的处理函数
         * __attribute__(appcall)   //应用程序通过软中断调用系统内核功能的处理函数
         */
    };
    return allKeyword.contains(txt);
}
//检查文本中变量/函数/结构体等的命名 是否符合语法要求
//只可是 文字 数字 _
bool judgeIsIdentifying(QString txt){
    foreach(QChar c,txt){
        //如果name中有字符非 _ 且非 文字 0-9的话就返回false
        if(!c.isNumber() && !c.isLetter() && c!='_'){
            return false;
        }
    }
    QChar c = txt.at(0);
    //name首字符不可是数字
    if(c.isNumber()){
        return false;
    }
    return true;
}

//判断是否是c语言基础数据类型的单词结构,如果时,将其单词顺序标准化(c语言标准数据类型的单词顺序是可变的,所以要对其标准化)
//如果不是c语言基础数据类型的单词结构,保持不变
QStringList stdFormatBaseTypeName(QStringList baseTypeName){
    //c语言基础数据类型描述词组的数量
    int charC = baseTypeName.count("char");
    int shortC = baseTypeName.count("short");
    int intC = baseTypeName.count("int");
    int longC = baseTypeName.count("long");
    int floatC = baseTypeName.count("float");
    int doubleC = baseTypeName.count("double");
    int signedC = baseTypeName.count("signed");
    int unsignedC = baseTypeName.count("unsigned");
    int otherNum = baseTypeName.length()-
                    charC-
                    shortC-
                    intC-
                    longC-
                    floatC-
                    doubleC-
                    signedC-
                    unsignedC;
    if(otherNum!=0)return baseTypeName;

    bool isChar = charC==1 && shortC==0 && intC==0 && longC==0 && floatC==0 && doubleC==0 && unsignedC==0 && signedC<=1;
    bool isShort = charC==0 && shortC==1 && intC<=1 && longC==0 && floatC==0 && doubleC==0 && unsignedC==0 && signedC<=1;
    bool isInt = charC==0 && shortC==0 && ((intC==1 || longC==1) && longC!=2) && floatC==0 && doubleC==0 && unsignedC==0 && signedC<=1;
    bool isLong = charC==0 && shortC==0 && intC<=1 && longC==2 && floatC==0 && doubleC==0 && unsignedC==0 && signedC<=1;

    bool isUChar = charC==1 && shortC==0 && intC==0 && longC==0 && floatC==0 && doubleC==0 && unsignedC==1 && signedC==0;
    bool isUShort = charC==0 && shortC==1 && intC<=1 && longC==0 && floatC==0 && doubleC==0 && unsignedC==1 && signedC==0;
    bool isUInt = charC==0 && shortC==0 && ((intC==1 || longC==1) && longC!=2) && floatC==0 && doubleC==0 && unsignedC==1 && signedC==0;
    bool isULong = charC==0 && shortC==0 && intC<=1 && longC==2 && floatC==0 && doubleC==0 && unsignedC==1 && signedC==0;

    bool isDouble = charC==0 && shortC==0 && intC==0 &&  floatC<=1 && (doubleC==1||longC==1) && unsignedC==0 && signedC==0;
    bool isFloat = charC==0 && shortC==0 && intC<=0 && longC==0 && floatC==1 && doubleC==0 && unsignedC==0 && signedC==0;

    if(isChar)return {"char"};
    if(isShort)return {"short"};
    if(isInt)return {"int"};
    if(isLong)return {"long","long"};
    if(isUChar)return {"unsigned","char"};
    if(isUShort)return {"unsigned","short"};
    if(isUInt)return {"unsigned","int"};
    if(isULong)return {"unsigned","long","long"};
    if(isDouble)return {"double"};
    if(isFloat)return {"float"};
    return baseTypeName;
}

//检查是否是符号
bool judgeIsDelimiter(QString txt){

    static const QStringList allDels = {
        "||","&&","->","==","!=",">=","<=","<<",">>","...",
        "|","&",".","=","!","~","^","+","-","*","/","%",
        ">","<",":",",",";","|=","&=","!=","^=","+=","-=","*=","/=","%=","?",
        "++","--","<<=",">>=",
        "#","##","#@",
        "(",")","{","}","[","]"
    };

    return allDels.contains(txt);
}
//检查是否是一个常量值
bool judgeIsConst(QString &txt,JudgeDefConstValueType &constType){
    if(txt.length()>=2){
        if(txt.left(1)=="\""){
            constType = JudgeDefConstValueType_String;
            return 1;
        }else if(txt.left(1)=="\'"){
            constType = JudgeDefConstValueType_Char;
            return 1;
        }
    }
    if(!txt[0].isNumber())return 0;

    //判断是否是浮点数
    if(txt.contains('D')){
        txt.remove("D");
        constType = JudgeDefConstValueType_Double;
        return 1;
    }else if(txt.contains("F")){
        txt.remove("F");
        constType = JudgeDefConstValueType_Float;
        return 1;
    }else if(txt.contains("UL")){
        txt.remove("UL");
        constType = JudgeDefConstValueType_Ulong;
        return 1;
    }else if(txt.contains("UI")){
        txt.remove("UI");
        constType = JudgeDefConstValueType_Uint;
        return 1;
    }else if(txt.contains("L")){
        txt.remove("L");
        constType = JudgeDefConstValueType_Long;
        return 1;
    }else if(txt.contains("I")){
        txt.remove("I");
        constType = JudgeDefConstValueType_Int;
        return 1;
    }
    return 0;
}



//将字符串中 " ' \ 换行等字符替换为 \起始的 转义字符
QString createEscapeCharacter(QString strData){
    QString retStr;
    for(int i = 0;i<strData.length();i++){
        if(strData[i] == '\\'){
            retStr += "\\\\";
        }else if(strData[i] == '\''){
            retStr += "\\\'";
        }else if(strData[i] == '\"'){
            retStr += "\\\"";
        }else if(strData[i] == '\t'){
            retStr += "\\\t";
        }else if(strData[i] == '\r'){
            retStr += "\\\r";
        }else if(strData[i] == '\n'){
            retStr += "\\\n";
        }else{
            retStr += strData[i];
        }
    }
    return retStr;
}


//替换字符串中的转义字符
QByteArray replaceTransChar(QString strData,CharCodeSystem codeSystem){//CharCodeSystem codeSystem
    if(strData.length()<2)return QByteArray();
    strData = strData.mid(1,strData.length()-2);

    QByteArray arr;

    if(codeSystem==CharCode_UTF8){
        arr = strData.toUtf8();
    }else if(codeSystem==CharCode_UTF16){
        arr = QTextCodec::codecForName("UTF-16")->fromUnicode(strData);
    }else if(codeSystem==CharCode_GBK){
        arr = QTextCodec::codecForName("GBK")->fromUnicode(strData);
    }


    QByteArray seplaceStr;
    for(int i=0;i<arr.length();i++){
        if(arr.at(i)!='\\'){
            seplaceStr.append(arr.at(i));
            continue;
        }

        if(i!=arr.length()-1){
            if(arr.at(i+1)=='a'){
                seplaceStr.append('\a');
                i++;
            }else if(arr.at(i+1)=='b'){
                seplaceStr.append('\b');
                i++;
            }else if(arr.at(i+1)=='f'){
                seplaceStr.append('\f');
                i++;
            }else if(arr.at(i+1)=='n'){
                seplaceStr.append('\n');
                i++;
            }else if(arr.at(i+1)=='r'){
                seplaceStr.append('\r');
                i++;
            }else if(arr.at(i+1)=='t'){
                seplaceStr.append('\t');
                i++;
            }else if(arr.at(i+1)=='v'){
                seplaceStr.append('\v');
                i++;
            }else if(arr.at(i+1)=='\\'){
                seplaceStr.append('\\');
                i++;
            }else if(arr.at(i+1)=='?'){
                seplaceStr.append('\?');
                i++;
            }else if(arr.at(i+1)=='\''){
                seplaceStr.append('\'');
                i++;
            }else if(arr.at(i+1)=='\"'){
                seplaceStr.append('\"');
                i++;
            }else if(arr.at(i+1)=='0'){
                seplaceStr.append('\0');
                i++;
            }else if(arr.at(i+1)=='x' && i<=arr.length()-3){
                QChar hex[2] = {arr.at(i+2),arr.at(i+3)};
                bool isHexSrceess;
                uchar d = QString(hex,2).toUInt(&isHexSrceess,16);
                if(isHexSrceess){
                    seplaceStr.append(d);
                    i+=3;
                }else{
                    seplaceStr.append(arr.at(i));
                }
            }
        }
    }
    seplaceStr.append((char)0);
    return seplaceStr;
}


//解析出字符常量定义的字符:传入去除''的字符常量定义文本
char analysisCharConstValue(QString charConstValue,bool&status){
    status = 0;
    if(charConstValue.length()==0)return 0;

    if(charConstValue[0]=='\\'){
        if(charConstValue.length()<2)return 0;
        char tmp = 0;
        if(charConstValue[1]=='0'){
            tmp = 0;
        }else if(charConstValue[1]=='n'){
            tmp =  10;
        }else if(charConstValue[1]=='r'){
            tmp =  13;
        }else if(charConstValue[1]=='t'){
            tmp =  9;
        }else if(charConstValue[1]=='v'){
            tmp =  11;
        }else if(charConstValue[1]=='a'){
            tmp =  7;
        }else if(charConstValue[1]=='b'){
            tmp =  8;
        }else if(charConstValue[1]=='f'){
            tmp =  12;
        }else if(charConstValue[1]=='\''){
            tmp =  39;
        }else if(charConstValue[1]=='\"'){
            tmp =  34;
        }else if(charConstValue[1]=='\\'){
            tmp = 92;
        }else if(charConstValue[1]=='\?'){
            tmp =  63;
        }else if(charConstValue.length()>=4){
            bool sucess;
            if(charConstValue[1]=='x'){
                tmp = charConstValue.mid(2,2).toInt(&sucess,16);
            }else{
                tmp = charConstValue.mid(1,3).toInt(&sucess,8);
            }
            if(sucess==0)return 0;
            status = 1;
            return tmp;
        }else{
            if(charConstValue[1].unicode() >255)return 0;
            return charConstValue[1].unicode();
        }
        status = 1;

        return tmp;
    }
    //不是一个ascii字符
    if(charConstValue[0].unicode() >255)return 0;
    status = 1;
    return charConstValue[0].unicode();
}



//判断前几个运算符,如果是运算符,返回运算符的字符数
int judgeFastCharsIsDelimiter(QString &delText){
    if(delText=="")return 0;
    QString tmp = delText;
    if(tmp.length()>=3){
        tmp = tmp.left(3);
        if(tmp=="<<="||
           tmp==">>="||
           tmp=="..."){
            return 3;
        }
    }
    if(tmp.length()>=2){
        tmp = tmp.left(2);
        if(tmp=="||" ||
            tmp=="&&" ||
            tmp=="->" ||
            tmp=="==" ||
            tmp=="!=" ||
            tmp==">=" ||
            tmp=="<="||
            tmp=="<<"||
            tmp==">>"||
            tmp=="++" ||
            tmp=="--" ||
            tmp=="|=" ||
            tmp=="&=" ||
            tmp=="!=" ||
            tmp=="^=" ||
            tmp=="+="||
            tmp=="-="||
            tmp=="*="||
            tmp=="/="||
            tmp=="%="||
            tmp=="\r\n"||
            tmp=="//"||
            tmp=="/*"||
            tmp=="*/"||
            tmp=="##"||
            tmp=="#@"){
            return 2;
        }
    }


    //1字符的运算符
    const static QList<QChar> del1 = {
        '|','&',',',':','.','=','!','~','^','+','-','*','/','%',';','>','<',
        '\n','\r',' ','\t',
        '(',')',
        '[',']',
        '{','}',
        '\"','\'','#','\\'
    };
    if(del1.contains(tmp[0])){
        return 1;
    }
    return 0;
}


}}
QT_END_NAMESPACE

