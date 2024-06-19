#include "OpenQinling_DebugFunction.h"
#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace OpenQinling {
void debugStringList(QStringList&list){
    foreach(QString str,list){
        qDebug()<<str.toUtf8().data();
    }
}

void printPhraseList(C_Compiler::PhraseList &exps){
    QStringList tmp;
    for(int j = 0;j<exps.length();j++){
        tmp+= exps[j].toString();
    }
    debugStringList(tmp);
}
}
QT_END_NAMESPACE
