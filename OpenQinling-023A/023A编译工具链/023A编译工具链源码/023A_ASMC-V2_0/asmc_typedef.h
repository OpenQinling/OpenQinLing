#ifndef ASMC_TYPEDEF_H
#define ASMC_TYPEDEF_H
#include <QObject>
#include <QList>

class WORD;
class DWORD;
class QWORD;
typedef  QList<WORD> QWordArray;
typedef  QList<DWORD> QDwordArray;
typedef  QList<QWORD> QQwordArray;
//2byte数据类型
class WORD{
public:
    //c数据类型转换 为dword
    WORD();
    WORD(ushort d);
    WORD(short d);
    WORD(uchar d0,uchar d1);//dword={d0,d1}

    //转换为c数据类型
    ushort toUShort();
    short toShort();

    //转换为char[2]
    QByteArray toByteArray();

    uchar& operator[](int i);

    //打印输出信息
    QString toHexString();
private:
    uchar data[2] = {0xff,0xff};
};
//4byte数据类型
class DWORD{
public:
    //c数据类型转换 为dword
    DWORD();
    DWORD(uint d);
    DWORD(float d);
    DWORD(int d);
    DWORD(WORD d0,WORD d1);//dword={d0,d1}
    DWORD(uchar d0,uchar d1,uchar d2,uchar d3);//dword={d0,d1,d2,d3}

    //转换为c数据类型
    uint toUInt();
    int toInt();
    float toFloat();

    //转换为char[4]
    QByteArray toByteArray();
    QWordArray toWordArray();


    QString toHexString();

    //转换为
    uchar& operator[](int i);
private:
    uchar data[4] = {0xff,0xff,0xff,0xff};
};

//8byte数据类型
class QWORD{
public:
    //c数据类型转换 为dword
    QWORD();
    QWORD(unsigned long long d);
    QWORD(double d);
    QWORD(long long d);
    QWORD(DWORD d0,DWORD d1);
    QWORD(WORD d0,WORD d1,WORD d2,WORD d3);
    QWORD(uchar d0,uchar d1,uchar d2,uchar d3,
          uchar d4,uchar d5,uchar d6,uchar d7);

    //转换为c数据类型
    unsigned long long toULong();
    long long toLong();
    double toDouble();

    //转换为char[4]
    QByteArray toByteArray();
    QWordArray toWordArray();
    QDwordArray toDWordArray();

    QString toHexString();

    //转换为
    uchar& operator[](int i);
private:
    uchar data[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
};


QDwordArray ByteToDwordArray(QByteArray array);
QWordArray ByteToWordArray(QByteArray array);
QQwordArray ByteToQwordArray(QByteArray array);
QDwordArray WordToDwordArray(QWordArray array);
QByteArray WordToByteArray(QWordArray array);
QQwordArray WordToQwordArray(QWordArray array);
QWordArray DwordToWordArray(QDwordArray array);
QByteArray DwordToByteArray(QDwordArray array);
QQwordArray DwordToQwordArray(QDwordArray array);
QWordArray QwordToWordArray(QQwordArray array);
QByteArray QwordToByteArray(QQwordArray array);
QDwordArray QwordToDwordArray(QQwordArray array);

//地址标记定义
class BlockMarkDef{
public:
    QString name;//标记名
    QString affiBlockName;//所属的内存块名
    uint affBlockOffset = 0;//相较于所属内存块基址的偏移地址(字节-左低右高)
    //isImport为1的话，affBlockOffset参数无效
    QString toName(){
        if(name!=""){
            return affiBlockName+"."+name;
        }
        return affiBlockName;
    }
};


//地址标记的引用
class MarkQuote{
public:
    bool isImport;//是否是从其他原文件引入的
    QString name;//标记名[如果为为空，就是指向内存块首地址的标记]
    QString affiBlockName;//定义该标记的内存块名
    //使用该地址标记的内存块偏移地址 第offset_Byte字节第offset_Bit位开始
    uint order_offset_Byte;//使用该地址标记的内存块偏移地址(字节-左低右高)
    uint order_offset_Bit;//使用该地址标记的内存块偏移地址(比特-左低右高)

    //32位cpu是31-0之间。例如将该地址标记的低16位用在 mov r1,[内存块.标记] 中，higth=15,low=0
    uint higth,low;//使用了该地址标记的哪几位(比特-左高右低)
    int offset = 0;//标记所指向的内存地址的偏移值

    QString toName(){
        if(name!=""){
            return affiBlockName+"."+name;
        }
        return affiBlockName;
    }
};


//二进制内存块对象
class MemBlockBin{
public:
    QList<BlockMarkDef> markDefList;//当前内存块对外开放的标记
    QList<MarkQuote> markQuoteList;//当前内存块引用外部的标记
    uint binLength;//内存块中二进制数据的字节数
    QByteArray bin;//内存块的二进制数据[如果是非初始化数据块，则为空]
};


//编译警告/错误提示
class Prompt{
public:

    void append(QString fromFilePath,
                QString srcText,
                uint startline,
                uint endline){
        this->endline.append(endline);
        this->srcText.append(srcText);
        this->startline.append(startline);
        this->fromFilePath.append(fromFilePath);
    }

    bool isError;//0为警告,1为错误。警告依然可以正常编译，警告将编译出错
    QString content;//提示内容

    int length(){
        return fromFilePath.length();
    }

    QString getPath(int i){
        return fromFilePath[i];
    }
    uint getStartline(int i){
        return startline[i];
    }
    uint getEndline(int i){
        return endline[i];
    }
    QString getSrcText(int i){
        return srcText[i];
    }

private:
    QStringList fromFilePath;//产生该提示信息的文件路径
    QList<uint> startline;//文件中第几行开始
    QList<uint> endline;//文件中第几行结束
    QStringList srcText;//产生该提示的源文件文本
};


//全局指令[汇编指令格式:  指令号 参数1,参数2,参数3{子指令列表}]
class GlobalOrder{
public:
    QString order;
    QStringList args;
    QString subOrder;//如果没有就为空,如果有则为标志号$02.....
    QString toString(){
        return order+" "+args.join(",")+(subOrder=="" ? "" : "{"+subOrder+"}")+";";
    }
};

#define DataMem false
#define CodeMem true

class AsmSrcText;

//定义在内存块中的指令代码
class AsmBlockCode{
public:
    QStringList marks;//指向该指令的标记名
    QString orderName;//指令名
    QStringList args;//指令参数
    int startLine,endLine;//指令代码定义在源码文件中的第几行
};

//汇编源文件中定义的内存块文本内容
class AsmMemBlock{
public:
    int startLine,endLine,blockStartLine;//内存块内定义文本的起始行
    QString blockName;//内存块名
    bool type;//内存块类型[0数据 1指令]
    bool isExport;//是否要导出
    //isConstData与isZIData互斥，不可能同时为真
    bool isConstData;//是否为常量数据块(指令内存块无效)
    bool isZIData;//是否为未初始化数据块(指令内存块无效)
    QString blockSrcText;//内存块中定义的源码文本

    QList<AsmBlockCode> codes;//内存块中定义的指令列表


    MemBlockBin bin;//编译生成的二进制数据
};
//汇编源码文件中引入的内存块/标记文本内容
class AsmImportMark{
public:
    int startLine,endLine;
    QString blockName;//标记所属的内存块名
    QString markName;//标记本身的名字[如果只是引入内存块，标记名位"*"]
    QString toName(){
        if(markName!=""){
            return blockName+"."+markName;
        }
        return blockName;
    }
};

//格式化过后的汇编源文件类型
class AsmSrc{
public:

    bool iseffective = false;//是否有效
    QString path;//该源码所属的计算机文件地址
    QString asmSrcText;//汇编源码内容
    QList<AsmMemBlock> memBlocks;//源码中定义的内存块
    QList<AsmImportMark> importMarks;//源码中引入的标记

    QStringList strBank;//提取出来的字符串库,用于后续的编译
};





//宏定义类型
class Macro{
public:
    QString order;//宏类型
    QStringList args;//宏参数
    int line;//宏定义文本定义在文件第几行的
    QString toString(){
        QString argStr;
        for(int i = 0;i<args.length();i++){
            if(i<args.length()-1){
                argStr+=args.at(i)+" ";
            }else{
                argStr+=args.at(i);
            }
        }
        return QString::number(line)+"@"+order+" "+argStr;
    }
};

//用于在编译过程中的全局代码索引、定位
class IndexV2{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
};
class IndexV3{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
    int index3;//子指令索引号
};
class IndexV4{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
    int index3;//子指令索引号
    int index4;//标记索引号
};

#endif // ASMC_TYPEDEF_H
