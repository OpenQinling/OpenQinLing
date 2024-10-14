#ifndef OPENQINLING_DEBUGFUNC_H
#define OPENQINLING_DEBUGFUNC_H


#include <QDebug>
#include "OpenQinling_C_Phrase.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {

void debugStringList(QStringList&list);
void printPhraseList(C_Compiler::PhraseList &exps);

}
QT_END_NAMESPACE
#endif // OPENQINLING_DEBUGFUNC_H
