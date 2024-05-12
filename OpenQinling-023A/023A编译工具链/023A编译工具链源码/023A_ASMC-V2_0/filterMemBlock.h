#ifndef FILTERMEMBLOCK_H
#define FILTERMEMBLOCK_H
#include <asmc_typedef.h>
#include <QHash>
#include <QList>
#include <QDebug>
#include <DebugTool.h>
//单个内存块使用的标记所属内存名称
typedef QStringList MarksBlockNames;


//根据名称获取内存块索引号
IndexV2 getBlockIndexFromName(QList<AsmSrc> &src,QString &name){
    for(int i = 0;i<src.length();i++){
        for(int j =0;j<src[i].memBlocks.length();j++){
            if(src[i].memBlocks[j].blockName==name){
                return IndexV2({i,j});
            }
        }
    }
    return IndexV2();
}

//查询allBlocks中有用的内存块
void inquireUsefulBlocks(MarksBlockNames &nodeUseBlock,QHash<QString,MarksBlockNames> &range,QStringList &usefulBlockNames){

    for(int i = 0;i<nodeUseBlock.length();i++){
        if(range.contains(nodeUseBlock[i]) && !usefulBlockNames.contains(nodeUseBlock[i])){
            usefulBlockNames.append(nodeUseBlock[i]);
            inquireUsefulBlocks(range[nodeUseBlock[i]],range,usefulBlockNames);
        }
    }
}

//过滤掉无用的内存块
void filtrMemBlock(QList<AsmSrc> &asmSrcList){
    QHash<QString,MarksBlockNames> noExportBlockMarkInfo;//非导出内存块，标记引用的信息
    MarksBlockNames exportBlockMarkInfos;//导出内存块，标记引用的信息
    for(int i = 0;i<asmSrcList.length();i++){
        for(int j = 0;j<asmSrcList[i].memBlocks.length();j++){
            AsmMemBlock & thisBlock = asmSrcList[i].memBlocks[j];

            QStringList blockNames;
            for(int k = 0;k<thisBlock.bin.markQuoteList.length();k++){
                blockNames.append(thisBlock.bin.markQuoteList[k].affiBlockName);
            }
            if(thisBlock.isExport){
                exportBlockMarkInfos.append(blockNames);
            }else{
                noExportBlockMarkInfo.insert(thisBlock.blockName,blockNames);
            }
        }
    }

    //已各个导出内存块为根节点，解析出所有使用了的非导出内存块
    QStringList usefulBlockNames;
    inquireUsefulBlocks(exportBlockMarkInfos,noExportBlockMarkInfo,usefulBlockNames);
    //获取要去除的内存块名称
    QStringList uselessBlockNames;
    QStringList allBlockNames = noExportBlockMarkInfo.keys();
    for(int i = 0;i<allBlockNames.length();i++){
        if(!usefulBlockNames.contains(allBlockNames[i])){
            uselessBlockNames.append(allBlockNames[i]);
        }
    }
    foreach(QString blockName,uselessBlockNames){
        IndexV2 index = getBlockIndexFromName(asmSrcList,blockName);
        asmSrcList[index.index1].memBlocks.removeAt(index.index2);
    }
}



#endif // FILTERMEMBLOCK_H
