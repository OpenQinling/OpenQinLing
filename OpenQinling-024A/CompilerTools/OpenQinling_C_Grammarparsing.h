#ifndef CGRAMMARPARSING_H
#define CGRAMMARPARSING_H
#include "OpenQinling_C_SrcObjectInfo.h"
#include "OpenQinling_C_Phrase.h"
#include "OpenQinling_PSDL_MiddleNode.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

PsdlIR_Compiler::MiddleNode analysisGrammatical_C(SrcObjectInfo &objInfo,
                                                  PhraseList & phraseList,
                                                  bool &status);
}}
QT_END_NAMESPACE

#endif // CGRAMMARPARSING_H
