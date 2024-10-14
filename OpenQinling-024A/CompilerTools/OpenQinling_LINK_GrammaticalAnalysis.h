#ifndef OPENQINLING_LINK_GRAMMATICALANALYSIS_H
#define OPENQINLING_LINK_GRAMMATICALANALYSIS_H
#include <QStringList>
#include "OpenQinling_LINK_Typedefs.h"
#include "OpenQinling_FormatText.h"
#include "OpenQinling_LINK_PlaceMath.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{
//解析链接脚本
LinkSprict linkGrammaticalAnalysis(QString str,QStringList&prompts,bool&success);
}}
QT_END_NAMESPACE

#endif // OPENQINLING_LINK_GRAMMATICALANALYSIS_H
