#ifndef GRAMMARTICALANALYSIS_H
#define GRAMMARTICALANALYSIS_H
#include <QStringList>
#include <QDebug>
#include <Middle_typedef.h>
#include <analysisFunction.h>
#include <constValueAnalysis.h>

//全局变量定义:
/*
 * 普通变量:
 *  VAR(变量类型,变量名,变量初始值);
 *
 * 数组变量:
 *  ARRAY(数组名,数组字节数);
 *  ARRAY(数组名).BODY{
 *      VAR(数组中初始化数据类型,数据值);
 *      ARRAY(数组中不初始化字节数);
 *  };
 *
 *
 *
 *
 *
 *
 *
 *
 */


//解析全局变量及外部标记引入
QString analysisGlobalVariable(MiddleNode &node,QStringList&allAddressLabel,bool&status){
    QString asmText;
    status = 0;

    if(node.areThereOnlyTheseAtt({"CONST","EXPORT","BODY"})){

        return asmText;
    }
    bool isConst = node.getAttFromName("CONST");
    bool isExport = node.getAttFromName("EXPORT");

    if(node.nodeType=="VAR"){

        if(node.getAttFromName("BODY")!=NULL){
            return asmText;
        }
        QString &type = node.args[0];
        type = type.toUpper();
        type = pointerTypeConvert(type);

        if(!varTypeIsConform(type)){
            return asmText;
        }
        QString &name = node.args[1];
        if(!nameIsConform(name)){
            return asmText;
        }
        asmText = "DATA";
        asmText += isExport ? "-EXPORT" : "";
        asmText += isConst ? "-CONST" : "";
        asmText += node.args.length()==2 ? "-NOINIT" : "";
        asmText += " "+name+"{";
        if(node.args.length()==2){
            if(isConst)return asmText;
            asmText += "ARR "+QString::number(getVarTypeSize(type))+"D,1D;}\r\n";
        }else{
            QString valueType;
            uint valueSize;
            bool suceess;
            QString valueText = analysisConstValue(node.args[2],allAddressLabel,suceess,valueType,valueSize);

            if(suceess==0)return asmText;
            if((valueType=="DOUBLE" || valueType=="FLOAT")&&(type!="DOUBLE" && type!="FLOAT")){
                return asmText;
            }else if((valueType!="DOUBLE" && valueType!="FLOAT") && (valueSize<=getVarTypeSize(valueType)||type=="DOUBLE" || type=="FLOAT")){
                return asmText;
            }

            asmText += type+" "+valueText+";};\r\n";
        }
        allAddressLabel.append(name);
    }else if(node.nodeType=="EXTERN"){
        if(node.atts.length()!=0 || node.args.length()!=1){
            return asmText;
        }

        QString &name = node.args[0];
        if(!nameIsConform(name)){
            return asmText;
        }

        asmText = "IMPORT "+name+";\r\n";
        allAddressLabel.append(name);
    }else if(node.nodeType=="GROUP"){
        if(node.getAttFromName("BODY")==NULL || node.args.length()!=1){
            return asmText;
        }

        QString &group_name = node.args[0];
        if(!nameIsConform(group_name)){
            return asmText;
        }

        MiddleNode_Att * bodyNode = node.getAttFromName("BODY");

        if(bodyNode->subNodes.length()==0)return asmText;
        QStringList allVarNames;
        bool notInit = 1;

        QString varAsmText;
        for(int i = 0;i<bodyNode->subNodes.length();i++){
            MiddleNode &varNode = bodyNode->subNodes[i];
            if(varNode.atts.length()!=0)return asmText;
            if(varNode.nodeType=="VAR"&&(varNode.args.length()==2||varNode.args.length()==3)){
                QString &type = varNode.args[0];
                type = type.toUpper();
                type = pointerTypeConvert(type);

                if(!varTypeIsConform(type)){
                    return asmText;
                }
                QString &name = varNode.args[1];
                if(!nameIsConform(name) || allVarNames.contains(name)){
                    return asmText;
                }

                if(varNode.args.length()==2){
                    varAsmText += "\t"+name+">ARR "+QString::number(getVarTypeSize(type))+"D,1D;\r\n";
                }else{
                    notInit = 0;
                    QString valueType;
                    uint valueSize;
                    bool suceess;
                    QString valueText = analysisConstValue(varNode.args[2],allAddressLabel,suceess,valueType,valueSize);
                    if(suceess==0)return asmText;
                    if((valueType=="DOUBLE" || valueType=="FLOAT")&&(type!="DOUBLE" && type!="FLOAT")){
                        return asmText;
                    }else if((valueType!="DOUBLE" && valueType!="FLOAT") && (valueSize<=getVarTypeSize(valueType)||type=="DOUBLE" || type=="FLOAT")){
                        return asmText;
                    }

                    varAsmText += "\t"+name+">"+type+" "+valueText+";\r\n";
                }
                allVarNames.append(name);
                allAddressLabel.append(group_name+"."+name);
            }else if(varNode.nodeType=="ARRAY" && varNode.args.length()==2){
                QString &name = varNode.args[0];
                if(!nameIsConform(name) || allVarNames.contains(name)){
                    return asmText;
                }
                QString valueType;
                uint valueSize;
                bool suceess;
                QString valueText = analysisConstValue(varNode.args[1],allAddressLabel,suceess,valueType,valueSize);
                if(suceess==0)return asmText;
                if(valueType=="DOUBLE" || valueType=="FLOAT"||valueText.at(0)=='-'){
                    return asmText;
                }
                varAsmText += "\t"+name+">ARR 1D,"+valueText+";\r\n";
                allVarNames.append(name);
                allAddressLabel.append(group_name+"."+name);
            }else return asmText;
        }

        asmText = "DATA";
        asmText += isExport ? "-EXPORT" : "";
        asmText += isConst ? "-CONST" : "";
        asmText += notInit ? "-NOINIT" : "";
        if(isConst && notInit)return asmText;
        asmText += " "+group_name+"{\r\n";
        asmText += varAsmText;
        asmText += "};\r\n";
    }else if(node.nodeType=="ARRAY"){
        if(node.args.length()==0){
            return asmText;
        }
        QString &name = node.args[0];
        if(!nameIsConform(name)){
            return asmText;
        }
        MiddleNode_Att * bodyNode = node.getAttFromName("BODY");
        asmText = "DATA";
        asmText += isExport ? "-EXPORT" : "";
        asmText += isConst ? "-CONST" : "";
        allAddressLabel.append(name);
        if(bodyNode==NULL && node.args.length()==2){
            if(isConst)return asmText;
            asmText += "-NOINIT";
            asmText += " "+name+"{";
            QString valueType;
            uint valueSize;
            bool suceess;
            QString valueText = analysisConstValue(node.args[1],allAddressLabel,suceess,valueType,valueSize);
            if(suceess==0)return asmText;
            if(valueType=="DOUBLE" || valueType=="FLOAT"||valueText.at(0)=='-'){
                return asmText;
            }
            asmText += "ARR 1D,"+valueText+";";
            asmText += "};\r\n";
        }else if(bodyNode!=NULL && node.args.length()==1){
            bool notInit = 1;
            QString varAsmText;
            for(int i = 0;i<bodyNode->subNodes.length();i++){
                MiddleNode &varNode = bodyNode->subNodes[i];
                if(varNode.atts.length()!=0)return asmText;
                if(varNode.nodeType=="VAR"&&(varNode.args.length()==2||varNode.args.length()==1)){
                    QString &type = varNode.args[0];
                    type = type.toUpper();
                    type = pointerTypeConvert(type);

                    if(!varTypeIsConform(type)){
                        return asmText;
                    }
                    if(varNode.args.length()==2){
                        notInit = 0;
                        QString valueType;
                        uint valueSize;
                        bool suceess;
                        QString valueText = analysisConstValue(varNode.args[1],allAddressLabel,suceess,valueType,valueSize);
                        if(suceess==0)return asmText;
                        if((valueType=="DOUBLE" || valueType=="FLOAT")&&(type!="DOUBLE" && type!="FLOAT")){
                            return asmText;
                        }else if((valueType!="DOUBLE" && valueType!="FLOAT") && (valueSize<=getVarTypeSize(valueType)||type=="DOUBLE" || type=="FLOAT")){
                            return asmText;
                        }
                        varAsmText += "\t"+type+" "+valueText+";\r\n";

                    }else{
                        varAsmText += "\tARR "+QString::number(getVarTypeSize(type))+"D,1D;\r\n";
                    }
                }else if(varNode.nodeType=="ARRAY" && varNode.args.length()==1){
                    QString valueType;
                    uint valueSize;
                    bool suceess;
                    QString valueText = analysisConstValue(varNode.args[0],allAddressLabel,suceess,valueType,valueSize);
                    if(suceess==0)return asmText;
                    if(valueType=="DOUBLE" || valueType=="FLOAT"||valueText.at(0)=='-'){
                        return asmText;
                    }
                    varAsmText += "\tARR 1D,"+valueText+";\r\n";
                }else if(varNode.nodeType=="STRING" && varNode.args.length()==1){
                    notInit = 0;
                    if(varNode.args[0].at(0)!='\"'){
                        return asmText;
                    }
                    varAsmText += "\tSTR "+varNode.args[0]+";\r\n";
                }else return asmText;

            }

            asmText += notInit ? "-NOINIT" : "";
            asmText += " "+name+"{\r\n";
            asmText += varAsmText;
            asmText += "};\r\n";

        }else{
            return asmText;
        }
    }else{
        return asmText;
    }
    status = 1;
    return asmText;
}

//解析汇编函数的定义[汇编函数中只能使用汇编指令]
QString analysisAsmFun(MiddleNode &node,bool&status){
    QString asmText;
    status = 0;

    if(node.getAttFromName("BODY")==NULL || node.args.length()!=1){
        return asmText;
    }

    QString &name = node.args[0];
    if(!nameIsConform(name)){
        return asmText;
    }
    bool isExport = node.getAttFromName("EXPORT");

    MiddleNode_Att * bodyNode = node.getAttFromName("BODY");

    if(bodyNode->subNodes.length()==0)return asmText;
    QString codeAsmText;
    for(int i = 0;i<bodyNode->subNodes.length();i++){
        MiddleNode &thisNode = bodyNode->subNodes[i];
        if(thisNode.nodeType=="LABEL"){
            codeAsmText+=thisNode.args[0]+">\r\n";
        }else{
            codeAsmText+="\t"+thisNode.nodeType+(thisNode.args.length()==0 ? "" :" "+thisNode.args.join(','))+";\r\n";
        }
    }

    asmText = "CODE";
    asmText += isExport ? "-EXPORT" : "";
    asmText += " "+name+"{\r\n";
    asmText += codeAsmText;
    asmText += "};\r\n";
    status = 1;
    return asmText;
}

//编译中级语言文本，生成汇编语言文本
QString complieMiddleSrc(QString srcText,bool &isSuceess){
    MiddleNode middleRootNode(srcText,isSuceess);
    if(isSuceess==0){
        qDebug()<<"解析节点描述文本失败";
        return QString();
    }else if(middleRootNode.nodeType!="PSDL" ||
             middleRootNode.areThereOnlyTheseAtt({"BODY"})||
             middleRootNode.args.length()!=0){
        isSuceess = 0;
        return QString();
    }
    MiddleNode_Att *body = middleRootNode.getAttFromName("BODY");
    if(body==NULL){
        isSuceess = 0;
        return QString();
    }

    QString asmText;
    QStringList allAddressLabel;//全局地址标记名(指向变量/函数/数组的首地址)
    QStringList oper64Fun;      //使用到的64位操作函数

    for(int i = 0;i<body->subNodes.length();i++){
        MiddleNode &thisNode = body->subNodes[i];
        QString &nodeTypeName = thisNode.nodeType;
        QString thisNodeAnalysisAsm;
        //全局变量定义解析
        if(nodeTypeName=="VAR" || nodeTypeName=="ARRAY" ||
           nodeTypeName=="GROUP" || nodeTypeName=="EXTERN"){
            bool suceess = 0;
            thisNodeAnalysisAsm = analysisGlobalVariable(thisNode,allAddressLabel,suceess);
            if(!suceess){
                isSuceess = 0;
                return QString();
            }
        }else if(nodeTypeName=="FUN"){
            bool suceess = 0;
            thisNodeAnalysisAsm = analysisFunction(thisNode,allAddressLabel,oper64Fun,suceess);
            if(!suceess){
                isSuceess = 0;
                return QString();
            }
        }else if(nodeTypeName=="ASMFUN"){
            bool suceess = 0;
            thisNodeAnalysisAsm = analysisAsmFun(thisNode,suceess);
            if(!suceess){
                isSuceess = 0;
                return QString();
            }
        }else{
            isSuceess = 0;
            return QString();
        }
        asmText+=thisNodeAnalysisAsm;
    }

    QString importOper64FunAsm;
    for(int i = 0;i<oper64Fun.length();i++){
        importOper64FunAsm+="IMPORT "+oper64Fun[i]+";\r\n";
    }
    asmText.prepend(importOper64FunAsm);

    return asmText.toUtf8().data();
}

#endif // GRAMMARTICALANALYSIS_H
