#include "OpenQinling_FormatText.h"
#include <QStringList>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
//去除注释[支持#]
QString removeExpNote(QString txt){
    QStringList list = txt.split('\n');//每行的文本
    QStringList out;
    foreach(QString line,list){
        QStringList d = line.split("#");
        if(d.length()>1){
            out.append(d.at(0)+"\n");
        }else{
            out.append(d.at(0)+"\n");
        }
    }
    return out.join("");
}

//去除掉\t和多个空格以及首尾的空格
QString dislodgeSpace(QString txt){
    QString tmp;
    //去除\t
    foreach(QChar c,txt){
        if(c=='\t'){
            tmp.append(" ");
        }else{
            tmp.append(c);
        }
    }

    if(txt.isEmpty())return txt;

    //将多个空格合并为1个空格
    QString ret;
    bool isSpace = 0;
    for(int i = 0;i<tmp.length();i++){
        QChar c = tmp.at(i);
        if(!isSpace && c==' '){
            ret.append(' ');

            isSpace = 1;
        }else if(c!=' '){
            ret.append(c);
            isSpace = 0;
        }
    }
    //去除首尾的空格
    while(ret.at(0)==' '){
        ret.remove(0,1);
    }
    while(ret.right(1)==' '){
        ret.remove(ret.length()-1,1);
    }
    return ret;
}


//文本整体tab右移[num为右移次数]
QString textRightShift(QString txt,int num){
    QString tmp;
    for(int j = 0;j<num;j++){
        tmp.append("\t");
    }
    for(int i = 0;i<txt.length();i++){
        tmp.append(txt[i]);
        if(txt[i]=='\n' && i != txt.length()-1){
            for(int j = 0;j<num;j++){
                tmp.append("\t");
            }
        }
    }


    return tmp;
}
}
QT_END_NAMESPACE
