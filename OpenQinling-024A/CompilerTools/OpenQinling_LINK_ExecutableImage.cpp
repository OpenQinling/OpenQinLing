#include "OpenQinling_LINK_ExecutableImage.h"
#pragma execution_character_set("utf-8")
#include <QStringList>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{
//mode: 0只打印出各个内存块的基址/大小 1还打印出各内存块的二进制数据
QString ExecutableImage::toString(bool mode){
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
bool ExecutableImage::fromEtbFileData(QByteArray data){
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
    titleText.remove(titleText.length()-7,7);
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


//转为coe格式的文件数据(Vivado BRAM初始化使用的数据格式)
QByteArray ExecutableImage::toCoeFileData(uint byteWidth){
    //先转为.bin格式,然后输出为coe文本
    QByteArray bin = toBinFileData();

    QString str = "memory_initialization_radix=16;\r\nmemory_initialization_vector=";

    int i = 0;
    while(i<bin.length()){
        QString tmp;
        for(uint j = 0;j<byteWidth;j++){
            if(i+j >= bin.length()){
                tmp += "00";
                continue;
            }
            QString byteTmp = QString::number((uint8_t)bin[i+j],16);
            while(byteTmp.length()<2){
                byteTmp.prepend('0');
            }
            tmp += byteTmp.right(2);
        }

        str += "\r\n"+tmp+",";
        i+=byteWidth;
    }

    if(str[str.length()-1]==','){
        str.removeLast();
    }
    str+=";";
    return str.toUtf8();
}


//转为etb格式的文件数据(OpenQinling-CPU编译工具链专用程序映像数据存储格式)
QByteArray ExecutableImage::toEtbFileData(){
    int length = this->length();

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
    stream << length;
    stream << (1-length);//校验码
    for(int i = 0;i<length;i++){
        stream << length-i;//校验码
        stream << this->at(i).baseAdd;
        stream << this->at(i).len;
        stream << this->at(i).isZiData;
        stream << this->at(i).data;
    }
    buffer.close();
    return bin;
}

//转为bin格式的文件数据(通用二进制文件存储格式)
QByteArray ExecutableImage::toBinFileData(){
    reorder();

    int length = this->length();

    while(this->length()!=0){
        if(this->at(length-1).isZiData){
            length-=1;
        }else{
            break;
        }
    }



    QByteArray bin;
    if(length==0)return bin;

    uint64_t lastEndAddress = 0;//上一个内存块最后一个字节的下一个字节的地址(上一个内存块基址+字节数)
    for(int i = 0;i<length;i++){
        //填充当前内存块到上一个内存块间空白区域(填充0x00)
        bin += QByteArray(this->at(i).baseAdd - lastEndAddress,0x00);

        if(this->at(i).isZiData){//如果是非初始化区域，填充0x00
            bin += QByteArray(this->at(i).len,0x00);
        }else{//初始化区域，填充初始化的数据
            bin += this->at(i).data;
        }

        lastEndAddress = this->at(i).baseAdd + this->at(i).len;
    }

    return bin;
}



//根据内存块地址从低到高排序
void ExecutableImage::reorder(){
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




}}
QT_END_NAMESPACE
