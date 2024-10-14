#ifndef LEXICALANALYZER_H
#define LEXICALANALYZER_H
#include "OpenQinling_C_SrcObjectInfo.h"
#include "OpenQinling_C_Phrase.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

//C语言词法分析器（输出扁平型token）
PhraseList lexicalAnalyzer_C(SrcObjectInfo &srcObjInfo,
                             QString &srcPath,
                             QString &src,
                             bool *sucess,
                             bool isMacroExp = 0);
}

}
QT_END_NAMESPACE

#endif // LEXICALANALYZER_H
