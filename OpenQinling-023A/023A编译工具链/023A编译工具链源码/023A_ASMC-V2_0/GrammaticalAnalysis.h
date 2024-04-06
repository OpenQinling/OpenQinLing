#ifndef GRAMMATICALANALYSIS_H
#define GRAMMATICALANALYSIS_H
#include "asmc_typedef.h"
#include "FormatText.h"
#include "DebugTool.h"
#include "GrammarDetection.h"
#include <QDebug>
#include "MacroAnalysis.h"
#include "AnalysisBlockCode.h"
#include "compilerData.h"
#include "compilerCode.h"
#include "BinaryObject_Linrary.h"
#include "filterMemBlock.h"
#pragma execution_character_set("utf-8")
//解析一个源文件的全局指令
AsmSrc SrcGrammaticalAnalysis(QString str,QString srcPath,QStringList*prompts){
    if(prompts==NULL)return AsmSrc();

    QStringList defStr;//用户在程序中定义的字符串表
    QStringList defBigParentStr;//用户定义在大括号中的程序文本内容
    QStringList importBlock;//外部引入的内存块
    QString srcText = str;//源文件文本内容
    QList<Prompt> pormpts;//产生的提示信息
    str = removeExpNote(str);//去除注释

    //检测文本中是否有$号，$号在编译时是重要的临时标志号。严禁在代码中使用，否则会导致编译出错
    for(int i = 0;i<str.length();i++){
        if(str[i]=='$' && !charIsInString(str,i)){
            //如果存在$号，并且不是字符串中定义的$，就报错退出
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "在代码文本中不能使用 '$' 符";
            uint line = getCharInStringLine(str,i);
            prpt.append(srcPath,srcText,line,line);
            pormpts.append(prpt);
        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            prompts->append(pormptsText);
        }
        return AsmSrc();
    }

    bool isWindup;//是否去除成功.如果用户定义了一个字符串。但是字符串只有开始的"号，无结束的"号。就会报错
    int strStartp;

    str = extractString(str,&defStr,&isWindup,&strStartp);//去除用户定义的字符串
    if(!isWindup){
        Prompt prpt;
        prpt.isError = 1;
        prpt.content = "定义的字符串没有结尾的\"限制符";
        prpt.append(srcPath,srcText,getCharInStringLine(srcText,strStartp),getCharInStringLine(srcText,srcText.length()-1));
        QString pormptsText = grenratePromptText(prpt);
        prompts->append(pormptsText);
        return AsmSrc();
    }

    str = dislodgeSpace(str);//去除多余空格
    str = extractParent(str,2,&defBigParentStr);//提取出大括号文本

    QList<uint> lineFeed,startLine;//各全局指令原先文本的换行数
    QStringList orderStrList = segment(str,&lineFeed,&startLine);//换行符转为空格，并以';'号分割程序文本


    //去除因换行符产生的多余空格
    for(int i = 0;i<orderStrList.length();i++){
        orderStrList[i] = dislodgeSpace(orderStrList[i]);
    }


    //判断最后一个定义的全局指令是否有分号结尾[如无就报错退出]
    if(orderStrList[orderStrList.length()-1] != ""){
        Prompt prpt;
        prpt.isError = 1;
        prpt.content = "最后一个全局指令没有定义分号";
        QList<int> line = getPromptLine(orderStrList.length()-1,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
        prpt.append(srcPath,srcText,line[0],line[1]);
        QString pormptsText = grenratePromptText(prpt);
        prompts->append(pormptsText);
        return AsmSrc();
    }else{
        orderStrList.removeLast();
    }


    //判断每一个全局变量中，是否存在了2个以上的大括号子内容[如果存在就报错退出]
    for(int i = 0;i<orderStrList.length();i++){
        if(getFlagQuantity(orderStrList[i],"$12")>=2){
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "全局指令语法格式错误";
            QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
            prpt.append(srcPath,srcText,line[0],line[1]);
            pormpts.append(prpt);
        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            prompts->append(pormptsText);
        }
        return AsmSrc();
    }

    //如果解析全局命令[如果全局命令的定义语法格式有错误就报错退出]
    QList<GlobalOrder> orderlist;//解析出全局命令
    for(int i = 0;i<orderStrList.length();i++){
        bool formatStatue;
        GlobalOrder order = formatOrderText(orderStrList[i],&formatStatue);
        if(formatStatue==0){
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "全局指令语法格式错误";
            QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
            prpt.append(srcPath,srcText,line[0],line[1]);
            pormpts.append(prpt);
        }else{
            orderlist.append(order);
        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            prompts->append(pormptsText);
        }
        return AsmSrc();
    }

    //判断全局指令是否都是存在的指令[如果是未知指令，就报错退出]
    for(int i = 0;i<orderlist.length();i++){
        if(orderlist[i].order!="DATA"&&orderlist[i].order!="data"&&//定义私有数据内存块-指令
           orderlist[i].order!="CODE"&&orderlist[i].order!="code"&&//定义私有指令内存块-指令
           orderlist[i].order!="CODE-EXPORT"&&orderlist[i].order!="code-export"&&//定义导出指令内存块-指令
           orderlist[i].order!="DATA-EXPORT"&&orderlist[i].order!="data-export"&&//定义导出数据内存块-指令
           orderlist[i].order!="DATA-EXPORT-CONST"&&orderlist[i].order!="data-export-const"&&//定义导出常量数据内存块-指令
           orderlist[i].order!="DATA-CONST-EXPORT"&&orderlist[i].order!="data-const-export"&&//定义导出常量数据内存块-指令
           orderlist[i].order!="DATA-CONST"&&orderlist[i].order!="data-const"&&//定义常量数据内存块-指令
           orderlist[i].order!="DATA-EXPORT-NOINIT"&&orderlist[i].order!="data-export-noinit"&&//定义导出未初始化数据内存块-指令
           orderlist[i].order!="DATA-NOINIT-EXPORT"&&orderlist[i].order!="data-noinit-export"&&//定义导出未初始化数据内存块-指令
           orderlist[i].order!="DATA-NOINIT"&&orderlist[i].order!="data-noinit"&&//定义导出未初始化数据内存块-指令
           orderlist[i].order!="IMPORT" && orderlist[i].order!="import"//导入外部的一个内存块/内存块标记-指令
           ){//导入一个头文件的标记-指令
            //2.0汇编编译器未定义的指令类型
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "定义了一个不支持的全局指令."
                           "该汇编编译器仅支持以下全局指令类型:\r\n"
                           "      (DATA CODE DATA-EXPORT CODE-EXPORT IMPORT DATA-EXPORT-CONST DATA-EXPORT-NOINIT DATA-CONST DATA-NOINIT).";
            QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
            prpt.append(srcPath,srcText,line[0],line[1]);
            pormpts.append(prpt);
        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            prompts->append(pormptsText);
        }
        return AsmSrc();
    }

    //判断全局指令的参数格式是否正确[不正确就报错退出]
    for(int i = 0;i<orderlist.length();i++){
        if(orderlist[i].order=="IMPORT" || orderlist[i].order=="import"){
            //IMPORT是导入外部内存块/标记指令
            //参数数量任意个，不允许使用子指令符{}
            //参数值:  内存块名
            //        内存块名.标记名
            //内存块/标记命名要求:0-9、A-Z、a-z、_的字符组成，首字符必须为_或字母。禁止与指令名冲突
            if(orderlist[i].subOrder!=""){
                //定义了子指令，报错退出
                Prompt prpt;
                prpt.isError = 1;
                prpt.content = "IMPORT全局指令不支持子指令块语法";
                QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
                prpt.append(srcPath,srcText,line[0],line[1]);
                pormpts.append(prpt);
            }

            foreach(QString name,orderlist[i].args){//判断引入的内存块/标记名是否有效
                QStringList np = name.split('.');//np[0]为内存块名,np[1]为内存块中标记名
                //引入内存中的一个标记名: import np[0].np[1]
                //仅仅引入内存块: import np[0]                 //注:该方法无法使用内存块中的标记，只是指向内存块起始地址
                bool isError = true;
                if(np.length()==1){
                    //引入内存块
                    isError = !nameIsConform(np[0]);
                }else if(np.length()==2){
                    //引入内存块中标记
                    isError = !(nameIsConform(np[0]) && nameIsConform(np[1]));
                }

                if(isError){
                    Prompt prpt;
                    prpt.isError = 1;//报错:内存块/标记命名不符合规范
                    prpt.content = "引入的标记名不符合规范";
                    QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
                    prpt.append(srcPath,srcText,line[0],line[1]);
                    pormpts.append(prpt);
                }
            }
        }else{
            //DATA/CODE/DATA-EXPORT/CODE-EXPORT是定义内存块指令
            //参数数量1,必须有子指令符{}
            //参数值:  内存块名
            //内存块/标记命名要求:0-9、A-Z、a-z、_的字符组成，首字符必须为_或字母。禁止与指令名重复
            if(orderlist[i].args.length()!=1){
                Prompt prpt;
                prpt.isError = 1;
                prpt.content = "CODE/DATA全局指令仅支持1个参数";
                QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
                prpt.append(srcPath,srcText,line[0],line[1]);
                pormpts.append(prpt);
                continue;
            }

            foreach(QString name,orderlist[i].args){//判断引入的内存块名定义的是否有效
                if(!nameIsConform(name)){
                    Prompt prpt;
                    prpt.isError = 1;//报错:内存块/标记命名不符合规范
                    prpt.content = "内存块名不符合规范";
                    QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
                    prpt.append(srcPath,srcText,line[0],line[1]);
                    pormpts.append(prpt);
                }
            }
        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            prompts->append(pormptsText);
        }
        return AsmSrc();
    }

    //将源文件数据信息整合为AsmSrcText对象
    AsmSrc asmSrcObj;
    asmSrcObj.iseffective = true;
    asmSrcObj.path = srcPath;
    asmSrcObj.asmSrcText = srcText;
    for(int i = 0;i<orderStrList.length();i++){

        if(orderlist[i].order=="IMPORT" || orderlist[i].order=="import"){

            foreach(QString name,orderlist[i].args){
                AsmImportMark mark;
                QStringList tmp = name.split('.');
                mark.blockName = tmp[0];
                if(tmp.length()==1){
                    mark.markName = "";
                }else{
                    mark.markName = tmp[1];
                }

                QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
                mark.startLine = line[0];
                mark.endLine = line[1];
                asmSrcObj.importMarks.append(mark);
            }
        }else{
            AsmMemBlock mem;

            if(orderlist[i].order=="CODE" || orderlist[i].order=="code"){
                mem.type = CodeMem;
                mem.isExport = 0;
                mem.isConstData = 0;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA" || orderlist[i].order=="data"){
                mem.type =  DataMem;
                mem.isExport = 0;
                mem.isConstData = 0;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-CONST" || orderlist[i].order=="data-const"){
                mem.type =  DataMem;
                mem.isExport = 0;
                mem.isConstData = 1;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-NOINIT" || orderlist[i].order=="data-noinit"){
                mem.type =  DataMem;
                mem.isExport = 0;
                mem.isConstData = 0;
                mem.isZIData = 1;
            }else if(orderlist[i].order=="CODE-EXPORT" || orderlist[i].order=="code-export"){
                mem.type = CodeMem;
                mem.isExport = 1;
                mem.isConstData = 0;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-EXPORT" || orderlist[i].order=="data-export"){
                mem.type =  DataMem;
                mem.isExport = 1;
                mem.isConstData = 0;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-EXPORT-CONST" || orderlist[i].order=="data-export-const"){
                mem.type =  DataMem;
                mem.isExport = 1;
                mem.isConstData = 1;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-EXPORT-NOINIT" || orderlist[i].order=="data-export-noinit"){
                mem.type =  DataMem;
                mem.isExport = 1;
                mem.isConstData = 0;
                mem.isZIData = 1;
            }else if(orderlist[i].order=="DATA-CONST-EXPORT" || orderlist[i].order=="data-const-export"){
                mem.type =  DataMem;
                mem.isExport = 1;
                mem.isConstData = 1;
                mem.isZIData = 0;
            }else if(orderlist[i].order=="DATA-NOINIT-EXPORT" || orderlist[i].order=="data-noinit-export"){
                mem.type =  DataMem;
                mem.isExport = 1;
                mem.isConstData = 0;
                mem.isZIData = 1;
            }
            mem.blockName = orderlist[i].args[0];

            QStringList tmp = separateFlag(orderStrList[i]);
            QStringList tmp2;
            foreach(QString str,tmp){
                if(textIsFlag(str)){
                    tmp2.append(str);
                    break;
                }
            }
            mem.blockSrcText = getStringFromFlag(tmp2[0],"$12",defBigParentStr);
            mem.blockSrcText = mem.blockSrcText.mid(1,mem.blockSrcText.length()-2);
            QList<int> line = getPromptLine(i,srcText,orderStrList,lineFeed,startLine,defBigParentStr);
            mem.startLine = line[0];
            mem.endLine = line[1];
            mem.blockStartLine = line[2];
            asmSrcObj.memBlocks.append(mem);
        }
    }

    asmSrcObj.strBank = defStr;
    for(int i = 0;i<asmSrcObj.strBank.length();i++){
        asmSrcObj.strBank[i] = asmSrcObj.strBank[i].mid(1,asmSrcObj.strBank[i].length()-2);
    }
    return asmSrcObj;
}

//将各个源文件中定义的内存块降维成线性表
QList<AsmMemBlock> montageBlock(QList<AsmSrc>& src){
    QList<AsmMemBlock> memBlockList;
    for(int i = 0;i<src.length();i++){
        memBlockList.append(src[i].memBlocks);
    }
    return memBlockList;
}
//将各个源文件中定义的IMPORT降维成线性表
QList<AsmImportMark> montageMark(QList<AsmSrc>& src){
    QList<AsmImportMark> memBlockList;
    for(int i = 0;i<src.length();i++){
        memBlockList.append(src[i].importMarks);
    }
    return memBlockList;
}

//内存块/IMPORT降维为1维后会丢失在索引信息。以下两个函数生成索引信息
QList<IndexV2> refreshMarkIndex(QList<AsmSrc>& src){
    QList<IndexV2> index;
    for(int i = 0;i<src.length();i++){
        for(int j = 0;j<src[i].importMarks.length();j++){
            index.append({i,j});
        }
    }
    return index;
}
QList<IndexV2> refreshBlockIndex(QList<AsmSrc>& src){
    QList<IndexV2> index;
    for(int i = 0;i<src.length();i++){
        for(int j = 0;j<src[i].memBlocks.length();j++){
            index.append({i,j});
        }
    }
    return index;
}

//检测所有的内存块名称是否有重复的[如果存在返回1]
bool detectDuplicateNames(QList<AsmSrc> &asmSrcList,QStringList*promptText){
    QList<AsmMemBlock> memBlockList = montageBlock(asmSrcList);
    QList<IndexV2> index = refreshBlockIndex(asmSrcList);

    QList<Prompt> pormpts;
    //判断定义的内存块间是否重名[重名就报错退出]
    for(int i = 0;i<memBlockList.length();i++){
        //排查当前内存块之前的所有内存块，是否有与当前内存块命名重复的。如果有就无需排查了，避免产生重复的报错信息
        bool isHaveRepeat = 0;
        for(int j = 0;j<i;j++){
            if(memBlockList[j].blockName==memBlockList[i].blockName){
                isHaveRepeat = 1;
            }
        }
        if(isHaveRepeat){
            continue;
        }

        QList<int> repeat;
        //从当前内存块后一个内存块开始排查命名与当前内存块命名相同的内存块，并将索引号存入repeat
        for(int j = i+1;j<memBlockList.length();j++){
            if(memBlockList[j].blockName==memBlockList[i].blockName){
                repeat.append(j);
            }
        }


        if(repeat.length()!=0){
            //如果存在重复，发出报错信息
            Prompt prpt;
            prpt.isError = 1;//报错:内存块/标记命名不符合规范

            prpt.content = "存在"+QString::number(repeat.length()+1)+ "个内存块的命名重复";

            prpt.append(asmSrcList[index.at(i).index1].path,asmSrcList[index.at(i).index1].asmSrcText,memBlockList[i].startLine,memBlockList[i].endLine);
            for(int j = 0;j<repeat.length();j++){
                prpt.append(asmSrcList[index.at(repeat[j]).index1].path,asmSrcList[index.at(repeat[j]).index1].asmSrcText,memBlockList[repeat[j]].startLine,memBlockList[repeat[j]].endLine);
            }
            pormpts.append(prpt);

        }
    }
    if(pormpts.length()>0){
        for(int i = 0;i<pormpts.length();i++){
            QString txt = grenratePromptText(pormpts[i]);
            promptText->append(txt);
        }
        return true;
    }
    return false;
}

//检测并删除存在IMPORT语句重复引入的内存块/标记[如果存在，仅发出警告]
void detectDuplicateImport(QList<AsmSrc> &asmSrcList,QStringList*promptText){
    QList<AsmSrc> newSrc = asmSrcList;

    QList<AsmImportMark> memMarkList = montageMark(newSrc);
    QList<AsmMemBlock> memBlockList = montageBlock(newSrc);

    QList<IndexV2> mark_index = refreshMarkIndex(newSrc);
    QList<IndexV2> block_index = refreshBlockIndex(newSrc);

    QList<uint> removeIndex;//要删除的引入标记的索引号
    //判断IMPORT指令引入的内存块，是否在当前源文件中就已经定义了[有就发出警告信息,并删除该IMPORT]
    for(int j = 0;j<memMarkList.length();j++){
        QList<int> alreadyExists;//该源文件中是否已经存在IMPORT指令要引入的内存块

        for(int i = 0;i<memBlockList.length();i++){
            if(memBlockList[i].blockName == memMarkList[j].blockName){
                alreadyExists.append(i);
            }
        }

        if(alreadyExists.length()!=0){
            //生成提示信息
            Prompt prpt;
            prpt.isError = 0;//发出警告信息，错误并不致命
            prpt.content = "引入的该标记已经在当前源文件中定义了";
            prpt.append(newSrc[mark_index.at(j).index1].path,newSrc[mark_index.at(j).index1].asmSrcText,memMarkList[j].startLine,memMarkList[j].endLine);
            for(int i = 0;i<alreadyExists.length();i++){
                prpt.append(newSrc[block_index.at(alreadyExists[i]).index1].path,newSrc[block_index.at(alreadyExists[i]).index1].asmSrcText,memBlockList[alreadyExists[i]].startLine,memBlockList[alreadyExists[i]].endLine);
            }

            promptText->append(grenratePromptText(prpt));

            //删除该IMPORT
            newSrc[mark_index.at(j).index1].importMarks.removeAt(mark_index.at(j).index2);
            mark_index = refreshMarkIndex(newSrc);
            memMarkList.removeAt(j);
            j--;//因为memMarkList列表去掉了1个元素，for累加索引号减1
        }
    }
    memBlockList.clear();




    //判断各个IMPORT指令中是否存在重复导入的内存块/标记点
    for(int j = 0;j<memMarkList.length();j++){
        bool repeat = false;
        for(int i = j+1;i<memMarkList.length();i++){
            if(memMarkList[j].toName() == memMarkList[i].toName()){
                repeat = 1;
                break;
            }
        }

        if(repeat){

            //删除该IMPORT
            newSrc[mark_index.at(j).index1].importMarks.removeAt(mark_index.at(j).index2);
            mark_index = refreshMarkIndex(newSrc);
            memMarkList.removeAt(j);
            j--;//因为memMarkList列表去掉了1个元素，for累加索引号减1
        }
    }

}


//提取出所有源文件的导入标记名称
QStringList extractAllImportLabel(QList<AsmSrc> &asmSrcList){
    QStringList marks;
    for(int i = 0;i<asmSrcList.length();i++){
        for(int j = 0;j<asmSrcList[i].importMarks.length();j++){
             marks.append(asmSrcList[i].importMarks[j].toName());
        }
    }
    return marks;
}
//提取出所有源文件内存块中定义的标记名称
QStringList extractAllDefineLabel(QList<AsmSrc> &asmSrcList){
    QStringList marks;

    for(int i = 0;i<asmSrcList.length();i++){
        AsmSrc & thisSrc = asmSrcList[i];
        for(int j = 0;j<thisSrc.memBlocks.length();j++){
            AsmMemBlock & thisBlock = thisSrc.memBlocks[j];
            marks.append(thisBlock.blockName);
            for(int k = 0;k<thisBlock.codes.length();k++){
                AsmBlockCode & thisCode = thisBlock.codes[k];
                for(int z = 0;z<thisCode.marks.length();z++){
                    marks.append(thisBlock.blockName + "." + thisCode.marks[z]);
                }
            }
        }
    }

    return marks;
}

//过滤掉无用的外部引入标记
void filtrImportMark(BinaryObject &obj){
    for(int i = 0;i<obj.marks_import.length();i++){
        bool isUser = 0;
        foreach(BinaryMemBlock block,obj.blocks){
            foreach(BinaryQuoteMark userMark,block.markQuotes){
                if(userMark.toName()==obj.marks_import[i].toName()){
                    isUser = 1;
                    goto pk;
                }
            }
        }
        pk:

        if(!isUser){
            obj.marks_import.removeAt(i);
            i--;
        }
    }
}
//生成BinaryObject对象
BinaryObject generateBinaryObj(QList<AsmSrc> &src){
    BinaryObject obj;
    for(int k = 0;k<src.length();k++){
        AsmSrc &thisSrc = src[k];
        //提取出IMPORT标记
        for(int i = 0;i<thisSrc.importMarks.length();i++){
            BinaryImportMark mark;
            mark.markName = thisSrc.importMarks[i].markName;
            mark.affiBlockName = thisSrc.importMarks[i].blockName;
            obj.marks_import.append(mark);
        }

        //提取出数据/指令内存块
        for(int i = 0;i<thisSrc.memBlocks.length();i++){
            BinaryMemBlock block;

            block.name = thisSrc.memBlocks[i].blockName;
            block.isExport = thisSrc.memBlocks[i].isExport;
            block.isZIData = thisSrc.memBlocks[i].isZIData;
            block.isConstData = thisSrc.memBlocks[i].isConstData;
            block.type = thisSrc.memBlocks[i].type;
            block.binLength = thisSrc.memBlocks[i].bin.binLength;
            block.bin = thisSrc.memBlocks[i].bin.bin;
            for(int j = 0;j<thisSrc.memBlocks[i].bin.markQuoteList.length();j++){
                MarkQuote &thisQuote = thisSrc.memBlocks[i].bin.markQuoteList[j];
                BinaryQuoteMark mark;
                mark.low = thisQuote.low;
                mark.higth = thisQuote.higth;
                mark.offset = thisQuote.offset;
                mark.isImport = thisQuote.isImport;
                mark.affiBlockName = thisQuote.affiBlockName;
                mark.name = thisQuote.name;
                mark.order_offset_Bit = thisQuote.order_offset_Bit;
                mark.order_offset_Byte = thisQuote.order_offset_Byte;
                block.markQuotes.append(mark);
            }
            //提取出定义的标记
            for(int j = 0;j<thisSrc.memBlocks[i].bin.markDefList.length();j++){
                BlockMarkDef &thisMark = thisSrc.memBlocks[i].bin.markDefList[j];
                BinaryMark mark;
                mark.markName = thisMark.name;
                mark.affiBlockName = thisMark.affiBlockName;
                mark.affBlockOffset = thisMark.affBlockOffset;
                block.marks.append(mark);
            }

            obj.blocks.append(block);
        }
    }
    //过滤掉不需要的外部引入标记
    filtrImportMark(obj);
    return obj;
}

BinaryObject complieAsmSrc(QString srcText,QString srcPath,bool * isSuceess,QStringList*prompts){
    if(prompts==NULL){
        //对传入参数做出限制: 要编译的源码文本不得为空  必须要传入返回提示信息的指针
        return BinaryObject();
    }
    if(srcText=="" || prompts==NULL){
        //对传入参数做出限制: 要编译的源码文本不得为空  必须要传入返回提示信息的指针
        prompts->append("\033[31m错误:\033[0m源码文件不是文本类型");
        return BinaryObject();
    }
    //已经引入了的文件路径
    bool analysisMacroSrceess;
    QList<QFileInfo> includePaths = {srcPath};
    //解析宏定义:通过@INCLUDE宏会包含进来多个源\头文件
    QList<AsmSrc> asmSrcList = analysisMacro(srcText,QFileInfo(srcPath),&analysisMacroSrceess,prompts,&includePaths);
    if(!analysisMacroSrceess){
        //解析宏定义失败，退出编译器程序
        if(isSuceess!=NULL)*isSuceess=0;
        return BinaryObject();
    }
    //解析各个源\头文件全局指令
    for(int i = 0;i<asmSrcList.length();i++){
        asmSrcList[i] = SrcGrammaticalAnalysis(asmSrcList[i].asmSrcText,asmSrcList[i].path,prompts);
    }
    for(int i = 0;i<asmSrcList.length();i++){
        if(asmSrcList[i].iseffective==false){
            //解析源文件全局指令失败，退出编译器程序
            if(isSuceess!=NULL)*isSuceess=0;
            return BinaryObject();
        }
    }
    bool isHaveBlockRepeat = detectDuplicateNames(asmSrcList,prompts);//检测所有定义的内存块是否有重复的名称
    detectDuplicateImport(asmSrcList,prompts);//去除重复引入的

    if(isHaveBlockRepeat){
        //内存块重复定义，报错退出
        if(isSuceess!=NULL)*isSuceess=0;
        return BinaryObject();
    }

    bool analysisBlockSuceess = true;
    //解析子指令
    for(int i = 0;i<asmSrcList.length();i++){
        analysisBlockSuceess &= analysisAsmBlockCode(asmSrcList[i],prompts);
    }
    if(analysisBlockSuceess==false){
        //解析内存块内子指令失败，退出
        if(isSuceess!=NULL)*isSuceess=0;
        return BinaryObject();
    }
    //判断各个内存块中定义的标记是否存在重复定义
    bool isMarkBlockRepeat = false;
    for(int i = 0;i<asmSrcList.length();i++){
        isMarkBlockRepeat |= detectDuplicateDefMark(asmSrcList[i],prompts);
    }
    if(isMarkBlockRepeat){
        //存在指向同一个地址的多个标记，退出
        if(isSuceess!=NULL)*isSuceess=0;
        return BinaryObject();
    }

    //提取出当前所有源文件定义或引入的标记
    QStringList allImportMarks = extractAllImportLabel(asmSrcList);//所有引入的标记
    QStringList allDefineMarks = extractAllDefineLabel(asmSrcList);//所有定义的标记

    //编译数据内存块
    bool dataBlockIsSuceess = true;
    for(int i = 0;i<asmSrcList.length();i++){
        dataBlockIsSuceess &= compileDataMemBlock(asmSrcList[i],allImportMarks,allDefineMarks,prompts);
    }

    //编译指令内存块
    bool codeBlockIsSuceess = true;
    for(int i = 0;i<asmSrcList.length();i++){
        codeBlockIsSuceess &= compileCodeMemBlock(asmSrcList[i],allImportMarks,allDefineMarks,prompts);
    }
    if(codeBlockIsSuceess==false || dataBlockIsSuceess==false){
        //指令内存块解析错误
        if(isSuceess!=NULL)*isSuceess=0;
        return BinaryObject();
    }

    //过滤掉不需要的内存块
    filtrMemBlock(asmSrcList);

    //整理编译出的指令/数据内存块数据
    if(isSuceess!=NULL)*isSuceess=1;
    return generateBinaryObj(asmSrcList);
}

#endif // GRAMMATICALANALYSIS_H
