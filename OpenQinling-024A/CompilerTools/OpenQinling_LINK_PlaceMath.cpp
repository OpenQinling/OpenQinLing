#include "OpenQinling_LINK_PlaceMath.h"
#include <QStringList>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


//获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
QList<bool> getBlockOverlap(QList<BlockInfo> memSpace){
    if(memSpace.length()==0){
        return QList<bool>();
    }
    QList<bool> isOverlap;
    for(int i = 0;i<memSpace.length()-1;i++){
        if(memSpace[i].baseAddress+memSpace[i].sizeBytes>=memSpace[i+1].baseAddress && memSpace[i].supportType == memSpace[i+1].supportType){
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
            if(memSpace[j].baseAddress>memSpace[j+1].baseAddress){
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
        memSpace[i].sizeBytes += memSpace[i].baseAddress;
        memSpace[i].sizeBytes -= 1;
    }

    bool isInBlock = 0;
    BlockInfo info;
    for(int i=0;i<overlap.length();i++){
        if(!isInBlock){
            info.baseAddress = memSpace[i].baseAddress;
            info.supportType = memSpace[i].supportType;
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
        overlapPlace.append(info);
    }
    for(int i = 0;i<overlapPlace.length();i++){
        overlapPlace[i].sizeBytes-=overlapPlace[i].baseAddress;
        overlapPlace[i].sizeBytes+=1;
    }
    return overlapPlace;
}

//判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
bool judgeContainBlockSet(QList<BlockInfo> memSpaceA,
                          uint64_t baseAddress,
                          ASM_Compiler::BinaryMemBlock &memBlock){
    if(memSpaceA.length()==0){
        return 0;
    }
    uint64_t endAddress = memBlock.binLength + baseAddress - 1;


    //获取块集A/B中所有块的基址和尾址
    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].sizeBytes+memSpaceA[i].baseAddress-1;
    }



    //依次判断块集中的每个块，这些块的基址和尾址是否能落在A中的同一个块里
    //当前B中块的基址和尾址所落在的A块索引号，如果没落在A中任何块则为-1
    int baseIndex = -1;
    int endIndex = -1;
    for(int j = 0;j<memSpaceA.length();j++){
        if(baseAddress>=memSpaceA[j].baseAddress && baseAddress<=memSpaceA[j].sizeBytes){
            baseIndex = j;
        }
        if(endAddress>=memSpaceA[j].baseAddress && endAddress<=memSpaceA[j].sizeBytes){
            endIndex = j;
        }
    }

    //但凡发现B块的基址/尾址没能落在A中任何一个块中，或者落在的块不同则直接返回非超集
    return !(baseIndex==-1 || endIndex==-1 || baseIndex!=endIndex);
}
//在块集A上减去块a
QList<BlockInfo> removeOccupiedMemBlock(QList<BlockInfo> memSpaceA,
                                        uint64_t baseAddress,uint64_t blockLength){
    if(blockLength==0){
        return memSpaceA;
    }
    uint64_t endAddress = blockLength + baseAddress - 1;

    for(int i = 0;i<memSpaceA.length();i++){
        if(baseAddress==memSpaceA[i].baseAddress && endAddress==memSpaceA[i].sizeBytes){
            memSpaceA.removeAt(i);
            return memSpaceA;
        }
    }

    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].baseAddress+memSpaceA[i].sizeBytes-1;
    }
    QList<BlockInfo> ret;

    BlockInfo info;

    for(int i = 0;i<memSpaceA.length();i++){
        info.supportType = memSpaceA[i].supportType;
        if(baseAddress==memSpaceA[i].baseAddress && endAddress==memSpaceA[i].sizeBytes){
            continue;
        }else if(baseAddress==memSpaceA[i].baseAddress && endAddress<memSpaceA[i].sizeBytes){
            //如果要去除的块对齐了剩余区块的头部,直接重设头部的索引=去除块的下一个单元
            info.baseAddress = endAddress+1;
            info.sizeBytes = memSpaceA[i].sizeBytes;
            ret.append(info);
        }else if(baseAddress>memSpaceA[i].baseAddress && endAddress==memSpaceA[i].sizeBytes){
            //如果要去除的块对齐的剩余区块的尾部，直接重设尾部的索引=去除块的前一个单元
            info.baseAddress = memSpaceA[i].baseAddress;
            info.sizeBytes = baseAddress-1;
            ret.append(info);
        }else if(baseAddress>memSpaceA[i].baseAddress && endAddress<memSpaceA[i].sizeBytes){
            info.baseAddress = memSpaceA[i].baseAddress;
            info.sizeBytes = baseAddress-1;
            ret.append(info);


            info.baseAddress = baseAddress+1;
            info.sizeBytes = memSpaceA[i].sizeBytes;
            ret.append(info);
        }else{
            ret.append(memSpaceA[i]);
        }
    }


    for(int i = 0;i<ret.length();i++){
        ret[i].sizeBytes = ret[i].sizeBytes-ret[i].baseAddress+1;
    }
    return ret;
}

//在块集A上找到能放下块a的位置
bool searchFreeMemarySpace(QList<BlockInfo> memSpaceA,ASM_Compiler::BinaryMemBlock &memBlock,uint64_t &retbaseAddress){
    if(memSpaceA.length()==0){
        return 0;
    }
    for(int i = 0;i<memSpaceA.length();i++){
        if(memSpaceA[i].sizeBytes >= memBlock.binLength && memSpaceA[i].isSupportType(memBlock)){
            retbaseAddress = memSpaceA[i].baseAddress;
            return 1;
        }
    }
    return 0;
}





}}
QT_END_NAMESPACE
