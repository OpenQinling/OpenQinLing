#include "OpenQinling_LINK_CompileMemBlock.h"
#include <QStringList>
#pragma execution_character_set("utf-8")

#include "OpenQinling_LINK_PlaceMath.h"
#include "OpenQinling_LINK_MemorySet.h"
#include "OpenQinling_DebugFunction.h"

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


//检查内存块的定义是否符合要求
bool checkMemBlockDef(QList<ASM_Compiler::LibraryFile> &files,LinkSprict &link,QStringList *prompts){
    QStringList public_blocksName;
    QStringList public_markDef;
    foreach(ASM_Compiler::LibraryFile lib,files){
        foreach(ASM_Compiler::BinaryObject obj,lib.libData){
            foreach(ASM_Compiler::BinaryMemBlock block,obj.blocks){
                if(block.isExport){
                    public_blocksName.append(block.name);
                    foreach(ASM_Compiler::BinaryMark mark,block.marks){
                        public_markDef.append(mark.toName());
                    }
                }
            }
        }
    }

    //根据link中的CHECK命令，检测所有静态库中是否存在未定义的公开内存块
    bool isHave = 0;
    foreach(QString checkName,link.check){
        if(!public_blocksName.contains(checkName)){
            prompts->append("\033[31m错误:\033[0m存在没有定义的内存块<"+checkName+">");
            isHave = 1;
        }
    }
    if(isHave){
        return 0;
    }

    //检测是否存在被引用了的公开内存块的标记并未被定义
    foreach(ASM_Compiler::LibraryFile lib,files){
        foreach(ASM_Compiler::BinaryObject obj,lib.libData){
            foreach(ASM_Compiler::BinaryImportMark mark,obj.marks_import){
                if(!public_markDef.contains(mark.toName())){
                    prompts->append("\033[31m错误:\033[0m在"+lib.fileName+"中引用了全局标记<"+mark.toName()+">,但是其未被定义");
                    isHave = 1;
                }
            }
        }
    }
    return !isHave;
}

//获取一个内存块中所引用了的标记所属的内存块名
QStringList getMemBlockUseBlock(ASM_Compiler::BinaryMemBlock &mem){
    QStringList names;
    foreach(ASM_Compiler::BinaryQuoteMark quote,mem.markQuotes){
        names.append(quote.affiBlockName);
    }
    return names;
}
//判断一个内存块是否属于指定的obj中的私有内存块
bool thisBlockIsPrivateFromObj(QString blockName,ASM_Compiler::BinaryObject & obj){
    foreach(ASM_Compiler::BinaryMemBlock mem,obj.blocks){
        if(blockName==mem.name){
            return !mem.isExport;
        }
    }
    return 0;
}
//根据名称在库中搜索到内存块的对象以及所属的obj
ASM_Compiler::BinaryMemBlock* getPublicMem(QList<ASM_Compiler::LibraryFile> &files,QString name,ASM_Compiler::BinaryObject*fromObj=NULL){
    for(int i = 0;i<files.length();i++){
        ASM_Compiler::LibraryFile &file = files[i];
        for(int j = 0;j<file.libData.length();j++){
            ASM_Compiler::BinaryObject &obj = file.libData[j];
            for(int k = 0;k<obj.blocks.length();k++){
                if(obj.blocks[k].name == name){
                    if(fromObj!=NULL){
                        *fromObj = obj;
                    }
                    return &obj.blocks[k];
                }
            }
        }
    }
    return NULL;
}
//在指定的obj中搜索内存块
ASM_Compiler::BinaryMemBlock* getMemFromObj(ASM_Compiler::BinaryObject & obj,QString name){
    for(int k = 0;k<obj.blocks.length();k++){
        if(name == obj.blocks[k].name){
            return &obj.blocks[k];
        }
    }
    return NULL;
}


//搜索出当前几个全局内存块所使用了的所有内存块的名字
void getAllUsePublicMem(QList<ASM_Compiler::LibraryFile> &files,QStringList root,QStringList*all){
    foreach(QString name,root){
        ASM_Compiler::BinaryObject obj;

        ASM_Compiler::BinaryMemBlock * block=NULL;
        block = getPublicMem(files,name,&obj);

        QStringList tmps = getMemBlockUseBlock(*block);
        foreach(QString tmp,tmps){
            if(!all->contains(tmp) && !thisBlockIsPrivateFromObj(tmp,obj)){
                all->append(tmp);
                getAllUsePublicMem(files,{tmp},all);
            }
        }
    }
}

//获取一个obj中所有使用的内存块的名字
void getAllUseMem(ASM_Compiler::BinaryObject & obj,QStringList root,QStringList*all){
    foreach(QString name,root){
        ASM_Compiler::BinaryMemBlock *block = getMemFromObj(obj,name);
        if(block==NULL){
            continue;
        }
        QStringList tmps = getMemBlockUseBlock(*block);
        foreach(QString tmp,tmps){
            if(!all->contains(tmp)){
                all->append(tmp);
                getAllUseMem(obj,{tmp},all);
            }
        }
    }
}

//过滤掉各个obj中无用的私有内存块
void removMemBlockInObj(ASM_Compiler::BinaryObject & obj){
    QStringList allUseBlockNames;
    foreach(ASM_Compiler::BinaryMemBlock mem,obj.blocks){
        if(mem.isExport){
            allUseBlockNames.append(mem.name);
        }
    }

    getAllUseMem(obj,allUseBlockNames,&allUseBlockNames);

    //根据名称删除没有被使用到的私有内存块
    for(int k = 0;k<obj.blocks.length();k++){
        if(!obj.blocks[k].isExport && !allUseBlockNames.contains(obj.blocks[k].name)){
            obj.blocks.removeAt(k);
            k--;
        }
    }
}

//获取各个check的内存块,其引用了所属obj的私有内存块后，间接引用的该obj的全局内存块
QStringList getIndirectUsePublicMen(QList<ASM_Compiler::LibraryFile> &files,QStringList checkMemNames){
    QStringList indirectUsePublicMenNames;
    foreach(QString checkMemName,checkMemNames){
        ASM_Compiler::BinaryObject obj;
        getPublicMem(files,checkMemName,&obj);

        QStringList thisObjUseMemNames;
        thisObjUseMemNames.append(checkMemName);
        getAllUseMem(obj,thisObjUseMemNames,&thisObjUseMemNames);

        foreach(QString thisBlockName,thisObjUseMemNames){
            ASM_Compiler::BinaryMemBlock *thisBlock = getMemFromObj(obj,thisBlockName);
            if(indirectUsePublicMenNames.contains(thisBlockName))continue;
            if(thisBlock==NULL){
                indirectUsePublicMenNames.append(thisBlockName);
            }else if(thisBlock->isExport){
                indirectUsePublicMenNames.append(thisBlockName);
            }
        }
    }
    return indirectUsePublicMenNames;
}

//过滤去除掉所有没有被任何其它objs所引用标记的且非CHECK的公开内存块以及私有内存块
void filterMemBlock(QList<ASM_Compiler::LibraryFile> &files,LinkSprict &link){
    //已被check的内存块为根进行搜索，搜索出所有被引用了的全局内存块名字
    QStringList allUseBlockNames = link.fixed.keys();

    //获取各个check的内存块,其引用了所属obj的私有内存块后，间接引用的该obj的全局内存块
    QStringList IndirectUseBlockNames = getIndirectUsePublicMen(files,link.fixed.keys());
    foreach(QString tmp,IndirectUseBlockNames){
        if(!allUseBlockNames.contains(tmp)){
            allUseBlockNames.append(tmp);
        }
    }

    //获取各个check的内存块所有引用的全局内存块名
    getAllUsePublicMem(files,allUseBlockNames,&allUseBlockNames);

    //根据名称删除没有被使用到的全局内存块
    for(int i = 0;i<files.length();i++){
        ASM_Compiler::LibraryFile &file = files[i];
        for(int j = 0;j<file.libData.length();j++){
            ASM_Compiler::BinaryObject &obj = file.libData[j];
            for(int k = 0;k<obj.blocks.length();k++){
                if(obj.blocks[k].isExport && !allUseBlockNames.contains(obj.blocks[k].name)){
                    obj.blocks.removeAt(k);
                    k--;
                }
            }
        }
    }

    //已各个obj为单位，删除自身当中未被任何全局块所应用的私有内存块
    for(int i = 0;i<files.length();i++){
        ASM_Compiler::LibraryFile &file = files[i];
        for(int j = 0;j<file.libData.length();j++){

            ASM_Compiler::BinaryObject &obj = file.libData[j];
            removMemBlockInObj(obj);
        }
    }
}

//尝试为内存块分配一块内存
//isAppoint是否指定分配的基址位置(0不指定，自动分配。1指定,appointAdd作为指定的分配地址)
bool tryAllocMem(ASM_Compiler::BinaryMemBlock &block,QList<BlockInfo> &memSpace,bool isAppoint = 0,uint64_t appointAdd = 0){
    if(isAppoint){
        if(!judgeContainBlockSet(memSpace,appointAdd,block)){
            return 0;
        }
        memSpace = removeOccupiedMemBlock(memSpace,appointAdd,block.binLength);
        block.baseAdd = appointAdd;
    }else{
        bool isScueess = searchFreeMemarySpace(memSpace,block,appointAdd);
        if(!isScueess){
            return 0;
        }
        memSpace = removeOccupiedMemBlock(memSpace,appointAdd,block.binLength);
        block.baseAdd = appointAdd;
    }
    return 1;
}

//根据名字获取内存块中定义的一个标记的对象
ASM_Compiler::BinaryMark * getMarkFromMem(ASM_Compiler::BinaryMemBlock &mem,QString markName){
    for(int i = 0;i<mem.marks.length();i++){
        if(mem.marks[i].markName == markName){
            return &mem.marks[i];
        }
    }
    return NULL;
}


//将byteArray中第offset_Byte字节第offset_Bit位开始的(higth-low)+1的位的数据替换为value中higth到low位之间的数据
void setBitDataFromByteArray(QByteArray & byteArray,uint offset_Byte,uint offset_Bit,
                             uint64_t value,uint higth,uint low){
    value <<= LinkCPU_Bits-1-higth;
    uchar tmp[LinkCPU_Bits/8];
    memcpy(tmp,&value,LinkCPU_Bits/8);
    uchar tmp2[8];
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        memreverse(tmp2,tmp,LinkCPU_Bits/8);
    #else
        memcpy(tmp2,tmp,LinkCPU_Bits/8);
    #endif
    QByteArray valueBytes(LinkCPU_Bits/8,0);
    for(int i = 0;i<LinkCPU_Bits/8;i++){
        valueBytes[i] = tmp2[i];
    }
    valueBytes = byteArrayRightShift(valueBytes,offset_Bit);
    uint8_t d = byteArray[offset_Byte];
    d &= ~((1<<(8-offset_Bit))-1);
    d |= valueBytes[0];
    byteArray[offset_Byte] = d;
    int endByteIndex = ((higth-low+offset_Bit)/8);
    d = byteArray[endByteIndex+offset_Byte];
    d &= ((1<<((higth-low+1+offset_Bit)%8))-1);
    d |= valueBytes[endByteIndex];
    byteArray[endByteIndex+offset_Byte] = d;
    for(int i = 1;i<endByteIndex;i++){
        byteArray[i+offset_Byte] = valueBytes[i];
    }
}

//参数: libs所有要链接的静态库文件，link是静态库脚本解析出的信息
ExecutableImage compilerMemBlock(QList<ASM_Compiler::LibraryFile> &files,LinkSprict &link,bool &isSuceess,QStringList &prompts){
    ExecutableImage image;
    //检测各个obj中公开的内存块是否有命名重复,存在则报错退出
    if(judgeHaveLibBlockDupName(files,&prompts)!=0){
        if(isSuceess!=NULL){
            isSuceess = 0;
        }
        return image;
    }
    //检查内存块的定义是否符合要求
    if(checkMemBlockDef(files,link,&prompts)==0){
        isSuceess = 0;
        return image;
    }

    //过滤去除掉所有没有被任何其它objs所引用标记的且非CHECK的公开内存块以及私有内存块
    filterMemBlock(files,link);

    //根据link中的FIXED命令，为指定的内存块分配基地址(只针对公开的内存块)
    bool allocFixedMemIsScueess = 1;
    QList<ASM_Compiler::BinaryMemBlock*> fixedMemPtrList;//用于后续为所有内存块分配基地址时,排除掉由FIXED命令指定基地址的内存块
    foreach(QString fixedBlockName,link.fixed.keys()){
        ASM_Compiler::BinaryMemBlock* block = getPublicMem(files,fixedBlockName);
        if(block!=NULL){
            //尝试为内存块分配内存，如果分配失败就报错退出
            if(!tryAllocMem(*block,link.memSpace,1,link.fixed.value(fixedBlockName))){
                prompts.append("\033[31m错误:\033[0m无法为<"+block->name+">分配内存区域");
                allocFixedMemIsScueess = 0;
            }
            fixedMemPtrList.append(block);
        }
    }
    if(!allocFixedMemIsScueess){
        isSuceess = 0;
        return image;
    }

    //为其它所有内存块自动分配基地址
    for(int i = 0;i<files.length();i++){
        ASM_Compiler::LibraryFile &file = files[i];
        for(int j = 0;j<file.libData.length();j++){
            ASM_Compiler::BinaryObject &obj = file.libData[j];
            for(int k = 0;k<obj.blocks.length();k++){
                ASM_Compiler::BinaryMemBlock *thisBlock = &obj.blocks[k];
                if(fixedMemPtrList.contains(thisBlock))continue;
                if(!tryAllocMem(obj.blocks[k],link.memSpace)){
                    prompts.append("\033[31m错误:\033[0m无法为<"+thisBlock->name+">分配内存区域");
                    allocFixedMemIsScueess = 0;
                }
            }
        }
    }
    if(!allocFixedMemIsScueess){
        isSuceess = 0;
        return image;
    }

    //解析内存块中引用的标记指向的绝对地址,替换掉内存块中使用标记的区域的二进制数据，最后整合所有内存块，生成可执行程序映像
    for(int i = 0;i<files.length();i++){
        ASM_Compiler::LibraryFile &file = files[i];
        for(int j = 0;j<file.libData.length();j++){
            ASM_Compiler::BinaryObject &obj = file.libData[j];
            for(int k = 0;k<obj.blocks.length();k++){
                ASM_Compiler::BinaryMemBlock &block = obj.blocks[k];
                if(block.isZIData && block.isCodeType==DataMem){
                    ExecutableMemBlock exeblock;
                    exeblock.baseAdd = block.baseAdd;
                    exeblock.data = block.bin;
                    exeblock.len = block.binLength;
                    exeblock.isZiData = 1;
                    image.append(exeblock);
                    continue;
                }

                QByteArray tmp = block.bin;
                //解析内存块中引用的标记指向的绝对地址,替换掉内存块中使用标记的区域的二进制数据
                for(int z = 0;z<block.markQuotes.length();z++){
                    ASM_Compiler::BinaryQuoteMark &quote =block.markQuotes[z];
                    //解析内存块中引用的标记指向的绝对地址
                    ASM_Compiler::BinaryMemBlock * markFromBlock;
                    if(quote.isImport){
                        markFromBlock = getPublicMem(files,quote.affiBlockName);
                    }else{
                        markFromBlock = getMemFromObj(obj,quote.affiBlockName);
                    }
                    if(markFromBlock==NULL){
                        qDebug()<<"编译器运行出错:compilerMemBlock.h - 374行代码 BinaryMemBlock * markFromBlock指针为NULL";
                        qDebug()<<quote.affiBlockName;
                        exit(0);
                    }
                    ASM_Compiler::BinaryMark * mark = getMarkFromMem(*markFromBlock,quote.name);
                    quote.value = quote.offset + markFromBlock->baseAdd + mark->affBlockOffset;
                    //替换掉内存块中使用标记的区域的二进制数据
                    setBitDataFromByteArray(block.bin,
                                            quote.order_offset_Byte,
                                            quote.order_offset_Bit,
                                            quote.value,
                                            quote.higth,
                                            quote.low);
                }
                ExecutableMemBlock exeblock;
                exeblock.baseAdd = block.baseAdd;
                exeblock.data = block.bin;
                exeblock.len = block.binLength;
                exeblock.isZiData = 0;
                image.append(exeblock);
            }
        }
    }
    isSuceess = 1;


    return image;
}





}}
QT_END_NAMESPACE
