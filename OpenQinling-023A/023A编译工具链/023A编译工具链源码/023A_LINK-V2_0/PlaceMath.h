#ifndef PLACEMATH_H
#define PLACEMATH_H
#include <asmc_typedef.h>

//获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
QList<bool> getBlockOverlap(QList<BlockInfo> memSpace){
    if(memSpace.length()==0){
        return QList<bool>();
    }
    QList<bool> isOverlap;
    for(int i = 0;i<memSpace.length()-1;i++){
        if(memSpace[i].baseAdress+memSpace[i].sizeBytes>memSpace[i+1].baseAdress){
            isOverlap.append(1);
        }else{
            isOverlap.append(0);
        }
    }
    return isOverlap;
}

//整合重叠的区域,并从小到大排序
QList<BlockInfo> integrationOverlapPlace(QList<BlockInfo> memSpace){
    if(memSpace.length()==0){
        return memSpace;
    }
    //先对块进行排序
    for(int i=0;i<memSpace.length();i++){
        for(int j=0;j<memSpace.length()-1-i;j++){
            if(memSpace[j].baseAdress>memSpace[j+1].baseAdress){
                BlockInfo tmp = memSpace[j];
                memSpace[j] = memSpace[j+1];
                memSpace[j+1] = tmp;
            }
        }
    }

    QList<BlockInfo> overlapPlace;
    //获取各相邻块是否有重叠
    QList<bool> overlap = getBlockOverlap(memSpace);

    //将length暂存块的结束地址
    for(int i=0;i<memSpace.length();i++){
        memSpace[i].sizeBytes += memSpace[i].baseAdress;
        memSpace[i].sizeBytes -= 1;
    }

    bool isInBlock = 0;
    BlockInfo info;
    for(int i=0;i<overlap.length();i++){
        if(!isInBlock){
            info.baseAdress = memSpace[i].baseAdress;
            isInBlock = true;
        }
        if(!overlap[i]){
            info.sizeBytes = memSpace[i].sizeBytes;
            overlapPlace.append(info);
            isInBlock = false;
        }
    }

    if(!isInBlock){
        info = memSpace[memSpace.length()-1];
        overlapPlace.append(info);
    }else{
        info.sizeBytes = memSpace[memSpace.length()-1].sizeBytes;
        qDebug()<<info.sizeBytes;
        overlapPlace.append(info);
    }
    for(int i = 0;i<overlapPlace.length();i++){
        overlapPlace[i].sizeBytes-=overlapPlace[i].baseAdress;
        overlapPlace[i].sizeBytes+=1;
    }
    return overlapPlace;
}

//判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
bool judgeContainBlockSet(QList<BlockInfo> memSpaceA,
                          QList<BlockInfo> memSpaceB){
    if(memSpaceA.length()==0 && memSpaceB.length()==0){
        return 1;
    }else if(memSpaceA.length()==0 && memSpaceB.length()==1){
        return 0;
    }else if(memSpaceA.length()==1 && memSpaceB.length()==0){
        return 1;
    }


    //获取块集A/B中所有块的基址和尾址
    for(int i = 0;i<memSpaceB.length();i++){
        memSpaceB[i].sizeBytes = memSpaceB[i].sizeBytes+memSpaceB[i].baseAdress-1;
    }
    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].sizeBytes+memSpaceA[i].baseAdress-1;
    }



    //依次判断块集中的每个块，这些块的基址和尾址是否能落在A中的同一个块里
    for(int i = 0;i<memSpaceB.length();i++){

        //当前B中块的基址和尾址所落在的A块索引号，如果没落在A中任何块则为-1
        int baseIndex = -1;
        int endIndex = -1;

        for(int j = 0;j<memSpaceA.length();j++){
            if(memSpaceB[i].baseAdress>=memSpaceA[j].baseAdress && memSpaceB[i].baseAdress<=memSpaceA[j].sizeBytes){
                baseIndex = j;
            }
            if(memSpaceB[i].sizeBytes>=memSpaceA[j].baseAdress && memSpaceB[i].sizeBytes<=memSpaceA[j].sizeBytes){
                endIndex = j;
            }
        }

        //但凡发现B中有一个块的基址/尾址没能落在A中任何一个块中，或者落在的块不同则直接返回非超集
        if(baseIndex==-1 || endIndex==-1 || baseIndex!=endIndex){
            return 0;
        }
    }
    return 1;
}
//在块集A上减去块a
QList<BlockInfo> removeOccupiedMemBlock(QList<BlockInfo> memSpaceA,
                                        BlockInfo mema){
    if(mema.sizeBytes==0){
        return memSpaceA;
    }

    for(int i = 0;i<memSpaceA.length();i++){
        if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){
            memSpaceA.removeAt(i);
            return memSpaceA;
        }
    }

    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].baseAdress+memSpaceA[i].sizeBytes-1;
    }
    mema.sizeBytes = mema.baseAdress+mema.sizeBytes-1;


    QList<uint>retBase,retLen;
    QList<BlockInfo> ret;
    for(int i = 0;i<memSpaceA.length();i++){
        if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){



            break;
        }else if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes<memSpaceA[i].sizeBytes){
            //如果要去除的块对齐了剩余区块的头部,直接重设头部的索引=去除块的下一个单元
            retBase.append(mema.sizeBytes+1);
            retLen.append(memSpaceA[i].sizeBytes);
        }else if(mema.baseAdress>memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){
            //如果要去除的块对齐的剩余区块的尾部，直接重设尾部的索引=去除块的前一个单元
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(mema.baseAdress-1);
        }else if(mema.baseAdress>memSpaceA[i].baseAdress && mema.sizeBytes<memSpaceA[i].sizeBytes){
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(mema.baseAdress-1);
            retBase.append(mema.sizeBytes+1);
            retLen.append(memSpaceA[i].sizeBytes);
        }else{
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(memSpaceA[i].sizeBytes);
        }
    }


    for(int i = 0;i<retBase.length();i++){
        BlockInfo info;
        info.baseAdress = retBase[i];
        info.sizeBytes = retLen[i]-retBase[i]+1;
        ret.append(info);
    }
    return ret;
}

//在块集A上找到能放下块a的位置
BlockInfo searchFreeMemarySpace(QList<BlockInfo> memSpaceA,uint mema_size,bool *isScueess){
     BlockInfo info;
    if(memSpaceA.length()==0){
        if(isScueess!=NULL){
            *isScueess = 0;
        }
        return info;
    }

    for(int i = 0;i<memSpaceA.length();i++){
        if(memSpaceA[i].sizeBytes >= mema_size){
            info.baseAdress = memSpaceA[i].baseAdress;
            info.sizeBytes = mema_size;
            if(isScueess!=NULL){
                *isScueess = 1;
            }
            return info;
        }
    }
    if(isScueess!=NULL){
        *isScueess = 0;
    }
    return info;
}

#endif // PLACEMATH_H
