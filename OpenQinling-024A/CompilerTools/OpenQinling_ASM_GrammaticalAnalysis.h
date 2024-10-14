#ifndef OPENQINLING_ASM_GRAMMATICALANALYSIS_H
#define OPENQINLING_ASM_GRAMMATICALANALYSIS_H
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_ASM_FormatText.h"
#include "OpenQinling_DebugFunction.h"
#include "OpenQinling_ASM_GrammarDetection.h"
#include <QDebug>
#include "OpenQinling_ASM_AnalysisBlockCode.h"
#include "OpenQinling_ASM_CompilerData.h"
#include "OpenQinling_ASM_CompilerCode.h"
#include "OpenQinling_ASM_BinaryObjectLib.h"
#include "OpenQinling_ASM_FilterMemBlock.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

BinaryObject complieAsmSrc(QString srcText,QString srcPath,bool * isSuceess,QStringList*prompts);

}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_GRAMMATICALANALYSIS_H
