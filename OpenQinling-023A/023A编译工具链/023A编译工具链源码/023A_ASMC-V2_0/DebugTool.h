#ifndef DEBUGTOOL_H
#define DEBUGTOOL_H
#include <QString>
#include <QDebug>
#include <FormatText.h>
void debugStringList(QStringList&list){
    foreach(QString str,list){
        qDebug()<<str.toUtf8().data();
    }
}
void debugOrderList(QList<GlobalOrder>&list){
    foreach(GlobalOrder str,list){
        qDebug()<<str.toString();
    }
}

void debugMacroList(QList<Macro>&list){
    foreach(Macro str,list){
        qDebug()<<str.toString();
    }
}

void debugIndexV3(QList<IndexV3>&list){
    foreach(IndexV3 str,list){
        qDebug()<<str.index1<<str.index2<<str.index3;
    }
}

void debugByteArray(QByteArray arr){
    QString str;
    foreach(uchar c,arr){
        str+= QString::number(c,16).right(2)+",";
    }
    qDebug()<<str;
}
#endif // DEBUGTOOL_H
