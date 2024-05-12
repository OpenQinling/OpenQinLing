#ifndef DEBUGTOOL_H
#define DEBUGTOOL_H
#include <QString>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#pragma execution_character_set("utf-8")
void debugStringList(QStringList&list){
    foreach(QString str,list){
        qDebug()<<str.toUtf8().data();
    }
}
void debugByteArray(QByteArray arr){
    QString str;
    foreach(uchar c,arr){
        str+= QString::number(c,16).right(2)+",";
    }
    qDebug()<<str;
}

QString thisProjectPath;
//用于生成提示信息时，文件的路径简化
bool setProjectPath(QString path){
    QFileInfo info(path);
    if(info.isDir()){
        thisProjectPath = info.absoluteFilePath();
        return 1;
    }
    return 0;
}
//获取一个绝对路径相对于指定的项目路径地址的相对路径
QString getProjectRelativePath(QString path){
    QFileInfo info(path);
    QDir dir(thisProjectPath);
    return dir.relativeFilePath(info.absoluteFilePath());
}

void printByteArrayBinary(QByteArray arr){
    QString str;
    for(int i = 0;i<arr.length();i++){
        QString tmp = QString::number(((uint)arr[i]),2);
        while(tmp.length()<8){
           tmp.prepend('0');
        }
        tmp = tmp.right(8);
        str += tmp +" ";
    }
    qDebug()<<str;
}

void printUintBinary(uint d){
    QString str = QString::number(d,2);
    while(str.length()<32){
       str.prepend('0');
    }
    QString tmp;
    for(int i = 0;i<str.length();i++){
        tmp.append(str[i]);
        if(((i+1)%8)==0){
            tmp.append('_');
        }
    }
    qDebug()<<tmp;
}
void printByteBinary(uint8_t d){
    QString str = QString::number(d,2);
    while(str.length()<8){
       str.prepend('0');
    }
    qDebug()<<str;
}

//比较打印
void cmpPrintByteArrayBinary(QByteArray arr1,QByteArray arr2){
    QString str;
    for(int i = 0;i<arr1.length();i++){
        QString tmp1 = QString::number(((uint)arr1[i]),2);
        while(tmp1.length()<8){
           tmp1.prepend('0');
        }
        tmp1 = tmp1.right(8);


        if(arr1[i]==arr2[i]){
            str += "["+tmp1 +","+tmp1+"] ";
        }else{
            QString tmp2 = QString::number(((uint)arr2[i]),2);
            while(tmp2.length()<8){
               tmp2.prepend('0');
            }
            tmp2 = tmp2.right(8);

            str += "\033[31m["+tmp1 +","+tmp2+"] \033[0m";
        }

        if((i+1)%4 == 0){
            str += "\r\n";
        }


    }
    qDebug()<<str.toUtf8().data();
}
#endif // DEBUGTOOL_H
