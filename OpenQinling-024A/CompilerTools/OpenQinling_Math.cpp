#include "OpenQinling_Math.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {

//uint从小到大排序
QList<uint> uintSortMinToMax(QList<uint> d){
    //将内存块已基地址进行从小到大排序
    for(int i = 0;i<d.length();i++){
        for(int j = i+1;j<d.length();j++){
            if(d[i]>d[j]){
                uint tmp = d[i];
                d[i] = d[j];
                d[j] = tmp;
            }
        }
    }
    return d;
}

//去除uint中的重复值
QList<uint> uintDeduplication(QList<uint> d){
    QList<uint> tmp;
    //将内存块已基地址进行从小到大排序
    for(int i = 0;i<d.length();i++){
        if(!tmp.contains(d[i])){
            tmp.append(d[i]);
        }
    }
    return tmp;
}

//判断一个uint类型变量的值，是否是正好为(2^n),如果是,输出n的值。否则输出-1
int isPowerOfTwo(uint64_t x){
    if(x == 0)return -1;
    if((x & (x-1)) == 0){
        int n = 0;
        while(x >1){
            x >>= 1;
            n++;
        }
        return n;
    }
    return -1;
}

}
QT_END_NAMESPACE
