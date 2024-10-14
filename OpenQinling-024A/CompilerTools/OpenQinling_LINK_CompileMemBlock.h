#ifndef OPENQINLING_LINK_COMPILEMEMBLOCK_H
#define OPENQINLING_LINK_COMPILEMEMBLOCK_H
#include "OpenQinling_ASM_BinaryObjectLib.h"
#include <QStringList>
#include "OpenQinling_LINK_Typedefs.h"
#include "OpenQinling_LINK_ExecutableImage.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


//参数: libs所有要链接的静态库文件，link是静态库脚本解析出的信息
ExecutableImage compilerMemBlock(QList<ASM_Compiler::LibraryFile> &files,
                                 LinkSprict &link,
                                 bool &isSuceess,
                                 QStringList &prompts);



}}
QT_END_NAMESPACE
#endif // OPENQINLING_LINK_COMPILEMEMBLOCK_H
