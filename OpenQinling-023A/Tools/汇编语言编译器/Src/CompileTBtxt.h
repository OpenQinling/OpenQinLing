#ifndef COMPILETBTXT_H
#define COMPILETBTXT_H
#include <QString>
#include <QByteArray>
//将二进制数据编译为tb测试文本
QString compileTBtxt(QByteArray bin,uint address){
    QString tb_txt;
    for(int i = 0;i<bin.length();i++){
        QString num_16sys = QString::number((uchar)bin.data()[i],16);
        while(num_16sys.length()<2){
            num_16sys = num_16sys.insert(0,"0");
        }
        tb_txt+="\tdata["+QString::number(i+address)+"] <= 8'h"+num_16sys+";\r\n";
    }
    return tb_txt;
}

#endif // COMPILETBTXT_H
