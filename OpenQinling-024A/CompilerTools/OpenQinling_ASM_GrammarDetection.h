#ifndef OPENQINLING_ASM_GRAMMARDETECTION_H
#define OPENQINLING_ASM_GRAMMARDETECTION_H
#include "OpenQinling_ASM_FormatText.h"
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_ASM_TextFlag.h"
#include <QDir>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

//获取一个全局指令定义的内存块，其子指令是在源码文件中的第几行开始定义的
int getSubBlockStartLine(QString src,int instStartLine,int instEndLine);
//获取全局指令中发出提示信息的起始、结束行数、子指令块开始行数
QList<int> getPromptLine(int orderIndex,QString src,QStringList &orderStrList,
                         QList<uint> &lineFeed,
                         QList<uint> &startLine,
                         QStringList &defBigParentStr);
//获取子指令发出提示信息的起始、结束行数
QList<int> getBlockPromptLine(int orderIndex,
                              int blockStartLine,
                              QList<uint> &lineFeed,
                              QList<uint> &startLine);
//生成指令的编译器输出提示文本信息
QString grenratePromptText(Prompt prompt);
}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_GRAMMARDETECTION_H
