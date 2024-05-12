#ifndef DEBUGTOOL_H
#define DEBUGTOOL_H
#include <QDebug>
#include <Middle_typedef.h>
void debugNodeLine(QString line,MiddleNode &node){
    QString nodeText = node.toString();

    QString text = "=======================================\033[31m\r\n错误:"+
            line+"\033[0m\r\n"+nodeText;
    qDebug()<<text.toUtf8().data();
}
#endif // DEBUGTOOL_H
