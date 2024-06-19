#include "OpenQinling_ASM_BinaryObjectLib.h"
#include "OpenQinling_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

QString BinaryMark::toString(){
    if(markName!=""){
        return affiBlockName+"."+markName+" 相较于所属内存块基址的偏移地址:"+QString::number(affBlockOffset);
    }
    return affiBlockName+" 相较于所属内存块基址的偏移地址:"+QString::number(affBlockOffset);
}
QString BinaryMark::toName(){
    if(markName!=""){
        return affiBlockName+"."+markName;
    }
    return affiBlockName;
}

QString BinaryImportMark::toString(){
    if(markName!=""){
        return affiBlockName+"."+markName;
    }
    return affiBlockName;
}
QString BinaryImportMark::toName(){
    return toString();
}


QString BinaryQuoteMark::toString(){
    QString markName;
    if(name!=""){
        markName =  affiBlockName+"."+name;
    }else{
        markName = affiBlockName;
    }


    QString isImportText = isImport ? "(引入的标记)" : "";

    QString orderOff = QString::number(order_offset_Byte)+"byte-"+QString::number(order_offset_Bit)+"bit";

    QString bitH_L = QString::number(higth)+"-"+QString::number(low);

    QString offsetText = QString::number(offset)+"byte";

    return markName+isImportText+"[标记偏移地址"+offsetText+"]"+"赋值地址高低位:"+bitH_L+" 赋值地址:"+orderOff;
}
QString BinaryQuoteMark::toName(){
    if(name!=""){
        return affiBlockName+"."+name;
    }
    return affiBlockName;
}


QString BinaryMemBlock::toString(){
    QStringList quoteTextList;
    for(int i = 0;i<markQuotes.length();i++){
        quoteTextList+=markQuotes[i].toString();
    }
    QStringList markTextList;
    for(int i = 0;i<marks.length();i++){
        markTextList+=marks[i].toString();
    }
    QString quoteText = markQuotes.length()==0 ? "(引用的标记:空)" : "(引用的标记:\r\n\t"+textRightShift(quoteTextList.join("\r\n"),1)+")";
    QString markText = marks.length()==0 ? "(定义的标记:空)" : "(定义的标记:\r\n\t"+textRightShift(markTextList.join("\r\n"),1)+")";
    QString isExportText = isExport ? "-对外导出" : "";
    QString isConstText = type==DataMem && isConstData ? "-常量" : "";
    QString isZIText = type==DataMem && isZIData ? "-非初始化" : "";
    QString typeText = type ? "指令型" : "数据型";
    return name+"["+typeText+isExportText+isConstText+isZIText+"][二进制码长度:"+QString::number(binLength)+"]"+quoteText+markText;
}

//将BinaryObject转为格式化的二进制数据
QByteArray BinaryObject::toFormatByteArray(){
    QByteArray bin;
    QBuffer buffer(&bin);
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);

    stream<<QString("File type = '023A_ASMC-2_0.BinaryObj'\r\n");//识别码

    stream<<marks_import.length();
    for(int i = 0;i<marks_import.length();i++){
        stream<<marks_import[i].affiBlockName;
        stream<<marks_import[i].markName;
    }
    stream<<blocks.length();
    for(int i = 0;i<blocks.length();i++){
        BinaryMemBlock &thisBlock = blocks[i];
        stream<<blocks[i].name;
        stream<<thisBlock.isExport;
        stream<<thisBlock.isZIData;
        stream<<thisBlock.isConstData;
        stream<<thisBlock.type;
        stream<<thisBlock.binLength;
        stream<<thisBlock.bin;
        stream<<thisBlock.marks.length();
        for(int j = 0;j<thisBlock.marks.length();j++){
            BinaryMark &thisMark = thisBlock.marks[j];
            stream<<thisMark.affiBlockName<<thisMark.markName
                <<thisMark.affBlockOffset;
        }
        stream<<thisBlock.markQuotes.length();
        for(int j = 0;j<thisBlock.markQuotes.length();j++){
            BinaryQuoteMark &thisMark = thisBlock.markQuotes[j];
            stream<<thisMark.affiBlockName<<thisMark.name
                <<thisMark.low<<thisMark.higth
                <<thisMark.isImport<<thisMark.offset
                <<thisMark.order_offset_Byte<<thisMark.order_offset_Bit;
        }
    }
    buffer.close();
    return bin;
}
//从格式化的二进制数据中解析出BinaryObject对象[isSuceess是否解析成功]
BinaryObject BinaryObject::fromFormatByteArray(QByteArray &bin,bool *isSuceess=NULL){
    BinaryObject obj;
    QBuffer buffer(&bin);
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);

    QString identify;
    stream>>identify;
    if(identify!="File type = '023A_ASMC-2_0.BinaryObj'\r\n"){
        if(isSuceess!=NULL)*isSuceess=0;
        return obj;
    }
    int marks_import_length = 0;
    stream>>marks_import_length;
    for(int i = 0;i<marks_import_length;i++){
        BinaryImportMark marks_import;
        stream>>marks_import.affiBlockName;
        stream>>marks_import.markName;
        obj.marks_import.append(marks_import);
    }
    int blocks_data_length = 0;
    stream>>blocks_data_length;
    for(int i = 0;i<blocks_data_length;i++){
        BinaryMemBlock blocks_data;
        stream>>blocks_data.name;
        stream>>blocks_data.isExport;
        stream>>blocks_data.isZIData;
        stream>>blocks_data.isConstData;
        stream>>blocks_data.type;
        stream>>blocks_data.binLength;
        stream>>blocks_data.bin;
        int mark_length = 0;
        stream>>mark_length;
        for(int j = 0;j<mark_length;j++){
            BinaryMark thisMark;
            stream>>thisMark.affiBlockName>>thisMark.markName
                >>thisMark.affBlockOffset;
            blocks_data.marks.append(thisMark);
        }
        int markQuotes_length = 0;
        stream>>markQuotes_length;
        for(int j = 0;j<markQuotes_length;j++){
            BinaryQuoteMark thisMark;
            stream>>thisMark.affiBlockName>>thisMark.name
                >>thisMark.low>>thisMark.higth
                >>thisMark.isImport>>thisMark.offset
                >>thisMark.order_offset_Byte>>thisMark.order_offset_Bit;
            blocks_data.markQuotes.append(thisMark);
        }
        obj.blocks.append(blocks_data);
    }
    buffer.close();
    if(isSuceess!=NULL)*isSuceess=1;
    return obj;
}
//将obj列表转为二进制
QByteArray BinaryObject::objsToFormatByteArray(QList<BinaryObject> objs){
    QByteArray bin;
    QBuffer buffer(&bin);
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);
    stream<<QString("File type = '023A_ASMC-2_0.StaticLibrary'\r\n");//识别码
    stream<<objs.length();
    for(int i = 0;i<objs.length();i++){
        stream<<objs[i].toFormatByteArray();
    }
    buffer.close();
    return bin;
}
//从二进制列表中解析出obj列表
QList<BinaryObject> BinaryObject::objsfromFormatByteArray(QByteArray &bin,bool *isSuceess=NULL){
    QList<BinaryObject> obj;
    QBuffer buffer(&bin);
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);

    QString identify;
    stream>>identify;
    if(identify!="File type = '023A_ASMC-2_0.StaticLibrary'\r\n"){
        if(isSuceess!=NULL)*isSuceess=0;
        return obj;
    }

    int list_length;
    stream>>list_length;
    for(int i = 0;i<list_length;i++){
        QByteArray tmp;
        stream>>tmp;
        bool suc;
        obj.append(fromFormatByteArray(tmp,&suc));
        if(!suc){
            if(isSuceess!=NULL)*isSuceess=0;
            return obj;
        }
    }
    if(isSuceess!=NULL)*isSuceess=1;
    return obj;
}

QString BinaryObject::toString(){
    QStringList marks_import_txt;
    for(int i = 0;i<marks_import.length();i++){
        marks_import_txt.append(marks_import[i].toString());
    }
    QStringList blocksText;
    for(int i = 0;i<blocks.length();i++){
        blocksText.append(blocks[i].toString());
    }
    return "引入标记:{\r\n\t"+textRightShift(marks_import_txt.join("\r\n"),1)+
           "\r\n}\r\n定义的内存块:{\r\n\t"+textRightShift(blocksText.join("\r\n"),1)+"\r\n}";
}

//判断是否有库文件之间存在公开内存块重名
bool judgeHaveLibBlockDupName(QList<LibraryFile> &libfiles,QStringList*prompts){
    QStringList allExportBlocks_fileName;
    QStringList allExportBlocks_BlockName;//key为库名，value是库中所有外部导出内存块的名字

    for(int i = 0;i<libfiles.length();i++){
        QList<BinaryObject> objs = libfiles[i].libData;
        foreach(BinaryObject bin,objs){
            for(int j = 0;j<bin.blocks.length();j++){
                if(bin.blocks[j].isExport){
                    allExportBlocks_fileName.append(libfiles[i].fileName);
                    allExportBlocks_BlockName.append(bin.blocks[j].name);
                }
            }
        }
    }

    QHash<QString,QStringList> dupBlock;//key存储重复的块名，value存储有哪些Lib文件中重复定义了这些块
    for(int i = 0;i<allExportBlocks_fileName.length();i++){
        bool isHaveDep = 0;
        for(int j = 0;j<i;j++){
            if(allExportBlocks_fileName[i]!=allExportBlocks_fileName[j] &&
               allExportBlocks_BlockName[i]==allExportBlocks_BlockName[j]){
                isHaveDep = 1;
            }
        }
        if(isHaveDep)continue;

        QList<uint> depIndex;
        for(int j = i+1;j<allExportBlocks_fileName.length();j++){
            if(allExportBlocks_fileName[i]!=allExportBlocks_fileName[j] &&
               allExportBlocks_BlockName[i]==allExportBlocks_BlockName[j]){
                depIndex.append(j);
            }
        }

        if(depIndex.length()>0){
            QStringList dupFile = {allExportBlocks_fileName[i]};
            foreach(uint index,depIndex){
                dupFile.append(allExportBlocks_fileName[index]);
            }
            dupBlock.insert(allExportBlocks_BlockName[i],dupFile);
        }
    }

    if(prompts!=NULL){
        for(int i = 0;i<dupBlock.keys().length();i++){
            QStringList dupLibsFilesName = dupBlock[dupBlock.keys()[i]];
            for(int i = 0;i<dupLibsFilesName.length();i++){
                dupLibsFilesName[i] = "\""+dupLibsFilesName[i]+"\"";
            }

            QString promptText = "\033[31m错误:\033[0m 在["+dupLibsFilesName.join(',')+"]中重复定义了公开的内存块:"+dupBlock.keys()[i];
            prompts->append(promptText);
        }
    }

    return dupBlock.keys().length();
}



}}
QT_END_NAMESPACE
