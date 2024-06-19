#ifndef OPENQINLING_ASM_TYPEDEFS_H
#define OPENQINLING_ASM_TYPEDEFS_H
#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{





//地址标记定义
class BlockMarkDef{
public:
    QString name;//标记名
    QString affiBlockName;//所属的内存块名
    uint affBlockOffset = 0;//相较于所属内存块基址的偏移地址(字节-左低右高)
    //isImport为1的话，affBlockOffset参数无效
    QString toName();

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

    QString toName();


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
    bool isError;//0为警告,1为错误。警告依然可以正常编译，警告将编译出错
    QString content;//提示内容
    void append(QString fromFilePath,
                QString srcText,
                uint startline,
                uint endline);
    int length();
    QString getPath(int i);
    uint getStartline(int i);
    uint getEndline(int i);
    QString getSrcText(int i);





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
    QString toString();

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
    QString toName();

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



}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_TYPEDEFS_H
