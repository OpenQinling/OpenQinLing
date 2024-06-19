#ifndef OPENQINLING_C_PRECOMPILE_H
#define OPENQINLING_C_PRECOMPILE_H

#include "OpenQinling_C_SrcObjectInfo.h"
#include "OpenQinling_C_Phrase.h"

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

bool analysisPrecompile(SrcObjectInfo &objInfo,
                        PhraseList & phraseList);


}}
QT_END_NAMESPACE

#endif // OPENQINLING_C_PRECOMPILE_H
