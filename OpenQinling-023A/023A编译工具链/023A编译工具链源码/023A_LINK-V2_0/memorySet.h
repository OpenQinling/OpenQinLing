#ifndef MEMORYSET_H
#define MEMORYSET_H
#include <asmc_typedef.h>
//内存数据翻转
bool memreverse(void*a,void*b,size_t size){
    if(a==NULL || b==NULL)return 0;

    uchar * a_t = (uchar*)a;
    uchar * b_t = (uchar*)b;

    for(uint i = 0;i<size;i++){
        a_t[i] = b_t[size-i-1];
    }
    return 1;
}

//一段内存数据右移(i:右移的位数)
QByteArray byteArrayRightShift(QByteArray bytes,int i){
    if(i==0){
        return bytes;
    }
    QByteArray ret;
    ret.append(((uint8_t)bytes[0])>>i);
    for(int d = 0;d<bytes.length();d++){
        uint8_t v = 0;
        v = ((uint8_t)bytes[d])<<(8-i);
        if(d != bytes.length()-1){
            v |= ((uint8_t)bytes[d+1])>>i;
        }
        ret.append(v);
    }
    return ret;
}

#endif // MEMORYSET_H
