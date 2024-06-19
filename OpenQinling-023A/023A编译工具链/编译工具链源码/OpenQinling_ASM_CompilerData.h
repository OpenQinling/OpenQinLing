#ifndef OPENQINLING_ASM_COMPILERDATA_H
#define OPENQINLING_ASM_COMPILERDATA_H
#include "OpenQinling_ASM_FormatText.h"
#include <QString>
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_ASM_Transplant.h"
#include "OpenQinling_ASM_GrammarDetection.h"
#include "OpenQinling_DebugFunction.h"
#include "OpenQinling_DataStruct.h"
#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
//编译数据内存块
bool compileDataMemBlock(AsmSrc & src,QStringList&importMarksName,QStringList&defineMarksName,QStringList * prompts);
//编译数据
//编译无符号整形
unsigned long long compileUInt(QString order,bool*isSuccess);
//编译有符号整形
long long compileInt(QString order,bool*isSuccess);
}}
QT_END_NAMESPACE



#endif // OPENQINLING_ASM_COMPILERDATA_H
