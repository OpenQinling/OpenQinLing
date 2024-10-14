#ifndef OPENQINLING_ASM_COMPILERCODE_H
#define OPENQINLING_ASM_COMPILERCODE_H
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_ASM_Transplant.h"
#include "OpenQinling_ASM_GrammarDetection.h"
#include "OpenQinling_ASM_CompilerData.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
//编译数据内存块
bool compileCodeMemBlock(AsmSrc & src,QStringList&importMarksName,QStringList&defineMarksName,QStringList * prompts);

}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_COMPILERCODE_H
