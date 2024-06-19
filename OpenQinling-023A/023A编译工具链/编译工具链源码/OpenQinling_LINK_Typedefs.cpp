#include "OpenQinling_LINK_Typedefs.h"
#include <QStringList>
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


QString LinkSprict::toString(){
    QString memSpaceStr = "MemSpace:";
    foreach(BlockInfo info,memSpace){
        QString temp = "["+QString::number(info.baseAdress,10)+","+QString::number(info.sizeBytes+info.baseAdress-1)+"] ";
        memSpaceStr.append(temp);
    }
    QString checkDataStr = "CheckDataBlock:"+checkData.join(",");
    QString checkCodeStr = "CheckCodeBlock:"+checkCode.join(",");

    QString fixedStr = "FixedBlock:";
    foreach(QString key,fixed.keys()){
        QString temp = "["+key+","+QString::number(fixed.value(key),10)+"] ";
        fixedStr.append(temp);
    }
    return memSpaceStr+"\r\n"+checkDataStr+"\r\n"+checkCodeStr+"\r\n"+fixedStr;
}



}}
QT_END_NAMESPACE
