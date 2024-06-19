#ifndef OPENQINLING_ASM_BINARYOBJECTLIB_H
#define OPENQINLING_ASM_BINARYOBJECTLIB_H
#include <QString>
#include <QStringList>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>
#include "OpenQinling_ASM_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

//定义的地址标记
class BinaryMark{
public:
    QString markName;//标记名
    QString affiBlockName;//所属的内存块名
    uint affBlockOffset = 0;//相较于所属内存块基址的偏移地址(字节-左低右高)
    QString toString();
    QString toName();
};
//外部引入的地址标记
class BinaryImportMark{
public:
    QString markName;//标记名
    QString affiBlockName;//所属的内存块名
    QString toString();
    QString toName();
};
//地址标记的引用
class BinaryQuoteMark{
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
    uint value = 0;//标记指向的绝对内存地址

    QString toString();
    QString toName();


};
//生成的二进制内存块
class BinaryMemBlock{
public:
    QString name;//内存块名
    bool isExport;//是否导出
    //isConstData与isZIData互斥，不可能同时为真
    bool isConstData;//是否为常量数据块(指令内存块无效)
    bool isZIData;//是否为非初始化数据块(指令内存块无效)

    bool type;//内存块的类型
    uint baseAdd;//内存块的基地址[在编译器中该参数无用，用在链接器中]
    QList<BinaryMark> marks;//内存块定义的标记
    QList<BinaryQuoteMark> markQuotes;//当前内存块引用的标记[如果是非初始化数据块，则必然为空]

    uint binLength;//内存块中二进制数据的字节数
    QByteArray bin;//内存块的二进制数据[如果是非初始化数据块，则为空]
    QString toString();

};

//当前汇编源文件最终编译生成的类型
class BinaryObject{
public:
    QList<BinaryImportMark> marks_import;//导入外部源文件的标记
    QList<BinaryMemBlock> blocks;//内存块
    QString toString();


    //将BinaryObject转为格式化的二进制数据
    QByteArray toFormatByteArray();
    //从格式化的二进制数据中解析出BinaryObject对象[isSuceess是否解析成功]
    static BinaryObject fromFormatByteArray(QByteArray &bin,bool *isSuceess);
    //将obj列表转为二进制
    static QByteArray objsToFormatByteArray(QList<BinaryObject> objs);
    //从二进制列表中解析出obj列表
    static QList<BinaryObject> objsfromFormatByteArray(QByteArray &bin,bool *isSuceess);

};

class LibraryFile{
public:
    QString fileName;
    QList<BinaryObject> libData;
};

//判断是否有库文件之间存在公开内存块重名
bool judgeHaveLibBlockDupName(QList<LibraryFile> &libfiles,QStringList*prompts);

}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_BINARYOBJECTLIB_H
