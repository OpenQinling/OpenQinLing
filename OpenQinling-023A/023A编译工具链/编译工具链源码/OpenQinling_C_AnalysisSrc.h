#ifndef OPENQINLING_C_ANALYSISSRC1_H
#define OPENQINLING_C_ANALYSISSRC1_H
#include <QString>
QT_BEGIN_NAMESPACE
namespace OpenQinling {

namespace C_Compiler{

//字符编码系统
enum CharCodeSystem{
    CharCode_UTF8,//默认为utf8编码(安卓/linux默认)
    CharCode_UTF16,//QT默认编码
    CharCode_GBK,//window系统的默认编码
};
//cpu内容的大小端
enum RamEndian{
    BIG_ENDIAN,
    LITTLE_ENDIAN
};

//cpu的位数
enum CpuRegBitwide{
    CpuRegBitwide_8bit,
    CpuRegBitwide_16bit,
    CpuRegBitwide_32bit,
    CpuRegBitwide_64bit
};

//编译过程中警告/报错的信息
struct PromptInfo{
    QString prompt;//提示内容
    QString srcPath;//产生该提示信息的源码路径
    int line;//产生该提示信息的源码行数
    int col;//产生该提示信息的源码列数
};

enum CompileType{
    Compile_To_IR,//将源码直接编译为中间表示语言
    Precompile_I,//仅将源码的预编译指令展开
};

QString compileSrc(QString src,//要编译的源码
                   QString srcPath,
                   CompileType compileType,//编译的功能类型
                   QStringList includeDir,//#include""默认搜索的目录
                   QStringList stdIncludeDir,//#include<>默认搜索的目录
                   QStringList exterdAttributes,//支持的扩展属性
                   bool&stutes,//编译结果
                   QList<PromptInfo> &prompts,//编译报错信息
                   CpuRegBitwide cpuBitwide = CpuRegBitwide_32bit,//cpu寄存器的位数
                   CharCodeSystem charCodeSys = CharCode_UTF8,//使用的字符编码系统
                   RamEndian ramEndian = BIG_ENDIAN//使用的内存大小端方式
                );

}


}
QT_END_NAMESPACE

#endif // OPENQINLING_C_ANALYSISSRC1_H
