#ifndef OPENQINLING_ASM_ANALYSISBLOCKCODE_H
#define OPENQINLING_ASM_ANALYSISBLOCKCODE_H
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_Typedefs.h"
#include <QDebug>
#include "OpenQinling_ASM_FormatText.h"
#include "OpenQinling_ASM_GrammarDetection.h"
#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
//解析源码文件定义的内存块中的指令
bool analysisAsmBlockCode(AsmSrc &asmSrc,QStringList*promptText);
//判断是否存在重名定义的标记
bool detectDuplicateDefMark(AsmSrc & asmSrc,QStringList*promptText);
}}
QT_END_NAMESPACE

#endif // OPENQINLING_ASM_ANALYSISBLOCKCODE_H
