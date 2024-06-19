#ifndef GRAMMARTICALANALYSIS_H
#define GRAMMARTICALANALYSIS_H
#include <QString>
#include "OpenQinling_PSDL_MiddleNode.h"


QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

//编译中级语言文本为汇编语言文本
QString complieMiddleSrc(QString srcText,bool &isSuceess);
}
}
QT_END_NAMESPACE

#endif // GRAMMARTICALANALYSIS_H
