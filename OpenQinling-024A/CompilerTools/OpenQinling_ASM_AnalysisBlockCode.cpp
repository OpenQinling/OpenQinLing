#include "OpenQinling_ASM_AnalysisBlockCode.h"
#include "OpenQinling_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

//解析源码文件定义的内存块中的指令
bool analysisAsmBlockCode(AsmSrc &asmSrc,QStringList*promptText){
    if(promptText==NULL)return false;
    QList<Prompt> errorPrompt,warnPrompt;
    //errorPrompt会造成程序强制退出。warnPrompt则不会退出编译
    for(int i = 0;i<asmSrc.memBlocks.length();i++){
        QList<uint> lineFeed,sLine;//各指令原先文本的换行数
        QStringList orderStrList = segment(asmSrc.memBlocks[i].blockSrcText,&lineFeed,&sLine);//换行符转为空格，并以';'号分割程序文本
        QList<uint> startLine,endLine;//各指令的起始行数和结束行数

        for(int j = 0;j<orderStrList.length();j++){
            QList<int> line = getBlockPromptLine(j,asmSrc.memBlocks[i].blockStartLine,lineFeed,sLine);
            startLine.append(line[0]);
            endLine.append(line[1]);
        }

        //去除因换行符产生的多余空格
        for(int j = 0;j<orderStrList.length();j++){
            orderStrList[j] = dislodgeSpace(orderStrList[j]);
        }

        //判断最后一个定义的指令是否有分号结尾[如无就报错退出]
        if(orderStrList[orderStrList.length()-1] != ""){
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "指令格式错误";
            prpt.append(asmSrc.path,asmSrc.asmSrcText,startLine[orderStrList.length()-1],endLine[orderStrList.length()-1]);
            errorPrompt.append(prpt);
            continue;
        }else{
            orderStrList.removeLast();
            startLine.removeLast();
            endLine.removeLast();
        }


        for(int j = 0;j<orderStrList.length();j++){
            int formatStatus;

            AsmBlockCode code = formatBlockOrderText(orderStrList[j],&formatStatus);
            code.startLine = startLine[j];
            code.endLine = endLine[j];
            if(formatStatus){
                Prompt prpt;
                prpt.isError = 1;
                if(formatStatus==1){
                    prpt.content = "标记名称为空";
                }else if(formatStatus==2){
                    prpt.content = "标记名中不得存在空格";
                }else if(formatStatus==3){
                    prpt.content = "指令语法格式不正确";
                }
                prpt.append(asmSrc.path,asmSrc.asmSrcText,startLine[j],endLine[j]);
                errorPrompt.append(prpt);
                continue;
            }

            asmSrc.memBlocks[i].codes.append(code);
        }
    }

    foreach(Prompt error,errorPrompt){
        promptText->append(grenratePromptText(error));
    }

    foreach(Prompt warn,warnPrompt){
        promptText->append(grenratePromptText(warn));
    }

    //存在错误的情况
    if(errorPrompt.length()>0){
        return false;
    }
    return true;
}

QList<IndexV2> refreshBlockCodeIndex(AsmMemBlock& block){
    QList<IndexV2> index;
    for(int i = 0;i<block.codes.length();i++){
        for(int j = 0;j<block.codes[i].marks.length();j++){
            index.append({i,j});
        }
    }
    return index;
}
QStringList montageBlockCodeMarksDef(AsmMemBlock& block){
    QStringList memBlockMarksDef;
    for(int i = 0;i<block.codes.length();i++){
        memBlockMarksDef.append(block.codes[i].marks);
    }
    return memBlockMarksDef;
}
//判断是否存在重名定义的标记
bool detectDuplicateDefMark(AsmSrc & asmSrc,QStringList*promptText){
    bool isDuplicate = false;
    //已单个内存块为判断域
    for(int k = 0;k<asmSrc.memBlocks.length();k++){
        AsmMemBlock&thisBlock = asmSrc.memBlocks[k];
        QStringList marks = montageBlockCodeMarksDef(thisBlock);
        QList<IndexV2> index = refreshBlockCodeIndex(thisBlock);
        //index[0]所属的子指令索引号，index[0]子指令中标记的索引号

        QList<Prompt> pormpts;
        for(int i = 0;i<marks.length();i++){
            //排查当前内存块之前的所有内存块，是否有与当前内存块命名重复的。如果有就无需排查了，避免产生重复的报错信息
            bool isHaveRepeat = 0;
            for(int j = 0;j<i;j++){
                if(marks[j]==marks[i]){
                    isHaveRepeat = 1;
                    break;
                }
            }
            if(isHaveRepeat){
                continue;
            }

            QList<int> repeat;
            //从当前标记后一个标记开始排查命名与当前标记命名相同的标记，并将索引号存入repeat
            for(int j = i+1;j<marks.length();j++){
                if(marks[j]==marks[i]){
                    repeat.append(j);
                }
            }

            if(repeat.length()!=0){
                //如果存在重复，发出报错信息
                Prompt prpt;
                prpt.isError = 1;//报错:内存块/标记命名不符合规范

                prpt.content = "There are "+QString::number(repeat.length()+1)+ " label with duplicate names pointing to different addresses";

                prpt.append(asmSrc.path,asmSrc.asmSrcText,thisBlock.codes[index[i].index1].startLine,thisBlock.codes[index[i].index1].endLine);
                for(int j = 0;j<repeat.length();j++){
                    prpt.append(asmSrc.path,asmSrc.asmSrcText,thisBlock.codes[index[repeat[j]].index1].startLine,thisBlock.codes[index[repeat[j]].index1].endLine);
                }
                pormpts.append(prpt);
            }
        }
        if(pormpts.length()>0){
            for(int i = 0;i<pormpts.length();i++){
                QString txt = grenratePromptText(pormpts[i]);
                promptText->append(txt);
            }
            isDuplicate |= true;
        }
    }
    return isDuplicate;
}



}}
QT_END_NAMESPACE
