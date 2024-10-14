#include "OpenQinling_LINK_GrammaticalAnalysis.h"
#include <QStringList>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{
//生成提示文本
QString grenratePromptText(int line,QString lineText,QString promptText,bool isError){
    QString lineNumText = "第"+QString::number(line)+"行";
    QString isErrorText = isError ? "\033[31m错误:\033[0m" : "\033[33m警告:\033[0m";
    return isErrorText+promptText+"\r\n\t"+lineNumText+":\t"+lineText;
}

uint64_t compileUInt(QString order,bool*isSuccess){
    if(isSuccess==NULL)return 0;
    if(order=="NULL"  || order=="null"){
        *isSuccess = 1;
        return 0;
    }
    uint64_t bin = 0;
    bool com_isSuccess = 0;
    if(order.length()>=2){
        QString numSys = order.right(1).toUpper();
        QString num = order.left(order.length()-1);
        if(numSys=="B"){
            bin = num.toULongLong(&com_isSuccess,2);
        }else if(numSys=="O"){
            bin = num.toULongLong(&com_isSuccess,8);
        }else if(numSys=="D"){
            bin = num.toULongLong(&com_isSuccess,10);
        }else if(numSys=="H"){
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

LinkSprict linkGrammaticalAnalysis(QString str,QStringList&prompts,bool&success){
    LinkSprict sprict;
    QString srcStr = str;
    str = removeExpNote(str);//去除注释
    str = dislodgeSpace(str);//去除多余空格
    success = 1;
    //将文本分割为行
    QStringList lins = str.remove('\r').split('\n');
    for(int i = 0;i<lins.length();i++){
        lins[i] =  dislodgeSpace(lins[i]);
        if(lins[i]=="")continue;
        QStringList args = lins[i].split(' ');
        QString order = args[0];
        args.removeAt(0);
        if((order.toUpper()=="SPACE")&&(args.length()==3 || args.length()==2)){
            BlockInfo blockInfo;
            if(args.length()==3){
                blockInfo.setSupportType(args[0]);
                args.removeFirst();
            }else{
                blockInfo.setSupportType();
            }
            bool isSuceess;
            uint64_t baseAdress = compileUInt(args[0],&isSuceess);
            if(!isSuceess){
                prompts.append(grenratePromptText(i,lins[i],"无法解析定义的内存范围基地址",1));
                success = 0;
                continue;
            }
            uint64_t sizeBytes;
            if(args[1].at(0).toUpper()=='L'){
                sizeBytes = compileUInt(args[1].remove(0,1),&isSuceess);
            }else{
                sizeBytes = compileUInt(args[1],&isSuceess)-baseAdress+1;
            }
            if(!isSuceess){
                prompts.append(grenratePromptText(i,lins[i],"无法解析定义的内存范围长度",1));
                success = 0;
                continue;
            }
            if(sizeBytes+baseAdress-1 <baseAdress){
                prompts.append(grenratePromptText(i,lins[i],"定义的内存块尾址在基址之前",1));
                success = 0;
                continue;
            }
            blockInfo.baseAddress = baseAdress;
            blockInfo.sizeBytes = sizeBytes;
            sprict.memSpace.append(blockInfo);
        }else if(order.toUpper()=="CHECK"&&args.length()==1){
            QString memName = args[0];
            if(!sprict.check.contains(memName)){
                sprict.check.append(memName);
            }
        }else if(order.toUpper()=="FIXED"&&args.length()==2){
            QString memName = args[0];
            bool isSuceess;
            uint64_t baseAdress = compileUInt(args[1],&isSuceess);
            if(!isSuceess){
                prompts.append(grenratePromptText(i,lins[i],"无法解析基地址",1));
                success = 0;
                continue;
            }
            sprict.fixed.insert(memName,baseAdress);
        }else{
            //指令格式不正确，报错
            prompts.append(grenratePromptText(i,lins[i],"指令格式不正确",1));
            success = 0;
        }
    }
    sprict.memSpace = integrationOverlapPlace(sprict.memSpace);
    return sprict;
}

}}
QT_END_NAMESPACE
