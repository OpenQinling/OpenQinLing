#ifndef COMPILERDATA_H
#define COMPILERDATA_H
#include <FormatText.h>
#include <QString>
#include "asmc_typedef.h"
#include "transplant.h"
#include "GrammarDetection.h"
#include "DebugTool.h"
#pragma execution_character_set("utf-8")
//编译数据
//编译无符号整形
unsigned long long compileUInt(QString order,bool*isSuccess){
    if(isSuccess==NULL)return 0;
    if(order=="NULL"  || order=="null"){
        *isSuccess = 1;
        return 0;
    }
    unsigned long long bin = 0;
    bool com_isSuccess = 0;

    if(order.length()>=2){
        QString numSys = order.right(1);
        QString num = order.left(order.length()-1);
        if(numSys=="B"||numSys=="b"){
            bin = num.toULongLong(&com_isSuccess,2);
        }else if(numSys=="O"||numSys=="o"){
            bin = num.toULongLong(&com_isSuccess,8);
        }else if(numSys=="D"||numSys=="d"){
            bin = num.toULongLong(&com_isSuccess,10);
        }else if(numSys=="H"||numSys=="h"){
            bin = num.toULongLong(&com_isSuccess,16);
        }else{
            *isSuccess = 0;
            return 0;
        }
    }else{
        *isSuccess = 0;
        return 0;
    }
    *isSuccess = com_isSuccess;
    return bin;
}

//编译有符号整形
long long compileInt(QString order,bool*isSuccess){
    if(isSuccess==NULL)return 0;
    if(order=="NULL" || order=="null"){
        *isSuccess = 1;
        return 0;
    }
    if(order.length()==0){
        *isSuccess = 0;
        return 0;
    }
    bool com_isSuccess = 0;
    long long bin = order.toLongLong(&com_isSuccess,10);
    *isSuccess = com_isSuccess;
    return bin;
}



#define BYTE_COM 1
#define UBYTE_COM 2
#define SHORT_COM 3
#define USHORT_COM 4
#define INT_COM 5
#define UINT_COM 6
#define LONG_COM 7
#define ULONG_COM 8
#define FLOAT_COM 9
#define DOUBLE_COM 10
#define STRING_COM 11
#define ARRAY_COM 12
#define UNKOWN_COM 0

//单条数据指令编译【isSuccess:0编译成功 1未知数据类型 2数据初始值格式不正确 3数值型数据立即数范围不正确 4参数数量不正确】
QByteArray compileDataAsm(bool isZIdata,uint order_type,QStringList &args,QStringList &strBank,uint* binLength,uint* isSuccess){
    if(isSuccess==NULL || binLength==NULL)return QByteArray();
    *isSuccess=0;
    *binLength = 0;
    if(isZIdata==1 && order_type!=ARRAY_COM){
        return QByteArray();
    }

    if(order_type==BYTE_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            long long tmp = compileInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp<-127 || tmp>127){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(QByteArray(1,(char)tmp));
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==UBYTE_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp>255){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(QByteArray(1,(uchar)tmp));
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==SHORT_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            long long tmp = compileInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp<-32767 || tmp>32767){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(WORD((short)tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==USHORT_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp>65535){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(WORD((ushort)tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==INT_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            long long tmp = compileInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp<-2147483647 || tmp>2147483647){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(DWORD((int)tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==UINT_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            else if(tmp>4294967295){
                *isSuccess=3;
                return QByteArray();
            }
            bin.append(DWORD((uint)tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==LONG_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            long long tmp = compileInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            bin.append(QWORD(tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==ULONG_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            bin.append(QWORD(tmp).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==FLOAT_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            float data = thisData.toFloat(&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            bin.append(DWORD(data).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==DOUBLE_COM){
        QByteArray bin;
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            double data = thisData.toDouble(&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            bin.append(QWORD(data).toByteArray());
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==STRING_COM){
        QByteArray bin;
        //已utf8格式编译字符串[str默认为utf8格式]
        foreach(QString thisData,args){
            bool cmpSuccess = 0;
            QString str = getStringFromFlag(thisData,"$01",strBank,&cmpSuccess);
            if(cmpSuccess==0){
                *isSuccess=2;
                return QByteArray();
            }
            bin.append(replaceTransChar(str));
        }
        *binLength = bin.length();
        return bin;
    }else if(order_type==ARRAY_COM){
        //数组(默认初始值0xCC)
        if(args.length()!=2){
            *isSuccess=4;
            return QByteArray();
        }
        bool size_isSrc = 0,len_isSrc = 0;
        uint size = compileUInt(args.at(0),&size_isSrc);
        uint len = compileUInt(args.at(1),&len_isSrc);
        if(size*len == 0 || size_isSrc==0 || len_isSrc==0){
            if(size*len == 0){
                *isSuccess=3;
            }else{
                *isSuccess=2;
            }
            return QByteArray();
        }
        *binLength = size*len;
        if(isZIdata==1){
            return QByteArray();
        }
        return QByteArray(*binLength,0xcc);
    }
    *isSuccess=0;
    *binLength = 0;
    return QByteArray();
}

//解析出单条数据定义的指令文本中引用了标记的情况。如果存在，提取出标记，并将标记原处的文本替换为"0"
//标记引用语法结构: [内存块.标记 +/- 偏移值](higth-low)
//status: 0不是一个标记 1成功解析标记 2解析标记失败 3当前数据类型不能定义标记 4标记描述的地址比特数过大 5高低位设置不正确(超过了cpu位数/low>higth)
MarkQuote analysisDataBlockMarkQuote(uint order_type,QString &arg,uint argIndex,uint *status){
    if(status==NULL)return MarkQuote();

    if(arg.left(1)=="["&&( arg.right(1)=="]" || arg.right(1)==")" )){
        if(order_type==DOUBLE_COM ||
                order_type==FLOAT_COM ||
                order_type==STRING_COM ||
                order_type==ARRAY_COM){
            *status = 3;
            return MarkQuote();
        }
        QString block_markName;//标记名
        bool offsetTXT_sign = 0;//0为正，1为负
        QString offsetTXT = "0H";//偏移文本
        QString sectTXT_high = QString::number(CPU_Bits-1);//位选文本高位
        QString sectTXT_low = "0";//位选文本低位

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
        int order_offset_Byte = 0;//在当前指令中，标记解析出的立即数在哪一字节开始
        int order_offset_Bit = 0;//在当前指令中，，标记解析出的立即数在哪一字节的第几比特开始

        bool higth_suceess,low_suceess,offset_suceess;

        high = sectTXT_high.toUInt(&higth_suceess);
        low = sectTXT_low.toUInt(&low_suceess);
        if(!offsetTXT_sign){
            offset = (int)compileUInt(offsetTXT,&offset_suceess);
        }else{
            int p = (int)compileUInt(offsetTXT,&offset_suceess);
            offset = 0-p;
        }


        //解析high low offset失败
        if(!higth_suceess || !low_suceess || !offset_suceess){
            *status = 2;
            return MarkQuote();
        }
        //higth和low的取值不能大于cpu架构的位数-1
        if(high>CPU_Bits-1 || low>CPU_Bits-1 || low>high){
            *status = 5;
            return MarkQuote();
        }

        //high和low确定的字节数如果大于数据类型的字节数，报错
        if(((order_type==BYTE_COM || order_type==UBYTE_COM)&&(high-low+1 >8)) ||
            ((order_type==SHORT_COM || order_type==USHORT_COM)&&(high-low+1 >16)) ||
            ((order_type==UINT_COM || order_type==INT_COM)&&(high-low+1 >32)) ||
            ((order_type==ULONG_COM || order_type==LONG_COM)&&(high-low+1 >64))){
            *status = 4;
            return MarkQuote();
        }

        //解析地址引用在编译为二进制后。应当在当前指令二进制值的第几字节的第几比特开始
        if(order_type==BYTE_COM || order_type==UBYTE_COM){
            order_offset_Byte = argIndex;
            order_offset_Bit = 7-(high-low);
        }else if(order_type==SHORT_COM || order_type==USHORT_COM){
            uint offbit = 15-(high-low);
            order_offset_Byte = offbit/8 + argIndex*2;
            order_offset_Bit = offbit%8;
        }else if(order_type==UINT_COM || order_type==INT_COM){
            uint offbit = 31-(high-low);
            order_offset_Byte = offbit/8 + argIndex*4;
            order_offset_Bit = offbit%8;
        }else if(order_type==ULONG_COM || order_type==LONG_COM){
            uint offbit = 63-(high-low);
            order_offset_Byte = offbit/8 + argIndex*8;
            order_offset_Bit = offbit%8;
        }

        //解析blockName和markName
        QString blockName;
        QString markName;

        QStringList tmp = block_markName.split('.');

        if(!nameIsConform(tmp[0])){
            *status = 2;
            return MarkQuote();
        }
        blockName = tmp[0];
        if(tmp.length()==1){
            markName = "";
        }else if(!nameIsConform(tmp[1])){
            *status = 2;
            return MarkQuote();
        }else{
            markName = tmp[1];
        }


        MarkQuote mark;
        mark.low =low;
        mark.higth = high;
        mark.offset = offset;
        mark.affiBlockName = blockName;
        mark.name = markName;
        mark.order_offset_Byte = order_offset_Byte;
        mark.order_offset_Bit = order_offset_Bit;
        arg = "NULL";
        *status = 1;
        return mark;
    }
    *status = 0;
    return MarkQuote();
}

//解析当前的数据指令类型
uint analysisDataAsmType(QString name){
    if(name=="UINT"||name=="uint"){
        return UINT_COM;
    }else if(name=="INT"||name=="int"){
        return INT_COM;
    }else if(name=="FLOAT"||name=="float"){
        return FLOAT_COM;
    }else if(name=="SHORT"||name=="short"){
        return SHORT_COM;
    }else if(name=="USHORT"||name=="ushort"){
        return USHORT_COM;
    }else if(name=="BYTE"||name=="byte"){
        return BYTE_COM;
    }else if(name=="UBYTE"||name=="ubyte"){
        return UBYTE_COM;
    }else if(name=="LONG"||name=="long"){
        return LONG_COM;
    }else if(name=="ULONG"||name=="ulong"){
        return ULONG_COM;
    }else if(name=="DOUBLE"||name=="double"){
        return DOUBLE_COM;
    }else if(name=="STR"||name=="str"||name=="STR(UTF8)"||name=="str(utf8)"){
        return STRING_COM;
    }else if(name=="ARR"||name=="arr"){
        return ARRAY_COM;
    }
    return UNKOWN_COM;
}



//编译数据内存块
bool compileDataMemBlock(AsmSrc & src,QStringList&importMarksName,QStringList&defineMarksName,QStringList * prompts){
    bool haveComplieError = false;
    for(int k = 0;k<src.memBlocks.length();k++){
        AsmMemBlock & thisMem = src.memBlocks[k];
        if(thisMem.type == CodeMem)continue;

        MemBlockBin blockBin;//编译出的块数据
        blockBin.binLength = 0;
        for(int j = 0;j<thisMem.codes.length();j++){
            AsmBlockCode & thisCode = thisMem.codes[j];
            uint order_type = analysisDataAsmType(thisCode.orderName);//解析当前数据指令的类型
            if(order_type!=12 && thisMem.isZIData){
                //报错退出:非初始化内存块仅支持定义数组类型数据
                Prompt prpt;
                prpt.isError = 1;
                prpt.content = "不初始化内存块仅支持定义数组";
                prpt.append(src.path,src.asmSrcText,thisCode.startLine,thisCode.endLine);
                prompts->append(grenratePromptText(prpt));
                haveComplieError |= true;
                continue;
            }
            QList<MarkQuote> quoteTmp;
            bool haveAnalysisMarkQuoteError = false;
            for(int i = 0;i<thisCode.args.length();i++){
                uint status;
                MarkQuote quote = analysisDataBlockMarkQuote(order_type,thisCode.args[i],i,&status);
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
                if(quote.affiBlockName=="this"||quote.affiBlockName=="THIS"){
                    quote.affiBlockName = thisMem.blockName;
                }
                //判断该标记是外部引入的还是内存块内定义的
                if(importMarksName.contains(quote.toName())){
                    quote.isImport = 1;
                }else if(defineMarksName.contains(quote.toName())){
                    quote.isImport = 0;
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
            uint complieStatus;
            uint binLength;//生成的二进制数据字节数
            //将数据指令编译为二进制数据(如果是非初始化数据块，则为空)
            QByteArray thisCodeBin = compileDataAsm(thisMem.isZIData,order_type,thisCode.args,src.strBank,&binLength,&complieStatus);

            if(complieStatus!=0){
                //指令编译出错
                Prompt prpt;
                prpt.isError = 1;
                if(complieStatus==1){
                    prpt.content = "未知的指令类型";
                }else if(complieStatus==2){
                    prpt.content = "数据的初始值设置格式不正确";
                }else if(complieStatus==3){
                    prpt.content = "设置的立即数大小不合适";
                }else if(complieStatus==4){
                    prpt.content = "指令的参数数量不正确";
                }
                prpt.append(src.path,src.asmSrcText,thisCode.startLine,thisCode.endLine);
                prompts->append(grenratePromptText(prpt));
                haveComplieError |= true;
                continue;
            }

            //提取出当前内存块定义的所有标记
            uint thisCodeBlockOffset = blockBin.binLength;
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
            blockBin.binLength += binLength;
            if(!thisMem.isZIData){
                blockBin.bin.append(thisCodeBin);
            }
        }
        BlockMarkDef def;
        def.name = "";
        def.affiBlockName = thisMem.blockName;
        def.affBlockOffset = 0;
        blockBin.markDefList.append(def);
        thisMem.bin = blockBin;
    }
    return !haveComplieError;
}


#endif // COMPILERDATA_H
