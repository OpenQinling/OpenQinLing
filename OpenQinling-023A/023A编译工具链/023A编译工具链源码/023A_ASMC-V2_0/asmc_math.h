#ifndef ASMC_MATH_H
#define ASMC_MATH_H
#include <QList>
#include <QDebug>
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
#endif // ASMC_MATH_H
