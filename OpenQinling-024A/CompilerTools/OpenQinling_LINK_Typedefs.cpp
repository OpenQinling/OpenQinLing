#include "OpenQinling_LINK_Typedefs.h"
#include <QStringList>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


QString LinkSprict::toString(){
    QString memSpaceStr = "MemSpace:";
    foreach(BlockInfo info,memSpace){
        QString temp = "["+QString::number(info.baseAddress,10)+","+QString::number(info.sizeBytes+info.baseAddress-1)+"] ";
        memSpaceStr.append(temp);
    }
    QString checkStr = "CheckBlock:"+check.join(",");

    QString fixedStr = "FixedBlock:";
    foreach(QString key,fixed.keys()){
        QString temp = "["+key+","+QString::number(fixed.value(key),10)+"] ";
        fixedStr.append(temp);
    }
    return memSpaceStr+"\r\n"+checkStr+"\r\n"+fixedStr;
}

void BlockInfo::setSupportType(QString typeText){
    if(typeText.isEmpty()){
        supportType.supCODE = 1;
        supportType.supConstDATA = 1;
        supportType.supInitDATA = 1;
        supportType.supZeroInitData = 1;
        return;
    }
    QStringList supportTypeText = typeText.split('+');
    for(int i = 0;i<supportTypeText.length();i++){
        supportTypeText[i] = supportTypeText[i].toUpper();
    }
    supportType.supCODE = supportTypeText.contains("TEXT");         //代码块
    supportType.supConstDATA = supportTypeText.contains("RODATA");  //常量数据块
    supportType.supInitDATA = supportTypeText.contains("DATA");     //初始化变量数据块
    supportType.supZeroInitData = supportTypeText.contains("BSS");  //非初始化变量数据块、堆区、栈区
}


bool BlockInfo::isSupportType(ASM_Compiler::BinaryMemBlock &block){
    if(block.isCodeType){
        return supportType.supCODE;
    }else if(block.isConstData){
        return supportType.supConstDATA;
    }else if(block.isZIData){
        return supportType.supZeroInitData;
    }else{
        return supportType.supInitDATA;
    }
}

bool BlockInfo::SupportType::operator==(SupportType &a){
    return a.supCODE == supCODE &&
           a.supConstDATA == supConstDATA &&
           a.supInitDATA == supInitDATA &&
           a.supZeroInitData == supZeroInitData;
}

bool BlockInfo::SupportType::operator!=(SupportType &a){
    return !(a==*this);
}



}}
QT_END_NAMESPACE
