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

//编译出单个块的coe文本
QString compileCOEtxt(QByteArray bin,int width=4){//width为几字节一组,默认4字节一组
    QString tb_txt;
    int remain = bin.length()%width;//去除完整的组后，剩下的字节数
    int groupNum = bin.length()/4;//一共有多少完整的组
    //示例: 一共35个字节,4字节一组。最终有8个完整的组，余3个字节单独组一组

    for(int i = 0;i<groupNum;i++){
        for(int j = 0;j<width;j++){
            char d = bin.at(i*width+j);
            QString d_hex_txt = QString::number(d,16);
            while(d_hex_txt.length()<2)d_hex_txt.prepend("0");

            tb_txt+=d_hex_txt.right(2);
        }
        if(i<groupNum-1){
            tb_txt.append("\r\n");
        }
    }

    if(remain!=0){
        tb_txt.append("\r\n");
        for(int j = 0;j<width;j++){
            char d = 0;
            if(j<remain){
                d = bin.at(groupNum*width+j);
            }
            QString d_hex_txt = QString::number(d,16);
            while(d_hex_txt.length()<2)d_hex_txt.prepend("0");

            tb_txt+=d_hex_txt.right(2);
        }
    }

    return tb_txt;
}


#endif // COMPILETBTXT_H
