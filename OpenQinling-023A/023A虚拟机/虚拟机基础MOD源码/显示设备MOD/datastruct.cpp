#include "datastruct.h"
#include <QDebug>


typedef uchar BYTE;
static bool memreverse(void*a,void*b,size_t size){
    if(a==NULL || b==NULL)return 0;

    uchar * a_t = (uchar*)a;
    uchar * b_t = (uchar*)b;

    for(uint i = 0;i<size;i++){
        a_t[i] = b_t[size-i-1];
    }
    return 1;
}


WORD::WORD(){}
WORD::WORD(uchar d0,uchar d1){
    data[0] = d0;
    data[1] = d1;
}
WORD::WORD(ushort d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[2];
    memcpy(tmp,&d,2);
    memreverse(this->data,tmp,2);
#else
    memcpy(this->data,&d,2);
#endif
}
WORD::WORD(short d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[2];
    memcpy(tmp,&d,2);
    memreverse(this->data,tmp,2);
#else
    memcpy(this->data,&d,2);
#endif
}

WORD& WORD::operator=(const WORD &d){
    for(int i = 0;i<2;i++){
        this->data[i] = d.data[i];
    }
    return *this;
}
//转换为c数据类型
ushort WORD::toUShort(){
    short d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[2];
    memreverse(tmp,this->data,2);
    memcpy(&d,tmp,2);
#else
    memcpy(&d,this->data,2);
#endif
    return d;
}
short WORD::toShort(){
    short d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[2];
    memreverse(tmp,this->data,2);
    memcpy(&d,tmp,2);
#else
    memcpy(&d,this->data,2);
#endif
    return d;
}
//转换为char[2]
QByteArray WORD::toByteArray(){
    return QByteArray((const char*)this->data,2);
}

uchar& WORD::operator[](int i){
    return this->data[i];
}
QString WORD::toHexString(){
    QStringList hexByte;
    for(int i = 0;i<2;i++){
        QString a = QString::number(data[i],16);
        while(a.length()<2){
            a.prepend("0");
        }
        a = a.right(2);
        a = a.toUpper();
        hexByte.append(a);
    }
    return "["+hexByte.join(' ')+"]";
}


DWORD::DWORD(){}
DWORD::DWORD(WORD d0,WORD d1){
    data[0] = d0[0];
    data[1] = d0[1];
    data[2] = d1[0];
    data[3] = d1[1];
}
DWORD::DWORD(uchar d0,uchar d1,uchar d2,uchar d3){
    data[0] = d0;
    data[1] = d1;
    data[2] = d2;
    data[3] = d3;
}
DWORD::DWORD(uint d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memcpy(tmp,&d,4);
    memreverse(this->data,tmp,4);
#else
    memcpy(this->data,&d,4);
#endif
}
DWORD::DWORD(int d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memcpy(tmp,&d,4);
    memreverse(this->data,tmp,4);
#else
    memcpy(this->data,&d,4);
#endif
}
DWORD::DWORD(float d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memcpy(tmp,&d,4);
    memreverse(this->data,tmp,4);
#else
    memcpy(this->data,&d,4);
#endif
}
DWORD& DWORD::operator=(const DWORD &d){
    for(int i = 0;i<4;i++){
        this->data[i] = d.data[i];
    }
    return *this;
}
//转换为c数据类型
uint DWORD::toUInt(){

    uint d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memreverse(tmp,this->data,4);
    memcpy(&d,tmp,4);
#else
    memcpy(&d,this->data,4);
#endif

    return d;
}
int DWORD::toInt(){
    int d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memreverse(tmp,this->data,4);
    memcpy(&d,tmp,4);
#else
    memcpy(&d,this->data,4);
#endif
    return d;
}
float DWORD::toFloat(){
    float d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[4];
    memreverse(tmp,this->data,4);
    memcpy(&d,tmp,4);
#else
    memcpy(&d,this->data,4);
#endif
    return d;
}
//转换为char[4]
QByteArray DWORD::toByteArray(){
    return QByteArray((const char*)this->data,4);
}
//转换为word[2]
QWordArray DWORD::toWordArray(){
    QWordArray arr = {WORD(data[0],data[1]),WORD(data[2],data[3])};
    return arr;
}

uchar& DWORD::operator[](int i){
    return this->data[i];
}
QString DWORD::toHexString(){
    QStringList hexByte;
    for(int i = 0;i<4;i++){
        QString a = QString::number(data[i],16);
        while(a.length()<2){
            a.prepend("0");
        }
        a = a.right(2);
        a = a.toUpper();
        hexByte.append(a);
    }
    return "["+hexByte.join(' ')+"]";
}




QWORD::QWORD(){

}
QWORD::QWORD(unsigned long long d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memcpy(tmp,&d,8);
    memreverse(this->data,tmp,8);
#else
    memcpy(this->data,&d,8);
#endif
}
QWORD::QWORD(double d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memcpy(tmp,&d,8);
    memreverse(this->data,tmp,8);
#else
    memcpy(this->data,&d,8);
#endif
}
QWORD::QWORD(long long d){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memcpy(tmp,&d,8);
    memreverse(this->data,tmp,8);
#else
    memcpy(this->data,&d,8);
#endif
}
QWORD& QWORD::operator=(const QWORD &d){
    for(int i = 0;i<8;i++){
        this->data[i] = d.data[i];
    }
    return *this;
}
QWORD::QWORD(DWORD d0,DWORD d1){
    data[0] = d0[0];
    data[1] = d0[1];
    data[2] = d0[2];
    data[3] = d0[3];
    data[4] = d1[0];
    data[5] = d1[1];
    data[6] = d1[2];
    data[7] = d1[3];
}
QWORD::QWORD(WORD d0,WORD d1,WORD d2,WORD d3){
    data[0] = d0[0];
    data[1] = d0[1];
    data[2] = d1[0];
    data[3] = d1[1];
    data[4] = d2[0];
    data[5] = d2[1];
    data[6] = d3[0];
    data[7] = d3[1];
}
QWORD::QWORD(uchar d0,uchar d1,uchar d2,uchar d3,
             uchar d4,uchar d5,uchar d6,uchar d7){
    data[0] = d0;
    data[1] = d1;
    data[2] = d2;
    data[3] = d3;
    data[4] = d4;
    data[5] = d5;
    data[6] = d6;
    data[7] = d7;
}

//转换为c数据类型
unsigned long long QWORD::toULong(){
    unsigned long long d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memreverse(tmp,this->data,8);
    memcpy(&d,tmp,8);
#else
    memcpy(&d,this->data,8);
#endif

    return d;
}
long long QWORD::toLong(){
    long long d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memreverse(tmp,this->data,8);
    memcpy(&d,tmp,8);
#else
    memcpy(&d,this->data,8);
#endif

    return d;
}
double QWORD::toDouble(){
    double d;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uchar tmp[8];
    memreverse(tmp,this->data,8);
    memcpy(&d,tmp,8);
#else
    memcpy(&d,this->data,8);
#endif
    return d;
}

//转换为char[4]
QByteArray QWORD::toByteArray(){
    return QByteArray((const char*)this->data,8);
}
QWordArray QWORD::toWordArray(){
    QWordArray arr = {WORD(data[0],data[1]),WORD(data[2],data[3]),WORD(data[4],data[5]),WORD(data[6],data[7])};
    return arr;
}
QDwordArray QWORD::toDWordArray(){
    QDwordArray arr = {DWORD(data[0],data[1],data[2],data[3]),DWORD(data[4],data[5],data[6],data[7])};
    return arr;
}

QString QWORD::toHexString(){
    QStringList hexByte;
    for(int i = 0;i<8;i++){
        QString a = QString::number(data[i],16);
        while(a.length()<2){
            a.prepend("0");
        }
        a = a.right(2);
        a = a.toUpper();
        hexByte.append(a);
    }
    return "["+hexByte.join(' ')+"]";
}

//转换为
uchar& QWORD::operator[](int i){
    return data[i];
}
















QDwordArray ByteToDwordArray(QByteArray array){
    uint len = array.length()/4;
    uint fm = array.length() - (len*4);
    array.append(fm,0);

    QDwordArray wordArr;
    for(uint i = 0;i<len;i++){
        wordArr.append(DWORD(array[i*4],array[i*4+1],array[i*4+2],array[i*4+3]));
    }
    return wordArr;
}
QDwordArray WordToDwordArray(QWordArray array){
    uint len = array.length()/2;
    uint fm = array.length() - (len*2);

    QDwordArray wordArr;
    for(uint i = 0;i<len;i++){
        wordArr.append(DWORD(array[i*2],array[i*2+1]));
    }
    if(fm){
        wordArr.append(DWORD(array[len*2],(ushort)0));
    }
    return wordArr;
}
QWordArray ByteToWordArray(QByteArray array){
    uint len = array.length()/2;
    uint fm = array.length() - (len*2);

    QWordArray wordArr;
    for(uint i = 0;i<len;i++){
        wordArr.append(WORD(array[i*2],array[i*2+1]));
    }
    if(fm){
        wordArr.append(WORD(array[len*2],0));
    }
    return wordArr;
}
QByteArray WordToByteArray(QWordArray array){
    QByteArray byteArr;
    for(int i = 0;i<array.length();i++){
        byteArr.append(array[i].toByteArray());
    }
    return byteArr;
}
QWordArray DwordToWordArray(QDwordArray array){
    QWordArray wordArr;
    for(int i = 0;i<array.length();i++){
        wordArr.append(array[i].toWordArray());
    }
    return wordArr;
}
QByteArray DwordToByteArray(QDwordArray array){
    QByteArray byteArr;
    for(int i = 0;i<array.length();i++){
        byteArr.append(array[i].toByteArray());
    }
    return byteArr;
}
