#include "OpenQinling_ASM_Typedefs.h"

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{


QString AsmImportMark::toName(){
    if(markName!=""){
        return blockName+"."+markName;
    }
    return blockName;
}


QString GlobalOrder::toString(){
    return order+" "+args.join(",")+(subOrder=="" ? "" : "{"+subOrder+"}")+";";
}


QString BlockMarkDef::toName(){
    if(name!=""){
        return affiBlockName+"."+name;
    }
    return affiBlockName;
}

QString MarkQuote::toName(){
    if(name!=""){
        return affiBlockName+"."+name;
    }
    return affiBlockName;
}



void Prompt::append(QString fromFilePath,
            QString srcText,
            uint startline,
            uint endline){
    this->endline.append(endline);
    this->srcText.append(srcText);
    this->startline.append(startline);
    this->fromFilePath.append(fromFilePath);
}



int Prompt::length(){
    return fromFilePath.length();
}

QString Prompt::getPath(int i){
    return fromFilePath[i];
}
uint Prompt::getStartline(int i){
    return startline[i];
}
uint Prompt::getEndline(int i){
    return endline[i];
}
QString Prompt::getSrcText(int i){
    return srcText[i];
}

}}
QT_END_NAMESPACE
