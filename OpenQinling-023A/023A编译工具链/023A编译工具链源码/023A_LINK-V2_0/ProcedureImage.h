#ifndef PROCEDUREIMAGE_H
#define PROCEDUREIMAGE_H
#include <QList>
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
class ExecutableMemBlock{
public:
    uint baseAdd = 0;//内存块的基地址
    uint len = 0;    //内存块的长度
    bool isZiData = 0;//是否为空数据块
    QByteArray data;//内存块中的数据
};

//可执行程序映像数据
class ExecutableImage:public QList<ExecutableMemBlock>{
public:
    //mode: 0只打印出各个内存块的基址/大小 1还打印出各内存块的二进制数据
    QString toString(bool mode=0){
        reorder();
        QString str;

        for(int i = 0;i<this->length();i++){
            str+="Block"+QString::number(i)+"[";

            QString addressNum = QString::number(this->at(i).baseAdd,16);

            str+="base:"+ addressNum + ",";

            str+="len:"+QString::number(this->at(i).len)+"]";

            if(mode==1 && !this->at(i).isZiData){
                QStringList list;
                str+="\r\n\tbin:\r\n\t ";
                for(int j=0;j<this->at(i).data.length();j++){
                    QString byteNum = QString::number((uint8_t)this->at(i).data[j],16);
                    byteNum = byteNum.toUpper();
                    while(byteNum.length()<2){
                        byteNum.prepend('0');
                    }
                    byteNum = byteNum.right(2);

                    if((j+1)%4 == 0){
                        byteNum += "\r\n\t";
                    }
                    list.append(byteNum);
                }
                str+=list.join(' ');
            }else if(mode==1 && this->at(i).isZiData){
                str+="Not Init";
            }

            str+="\r\n";
        }

        return str;
    }


    //从etb格式的文件数据中提取出可执行程序映像数据对象
    bool fromEtbFileData(QByteArray data){
        this->clear();
        QBuffer buffer(&data);
        buffer.open(QBuffer::ReadWrite);
        QDataStream stream(&buffer);

        QByteArray arr;
        int8_t charByte;
        do {
            stream >>charByte;
            arr.append(charByte);
        }while (charByte!=0);

        QString titleText = arr;

        if(titleText.length()<40)return 0;
        QString txt = titleText.left(34);
        titleText.remove(0,34);

        if(txt!="OpenQinling-CPU ETB file\r\ntitle:\r\n"){
            return 0;
        }
        titleText.remove(titleText.length()-6,6);


        int check;
        int len;
        stream >> len;
        stream >> check;
        if(check!=1-len){
            return 0;
        }

        for(int i = 0;i<len;i++){
            ExecutableMemBlock block;

            stream >> check;
            if(check!=len-i){
                return 0;
            }
            stream >> block.baseAdd;
            stream >> block.len;
            stream >> block.isZiData;
            stream >> block.data;

            this->append(block);
        }

        buffer.close();
        reorder();

        if(this->toString()!=titleText){

            this->clear();
            return 0;
        }
        return 1;
    }

    //转为etb格式的文件数据(OpenQinling-CPU编译工具链专用程序映像数据存储格式)
    QByteArray toEtbFileData(){
        reorder();
        QByteArray bin;
        QBuffer buffer(&bin);
        buffer.open(QBuffer::ReadWrite);

        QDataStream stream(&buffer);

        QString titleText = "OpenQinling-CPU ETB file\r\n";
        titleText += "title:\r\n"+this->toString();
        titleText += "bin:\r\n";
        QByteArray arr = titleText.toUtf8();
        for(int i = 0;i<arr.length();i++){
            stream <<(int8_t)arr[i];
        }
        stream <<(int8_t)0;
        stream << this->length();
        stream << (1-this->length());//校验码
        for(int i = 0;i<this->length();i++){
            stream << this->length()-i;//校验码
            stream << this->at(i).baseAdd;
            stream << this->at(i).len;
            stream << this->at(i).isZiData;
            stream << this->at(i).data;
        }
        buffer.close();
        return bin;
    }


private:
    //根据内存块地址从低到高排序
    void reorder(){
        for(int i = 0;i<this->length();i++){
            for(int j = i+1;j<this->length();j++){
                if(this->at(i).baseAdd>this->at(j).baseAdd){
                    ExecutableMemBlock tmp = this->at(i);
                    this->operator[](i) = this->at(j);
                    this->operator[](j) = tmp;
                }
            }
        }
    }
};



#endif // PROCEDUREIMAGE_H
