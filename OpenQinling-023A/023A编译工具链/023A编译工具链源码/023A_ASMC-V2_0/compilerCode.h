#ifndef COMPILERCODE_H
#define COMPILERCODE_H
#include "asmc_typedef.h"
#include "transplant.h"
#include "GrammarDetection.h"
#include "compilerData.h"
#pragma execution_character_set("utf-8")



//解析出单条代码定义的指令文本中引用了标记的情况。如果存在，提取出标记，并将标记原处的文本替换为"0"
//标记引用语法结构: [内存块.标记 +/- 偏移值](higth-low)
//status: 0不是一个标记 1成功解析标记 2解析标记失败 3当前数据类型不能定义标记 4标记描述的地址比特数过大 5高低位设置不正确(超过了cpu位数/low>higth)
QList<MarkQuote> analysisCodeBlockMarkQuote(QString orderName,QString &arg,uint argIndex,uint argTotal,uint *status){
    if(status==NULL)return QList<MarkQuote>();

    if(arg.left(1)=="["&&( arg.right(1)=="]" || arg.right(1)==")" )){
        if(!thisCodeOrderIsSupportLabel(orderName,argIndex,argTotal)){
            *status = 3;
            return QList<MarkQuote>();
        }
        QString block_markName;//标记名
        bool offsetTXT_sign = 0;//0为正，1为负
        QString offsetTXT = "0H";//偏移文本
        QString sectTXT_high;//位选文本高位
        QString sectTXT_low;//位选文本低位

        int p = 0;
        for(int j = 1;j<arg.length();j++){
            if(arg.at(j)!="+" && arg.at(j)!="-" && arg.at(j)!="]"&&p==0){
                block_markName.append(arg.at(j));
            }else if((arg.at(j)=="+")&&p==0){
                p++;
                offsetTXT_sign = 0;
                offsetTXT.clear();
            }else if((arg.at(j)=="-")&&p==0){
                p++;
                offsetTXT_sign = 1;
                offsetTXT.clear();
            }else if(arg.at(j)=="]"&&p==0){
                p = 2;
            }else if(arg.at(j)!="]"&&p==1){
                offsetTXT.append(arg.at(j));
            }else if(arg.at(j)=="]"&&p==1){
                p++;
            }else if(arg.at(j)=="("&&p==2){
                sectTXT_high.clear();
                p++;
            }else if(arg.at(j)!=")"&&arg.at(j)!="-" && p==3){
                sectTXT_high.append(arg.at(j));
            }else if(arg.at(j)=="-" && p==3){
                sectTXT_low.clear();
                p++;
            }else if(arg.at(j)!=")"&&p==4){
                sectTXT_low.append(arg.at(j));
            }else if(arg.at(j)==")"&&p==3){
                p++;
            }
        }

        //将标记所描述的内存地址转为立即数后，立即数取内存地址32bit中的哪几位
        uint high = 0;
        uint low = 0;

        int offset = 0;//标记所指向的内存地址的偏移值

        bool higth_suceess,low_suceess,offset_suceess;

        if(sectTXT_high==""&&sectTXT_low==""){
            getMarkDefaultHL(orderName,high,low);
            higth_suceess = 1;
            low_suceess = 1;
        }else{
            high = sectTXT_high.toUInt(&higth_suceess);
            low = sectTXT_low.toUInt(&low_suceess);
        }
        if(!offsetTXT_sign){
            offset = (int)compileUInt(offsetTXT,&offset_suceess);
        }else{
            int p = (int)compileUInt(offsetTXT,&offset_suceess);
            offset = 0-p;
        }



        //解析high low offset失败
        if(!higth_suceess || !low_suceess || !offset_suceess){
            *status = 2;
            return QList<MarkQuote>();
        }
        //higth和low的取值不能大于cpu架构的位数-1
        if(high>CPU_Bits-1 || low>CPU_Bits-1 || low>high){
            *status = 5;
            return QList<MarkQuote>();
        }

        //high和low确定的字节数如果大于数据类型的字节数，报错
        if(high-low+1 > thisCodeOrderSupportMaxBits(orderName)){
            *status = 4;
            return QList<MarkQuote>();
        }

        //解析blockName和markName
        QString blockName;
        QString markName;

        QStringList tmp = block_markName.split('.');

        if(!nameIsConform(tmp[0])){
            *status = 2;
            return QList<MarkQuote>();
        }
        blockName = tmp[0];
        if(tmp.length()==1){
            markName = "";
        }else if(!nameIsConform(tmp[1])){
            *status = 2;
            return QList<MarkQuote>();
        }else{
            markName = tmp[1];
        }


        QList<MarkQuote> marks = markQuoteFromCodeOrder(
                        orderName,
                        low,high,offset,blockName,markName
                    );
        arg = "NULL";
        *status = 1;
        return marks;
    }
    *status = 0;
    return QList<MarkQuote>();
}


//编译数据内存块
bool compileCodeMemBlock(AsmSrc & src,QStringList&importMarksName,QStringList&defineMarksName,QStringList * prompts){
    bool haveComplieError = false;
    for(int k = 0;k<src.memBlocks.length();k++){
        AsmMemBlock & thisMem = src.memBlocks[k];
        if(thisMem.type == DataMem)continue;

        MemBlockBin blockBin;//编译出的块数据

        for(int j = 0;j<thisMem.codes.length();j++){

            AsmBlockCode & thisCode = thisMem.codes[j];

            bool haveAnalysisMarkQuoteError = false;
            QList<MarkQuote> quoteTmp;
            for(int i = 0;i<thisCode.args.length();i++){
                uint status;
                QList<MarkQuote> quote = analysisCodeBlockMarkQuote(thisCode.orderName,thisCode.args[i],i,thisCode.args.length(),&status);
                if(status>=2){
                    //报错退出:解析标记引用失败
                    Prompt prpt;
                    prpt.isError = 1;
                    if(status==2){
                        prpt.content = "标记引用的语法格式错误";
                    }else if(status==3){
                        prpt.content = "当前指令不支持标记引用语法";
                    }else if(status==4){
                        prpt.content = "引用的标记转换为立即数后，其位数过大，不支持当前指令";
                    }else if(status==5){
                        prpt.content = "引用的标记高低位选择不合适";
                    }

                    prpt.append(src.path,src.asmSrcText,thisCode.startLine,thisCode.endLine);
                    prompts->append(grenratePromptText(prpt));
                    haveAnalysisMarkQuoteError |= true;
                    continue;
                }else if(status==0){
                    continue;
                }
                //如果是this,替换为当前内存块名
                for(int z = 0;z<quote.length();z++){
                    if(quote[z].affiBlockName=="this"||quote[z].affiBlockName=="THIS"){
                        quote[z].affiBlockName = thisMem.blockName;
                    }
                }

                //判断该标记是外部引入的还是内存块内定义的
                if(importMarksName.contains(quote[0].toName())){
                    for(int z = 0;z<quote.length();z++){
                        quote[z].isImport = 1;
                    }
                }else if(defineMarksName.contains(quote[0].toName())){
                    for(int z = 0;z<quote.length();z++){
                        quote[z].isImport = 0;
                    }
                }else{
                    //报错退出:使用的标记未定义
                    Prompt prpt;
                    prpt.isError = 1;
                    prpt.content = "引用的标记未定义";
                    prpt.append(src.path,src.asmSrcText,thisCode.startLine,thisCode.endLine);
                    prompts->append(grenratePromptText(prpt));
                    haveAnalysisMarkQuoteError |= true;
                    continue;
                }
                quoteTmp.append(quote);
            }


            //如果已经发生了标记解析错误，则当下指令不再编译。直接尝试编译下一个指令
            if(haveAnalysisMarkQuoteError){
                haveComplieError |= true;
                continue;
            }

            int complieStatus;
            //将数据指令编译为二进制
            QByteArray thisCodeBin = compileCodeAsm(thisCode.orderName,thisCode.args,&complieStatus);
            if(complieStatus!=0){
                //指令编译出错
                Prompt prpt;
                prpt.isError = 1;
                if(complieStatus==1){
                    prpt.content = "使用了指令集中未定义的指令";
                }else if(complieStatus==2){
                    prpt.content = "指令的参数数量不正确";
                }else if(complieStatus==3){
                    prpt.content = "使用了指令集中未定义的寄存器";
                }else if(complieStatus==4){
                    prpt.content = "使用了该参数位上禁止使用的寄存器";
                }else if(complieStatus==5){
                    prpt.content = "指令中立即数的大小超过了该指令允许的范围";
                }else if(complieStatus==6){
                    prpt.content = "在禁止使用立即数的参数位上使用了立即数";
                }
                prpt.append(src.path,src.asmSrcText,thisCode.startLine,thisCode.endLine);
                prompts->append(grenratePromptText(prpt));
                haveComplieError |= true;
                continue;
            }
            //提取指向当前指令的标记
            uint thisCodeBlockOffset = blockBin.bin.length();
            for(int i = 0;i<thisCode.marks.length();i++){
                BlockMarkDef def;
                def.name = thisCode.marks[i];
                def.affiBlockName = thisMem.blockName;
                def.affBlockOffset = thisCodeBlockOffset;
                blockBin.markDefList.append(def);
            }

            //标记引用当中的order_offset_Byte只是相对于单个指令二进制码的偏移字节数，
            //现在加上前面所有指令数据的字节数，得到标记引用order_offset_Byte相对于整个内存块bin数据的偏移字节数
            for(int i = 0;i<quoteTmp.length();i++){
                quoteTmp[i].order_offset_Byte += blockBin.bin.length();
            }


            blockBin.markQuoteList.append(quoteTmp);

            //将编译出的二进制数据存入bin
            blockBin.bin.append(thisCodeBin);
        }
        BlockMarkDef def;
        def.name = "";
        def.affiBlockName = thisMem.blockName;
        def.affBlockOffset = 0;
        blockBin.markDefList.append(def);
        blockBin.binLength = blockBin.bin.length();
        thisMem.bin = blockBin;

    }
    return !haveComplieError;
}





#endif // COMPILERCODE_H
