#ifndef COMPILEERRORHANDLE_H
#define COMPILEERRORHANDLE_H
#include <QByteArray>
#include <QString>
//编译出错的提示文本生成
QByteArray compileError(int index,QString asm_txt,QString prompt){
    char tmp[256];
    sprintf(tmp,"%d.<%s>=%s",index,asm_txt.toUtf8().data(),prompt.toUtf8().data());
    return tmp;
}

#endif // COMPILEERRORHANDLE_H
