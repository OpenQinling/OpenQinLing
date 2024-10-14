#ifndef OPENQINLING_COMPILECORE_H
#define OPENQINLING_COMPILECORE_H
#include <QString>
#include "OpenQinling_ASM_BinaryObjectLib.h"
#include "OpenQinling_LINK_ExecutableImage.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {

//c源码编译,编译为IR代码文本
QString compileC_IR(QString src,
                    QString srcPath,
                    QStringList includeDir,//#include""默认搜索的目录
                    QStringList stdIncludeDir,//#include<>默认搜索的目录
                    bool&stutes,
                    QStringList &promptText);

//将c源码中的预编译指令展开，生成展开后的c代码文本
QString compileC_I(QString src,
                   QString srcPath,
                   QStringList includeDir,//#include""默认搜索的目录
                   QStringList stdIncludeDir,//#include<>默认搜索的目录
                   bool&stutes,
                   QStringList &promptText);

//将IR代码编译为汇编语言文本
QString compileIR_ASM(QString src,
                      bool&stutes);

//将汇编语言编译为静态库
ASM_Compiler::LibraryFile compileASM_LIB(QString src,
                                         bool&stutes,
                                         QStringList &promptText,
                                         QString srcPath);

//合并多个静态库为一个静态库
ASM_Compiler::LibraryFile mergeLIB(QList<ASM_Compiler::LibraryFile> libs,
                                   bool&stutes,
                                   QStringList &promptText);

/*链接静态库为可执行文件
返回值:            链接完成的可执行程序的镜像数据
参数libs:         待链接的所有静态库
参数linkSprict:   指示如何链接的脚本代码文本*/
LIB_StaticLink::ExecutableImage linkLIB(QList<ASM_Compiler::LibraryFile> libs,
                                        QString linkSprict,
                                        bool&stutes,
                                        QStringList &promptText);
}
QT_END_NAMESPACE

#endif // OPENQINLING_COMPILECORE_H
