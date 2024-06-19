#ifndef OPENQINLING_PSDL_ANALYSISFUNCTION_H
#define OPENQINLING_PSDL_ANALYSISFUNCTION_H

#include <OpenQinling_PSDL_MiddleNode.h>


QT_BEGIN_NAMESPACE
namespace OpenQinling {


namespace PsdlIR_Compiler{
QString analysisFunction(MiddleNode &node,QStringList &allAddressLabel,QStringList &operFun,bool&status);
}

}
QT_END_NAMESPACE


#endif // OPENQINLING_PSDL_ANALYSISFUNCTION_H
