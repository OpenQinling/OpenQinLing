#include "OpenQinling_C_Grammarparsing.h"
#include "OpenQinling_DebugFunction.h"
#include "OpenQinling_C_JudgestrtoolFunction.h"
#include "OpenQinling_C_Initblock.h"
#include "cmath"
#include "iostream"
#include "OpenQinling_PSDL_MiddleNode.h"
#include "OpenQinling_Math.h"
#include "OpenQinling_DebugFunction.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{
//常量运算式子的运算(其内不能有任何变量/const常量参与运算,不能有任何赋值操作,且只能有整型参与运算,可以是enum中定义的常量)
//只能存在 整数常量 sizeof(数据类型) sizeof(变量) 的运算。只能有+-*/%()的符号
//专用于定义数组变量时,数组的长度解析
static int analysisConstValueOperator(PhraseList &sentence,SrcObjectInfo &objInfo,bool&status);
//解析出一个枚举的定义信息,如果失败了返回空
static bool analysisTypeDefineSentence(PhraseList &sentence,
                                SrcObjectInfo &objInfo);
//解析出一个结构体/共用体的定义信息
//如果解析成功,注入到SrcAllDataTypeTree中,并返回类型的指针
static IndexVN analysisStructUnionTypeDef(PhraseList &sentence,SrcObjectInfo &objInfo);
//解析出一个枚举的定义信息,如果失败了返回空
static IndexVN analysisEnumTypeDef(PhraseList &sentence,SrcObjectInfo &objInfo);
//解析一个函数指针的定义
//name:返回函数指针名,或者函数指针类型名
//pointerLevel:几级的函数指针(函数指针本身就是1级指针,不算在pointerLevel内)
static IndexVN analysisFunctionPtrTypeDef(PhraseList &sentence,
                                   SrcObjectInfo &objInfo,
                                   QString &name,
                                   bool &isHaveName,
                                   bool &isPointerConst,
                                   int &pointerLevel,
                                   QList<int>&arraySize,
                                   bool &isAutoArrSize);

//解析一个函数指针的定义
//retTypeIndex:函数返回值类型信息的索引号
//retPointerLevel:函数返回类型的指针级数
//name:返回函数指针名,或者函数指针类型名
//pointerLevel:几级的函数指针(函数指针本身就是1级指针,不算在pointerLevel内)
static IndexVN analysisFunctionPtrTypeDef(IndexVN retTypeIndex,
                                   int retPointerLevel,
                                   PhraseList &sentence,
                                   SrcObjectInfo &objInfo,
                                   QString &name,
                                   bool &isHaveName,
                                   bool &isPointerConst,
                                   int &pointerLevel,
                                   QList<int>&arraySize,
                                   bool &isAutoArrSize);

//函数/函数指针声明时,解析括号中参数信息
struct FunArgDefInfo{
    QString name;//参数名
    bool isAnnony = 0;//是否匿名
    StructUnion_AttInfo info;//信息
};
static QList<FunArgDefInfo> analysisFunctionArgDef(PhraseList &sentence,
                                            SrcObjectInfo &objInfo,
                                            bool &isStdcall,
                                            bool &status){
    status = 0;
    QList<FunArgDefInfo> argDefInfo;
    //////////////////////解析函数参数///////////////////////////

    PhraseList functionArgsSence = sentence;
    if(functionArgsSence.length()==0 ||
       (functionArgsSence.length()==1 && functionArgsSence[0].isKeyWord("void"))){
        status = 1;
        isStdcall = 0;
        return QList<FunArgDefInfo>();
    }

    if(functionArgsSence[functionArgsSence.length()-1].isDelimiter(",")){
        return QList<FunArgDefInfo>();
    }
    if(functionArgsSence[0].isDelimiter(","))return QList<FunArgDefInfo>();
    QList<PhraseList> allArgSence = functionArgsSence.splitFromDelimiter(",");
    isStdcall = 0;
    for(int i = 0;i<allArgSence.length();i++){
        QString argName;
        bool isAnnony = 1;
        if(allArgSence[i].length()==0)return QList<FunArgDefInfo>();
        if(i+1==allArgSence.length() && allArgSence[i].length()==1){
            if(allArgSence[i][0].isDelimiter("...")){
                isStdcall = 1;
                break;
            }
        }
        bool argIsConst = 0;
        bool argIsPointerConst = 0;
        int argPointerLevel = 0;
        IndexVN attTypeIndex;

        while(allArgSence[i].length() != 0){
            if(allArgSence[i][0].isKeyWord("const")){
                argIsConst = 1;
                allArgSence[i].removeFirst();
            }else{
                break;
            }
        }


        int isStructEnumUnionFun = 0;
        if(allArgSence[i][0].isKeyWord("struct")){
            isStructEnumUnionFun = 1;
        }else if(allArgSence[i][0].isKeyWord("union")){
            isStructEnumUnionFun = 2;
        }else if(allArgSence[i][0].isKeyWord("enum")){
            isStructEnumUnionFun = 3;
        }else if(allArgSence[i].getDelimiterNum("()")==2){
            isStructEnumUnionFun = 4;
        }

        if(isStructEnumUnionFun==4){
            QString name;
            bool isHaveName;
            QList<int> arraySize;
            bool isAutoArrSize;
            attTypeIndex = analysisFunctionPtrTypeDef(allArgSence[i],objInfo,name,isHaveName,argIsPointerConst,argPointerLevel,arraySize,isAutoArrSize);
            argPointerLevel+=arraySize.length();
            if(attTypeIndex.isEmpty() || isAutoArrSize)return QList<FunArgDefInfo>();
        }else{
            if(isStructEnumUnionFun){
                if(allArgSence[i].length()<2)return QList<FunArgDefInfo>();
                allArgSence[i].removeFirst();
                if(!allArgSence[i][0].isIdentifier())return QList<FunArgDefInfo>();

                if(isStructEnumUnionFun==1){
                    attTypeIndex = objInfo.getDataTypeInfoNode()->searchStructType(allArgSence[i][0].text);
                }else if(isStructEnumUnionFun==2){
                     attTypeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(allArgSence[i][0].text);
                }else if(isStructEnumUnionFun==3){
                     attTypeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(allArgSence[i][0].text);
                }
                if(attTypeIndex.isEmpty())return QList<FunArgDefInfo>();
                allArgSence[i].removeFirst();
            }else if(allArgSence[i][0].isIdentifier()){
                //自定义数据类型
                QString text = allArgSence[i][0].text;
                bool status;
                MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo({text},status);
                if(status==0)return QList<FunArgDefInfo>();

                allArgSence[i].removeFirst();
                attTypeIndex = map.index;
                argPointerLevel += map.pointerLevel;
                argIsConst = argIsConst || map.isDefaultConst;
            }else{

                //c原生数据类型
                QStringList typeName;
                for(int j = 0;j<allArgSence[i].length();j++){
                    if(allArgSence[i][j].PhraseType != Phrase::KeyWord)break;
                    typeName.append(allArgSence[i][j].text);
                    allArgSence[i].removeFirst();
                    j--;
                }
                typeName = stdFormatBaseTypeName(typeName);
                attTypeIndex = objInfo.getDataTypeInfoNode()->searchDataType(typeName);
                if(attTypeIndex.isEmpty())return QList<FunArgDefInfo>();
            }
            while(allArgSence[i].length() != 0){
                if(allArgSence[i][0].isDelimiter("*")){
                    argPointerLevel++;
                    allArgSence[i].removeFirst();
                }else{
                    break;
                }
            }

            while(allArgSence[i].length() != 0){
                if(allArgSence[i][0].isKeyWord("const")){
                    if(argPointerLevel == 0){
                        argIsConst = 1;
                    }else{
                        argIsPointerConst = 1;
                    }
                    allArgSence[i].removeFirst();
                }else{
                    break;
                }
            }

            if(allArgSence[i].length()!=0 && allArgSence[i][0].isIdentifier()){
                argName = allArgSence[i][0].text;
                isAnnony = 0;
                allArgSence[i].removeFirst();
            }
            for(int j = 0;j<allArgSence[i].length();j++){
                if(allArgSence[i][j].isDelimiter("[]")){
                    argPointerLevel++;
                    allArgSence[i].removeFirst();
                    j--;
                }else{
                    break;
                }
            }
            if(allArgSence[i].length()!=0)return QList<FunArgDefInfo>();
        }
        StructUnion_AttInfo argAtt;
        argAtt.typeIndex = attTypeIndex;
        argAtt.isConst = argIsConst;
        argAtt.isPointerConst = argIsPointerConst;
        argAtt.arraySize.clear();
        argAtt.pointerLevel = argPointerLevel;

        FunArgDefInfo funArgInfo;
        funArgInfo.info = argAtt;
        funArgInfo.isAnnony = isAnnony;
        funArgInfo.name = argName;

        if(objInfo.getRootDataTypeInfoNode()->getDataType(attTypeIndex)->baseType == DataBaseType_VOID && argPointerLevel==0){
            return QList<FunArgDefInfo>();
        }

        argDefInfo.append(funArgInfo);
    }
    status = 1;
    return argDefInfo;
}


//解析一个函数指针的定义
//name:返回函数指针名,或者函数指针类型名
//pointerLevel:几级的函数指针(函数指针本身就是1级指针,不算在pointerLevel内)
static IndexVN analysisFunctionPtrTypeDef(PhraseList &sentence,
                                   SrcObjectInfo &objInfo,
                                   QString &name,
                                   bool &isHaveName,
                                   bool &isPointerConst,
                                   int &pointerLevel,
                                   QList<int>&arraySize,
                                   bool &isAutoArrSize){
    if(sentence.length()<2)return IndexVN();
    int retPointerLevel = 0;
    //解析函数返回值级数
    while(1){
        if(sentence[0].isDelimiter("*")){
            retPointerLevel++;
            sentence.removeFirst();
        }else break;
    }

    //可能是定义的函数指针类型
    Phrase argPse = sentence[sentence.length()-1];//描述函数指针参数类型的词组
    Phrase functionNamePse = sentence[sentence.length()-2];//新类型名的描述词组
    if(!argPse.isDelimiter("()") || !functionNamePse.isDelimiter("()"))return IndexVN();
    sentence.removeLast();
    sentence.removeLast();



    //////////////////解析出返回值类型///////////////////////
    IndexVN retTypeIndex;

    sentence.removeKeyWord("const");

    //解析返回值有几级指针
    for(int i = sentence.length()-1;i>=0;i--){
        if(sentence[i].isDelimiter("*")){
            retPointerLevel++;
            sentence.removeLast();
        }else{
            break;
        }
    }
    if(!sentence.isOnlyHaveKeyWordAndIdentifier() || sentence.length()==0)return IndexVN();
    if(sentence.length()==2 && sentence[0].isKeyWord("enum")){
        if(!sentence[1].isIdentifier())return IndexVN();
        retTypeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(sentence[1].text);
    }else if(sentence.length()==2 && sentence[0].isKeyWord("struct")){
        if(!sentence[1].isIdentifier())return IndexVN();
        retTypeIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[1].text);
    }else if(sentence.length()==2 && sentence[0].isKeyWord("union")){
        if(!sentence[1].isIdentifier())return IndexVN();
        retTypeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(sentence[1].text);
    }else{
        QStringList retTypeName = sentence.getKeyWordAndIdentifierText();
        retTypeName = stdFormatBaseTypeName(retTypeName);
        bool status;
        MappingInfo retTypeMapping = objInfo.getDataTypeInfoNode()->searchMappingInfo(retTypeName,status);
        if(status==0)return IndexVN();
        retTypeIndex = retTypeMapping.index;
        retPointerLevel += retTypeMapping.pointerLevel;
    }
    sentence.clear();
    sentence.append(functionNamePse);
    sentence.append(argPse);

    return analysisFunctionPtrTypeDef(retTypeIndex,
                                      retPointerLevel,
                                      sentence,
                                      objInfo,
                                      name,
                                      isHaveName,
                                      isPointerConst,
                                      pointerLevel,
                                      arraySize,
                                      isAutoArrSize);
}
//解析一个函数指针的定义
//retTypeIndex:函数返回值类型信息的索引号
//retPointerLevel:函数返回类型的指针级数
//name:返回函数指针名,或者函数指针类型名
//pointerLevel:几级的函数指针(函数指针本身就是1级指针,不算在pointerLevel内)
static IndexVN analysisFunctionPtrTypeDef(IndexVN retTypeIndex,
                                   int retPointerLevel,
                                   PhraseList &sentence,
                                   SrcObjectInfo &objInfo,
                                   QString &name,
                                   bool &isHaveName,
                                   bool &isPointerConst,
                                   int &pointerLevel,
                                   QList<int>&arraySize,
                                   bool &isAutoArrSize){
    if(sentence.length()<2)return IndexVN();
    isAutoArrSize = 0;

    while(1){
        if(sentence[0].isDelimiter("*")){
            retPointerLevel++;
            sentence.removeFirst();
        }else if(sentence[0].isKeyWord("const")){
            sentence.removeFirst();
        }else break;
    }

    //可能是定义的函数指针类型
    Phrase argPse = sentence[1];//描述函数指针参数类型的词组
    Phrase functionNamePse = sentence[0];//新类型名的描述词组
    if(!argPse.isDelimiter("()") || !functionNamePse.isDelimiter("()"))return IndexVN();
    sentence.removeLast();
    sentence.removeLast();

    DataTypeInfo functionInfo;
    functionInfo.baseType = DataBaseType_FUNPTR;
    functionInfo.dataBaseTypeBytes = objInfo.CPU_BIT/8;


    while(sentence.length() != 0){
        if(sentence[0].isDelimiter("*")){
            retPointerLevel ++;
            sentence.removeFirst();
        }else{
            break;
        }
    }
    if(sentence.length()!=0)return IndexVN();


    StructUnion_AttInfo attTmp;
    attTmp.typeIndex = retTypeIndex;
    attTmp.isConst = 0;//函数返回值不可能是一个const型
    attTmp.arraySize.clear();
    attTmp.pointerLevel = retPointerLevel;
    functionInfo.funArgInfos.append(attTmp);

    //////////////////////解析函数名称//////////////////////////
    PhraseList functionNameSence = functionNamePse.subPhraseList;
    pointerLevel = 0;
    isPointerConst = 0;
    while(functionNameSence.length() != 0){
        if(functionNameSence[0].isDelimiter("*")){
            pointerLevel ++;
            functionNameSence.removeFirst();
        }else{
            break;
        }
    }
    while(functionNameSence.length() != 0){
        if(functionNameSence[0].isKeyWord("const")){
            isPointerConst = 1;
            functionNameSence.removeFirst();
        }else{
            break;
        }
    }
    if(pointerLevel==0)return IndexVN();
    pointerLevel--;

    isHaveName = 0;
    if(functionNameSence.length()>=1){
        if(functionNameSence[0].isIdentifier()){
            name = functionNameSence[0].text;
            isHaveName = 1;
            functionNameSence.removeFirst();
        }
    }
    arraySize.clear();
    for(int i = 0;i<functionNameSence.length();i++){
        if(functionNameSence[i].isDelimiter("[]")){
            bool status;
            PhraseList subPhs = functionNameSence[i].subPhraseList;
            if(subPhs.length()==0 && i==0){
                isAutoArrSize = 1;
                arraySize.append(0);
            }else{
                int size =analysisConstValueOperator(subPhs,objInfo,status);
                if(status==0)return IndexVN();
                arraySize.append(size);
            }
            functionNameSence.removeFirst();
            i--;
        }else{
            break;
        }
    }
    if(functionNameSence.length()!=0)return IndexVN();

    //////////////////////解析函数参数///////////////////////////
    PhraseList argDefsentence = argPse.subPhraseList;
    bool isStdcall;
    bool status;
    QList<FunArgDefInfo> argDef = analysisFunctionArgDef(argDefsentence,
                                                objInfo,
                                                isStdcall,
                                                status);
    if(status==0)return IndexVN();
    for(int i = 0;i<argDef.length();i++){
        functionInfo.funArgInfos.append(argDef[i].info);
    }
    functionInfo.isStdcallFun = isStdcall;
    return objInfo.getRootDataTypeInfoNode()->appendFunctionType(functionInfo);
}

//解析一个函数的定义
//retTypeIndex:函数返回值类型信息的索引号
//retPointerLevel:函数返回类型的指针级数
//name:返回函数指针名,或者函数指针类型名
//pointerLevel:几级的函数指针(函数指针本身就是1级指针,不算在pointerLevel内)
static IndexVN analysisFunctionTypeDef(IndexVN retTypeIndex,
                                   int retPointerLevel,
                                   PhraseList &sentence,
                                   SrcObjectInfo &objInfo,
                                   QString &name){
    if(sentence.length()<2)return IndexVN();
    //可能是定义的函数指针类型
    Phrase argPse = sentence[sentence.length()-1];//描述函数指针参数类型的词组
    if(!argPse.isDelimiter("()"))return IndexVN();
    sentence.removeLast();

    DataTypeInfo functionInfo;
    functionInfo.baseType = DataBaseType_FUNPTR;
    functionInfo.dataBaseTypeBytes = objInfo.CPU_BIT/8;
    //////////////////////解析函数名称//////////////////////////
    for(int i = 0;i<sentence.length();i++){
        if(sentence[i].isDelimiter("*")){
            retPointerLevel ++;
            sentence.removeFirst();
            i--;
        }else{
            break;
        }
    }

    if(sentence.length()>=1){
        if(sentence[0].isIdentifier()){
            name = sentence[0].text;
            sentence.removeFirst();
        }
    }else{
        return IndexVN();
    }
    if(sentence.length()!=0)return IndexVN();

    //返回值类型信息
    StructUnion_AttInfo attTmp;
    attTmp.typeIndex = retTypeIndex;
    attTmp.isConst = 0;//函数返回值不可能是一个const型
    attTmp.arraySize.clear();
    attTmp.pointerLevel = retPointerLevel;
    functionInfo.funArgInfos.append(attTmp);



    //////////////////////解析函数参数///////////////////////////

    PhraseList argDefsentence = argPse.subPhraseList;
    bool isStdcall;
    bool status;
    QList<FunArgDefInfo> argDef = analysisFunctionArgDef(argDefsentence,
                                                objInfo,
                                                isStdcall,
                                                status);
    if(status==0)return IndexVN();
    for(int i = 0;i<argDef.length();i++){
        functionInfo.funArgInfos.append(argDef[i].info);
    }
    functionInfo.isStdcallFun = isStdcall;
    return objInfo.getRootDataTypeInfoNode()->appendFunctionType(functionInfo);
}



//解析定义的变量/属性的名称、指针、数组
static QString analysisDefVarOrAtt_Name_Ptr_Arr(PhraseList &sentence,
                                         SrcObjectInfo &objInfo,
                                         int &pointerLevel, //指针层数
                                         QList<int>&arraySize, //数组大小
                                         bool &isHaveName, //是否有名称
                                         bool &isAutoArrSize, //是否自动分配数组大小
                                         bool &isHavePrefixConst, //前缀const
                                         bool &isHaveSuffixConst, //后缀const
                                         bool &status){
     pointerLevel = 0;
     arraySize.clear();
     QString name;
     isAutoArrSize = 0;
     for(int i = sentence.length()-1;i>=0;i--){
         if(sentence[i].isDelimiter("[]")){
             PhraseList sub = sentence[i].subPhraseList;

             if(sub.length()==0 && (i==0 || !sentence[i-1].isDelimiter("[]"))){
                 isAutoArrSize = 1;
                 arraySize.prepend(0);
             }else{
                 int size = analysisConstValueOperator(sub,objInfo,status);
                 if(status==0)return QString();
                 status = 0;
                 arraySize.prepend(size);
             }

             sentence.removeLast();
         }else break;
     }
     isHaveName = 0;
     if(sentence[sentence.length()-1].isIdentifier()){
         isHaveName = 1;
         name = sentence[sentence.length()-1].text;
         sentence.removeLast();
     }
     isHaveSuffixConst = 0,isHavePrefixConst = 0;

     for(int i = sentence.length()-1;i>=0;i--){
         if(sentence[i].isKeyWord("const")){
             isHaveSuffixConst = 1;
             sentence.removeLast();
         }
     }
     for(int i = sentence.length()-1;i>=0;i--){
         if(sentence[i].isDelimiter("*")){
             pointerLevel++;
             sentence.removeLast();
         }
     }

     for(int i = sentence.length()-1;i>=0;i--){
         if(sentence[i].isKeyWord("const")){
             isHavePrefixConst = 1;
             sentence.removeLast();
         }
     }


     if(sentence.length()!=0)return QString();

     status = 1;
     return name;
}

//sizeof中是一个数据类型
static int sizeofOperator_Type(PhraseList sentence,SrcObjectInfo &objInfo,bool&status){
    status = 0;
    bool isHavePointer = 0;
    for(int i = sentence.length()-1;i>=0;i--){
        if(sentence[i].isDelimiter("*")){
            isHavePointer = 1;
            sentence.removeLast();
        }else{
            break;
        }
    }

    if(!sentence.isOnlyHaveKeyWordAndIdentifier() || sentence.length()==0){
        return 0;
    }

    IndexVN typeIndex;
    if(sentence[0].isKeyWord("struct")){
        sentence.removeFirst();
        if(sentence.length()!=1 && sentence[0].isIdentifier())return 0;
        typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[0].text);
    }else if(sentence[0].isKeyWord("union")  && sentence[0].isIdentifier()){
        //判断是否是一个
        sentence.removeFirst();
        if(sentence.length()!=1)return 0;
        typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(sentence[0].text);
    }else if(sentence[0].isKeyWord("enum")  && sentence[0].isIdentifier()){
        //判断是否是一个枚举类型
        sentence.removeFirst();
        if(sentence.length()!=1)return 0;
        typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(sentence[0].text);
    }else if(sentence.length()>=1){
        //检测是否是一个普通数据类型
        QStringList name = stdFormatBaseTypeName(sentence.getKeyWordAndIdentifierText());
        typeIndex = objInfo.getDataTypeInfoNode()->searchDataType(name);
    }
    if(typeIndex.isEmpty())return 0;



    if(isHavePointer){
        status = 1;
        return objInfo.CPU_BIT/8;
    }

    if(objInfo.getDataTypeInfoNode()->getDataType(typeIndex)->isOnlyStatement){
        return 0;
    }
    status = 1;
    return objInfo.getDataTypeInfoNode()->getTypeSize(typeIndex);
}


struct SizeofToken{
    enum{
        ERROR,//出错
        Value,//变量(包括常量值、函数(指针)、普通变量/数组/结构体)
        Operator,//运算符
        SubOper,//()子表达式
        GetPtrAtt,//->指针取结构体属性运算符
        GetAtt//. 变量取结构体属性运算符
    }tokenType = ERROR;
    SizeofToken(){

    }



    //创建一个常量值的token
    SizeofToken(QStringList typeName,int pointerLevel,SrcObjectInfo &objInfo){
        IndexVN typeIndex = objInfo.getDataTypeInfoNode()->searchDataType(typeName);
        if(typeIndex.isEmpty())return;
        tokenType = Value;
        valueInfo.baseType = typeIndex;
        valueInfo.pointerLevel = pointerLevel;
        valueInfo.arraySize.clear();
        this->objInfo = &objInfo;
    }

    //创建一个变量/函数的token
    SizeofToken(IndexVN baseType,
                int pointerLevel,
                QList<int> arraySize,
                SrcObjectInfo &objInfo){
        this->objInfo = &objInfo;
        tokenType = Value;
        valueInfo.baseType = baseType;
        valueInfo.pointerLevel = pointerLevel;
        valueInfo.arraySize = arraySize;
    }

    //创建一个子表达式token
    SizeofToken(QList<SizeofToken> subTokens,
                SrcObjectInfo &objInfo){
        tokenType = SubOper;
        subOperInfo.subTokens = subTokens;
        this->objInfo = &objInfo;
    }
    //创建一个符号token
    SizeofToken(QString operName,
                SrcObjectInfo &objInfo){
        tokenType = Operator;
        operatorInfo.operTypeName = operName;
        this->objInfo = &objInfo;
    }
    //创建一个取属性的token
    SizeofToken(bool isPtrGetAtt,QString attName,
                SrcObjectInfo &objInfo){
        if(isPtrGetAtt){
            tokenType = GetPtrAtt;
        }else{
            tokenType = GetAtt;
        }
        getAttPtrAtt_Info.attName = attName;
        this->objInfo = &objInfo;
    }


    QString toString(){
        if(tokenType == Value){
            QString tmp = "Value(Type:";
            tmp += objInfo->getDataTypeInfoNode()->getDataType(valueInfo.baseType)->toString();
            tmp += ",PointerLevel:"+QString::number(valueInfo.pointerLevel);
            tmp += ",ArraySize:";
            for(int i = 0;i<valueInfo.arraySize.length();i++){
                tmp += "["+QString::number(valueInfo.arraySize[i])+"]";
            }
            tmp += ")";
            return tmp;
        }else if(tokenType == Operator){
            return operatorInfo.operTypeName;
        }else if(tokenType == SubOper){
            QString tmp = "(";
            for(int i = 0;i<subOperInfo.subTokens.length();i++){
                tmp += subOperInfo.subTokens[i].toString();
            }
            tmp += ")";
            return tmp;
        }else if(tokenType == GetPtrAtt){
            return "->"+getAttPtrAtt_Info.attName;
        }else if(tokenType == GetAtt){
            return "."+getAttPtrAtt_Info.attName;
        }else return "<ERROR>";
    }

    //()      子表达式
    //(*)     解引用
    //(&)     取地址
    //其余保持不变
    bool isOperator(QString operName){
        if(operName == "->")return tokenType==GetPtrAtt;
        else if(operName == "()")return tokenType==SubOper;
        else if(operName == ".")return tokenType==GetAtt;
        else return tokenType == Operator && operatorInfo.operTypeName==operName;
    }
    bool isValue(){
        return tokenType==Value;
    }
    bool isError(){
        return tokenType==ERROR;
    }

    //Value类型专有
    struct {
        IndexVN baseType;//变量基类型
        int pointerLevel;//变量的指针级数
        QList<int> arraySize;//变量的数组维度
    }valueInfo;

    //Operator类型专有
    struct{
        QString operTypeName;//运算符名称
    }operatorInfo;

    //SubOper类型专有
    struct{
        QList<SizeofToken> subTokens;//()内的token
    }subOperInfo;

    //GetPtrAtt/GetAtt类型专有
    struct{
        QString attName;//属性名称
    }getAttPtrAtt_Info;

    SrcObjectInfo *objInfo = NULL;


    ////////////指针相关运算//////////////
    //解指针的引用 C语法: *指针变量
    SizeofToken dereference(){
        if(tokenType != Value || (valueInfo.pointerLevel == 0 && valueInfo.arraySize.length()==0 ))return SizeofToken();
        SizeofToken ret = *this;
        if(valueInfo.arraySize.length()){
            ret.valueInfo.arraySize.removeFirst();
        }else{
            ret.valueInfo.pointerLevel -= 1;
        }

        return ret;
    }

    //取变量地址 C语法: &被取地址变量
    SizeofToken getAddress(){
        if(tokenType != Value)return SizeofToken();
        SizeofToken ret = *this;
        if(valueInfo.arraySize.length()){
            ret.valueInfo.arraySize.prepend(1);
        }else{
            ret.valueInfo.pointerLevel += 1;
        }
        return ret;
    }
    ////////////获取引用相关运算////////////
    //获取结构体变量中属性的引用 C语法: 结构体变量.属性名
    SizeofToken getStructAtt(QString attName){
        if(tokenType != Value || valueInfo.pointerLevel != 0)return SizeofToken();
        DataTypeInfo * typeInfo = objInfo->getDataTypeInfoNode()->getDataType(valueInfo.baseType);
        if(typeInfo->baseType != DataBaseType_UNION && typeInfo->baseType != DataBaseType_STRUCT)return SizeofToken();
        else if(!typeInfo->attInfos.contains(attName))return SizeofToken();

        StructUnion_AttInfo &attInfo = typeInfo->attInfos[attName];

        SizeofToken att(attInfo.typeIndex,attInfo.pointerLevel,attInfo.arraySize,*objInfo);
        return att;
    }
    //获取结构体指针变量中属性的引用 C语法: 结构体指针->属性名
    SizeofToken getPointerStructAtt(QString attName){
        if(tokenType != Value || valueInfo.pointerLevel != 1)return SizeofToken();
        DataTypeInfo * typeInfo = objInfo->getDataTypeInfoNode()->getDataType(valueInfo.baseType);
        if(typeInfo->baseType != DataBaseType_UNION && typeInfo->baseType != DataBaseType_STRUCT)return SizeofToken();
        else if(!typeInfo->attInfos.contains(attName))return SizeofToken();
        StructUnion_AttInfo &attInfo = typeInfo->attInfos[attName];
        SizeofToken att(attInfo.typeIndex,attInfo.pointerLevel,attInfo.arraySize,*objInfo);
        return att;
    }
};

//计算sizeof中表达式最终输出的数据的类型
static SizeofToken getSizeofDataType(QList<SizeofToken> tokens){
    SizeofToken retValue;

    ///////////////////优先级1运算符:从左至右/////////////////////
    //运算() . -> []
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length();i++){
            if(tokens[i].isOperator("()")){
                operCount++;
                QList<SizeofToken> subTokens = tokens[i].subOperInfo.subTokens;
                tokens[i] = getSizeofDataType(subTokens);
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("[]")){
                tokens[i-1] = tokens[i-1].dereference();
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator(".")){
                tokens[i-1] = tokens[i-1].getStructAtt(tokens[i].getAttPtrAtt_Info.attName);
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("->")){
                tokens[i-1] = tokens[i-1].getPointerStructAtt(tokens[i].getAttPtrAtt_Info.attName);
                tokens.removeAt(i);
                i-=1;
            }
        }
        if(operCount==0)break;
    }

    ///////////////////优先级2运算符:从右至左/////////////////////
    //运算(*)  (&)
    while(1){
        int operCount = 0;
        for(int i = tokens.length()-1;i>=0;i--){
            if(!tokens[i].isValue())continue;
            else if(i<=0)break;
            if(tokens[i-1].isOperator("(*)")){
                tokens[i-1] = tokens[i].dereference();
            }else if(tokens[i-1].isOperator("(&)")){
                tokens[i-1] = tokens[i].getAddress();
            }else{
                continue;
            }

            tokens.removeAt(i);
            i--;
            operCount++;
        }
        if(operCount==0)break;
    }

    if(tokens.length()!=1)return retValue;

    return tokens[0];
}

//取sizeof运算的表达式token
static QList<SizeofToken> getSizeofTokens(PhraseList sentence,SrcObjectInfo &objInfo,bool&suceess){
    {
        QList<PhraseList> phrs = sentence.splitFromDelimiter(",");
        if(phrs.length()==0){
            return QList<SizeofToken>();
        }
        sentence = phrs[phrs.length()-1];
    }
    suceess = 0;
    QList<SizeofToken> tmp;
    bool lastIsValue = 0;
    for(int i = 0;i<sentence.length();i++){
        bool currentIsValue = 0;

        if(sentence[i].isKeyWord("sizeof") && i+1<sentence.length() &&
           sentence[i+1].isDelimiter("()")){
            //sizeof运算 转为int32的常量
            tmp.append(SizeofToken(QStringList({"int"}),0,objInfo));
            i+=1;
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT32){
            tmp.append(SizeofToken(QStringList({"int"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT32){
            tmp.append(SizeofToken(QStringList({"unsigned","int"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT64){
            tmp.append(SizeofToken(QStringList({"unsigned","long","long"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT64){
            tmp.append(SizeofToken(QStringList({"long","long"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT32){
            //浮点型常量 转为float32的常量
            tmp.append(SizeofToken(QStringList({"float"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT64){
            //浮点型常量 转为float64的常量
            tmp.append(SizeofToken(QStringList({"double"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_CHAR){
            //字符常量 转为char
            tmp.append(SizeofToken(QStringList({"char"}),0,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_STRING){
            //字符串常量 已指针来表示
            tmp.append(SizeofToken(QStringList({"char"}),1,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].isDelimiter("()")){
            //子表达式
            PhraseList sub = sentence[i].subPhraseList;
            bool subSuccess;
            QList<SizeofToken> subToken = getSizeofTokens(sub,objInfo,subSuccess);
            if(!subSuccess)return QList<SizeofToken>();
            tmp.append(SizeofToken(subToken,objInfo));
            currentIsValue = 1;
        }else if(sentence[i].isDelimiter("[]")){//索引表达式可以被看做是解引用
            //索引子表达式
            tmp.append(SizeofToken("[]",objInfo));
            currentIsValue = 1;
        }else if(sentence[i].isIdentifier() || (sentence[i].isKeyWord("__valist__"))){
            //标识符,可能是enum常量或者是其它全局常量
            //判断是否是一个变量
            IndexVN varIndex = objInfo.getVarDefineInfoNode()->searchVar(sentence[i].text);

            bool isEnumConstValue;
            objInfo.getDataTypeInfoNode()->getEnumConstValue(sentence[i].text,&isEnumConstValue);
            //判断是否是一个函数
            int funIndex = objInfo.serachFunction(sentence[i].text);
            if(!varIndex.isEmpty()){
                //是一个变量
                VarDefineInfo *info = objInfo.getVarDefineInfoNode()->getVar(varIndex);
                tmp.append(SizeofToken(info->baseTypeIndex,info->pointerLevel,info->arraySize,objInfo));
            }else if(isEnumConstValue){
                //是一个枚举常量
                tmp.append(SizeofToken(QStringList({"int"}),0,objInfo));
            }else if(funIndex!=-1){
                //是一个函数
                FunctionInfo *info = objInfo.getFunctionInfo(funIndex);
                tmp.append(SizeofToken(info->baseTypeIndex,0,QList<int>(),objInfo));
            }else{
                return QList<SizeofToken>();
            }
            currentIsValue = 1;
        }else if(sentence[i].isDelimiter("&") && !lastIsValue){//token中符号表示为(&)
            //取地址
            tmp.append(SizeofToken("(&)",objInfo));
        }else if(sentence[i].isDelimiter("*") && !lastIsValue){//token中符号表示为(*)
            //解引用
            tmp.append(SizeofToken("(*)",objInfo));
        }else if(i+1<sentence.length() &&
                 sentence[i].isDelimiter("->")&&
                 sentence[i+1].isIdentifier()){

            tmp.append(SizeofToken(1,sentence[i+1].text,objInfo));
            currentIsValue = 1;
            i+=1;
        }else if(i+1<sentence.length() &&
                 sentence[i].isDelimiter(".")&&
                 sentence[i+1].isIdentifier()){
            tmp.append(SizeofToken(0,sentence[i+1].text,objInfo));
            currentIsValue = 1;
            i+=1;
        }else {
            return QList<SizeofToken>();
        }
        lastIsValue = currentIsValue;
    }
    suceess = 1;
    return tmp;
}

//sizeof中是一个变量
static int sizeofOperator_Var(PhraseList sentence,SrcObjectInfo &objInfo,bool&status){
    //将PhraseList转为sizeof运算方便的token
    QList<SizeofToken> tokens = getSizeofTokens(sentence,objInfo,status);
    if(!status || tokens.length() == 0)return 0;

    status = 0;
    //计算最终sizeof的是一个什么数据类型
    SizeofToken result = getSizeofDataType(tokens);

    if(!result.isValue())return 0;

    int typeBytes;
    if(result.valueInfo.pointerLevel){
        typeBytes = objInfo.CPU_BIT / 8;
    }else{
        typeBytes = objInfo.getDataTypeInfoNode()->getDataType(result.valueInfo.baseType)->dataBaseTypeBytes;
    }
    for(int i = 0;i<result.valueInfo.arraySize.length();i++){
        typeBytes *= result.valueInfo.arraySize[i];
    }
    status = 1;
    return typeBytes;
}

//sizeof运算的结果的解析
static int sizeofOperator(PhraseList &sentence,SrcObjectInfo &objInfo,bool&status){
    int size = sizeofOperator_Type(sentence,objInfo,status);
    if(status){
        return size;
    }
    return sizeofOperator_Var(sentence,objInfo,status);
}

//解析结构体/共用体中属性的定义信息
//structIndex:结构体/共用体的索引号
static bool analysisStructUnionType_AttDef(int attIndex,IndexVN structIndex,PhraseList &sentence,SrcObjectInfo &objInfo,
                                           uint &maxAttSize,//当前结构体中最大的c语言基础数据类型的总字节数
                                           uint &offsetAddress//当前结构体中最后一个属性中最后一个字节存放的偏移地址+1
                                        ){
    if(sentence.getDelimiterNum("="))return 0;
    QList<PhraseList> pls = sentence.splitFromDelimiter(",");
    if(pls.length()==0)return 0;

    PhraseList defineVarTypePls;

    enum DefineVarType{
        DefineFun,
        DefineFunPtr,
        DefineVar
    };

    struct DefineVarPlsInfo{
        PhraseList pls;
        DefineVarType type;
    };
    QList<DefineVarPlsInfo> defineVarInfos;

    DefineVarPlsInfo tmp;
    QList<Phrase> pl;
    if(pls[0].length()>=2 && pls[0][pls[0].length()-1].isDelimiter("()")&&
       pls[0][pls[0].length()-2].isDelimiter("()")){
        tmp.type = DefineFunPtr;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*") || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else if(pls[0].length()>=2 &&pls[0][pls[0].length()-1].isDelimiter("()")&&
             pls[0][pls[0].length()-2].isIdentifier()){
        tmp.type = DefineFun;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int j = pls[0].length()-1;j>=0;j--){
            if(pls[0][j].isDelimiter("*")  || pls[0][j].isKeyWord("const")){
                tmp.pls.prepend(pls[0][j]);
                pls[0].removeLast();
            }
        }
        defineVarInfos.append(tmp);
    }else{
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("[]")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(pls[0].length()>=1 && pls[0][pls[0].length()-1].isIdentifier()){
            if(pls[0].length()>=2 &&
                    !pls[0][pls[0].length()-2].isKeyWord("struct")&&
                    !pls[0][pls[0].length()-2].isKeyWord("union")&&
                    !pls[0][pls[0].length()-2].isKeyWord("enum")){
                tmp.pls.prepend(pls[0][pls[0].length()-1]);
                pls[0].removeLast();
            }
        }
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*")  || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(tmp.pls.length()!=0){
            tmp.type = DefineVar;
            defineVarInfos.append(tmp);
        }
    }
    defineVarTypePls = pls[0];
    for(int i = 1;i<pls.length();i++){
        DefineVarPlsInfo tmp;
        if(pls[i].length()==2 && pls[i][pls[i].length()-1].isDelimiter("()")&&
           pls[i][pls[i].length()-2].isDelimiter("()")){
            tmp.type = DefineFunPtr;
        }else if(pls[i].length()>=2 &&pls[i][pls[i].length()-1].isDelimiter("()")&&
                 pls[i][pls[i].length()-2].isIdentifier()){
            tmp.type = DefineFun;
        }else{
            tmp.type = DefineVar;
        }
        tmp.pls = pls[i];
        defineVarInfos.append(tmp);
    }


    bool isConst = defineVarTypePls.getKeyWordNum("const");//是否是常量类型
    int pointerLevel = 0;//类型的基础指针级数
    IndexVN typeIndex;//类型索引号
    defineVarTypePls.removeKeyWord("const");

    bool isDefaultPointerConst = 0;

    //解析使用的类型
    if(defineVarTypePls[0].isKeyWord("struct") || defineVarTypePls[0].isKeyWord("union") || defineVarTypePls[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(defineVarTypePls.length()==2 && !defineVarTypePls[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!defineVarTypePls[1].isIdentifier())return 0;
            if(defineVarTypePls[0].isKeyWord("struct")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("union")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(defineVarTypePls[1].text);
            }
            if(typeIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(defineVarTypePls[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(defineVarTypePls[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(defineVarTypePls[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = defineVarTypePls[1].text;
                enumInfo.isOnlyStatement = 1;
                typeIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }

        }else if((defineVarTypePls.length()==2 && defineVarTypePls[1].isDelimiter("{}")) ||
                 (defineVarTypePls.length()==3 && defineVarTypePls[1].PhraseType==Phrase::Identifier && defineVarTypePls[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = analysisEnumTypeDef(defineVarTypePls,objInfo);
            }else{
                typeIndex = analysisStructUnionTypeDef(defineVarTypePls,objInfo);
            }
            if(typeIndex.isEmpty())return 0;
        }else{
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!defineVarTypePls.isOnlyHaveKeyWordAndIdentifier())return 0;

        //被重定义类型的数据类型名
        QStringList oyName = defineVarTypePls.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        typeIndex = map.index;
        isConst = isConst || map.isDefaultConst;
        isDefaultPointerConst = map.isDefaultPointerConst;
        pointerLevel = map.pointerLevel;
    }

    if(typeIndex.isEmpty()){
        return 0;
    }

    DataTypeInfo *structPtr = objInfo.getRootDataTypeInfoNode()->getDataType(structIndex);



    for(int i = 0;i<defineVarInfos.length();i++){
        StructUnion_AttInfo att;
        QString name;
        DataTypeInfo *attTypePtr;
        uint attSize = 0;
        uint attBaseSize = 0;
        if(defineVarInfos[i].type==DefineFun){
            return 0;
        }else if(defineVarInfos[i].type==DefineFunPtr){
            bool isHaveName;
            int fun_pointerLevel;
            QList<int> fun_ptr_array_size;
            bool isAutoArrSize;
            bool isPointerConst;
            IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(typeIndex,
                                                                 pointerLevel,
                                                                 defineVarInfos[i].pls,
                                                                 objInfo,
                                                                 name,
                                                                 isHaveName,
                                                                 isPointerConst,
                                                                 fun_pointerLevel,
                                                                 fun_ptr_array_size,
                                                                 isAutoArrSize);
            if(funPtrTypeIndex.isEmpty() || isAutoArrSize)return 0;
            if(isHaveName==0)continue;
            att.isConst = isConst;
            att.isPointerConst = isDefaultPointerConst || isPointerConst;
            att.arraySize = fun_ptr_array_size;
            att.pointerLevel = fun_pointerLevel;
            att.typeIndex = funPtrTypeIndex;
            att.attIndex = attIndex;
            attTypePtr = objInfo.getRootDataTypeInfoNode()->getDataType(funPtrTypeIndex);
            attSize = attTypePtr->dataBaseTypeBytes;

            attBaseSize = attSize;

            for(int i = 0;i<att.arraySize.length();i++){
                attSize*=att.arraySize[i];
            }
        }else if(defineVarInfos[i].type==DefineVar){
            bool isHaveName;
            int var_pointerLevel;
            QList<int> var_array_size;
            bool status;
            bool isAutoArrSize;
            bool isHavePrefixConst;
            bool isHaveSuffixConst;
            name = analysisDefVarOrAtt_Name_Ptr_Arr(defineVarInfos[i].pls,
                                                            objInfo,var_pointerLevel,var_array_size,isHaveName,isAutoArrSize,
                                                            isHavePrefixConst,isHaveSuffixConst,
                                                            status);
            if(status==0 || isAutoArrSize)return 0;
            if(isHaveName==0)continue;
            att.pointerLevel = var_pointerLevel + pointerLevel;
            if(att.pointerLevel){
                att.isConst = isConst || isHavePrefixConst;
                att.isPointerConst = isHaveSuffixConst || isDefaultPointerConst;
            }else{
                att.isConst = isConst || isHavePrefixConst || isDefaultPointerConst;
                att.isPointerConst = 0;
            }
            att.arraySize = var_array_size;
            att.typeIndex = typeIndex;
            att.attIndex = attIndex;
            attTypePtr = objInfo.getRootDataTypeInfoNode()->getDataType(att.typeIndex);

            if(attTypePtr->isOnlyStatement && att.pointerLevel==0){
                return 0;
            }

            if(att.pointerLevel>0){
                attBaseSize = objInfo.CPU_BIT/8;
                attSize = attBaseSize;
            }else{
                attSize = attTypePtr->dataBaseTypeBytes;
                if(attTypePtr->baseType == DataBaseType_STRUCT || attTypePtr->baseType == DataBaseType_UNION){
                    attBaseSize = attTypePtr->maxAttSize;

                }else{
                    attBaseSize = attTypePtr->dataBaseTypeBytes;
                }
            }


            for(int i = 0;i<att.arraySize.length();i++){
                attSize*=att.arraySize[i];
            }
        }else return 0;

        if(attBaseSize > maxAttSize){
            maxAttSize = attBaseSize;
        }

        bool noAlignment = offsetAddress % attBaseSize;
        uint assignAdd = ((offsetAddress / attBaseSize)+noAlignment) * attBaseSize;//计算出当前属性分配的地址
        att.offset = assignAdd;
        if(structPtr->baseType!=DataBaseType_UNION){
            offsetAddress = assignAdd + attSize;
        }
        structPtr->attInfos.insert(name,att);
    }
    bool noAlignment = offsetAddress % maxAttSize;
    uint assignAdd = ((offsetAddress / maxAttSize)+noAlignment) * maxAttSize;
    structPtr->dataBaseTypeBytes = assignAdd;
    structPtr->maxAttSize = maxAttSize;
    return 1;
}
//解析出一个结构体/共用体的定义信息
//如果解析成功,注入到SrcAllDataTypeTree中,并返回类型的指针
static IndexVN analysisStructUnionTypeDef(PhraseList &sentence,SrcObjectInfo &objInfo){
    IndexVN index;
    bool isStructOrUnio;//1为union
    if(sentence[0].isKeyWord("struct")){
        isStructOrUnio = 0;
    }else if(sentence[0].isKeyWord("union")){
        isStructOrUnio = 1;
    }else{
        return index;
    }
    sentence.removeFirst();
    QString structName;//定义的结构名称
    bool isAnnony = 1;//是否匿名
    if(sentence[0].isIdentifier()){
        isAnnony = 0;
        structName = sentence[0].text;
        sentence.removeFirst();
    }

    //在SrcAllDataTypeTree中声明该结构体
    DataTypeInfo *ptr = NULL;
    if(isAnnony){
        //如果是匿名的必须要定义
        //如果该类型不存在,将要定义的类型先声明
        DataTypeInfo enumInfo;
        enumInfo.baseType = isStructOrUnio ? DataBaseType_UNION : DataBaseType_STRUCT;
        enumInfo.isAnnony = isAnnony;
        enumInfo.structName = structName;
        enumInfo.dataBaseTypeBytes = 0;
        enumInfo.isOnlyStatement = 1;
        index = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
        ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
    }else{
        isStructOrUnio ?
                    index = objInfo.getDataTypeInfoNode()->searchUnionType(structName):
                    index = objInfo.getDataTypeInfoNode()->searchStructType(structName) ;
        if(index.isEmpty()){
            //如果该类型不存在,将要定义的类型先声明
            DataTypeInfo enumInfo;
            enumInfo.baseType = isStructOrUnio ? DataBaseType_UNION : DataBaseType_STRUCT;
            enumInfo.isAnnony = isAnnony;
            enumInfo.structName = structName;
            enumInfo.dataBaseTypeBytes = 0;
            enumInfo.isOnlyStatement = 1;
            index = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
        }else{
            //如果类型已存在,判断是否仅仅声明了,如果已经被定义了报错退出
            ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
            if(!ptr->isOnlyStatement){
                index.clear();
                return IndexVN();
            }
        }
    }

    if(!sentence[0].isDelimiter("{}"))return IndexVN();
    PhraseList tmp = sentence[0].subPhraseList;
    if(tmp.length()==0)return IndexVN();
    if(!tmp[tmp.length()-1].isDelimiter(";"))return IndexVN();
    QList<PhraseList> attDefineS = tmp.splitFromDelimiter(";");
    objInfo.startAnalysisSturctUnionAtt();

    uint maxAttSize = 0;//当前结构体中最大的c语言基础数据类型的总字节数
    uint offsetAddress = 0;//当前结构体中最后一个属性中最后一个字节存放的偏移地址+1
    for(int i = 0;i<attDefineS.length();i++){
        //定义属性
        if(attDefineS[i].getKeyWordNum("typedef")){
            return IndexVN();//结构体中不允许定义一个数据类型
        }else{
            //定义一个属性
            if(analysisStructUnionType_AttDef(i,index,attDefineS[i],objInfo,maxAttSize,offsetAddress)==0){
                return IndexVN();
            }
        }
    }

    objInfo.getRootDataTypeInfoNode()->getDataType(index)->isOnlyStatement = 0;
    objInfo.endAnalysisSturctUnionAtt();
    //在SrcAllDataTypeTree中定义该结构体
    return index;
}
//解析出一个枚举的定义信息,如果失败了返回空
static IndexVN analysisEnumTypeDef(PhraseList &sentence,SrcObjectInfo &objInfo){
    sentence.removeFirst();
    IndexVN index;

    QString structName;//定义的结构名称
    bool isAnnony = 1;//是否匿名
    if(sentence[0].isIdentifier()){
        isAnnony = 0;
        structName = sentence[0].text;
        sentence.removeFirst();
    }
    PhraseList definePhrase = sentence[0].subPhraseList;

    QList<PhraseList> defineC = definePhrase.splitFromDelimiter(",");
    if(defineC.length()==0)return IndexVN();

    DataTypeInfo *ptr = NULL;

    if(isAnnony){
        //如果是匿名的必须要定义
        //如果该类型不存在,将要定义的类型先声明
        DataTypeInfo enumInfo;
        enumInfo.baseType = DataBaseType_ENUM;
        enumInfo.isAnnony = isAnnony;
        enumInfo.structName = structName;
        enumInfo.dataBaseTypeBytes = 4;
        enumInfo.isOnlyStatement = 1;
        index = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
        ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
    }else{
        index = objInfo.getDataTypeInfoNode()->searchEnumType(structName);
        if(index.isEmpty()){
            //如果该类型不存在,将要定义的类型先声明
            DataTypeInfo enumInfo;
            enumInfo.baseType = DataBaseType_ENUM;
            enumInfo.isAnnony = isAnnony;
            enumInfo.structName = structName;
            enumInfo.dataBaseTypeBytes = 4;
            enumInfo.isOnlyStatement = 1;
            index = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
        }else{
            //如果类型已存在,判断是否仅仅声明了,如果已经被定义了报错退出
            ptr=objInfo.getDataTypeInfoNode()->getDataType(index);
            if(!ptr->isOnlyStatement){
                index.clear();
                return IndexVN();
            }
        }
    }


    //解析出的枚举常量值
    QMap<QString,int> enumConstValue;
    int thisValue = 0;//默认情况下,定义的枚举值从0开始累加,此处暂存累加的值
    for(int i = 0;i<defineC.length();i++){
        PhraseList &thisPl = defineC[i];
        if(thisPl.length()<1)return IndexVN();//出错
        int isSetConstValue = thisPl.getDelimiterNum("=");
        if(isSetConstValue==1 && thisPl.length()>=3 && thisPl[0].isIdentifier() && thisPl[1].isDelimiter("=")){//重置累加常量值
            if(!objInfo.judegDefineVarNameIsCopce(thisPl[0].text))return IndexVN();
            QList<Phrase> oper = thisPl.mid(2,thisPl.length()-2);
            PhraseList tmp = oper;
            bool status;
            int constValue = analysisConstValueOperator(tmp,objInfo,status);
            if(!status)return IndexVN();

            ptr->enumConstValue.insert(thisPl[0].text,constValue);
            thisValue = constValue+1;
        }else if(isSetConstValue==0 && thisPl.length()==1 && thisPl[0].isIdentifier()){//不重置累加常量值
            if(!objInfo.judegDefineVarNameIsCopce(thisPl[0].text))return index;
            ptr->enumConstValue.insert(thisPl[0].text,thisValue);
            thisValue++;
        }else return IndexVN();//出错
    }
    ptr->isOnlyStatement = 0;//定义完成
    return index;
}

//解析定义新数据类型的语句
static bool analysisTypeDefineSentence(PhraseList &sentence,
                                SrcObjectInfo &objInfo){
    sentence.removeLast();
    if(sentence.judgeIsHaveAnyKeyWord({"if",
                                      "else",
                                      "while",
                                      "for",
                                      "goto",
                                      "continue",
                                      "break",
                                      "return",
                                      "switch",
                                      "case",
                                      "default",
                                      "extern",
                                      "static",
                                      "auto"})){
        //语句中存在不合语法的关键字,出错
        objInfo.appendPrompt("语句中存在不合语法的关键字",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }
    //解析出定义的变量信息
    QList<PhraseList> pls = sentence.splitFromDelimiter(",");
    if(pls.length()==0){
        objInfo.appendPrompt("定义数据类型的语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }

    PhraseList defineVarTypePls;

    enum DefineVarType{
        DefineFun,
        DefineFunPtr,
        DefineVar
    };

    struct DefineVarPlsInfo{
        PhraseList pls;
        DefineVarType type;
    };
    QList<DefineVarPlsInfo> defineVarInfos;

    DefineVarPlsInfo tmp;
    QList<Phrase> pl;
    if(pls[0].length()>=2 && pls[0][pls[0].length()-1].isDelimiter("()")&&
       pls[0][pls[0].length()-2].isDelimiter("()")){
        tmp.type = DefineFunPtr;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else if(pls[0].length()>=2 &&pls[0][pls[0].length()-1].isDelimiter("()")&&
             pls[0][pls[0].length()-2].isIdentifier()){
        tmp.type = DefineFun;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int j = pls[0].length()-1;j>=0;j--){
            if(pls[0][j].isDelimiter("*")){
                tmp.pls.prepend(pls[0][j]);
                pls[0].removeLast();
            }
        }
        defineVarInfos.append(tmp);
    }else{
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("[]")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(pls[0].length()>=1 && pls[0][pls[0].length()-1].isIdentifier()){
            if(pls[0].length()>=2 &&
                    !pls[0][pls[0].length()-2].isKeyWord("struct")&&
                    !pls[0][pls[0].length()-2].isKeyWord("union")&&
                    !pls[0][pls[0].length()-2].isKeyWord("enum")){
                tmp.pls.prepend(pls[0][pls[0].length()-1]);
                pls[0].removeLast();
            }
        }
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(tmp.pls.length()!=0){
            tmp.type = DefineVar;
            defineVarInfos.append(tmp);
        }
    }
    defineVarTypePls = pls[0];
    for(int i = 1;i<pls.length();i++){
        DefineVarPlsInfo tmp;
        if(pls[i].length()==2 && pls[i][pls[i].length()-1].isDelimiter("()")&&
           pls[i][pls[i].length()-2].isDelimiter("()")){
            tmp.type = DefineFunPtr;
        }else if(pls[i].length()>=2 &&pls[i][pls[i].length()-1].isDelimiter("()")&&
                 pls[i][pls[i].length()-2].isIdentifier()){
            tmp.type = DefineFun;
        }else{
            tmp.type = DefineVar;
        }
        tmp.pls = pls[i];
        defineVarInfos.append(tmp);
    }
    defineVarTypePls.removeKeyWord("typedef");

    bool isConst = defineVarTypePls.getKeyWordNum("const");//是否是常量类型
    int pointerLevel = 0;//类型的基础指针级数
    IndexVN typeIndex;//类型索引号
    defineVarTypePls.removeKeyWord("const");

    bool isDefaultPointerConst = 0;

    //解析使用的类型
    if(defineVarTypePls[0].isKeyWord("struct") || defineVarTypePls[0].isKeyWord("union") || defineVarTypePls[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(defineVarTypePls.length()==2 && !defineVarTypePls[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!defineVarTypePls[1].isIdentifier()){
                objInfo.appendPrompt("定义数据类型的语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            if(defineVarTypePls[0].isKeyWord("struct")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("union")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(defineVarTypePls[1].text);
            }
            if(typeIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(defineVarTypePls[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(defineVarTypePls[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(defineVarTypePls[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = defineVarTypePls[1].text;
                enumInfo.isOnlyStatement = 1;
                typeIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }

        }else if((defineVarTypePls.length()==2 && defineVarTypePls[1].isDelimiter("{}")) ||
                 (defineVarTypePls.length()==3 && defineVarTypePls[1].PhraseType==Phrase::Identifier && defineVarTypePls[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = analysisEnumTypeDef(defineVarTypePls,objInfo);
            }else{
                typeIndex = analysisStructUnionTypeDef(defineVarTypePls,objInfo);
            }
            if(typeIndex.isEmpty()){
                objInfo.appendPrompt("定义数据类型语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
        }else{
            objInfo.appendPrompt("定义数据类型语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!defineVarTypePls.isOnlyHaveKeyWordAndIdentifier()){
            objInfo.appendPrompt("定义数据类型语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }

        //被重定义类型的数据类型名
        QStringList oyName = defineVarTypePls.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        if(!status){
            objInfo.appendPrompt("指定的数据类型未定义",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
        typeIndex = map.index;
        isConst = isConst || map.isDefaultConst;
        isDefaultPointerConst = map.isDefaultPointerConst;
        pointerLevel = map.pointerLevel;
    }

    bool isHaveError = 0;
    for(int i = 0;i<defineVarInfos.length();i++){
        if(defineVarInfos[i].pls.length()==0){
            isHaveError = 1;
            objInfo.appendPrompt("数据类型定义失败",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            continue;
        }
        int thisLine = defineVarInfos[i].pls[0].line;
        int thisCol = defineVarInfos[i].pls[0].col;
        QString thisSrcPath = defineVarInfos[i].pls[0].srcPath;


        if(defineVarInfos[i].type==DefineFun){
            isHaveError = 1;
            objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
            continue;
        }else if(defineVarInfos[i].type==DefineFunPtr){
            QString name;
            bool isHaveName;
            int fun_pointerLevel;
            QList<int> fun_ptr_array_size;
            bool isAutoArrSize;
            bool isPointerConst;
            IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(typeIndex,
                                                                 pointerLevel,
                                                                 defineVarInfos[i].pls,
                                                                 objInfo,
                                                                 name,
                                                                 isHaveName,
                                                                 isPointerConst,
                                                                 fun_pointerLevel,
                                                                 fun_ptr_array_size,isAutoArrSize);
            if(funPtrTypeIndex.isEmpty()){
                isHaveError = 1;
                objInfo.appendPrompt("函数参数定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }

            if(isAutoArrSize){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(fun_ptr_array_size.length()!=0){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }

            int index = funPtrTypeIndex.getEndIndex();

            if(!objInfo.getDataTypeInfoNode()->appendNewTypeName(name,//类型名
                                       funPtrTypeIndex,//绑定到的表
                                       index,//表中的哪个类型
                                       0,
                                       isPointerConst || isDefaultPointerConst,
                                       fun_pointerLevel)){
                isHaveError = 1;
                objInfo.appendPrompt("该数据类型已存在,不可重复定义",thisSrcPath,thisLine,thisCol);
                continue;
            }


        }else if(defineVarInfos[i].type==DefineVar){

            bool isHaveName;
            int var_pointerLevel;
            QList<int> var_array_size;
            bool status;
            bool isAutoArrSize;
            bool isHavePrefixConst;
            bool isHaveSuffixConst;
            QString name = analysisDefVarOrAtt_Name_Ptr_Arr(defineVarInfos[i].pls,
                                                            objInfo,var_pointerLevel,
                                                            var_array_size,
                                                            isHaveName,
                                                            isAutoArrSize,
                                                            isHavePrefixConst,
                                                            isHaveSuffixConst,
                                                            status);
            if(status==0){
                isHaveError = 1;
                objInfo.appendPrompt("变量定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }

            if(isAutoArrSize){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(var_array_size.length()!=0){
                isHaveError = 1;
                objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
                continue;
            }

            int index = typeIndex.getEndIndex();

            if(!objInfo.getDataTypeInfoNode()->appendNewTypeName(name,//类型名
                                       typeIndex,//绑定到的表
                                       index,//表中的哪个类型
                                       isConst || isHavePrefixConst,
                                       var_pointerLevel + pointerLevel,
                                       isDefaultPointerConst || isHaveSuffixConst)){
                isHaveError = 1;
                objInfo.appendPrompt("该数据类型已存在,不可重复定义",thisSrcPath,thisLine,thisCol);
                continue;
             }

        }else{
            isHaveError = 1;
            objInfo.appendPrompt("不符合定义数据类型的语法",thisSrcPath,thisLine,thisCol);
            continue;
        }

    }
    return !isHaveError;
}

//解析出初始值要赋值到全局变量的偏移地址位置,以及此处的偏移地址的变量类型
struct InitValuePlace{
    int offset = 0;//偏移地址
    int isNoInitArray = 0;//是否是非初始化数组块>=1为是,数值为字节数
    IndexVN typeIndex;//该偏移地址处的数据类型
    bool isStringPtr = 0;//是否是一个字符串指针
    int pointerLevel = 0;//是否是指针
    int isStringArray = 0;//类型是否是字符串数组,如果是字符串数组,字符串占用的字节数
    int strArraySize = 0;//如果是字符串数组,字符串数组允许存放的总字节数
    PhraseList ext;//初始值的运算式
};

//解析出全局变量初始值的存储区域
static QList<InitValuePlace> analysisGlobalVarInitValuePlace(IndexVN typeIndex,
                                                      int pointerLevel,
                                                      int thisOffset,
                                                      QList<int>arrSize,
                                                      //c语言定义数组型变量时,支持最高维度的数组的单元数可不写,通过写入的元素来自动求出总单元数,如果要用该功能isAutoArrSize==1
                                                      bool isAutoArrSize,
                                                      PhraseList &sentence,
                                                      SrcObjectInfo &objInfo,
                                                      int &retAutoArrSize){
    if(typeIndex.length()<2)return QList<InitValuePlace>();
    DataTypeInfo * typeInfo = objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex);
    DataBaseType baseType = typeInfo->baseType;
    QList<InitValuePlace> placeList;
    if(arrSize.length()==1 &&
            (baseType==DataBaseType_CHAR || baseType==DataBaseType_UCHAR) &&
            pointerLevel==0 &&
            (sentence.length()==1 && sentence[0].PhraseType==Phrase::ConstValue_STRING)){
        //字符数组型,或者是字符串组
        InitValuePlace place;
        place.ext = sentence;

        place.offset = thisOffset;
        place.typeIndex = typeIndex;
        place.isStringArray = replaceTransChar(sentence[0].text).length();//
        place.strArraySize = arrSize[0];
        place.pointerLevel = pointerLevel;

        if(isAutoArrSize){
            retAutoArrSize = place.isStringArray;
            place.strArraySize = retAutoArrSize;
        }else if(place.strArraySize<place.isStringArray){
            return  QList<InitValuePlace>();
        }
        placeList.append(place);
        return placeList;
    }else if(pointerLevel==1 &&
             (baseType==DataBaseType_CHAR || baseType==DataBaseType_UCHAR) &&
             arrSize.length()==0 &&
             (sentence.length()==1 && sentence[0].PhraseType==Phrase::ConstValue_STRING)){
        //字符指针类型,或者是指向字符串常量的字符指针
        int strByteSize;
        QString strConstVarName = objInfo.appendAnonyConstStrValue(sentence[0].text,strByteSize);


        VarDefineInfo strConstVarInfo;
        strConstVarInfo.varName = strConstVarName;
        strConstVarInfo.isConst = 1;
        strConstVarInfo.isExtern = 0;
        strConstVarInfo.pointerLevel = 0;
        strConstVarInfo.isStatic = 1;
        strConstVarInfo.baseTypeIndex = typeIndex;
        strConstVarInfo.arraySize.append(strByteSize);
        IndexVN constStrIndex = objInfo.defineVar(strConstVarInfo);
        if(constStrIndex.isEmpty())return  QList<InitValuePlace>();
        InitValuePlace place;
        Phrase tmp;
        tmp.PhraseType = Phrase::Identifier;
        tmp.text = strConstVarName;
        place.ext.append(tmp);

        place.offset = thisOffset;
        place.typeIndex = typeIndex;
        place.isStringPtr = 1;
        place.isStringArray = 0;
        place.pointerLevel = pointerLevel;
        placeList.append(place);
        return placeList;
    }else if(arrSize.length()!=0){
        //数组类型
        if(sentence.length()!=1)return QList<InitValuePlace>();
        if(!sentence[0].isDelimiter("{}"))return QList<InitValuePlace>();

        sentence = sentence[0].subPhraseList;

        if(sentence.length()==0){
            InitValuePlace noinit;
            noinit.offset = thisOffset;
            noinit.isNoInitArray = objInfo.getRootDataTypeInfoNode()->getTypeSize(typeIndex);
            if(pointerLevel!=0){
                noinit.isNoInitArray = objInfo.CPU_BIT/8;
            }
            for(int i = 0;i<arrSize.length();i++){
                noinit.isNoInitArray *= arrSize[i];
            }
            placeList.append(noinit);
            return placeList;
        }

        QList<PhraseList> attInitPhrases = sentence.splitFromDelimiter(",");
        if(attInitPhrases.length()==0)return QList<InitValuePlace>();


        int allUnitCount = 0;
        if(!isAutoArrSize){
            allUnitCount = arrSize[0];
        }
        int initIndex = 0;


        foreach(PhraseList attInitPhrase,attInitPhrases){
            int offset = 0;
            QList<int> attArraySize;

            int isSetInitIndex = attInitPhrase.getDelimiterNum("=");
            if(isSetInitIndex>=2)return QList<InitValuePlace>();
            if(isSetInitIndex){
                if(attInitPhrase.length()<3)return QList<InitValuePlace>();
                if(!attInitPhrase[1].isDelimiter("=") || !attInitPhrase[0].isDelimiter("[]"))return QList<InitValuePlace>();
                PhraseList indexPhrst = attInitPhrase[0].subPhraseList;
                bool status;
                initIndex = analysisConstValueOperator(indexPhrst,objInfo,status);
                if(status==0)return QList<InitValuePlace>();
                attInitPhrase.removeFirst();
                attInitPhrase.removeFirst();
            }
            if(!isAutoArrSize && initIndex>=allUnitCount){
                return QList<InitValuePlace>();
            }else if(initIndex>=allUnitCount && isAutoArrSize){
                allUnitCount = initIndex+1;
            }

            offset =  objInfo.getRootDataTypeInfoNode()->getTypeSize(typeIndex);
            if(pointerLevel!=0){
                offset = objInfo.CPU_BIT/8;
            }
            for(int i = 1;i<arrSize.length();i++){
                offset*=arrSize[i];
            }
            offset*=initIndex;
            offset += thisOffset;

            attArraySize = arrSize;
            attArraySize.removeFirst();
            int retAutoArrSize;
            QList<InitValuePlace> placeTemp = analysisGlobalVarInitValuePlace(typeIndex,
                                                                              pointerLevel,
                                                                              offset,
                                                                              attArraySize,
                                                                              0,
                                                                              attInitPhrase,
                                                                              objInfo,
                                                                              retAutoArrSize);
            if(placeTemp.length()==0)return QList<InitValuePlace>();
            placeList.append(placeTemp);
            initIndex++;
        }
        if(isAutoArrSize){
            retAutoArrSize = allUnitCount;
        }

        return placeList;
    }else if((baseType==DataBaseType_UNION || baseType==DataBaseType_STRUCT) && pointerLevel==0){

        //共用体/结构体类型
        if(sentence.length()!=1)return QList<InitValuePlace>();
        if(!sentence[0].isDelimiter("{}"))return QList<InitValuePlace>();
        sentence = sentence[0].subPhraseList;
        if(sentence.length()==0){
            InitValuePlace noinit;
            noinit.offset = thisOffset;
            noinit.isNoInitArray = objInfo.getRootDataTypeInfoNode()->getTypeSize(typeIndex);
            placeList.append(noinit);
            return placeList;
        }

        QList<PhraseList> attInitPhrases = sentence.splitFromDelimiter(",");
        if(attInitPhrases.length()==0)return QList<InitValuePlace>();


        int attIndex = 0;
        foreach(PhraseList attInitPhrase,attInitPhrases){
            int offset = 0;
            IndexVN attTypeIndex;
            QList<int> attArraySize;
            int attPointerLevel = 0;
            PhraseList ext;
            if(!attInitPhrase.getDelimiterNum("=")){
                bool isHaveAtt;
                StructUnion_AttInfo attInfo = typeInfo->structGetAttFromIndex(attIndex,isHaveAtt);
                if(isHaveAtt==0)return QList<InitValuePlace>();
                attTypeIndex = attInfo.typeIndex;
                attArraySize = attInfo.arraySize;
                attPointerLevel = attInfo.pointerLevel;
                offset = attInfo.offset + thisOffset;
                ext = attInitPhrase;
                attIndex++;
            }else{
                QList<PhraseList> tmpPhraseList =attInitPhrase.splitFromDelimiter("=");
                if(tmpPhraseList.length()!=2)return QList<InitValuePlace>();

                ext = tmpPhraseList[1];
                attInitPhrase = tmpPhraseList[0];

                if(attInitPhrase.length()<=1)return QList<InitValuePlace>();
                if(!attInitPhrase[0].isDelimiter(".") || attInitPhrase[attInitPhrase.length()-1].isDelimiter("."))return QList<InitValuePlace>();
                attInitPhrase.removeFirst();
                tmpPhraseList = attInitPhrase.splitFromDelimiter(".");

                //tmpPhraseList:要操作的属性的名称列表
                //ext:运算数
                DataTypeInfo * attTypeInfo = typeInfo;
                for(int i = 0;i<tmpPhraseList.length();i++){
                    if(attArraySize.length()!=0 || attPointerLevel!=0)return QList<InitValuePlace>();
                    QList<int> arrIndexs;
                    for(int j = tmpPhraseList[i].length()-1;j>=0;j--){
                        if(tmpPhraseList[i][j].isDelimiter("[]")){
                            PhraseList tmp =tmpPhraseList[i][j].subPhraseList;
                            bool status;
                            int arrIndex = analysisConstValueOperator(tmp,objInfo,status);
                            if(status==0)return QList<InitValuePlace>();
                            arrIndexs.prepend(arrIndex);
                            tmpPhraseList[i].removeLast();
                        }else{
                            break;
                        }
                    }

                    if(tmpPhraseList[i].length()!=1)return QList<InitValuePlace>();
                    if(!tmpPhraseList[i][0].isIdentifier())return QList<InitValuePlace>();
                    QString attName = tmpPhraseList[i][0].text;
                    bool isHaveAtt = attTypeInfo->attInfos.contains(attName);
                    if(!isHaveAtt){
                        return QList<InitValuePlace>();
                    }
                    StructUnion_AttInfo attInfo = attTypeInfo->attInfos[attName];
                    if(i==0){
                        attIndex = attInfo.attIndex+1;
                    }

                    attTypeIndex = attInfo.typeIndex;
                    attPointerLevel = attInfo.pointerLevel;
                    attTypeInfo = objInfo.getRootDataTypeInfoNode()->getDataType(attTypeIndex);
                    offset+=attInfo.offset;

                    int unitSize = attTypeInfo->dataBaseTypeBytes;
                    if(attPointerLevel!=0){
                        unitSize = objInfo.CPU_BIT/8;
                    }
                    while(arrIndexs.length()!=0){
                        int arrOffset = unitSize;
                        if(attInfo.arraySize.length()==0)return QList<InitValuePlace>();
                        if(arrIndexs[0]>=attInfo.arraySize[0])return QList<InitValuePlace>();
                        for(int j = 1;j<attInfo.arraySize.length();j++){
                            arrOffset*=attInfo.arraySize[j];
                        }
                        arrOffset*=arrIndexs[0];
                        offset+=arrOffset;
                        attInfo.arraySize.removeFirst();
                        arrIndexs.removeFirst();
                    }

                    if(attInfo.arraySize.length()!=0){
                        attArraySize = attInfo.arraySize;
                    }
                }
                offset += thisOffset;

            }

            int retAutoArrSize;
            QList<InitValuePlace> placeTemp = analysisGlobalVarInitValuePlace(attTypeIndex,
                                                                              attPointerLevel,
                                                                              offset,
                                                                              attArraySize,
                                                                              0,
                                                                              ext,
                                                                              objInfo,
                                                                              retAutoArrSize);
            if(placeTemp.length()==0)return QList<InitValuePlace>();
            placeList.append(placeTemp);
        }
        return placeList;
    }else{
        //普通变量/enum/指针类型
        InitValuePlace place;
        place.ext = sentence;
        place.offset = thisOffset;
        place.typeIndex = typeIndex;
        place.isStringArray = 0;
        place.strArraySize = 0;
        place.pointerLevel = pointerLevel;
        placeList.append(place);
        return placeList;
    }
}

enum ConstOperatorType{
    ConstOperatorType_ConstValue,//是个常量值
    ConstOperatorType_Operator,//+-*/之类的运算符
    ConstOperatorType_bracket,//()子表达式
    ConstOperatorType_ConverType,//强转类型
};
struct GlobalInitValueOperator{
    ConstOperatorType operatorType;

    //如果是常量值,存储常量值
    GlobalInitValueTmp constValue;

    //如果是+*/类的运算符,运算符的字符串
    QString operatorStr;

    //如果是()子表达式,存储子表达式的内容
    QList<GlobalInitValueOperator> brackOperator;

    //如果是强转类型,就存储要强转的数据类型
    ConstValueType converType;
    int converPointerBytes;//如果是强转的指针类型,指针指向的数据的字节数

    bool isOperator(QString oper){
        if(operatorType!=ConstOperatorType_Operator)return 0;

        return operatorStr==oper;
    }


    QString converTypeToString(){
        switch (converType) {
            case ConstValueType_INT64:return "(INT64)";
            case ConstValueType_INT32:return "(INT32)";
            case ConstValueType_INT16:return "(INT16)";
            case ConstValueType_INT8:return "(INT8)";
            case ConstValueType_UINT64:return "(UINT64)";
            case ConstValueType_UINT32:return "(UINT32)";
            case ConstValueType_UINT16:return "(UINT16)";
            case ConstValueType_UINT8:return "(UINT8)";
            case ConstValueType_FLOAT:return "(FLOAT32)";
            case ConstValueType_DOUBLE:return "(FLOAT64)";
            case ConstValueType_POINTER:return "(POINTER:"+QString::number(converPointerBytes)+"B)";
            default:return "ERROR";
        }
        return "ERROR";
    }

    QString toString(){
        QString txt;
        if(operatorType == ConstOperatorType_ConstValue){
            txt = constValue.toStirng();
        }else if(operatorType == ConstOperatorType_Operator){
            txt = operatorStr;
        }else if(operatorType == ConstOperatorType_bracket){
            txt += "(";
            for(int i = 0;i<brackOperator.length();i++){
                txt += brackOperator[i].toString();
            }
            txt += ")";
        }else if(operatorType == ConstOperatorType_ConverType){
            txt = converTypeToString();
        }
        return txt;
    }
};

//判断()内的是 强转类型符 还是 子表达式符 (如果是强转类型符号,返回强转的类型)
static ConstValueType judgeConverTypeOperator(SrcObjectInfo &objInfo,PhraseList &sentence,int getAddressOperatorIndex,int &converPointerBytes){
    if(getAddressOperatorIndex<0 || getAddressOperatorIndex>=sentence.length())return ConstValueType_ERROR;
    if(!sentence[getAddressOperatorIndex].isDelimiter("()"))return ConstValueType_ERROR;

    QList<Phrase> subPhrase = sentence[getAddressOperatorIndex].subPhraseList;
    if(subPhrase.length()==0)return ConstValueType_ERROR;

    int isPtr = subPhrase[subPhrase.length()-1].isDelimiter("*");
    if(isPtr){
        for(int i = subPhrase.length()-1;i>=1;i--){
            if(subPhrase[i-1].isDelimiter("*")){
                isPtr++;
                subPhrase.removeLast();
            }else{
                break;
            }
        }
    }

    IndexVN index;
    if(subPhrase.length()>=2 &&
        subPhrase[0].isKeyWord("enum")  && !objInfo.getDataTypeInfoNode()->searchEnumType(subPhrase[1].text).isEmpty()){
        if(subPhrase.length()==3 && isPtr){
            if(isPtr>=2){
                converPointerBytes = objInfo.CPU_BIT/8;
            }else{
                converPointerBytes = 4;
            }

            return ConstValueType_POINTER;
        }else if(subPhrase.length()==2){
            return ConstValueType_INT32;
        }else{
            return ConstValueType_ERROR;
        }
    }else if(subPhrase.length()==3 &&
             subPhrase[0].isKeyWord("struct") &&
             !(index=objInfo.getDataTypeInfoNode()->searchStructType(subPhrase[1].text)).isEmpty() &&
             isPtr){
        if(isPtr>=2){
            converPointerBytes = objInfo.CPU_BIT/8;
        }else{
            converPointerBytes = objInfo.getDataTypeInfoNode()->getTypeSize(index);
        }
        return ConstValueType_POINTER;
    }else if(subPhrase.length()==3 &&
             subPhrase[0].isKeyWord("union") &&
             !(index=objInfo.getDataTypeInfoNode()->searchUnionType(subPhrase[1].text)).isEmpty() &&
             isPtr){
        if(isPtr>=2){
            converPointerBytes = objInfo.CPU_BIT/8;
        }else{
            converPointerBytes = objInfo.getDataTypeInfoNode()->getTypeSize(index);
        }
        return ConstValueType_POINTER;
    }else{
        if(isPtr){
            subPhrase.removeLast();
        }
        if(subPhrase.length()==0){
            return ConstValueType_ERROR;
        }

        QStringList typeName;


        for(int i = 0;i<subPhrase.length();i++){
            if(subPhrase[i].PhraseType!=Phrase::KeyWord && !subPhrase[i].isIdentifier()){
                return ConstValueType_ERROR;
            }

            typeName.append(subPhrase[i].text);
        }

        typeName = stdFormatBaseTypeName(typeName);
        IndexVN typeIndex = objInfo.getDataTypeInfoNode()->searchDataType(typeName);

        if(typeIndex.isEmpty()){
            return ConstValueType_ERROR;
        }
        DataTypeInfo * typeInfo = objInfo.getDataTypeInfoNode()->getDataType(typeIndex);
        DataBaseType baseType = typeInfo->baseType;

        if((baseType == DataBaseType_STRUCT ||
           baseType==DataBaseType_UNION ||
           baseType==DataBaseType_VOID)){
            if(isPtr){
                if(isPtr>=2){
                    converPointerBytes = objInfo.CPU_BIT/8;
                }else{
                    converPointerBytes = typeInfo->dataBaseTypeBytes;
                }
                return ConstValueType_POINTER;
            }else{
                return ConstValueType_ERROR;
            }
        }else if(baseType == DataBaseType_FUNPTR){

            converPointerBytes = objInfo.CPU_BIT/8;
            return ConstValueType_POINTER;
        }else if(baseType == DataBaseType_ENUM){
            if(isPtr){
                if(isPtr>=2){
                    converPointerBytes = objInfo.CPU_BIT/8;
                }else{
                    converPointerBytes = 4;
                }
                return ConstValueType_POINTER;
            }
            else return ConstValueType_INT32;
        }else if(isPtr){
            if(isPtr>=2){
                converPointerBytes = objInfo.CPU_BIT/8;
            }else{
                converPointerBytes = typeInfo->dataBaseTypeBytes;
            }

            return ConstValueType_POINTER;
        }else if(baseType == DataBaseType_INT){
            return ConstValueType_INT32;
        }else if(baseType == DataBaseType_UINT){
            return ConstValueType_UINT32;
        }else if(baseType == DataBaseType_SHORT){
            return ConstValueType_INT16;
        }else if(baseType == DataBaseType_USHORT){
            return ConstValueType_UINT16;
        }else if(baseType == DataBaseType_CHAR){
            return ConstValueType_INT8;
        }else if(baseType == DataBaseType_UCHAR){
            return ConstValueType_UINT8;
        }else if(baseType == DataBaseType_LONG){
            return ConstValueType_INT64;
        }else if(baseType == DataBaseType_ULONG){
            return ConstValueType_UINT64;
        }else if(baseType == DataBaseType_DOUBLE){
            return ConstValueType_DOUBLE;
        }else if(baseType == DataBaseType_FLOAT){
            return ConstValueType_FLOAT;
        }
        return ConstValueType_ERROR;
    }
}

//判断&是 取地址符 还是 位运算符 (取地址符返回1)
static bool judgeIsGetAddressOperator(SrcObjectInfo &objInfo,PhraseList &sentence,int getAddressOperatorIndex){
    if(getAddressOperatorIndex<0 || getAddressOperatorIndex>=sentence.length())return 0;
    if(!sentence[getAddressOperatorIndex].isDelimiter("&"))return 0;

    if(getAddressOperatorIndex==0)return 1;

    int converPointerBytes;
    if(judgeConverTypeOperator(objInfo,sentence,getAddressOperatorIndex-1,converPointerBytes)!=ConstValueType_ERROR){
        //如果前面是一个强转类型符,那就必然是取地址符
        return 1;
    }else if(sentence[getAddressOperatorIndex-1].isDelimiter("()")||//如果前面是一个括号，并且已经排除了强转符的可能,那这个必然是位运算符
             sentence[getAddressOperatorIndex-1].isDelimiter("[]")){//[]后面只可能是位运算符
        return 0;
    }else if(sentence[getAddressOperatorIndex-1].PhraseType == Phrase::Delimiter){
        //所有常量运算表达式所支持的运算符(如果是在这些运算符后面有&,那么必然是取地址符)
        const static QStringList allDel ={
            "+","-","*","/","%",
            "|","&","~","^",
            "||","&&","!",
            ">",">=","<","<=","==","!=",
            "?",":"
        };
        return allDel.contains(sentence[getAddressOperatorIndex-1].text);
    }
    return 0;
}

//解析取址符取的变量的标记名、标记偏移地址、变量的字节数
struct PointInfo{
    //取址语句占用的词数(不包括&)
    int usePhraseCount = 0;//如果最终为0，说明取指语句有问题
    QString markName;
    int markOffset = 0;
    int pointDataBytes;
};

//getAddressOperatorIndex:取指语句开始的位置(&下一个词开始算)
static PointInfo analysisOperatorGetPointInfo(SrcObjectInfo &objInfo,PhraseList &sentence,int getAddressOperatorIndex){
    PointInfo info;
    if(getAddressOperatorIndex<=0 || getAddressOperatorIndex>=sentence.length())return info;
    if(!sentence[getAddressOperatorIndex].isIdentifier() &&
       !sentence[getAddressOperatorIndex].isDelimiter("()"))return info;

    /////////获取到取指语句的段落存在operatorPhr,并将段落占用的顶层Phrase数量存在usePhraseCount////////
    QList<Phrase> operatorPhr = sentence[getAddressOperatorIndex].expandAllSubPhrase("()");
    int usePhraseCount = 1;

    for(int i = getAddressOperatorIndex+1;i<sentence.length();i++){
        if(sentence[i].isIdentifier() || sentence[i].isDelimiter(".") || sentence[i].isDelimiter("[]")){
            operatorPhr.append(sentence[i]);
            usePhraseCount++;
        }else{
            break;
        }
    }


    /////////////解析取指语句的段落////////////////
    //获取要取地址的变量
    if(operatorPhr.length()==0 || !operatorPhr[0].isIdentifier())return info;
    IndexVN varIndex = objInfo.getVarDefineInfoNode()->searchVar(operatorPhr[0].text);
    if(varIndex.isEmpty())return info;
    VarDefineInfo * varObj = objInfo.getVarDefineInfoNode()->getVar(varIndex);


    IndexVN varBaseTypeIndex = varObj->baseTypeIndex;//变量的基础类型
    QList<int> varArraySize = varObj->arraySize;//变量的数组长度
    int varPointerLevel = varObj->pointerLevel;//变量指针级数

    info.markName = varObj->psdlName;
    int markOffset = 0;
    enum{
        Identifier,//变量或属性的名字
        GetAtt,// .
        GetUnit,// []吗
    }lastPhrType = Identifier;

    for(int i = 1;i<operatorPhr.length();i++){
        if(operatorPhr[i].isIdentifier() && lastPhrType==GetAtt){
            lastPhrType = Identifier;

            if(varPointerLevel!=0 || varArraySize.length()!=0){
                return info;
            }
            DataTypeInfo * typeInfo = objInfo.getDataTypeInfoNode()->getDataType(varBaseTypeIndex);
            if(typeInfo->baseType != DataBaseType_STRUCT &&
               typeInfo->baseType != DataBaseType_UNION){
                return info;
            }
            if(typeInfo->attInfos.contains(operatorPhr[i].text)){
                StructUnion_AttInfo attInfo = typeInfo->attInfos[operatorPhr[i].text];
                markOffset += attInfo.offset;

                varArraySize = attInfo.arraySize;
                varPointerLevel = attInfo.pointerLevel;
                varBaseTypeIndex = attInfo.typeIndex;
            }else{
                return info;
            }
        }else if(operatorPhr[i].isDelimiter(".") && (lastPhrType==Identifier || lastPhrType==GetUnit)){
            lastPhrType = GetAtt;
        }else if(operatorPhr[i].isDelimiter("[]") && (lastPhrType==Identifier || lastPhrType==GetUnit)){
            lastPhrType = GetUnit;
            if(varArraySize.length()==0 && varPointerLevel!=1){
                return info;
            }

            bool su;
            PhraseList opPhr = operatorPhr[i].subPhraseList;
            if(varArraySize.length()>0){

                int arrBytes = analysisConstValueOperator(opPhr,objInfo,su);
                if(!su)return info;
                for(int i = 1;i<varArraySize.length();i++){
                    arrBytes*=varArraySize[i];
                }

                if(varPointerLevel!=0){
                    arrBytes *= objInfo.CPU_BIT/8;
                }else{
                    arrBytes *= objInfo.getDataTypeInfoNode()->getDataType(varBaseTypeIndex)->dataBaseTypeBytes;
                }

                markOffset+=arrBytes;
                varArraySize.removeFirst();
            }else{
                int arrBytes = analysisConstValueOperator(opPhr,objInfo,su);
                if(!su)return info;
                arrBytes *= objInfo.getDataTypeInfoNode()->getDataType(varBaseTypeIndex)->dataBaseTypeBytes;

                markOffset+=arrBytes;
                varPointerLevel--;
            }
        }else{
            return info;
        }
    }
    info.markOffset = markOffset;

    int pointDataBytes;
    if(varPointerLevel!=0){
        pointDataBytes = objInfo.CPU_BIT/8;
    }else{
        pointDataBytes = objInfo.getDataTypeInfoNode()->getDataType(varBaseTypeIndex)->dataBaseTypeBytes;
    }

    for(int i = 0;i<varArraySize.length();i++){
        pointDataBytes*=varArraySize[i];
    }
    info.pointDataBytes = pointDataBytes;
    info.usePhraseCount = usePhraseCount;
    return info;
}

//将PhraseList词组序列转为GlobalInitValueOperator表达式
static QList<GlobalInitValueOperator> analysisGlobalInitValueOperator(PhraseList &sentence,
                                                               SrcObjectInfo &objInfo,
                                                               bool &isHaveGetAddress,
                                                               bool&status){
    QList<GlobalInitValueOperator> opers;
    isHaveGetAddress = 0;
    status = 0;
    for(int i = 0;i<sentence.length();i++){
        int converPointerBytes;
        ConstValueType converType = judgeConverTypeOperator(objInfo,sentence,i,converPointerBytes);//如果是强转类型运算符,存储强转的类型
        if(sentence[i].isKeyWord("sizeof") && i+1<sentence.length()){
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_UINT64;

            PhraseList sizeofPhr = sentence[i+1].subPhraseList;
            bool su;
            value.intData = sizeofOperator(sizeofPhr,objInfo,su);
            if(!su){
                return opers;
            }

            op.constValue = value;
            opers.append(op);
            i+=1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT32){
            //整型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_INT32;

            value.intData = sentence[i].text.toInt();

            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT64){
            //整型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_INT64;

            value.intData = sentence[i].text.toLongLong();

            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT64){
            //整型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_UINT64;

            value.intData = sentence[i].text.toULongLong();

            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT32){
            //整型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_UINT32;

            value.intData = sentence[i].text.toUInt();

            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT32){
            //浮点型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_FLOAT;
            value.floatData = sentence[i].text.toFloat();
            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT64){
            //浮点型常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_DOUBLE;
            value.floatData = sentence[i].text.toDouble();
            op.constValue = value;
            opers.append(op);
        }else if(sentence[i].PhraseType == Phrase::ConstValue_CHAR){
            //字符常量(默认数据类型为uint8)
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_UINT8;
            bool su;

            QString txt = sentence[i].text;
            char ch = analysisCharConstValue(txt,su);
            if(!su){
                return opers;
            }
            value.intData = ch;
            op.constValue = value;

            opers.append(op);
        }else if(sentence[i].isIdentifier()){
            //标识符,可能是enum常量或者是其它全局常量
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            //判断是否是一个变量
            IndexVN varIndex = objInfo.getVarDefineInfoNode()->searchVar(sentence[i].text);

            bool isEnumConstValue;
            int enumValue = objInfo.getDataTypeInfoNode()->getEnumConstValue(sentence[i].text,&isEnumConstValue);
            //判断是否是一个函数
            int funIndex = objInfo.serachFunction(sentence[i].text);



            if(!varIndex.isEmpty()){
                //是一个变量
                VarDefineInfo * info = objInfo.getVarDefineInfoNode()->getVar(varIndex);


                if(!info->globalVarIsInitValue)return opers;
                op.constValue = info->globalInitValue;


                if(op.constValue.factIsPointer){
                    isHaveGetAddress = 1;
                }
            }else if(isEnumConstValue){
                //是一个枚举
                GlobalInitValueTmp value;
                value.valueType = ConstValueType_INT32;
                value.intData = enumValue;
                op.constValue = value;
            }else if(funIndex!=-1){
                GlobalInitValueTmp value;
                value.factIsPointer = 1;
                value.valueType = ConstValueType_POINTER;
                value.pointerBytes = 0;
                value.pointerData_mark = objInfo.getFunctionInfo(funIndex)->functionName;
                value.pointerData_offset = 0;
                op.constValue = value;
            }else{
                return opers;
            }
            opers.append(op);
        }else if(sentence[i].isDelimiter("&") && judgeIsGetAddressOperator(objInfo,sentence,i)){
            if(i+1==sentence.length())return opers;
            //取地址符
            PointInfo pointInfo = analysisOperatorGetPointInfo(objInfo,sentence,i+1);
            if(pointInfo.usePhraseCount==0){
                return opers;
            }
            i+=pointInfo.usePhraseCount;

            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConstValue;

            GlobalInitValueTmp value;
            value.valueType = ConstValueType_POINTER;

            value.factIsPointer = 1;
            value.pointerBytes = pointInfo.pointDataBytes;
            value.pointerData_mark = pointInfo.markName;
            value.pointerData_offset = pointInfo.markOffset;
            op.constValue = value;
            opers.append(op);
            isHaveGetAddress = 1;
        }else if(sentence[i].isDelimiter("()") && converType!=ConstValueType_ERROR){
            //强转类型
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_ConverType;
            op.converType = converType;
            op.converPointerBytes =converPointerBytes;
            opers.append(op);
        }else if(sentence[i].isDelimiter("()")){
            //子表达式
            PhraseList sublist = sentence[i].subPhraseList;
            bool su;
            bool subOperIsHaveGetAddress = 0;
            QList<GlobalInitValueOperator> subOper =
                    analysisGlobalInitValueOperator(sublist,objInfo,subOperIsHaveGetAddress,su);
            if(!su){
                return opers;
            }
            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_bracket;
            op.brackOperator = subOper;
            opers.append(op);
            if(subOperIsHaveGetAddress){
                isHaveGetAddress = 1;
            }
        }else if(sentence[i].PhraseType == Phrase::Delimiter){
            QString delText = sentence[i].text;

            //所有常量运算表达式所支持的运算符
            const static QStringList allDel ={
                "+","-","*","/","%",
                "|","&","~","^",
                "||","&&","!",
                ">",">=","<","<=","==","!=","<<",">>",
                "?",":"
            };
            if(!allDel.contains(delText)){
                return opers;
            }

            GlobalInitValueOperator op;
            op.operatorType = ConstOperatorType_Operator;
            op.operatorStr = delText;
            opers.append(op);
        }
    }
    status = 1;
    return opers;
}

//运算GlobalInitValueOperator表达式(常量运算表达式),得出最终的初始值结果
static GlobalInitValueTmp runGlobalInitConstValueOperator(QList<GlobalInitValueOperator> &operators){
    GlobalInitValueTmp retValue;
    if(operators.length()==0)return retValue;

    //运算括号内表达式
    for(int i = 0;i<operators.length();i++){
        if(operators[i].operatorType == ConstOperatorType_bracket){
            GlobalInitValueTmp subValue = runGlobalInitConstValueOperator(operators[i].brackOperator);
            if(subValue.valueType == ConstValueType_ERROR){
                return retValue;
            }

            operators[i].operatorType = ConstOperatorType_ConstValue;
            operators[i].constValue = subValue;
        }
    }

    //运算(类型强转)
    while(1){
        QList<int> converTypeOperIndexs;
        for(int i = 0;i<operators.length()-1;i++){
            if(operators[i].operatorType == ConstOperatorType_ConverType&&
               operators[i+1].operatorType == ConstOperatorType_ConstValue){
                converTypeOperIndexs.append(i);
            }
        }
        if(converTypeOperIndexs.length()==0)break;


        for(int i = 0;i<converTypeOperIndexs.length();i++){
            int index = converTypeOperIndexs[i];
            GlobalInitValueOperator varOper;
            varOper.operatorType = ConstOperatorType_ConstValue;

            varOper.constValue = operators[index+1].constValue.converType(operators[index].converType,operators[index].converPointerBytes);
            if(varOper.constValue.valueType == ConstValueType_ERROR){
                return retValue;
            }
            operators[index] = varOper;
            operators.removeAt(index+1);
        }
    }
    for(int i = 0;i<operators.length();i++){
        if(operators[i].operatorType == ConstOperatorType_ConverType){
            return retValue;
        }
    }

    GlobalInitValueTmp zeroValue;
    zeroValue.valueType = ConstValueType_INT32;
    zeroValue.intData = 0;

    //运算~ ! 正负号
    while(1){
        int operCount = 0;
        for(int i = operators.length()-1;i>=0;i--){
            if(operators[i].operatorType != ConstOperatorType_ConstValue)continue;
            else if(i<=0)break;
            if(operators[i-1].isOperator("~")){
                operators[i].constValue = ~operators[i].constValue;
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("!")){
                operators[i].constValue = !operators[i].constValue;
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("+") &&
                     (i-1 == 0 || operators[i-2].operatorType != ConstOperatorType_ConstValue)){
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("-") &&
                     (i-1 == 0 || operators[i-2].operatorType != ConstOperatorType_ConstValue)){
                operators[i].constValue = zeroValue - operators[i].constValue;
                operators[i-1] = operators[i];
            }else{
                continue;
            }
            if(operators[i-1].constValue.valueType == ConstValueType_ERROR)return retValue;
            operators.removeAt(i);
            i--;
            operCount++;
        }
        if(operCount==0)break;

    }

    //运算*/%
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("*")){
                isOper = 1;
                tmp = operators[i].constValue*operators[i+2].constValue;
            }else if(operators[i+1].isOperator("/")){
                isOper = 1;
                tmp = operators[i].constValue/operators[i+2].constValue;
            }else if(operators[i+1].isOperator("%")){
                isOper = 1;
                tmp = operators[i].constValue%operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    //运算+-
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("+")){
                isOper = 1;
                tmp = operators[i].constValue+operators[i+2].constValue;
            }else if(operators[i+1].isOperator("-")){
                isOper = 1;
                tmp = operators[i].constValue-operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算<< >>
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("<<")){
                isOper = 1;
                tmp = operators[i].constValue<<operators[i+2].constValue;
            }else if(operators[i+1].isOperator(">>")){
                isOper = 1;
                tmp = operators[i].constValue>>operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }


    //运算<= >= < >
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("<=")){
                isOper = 1;
                tmp = operators[i].constValue<=operators[i+2].constValue;
            }else if(operators[i+1].isOperator(">=")){
                isOper = 1;
                tmp = operators[i].constValue>=operators[i+2].constValue;
            }else if(operators[i+1].isOperator("<")){
                isOper = 1;
                tmp = operators[i].constValue<operators[i+2].constValue;
            }else if(operators[i+1].isOperator(">")){
                isOper = 1;
                tmp = operators[i].constValue>operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算== !=
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("==")){
                isOper = 1;
                tmp = operators[i].constValue==operators[i+2].constValue;
            }else if(operators[i+1].isOperator("!=")){
                isOper = 1;
                tmp = operators[i].constValue!=operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算&
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("&")){
                isOper = 1;
                tmp = operators[i].constValue&operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算^
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("^")){
                isOper = 1;
                tmp = operators[i].constValue^operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算|
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("|")){
                isOper = 1;
                tmp = operators[i].constValue|operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算&&
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("&&")){
                isOper = 1;
                tmp = operators[i].constValue&&operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算||
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].operatorType != ConstOperatorType_ConstValue ||
               operators[i+2].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            bool isOper = 0;
            GlobalInitValueTmp tmp;

            if(operators[i+1].isOperator("||")){
                isOper = 1;
                tmp = operators[i].constValue||operators[i+2].constValue;
            }

            if(isOper){
                if(tmp.valueType == ConstValueType_ERROR){
                    return retValue;
                }
                operators[i].constValue = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算?:  (修改)
    while(1){
        int operCount = 0;
        for(int i = operators.length()-1;i>=4;i--){
            if(operators[i-4].operatorType != ConstOperatorType_ConstValue ||
               !operators[i-3].isOperator("?")||
               operators[i-2].operatorType != ConstOperatorType_ConstValue||
               !operators[i-1].isOperator(":")||
               operators[i].operatorType != ConstOperatorType_ConstValue){
                continue;
            }

            operCount++;

            GlobalInitValueTmp judgeValue = operators[i-4].constValue;
            GlobalInitValueTmp ifValue = operators[i-3].constValue;
            GlobalInitValueTmp elseValue = operators[i].constValue;

            if(judgeValue.factIsPointer)return retValue;
            bool judge;
            if(judgeValue.valueType == ConstValueType_DOUBLE ||
               judgeValue.valueType == ConstValueType_FLOAT){
                judge = judgeValue.floatData;
            }else {
                judge = judgeValue.intData;
            }

            if(judge){
                operators[i-4].constValue = ifValue;
            }else{
                operators[i-4].constValue = elseValue;
            }

            operators.removeLast();
            operators.removeLast();
            operators.removeLast();
            operators.removeLast();
            i-=4;
        }
        if(operCount==0){
            break;
        }
    }


    if(operators.length()!=1)return retValue;
    if(operators[0].operatorType != ConstOperatorType_ConstValue)return retValue;
    return operators[0].constValue;
}

//数组大小的常量运算式子的运算(其内不能有任何变量/const常量参与运算,不能有任何赋值操作,且只能有整型参与运算,可以是enum中定义的常量)
//只能存在 整数常量 sizeof(数据类型) sizeof(变量) 的运算。只能有+-*/%()的符号
//专用于定义数组变量时,数组的长度解析
static int analysisConstValueOperator(PhraseList &sentence,SrcObjectInfo &objInfo,bool&status){
    bool isHaveGetAddress;
    QList<GlobalInitValueOperator> opers = analysisGlobalInitValueOperator(sentence,
                                                                           objInfo,
                                                                           isHaveGetAddress,
                                                                           status);
    if(isHaveGetAddress || !status){
        return 0;
    }

    GlobalInitValueTmp value = runGlobalInitConstValueOperator(opers);
    if(value.valueType == ConstValueType_ERROR ||
       value.valueType == ConstValueType_FLOAT ||
       value.valueType == ConstValueType_DOUBLE||
       value.factIsPointer){
        status = 0;
        return 0;
    }

    int data = value.intData;

    return data;
}

//解析全局变量的初始值
static bool analysisGlobalVarInitValue(IndexVN varIndex,
                                   PhraseList &sentence,
                                   SrcObjectInfo &objInfo){

    VarDefineInfo &varInfo = *objInfo.getVarDefineInfoNode()->getVar(varIndex);
    //解析出全局变量中各个初始值存储的地址
    int retAutoArrSize;
    QList<InitValuePlace> places = analysisGlobalVarInitValuePlace(varInfo.baseTypeIndex,
                                                                   varInfo.pointerLevel,
                                                                   0,
                                                                   varInfo.arraySize,
                                                                   varInfo.isAutoArrSize,
                                                                   sentence,
                                                                   objInfo,
                                                                   retAutoArrSize);
    if(places.length()==0){
        return 0;
    }

    if(varInfo.isAutoArrSize){
        varInfo.arraySize[0] = retAutoArrSize;

    }

    //sizeBytes:该全局变量占用的总字节数
    unsigned long long sizeBytes = objInfo.getRootDataTypeInfoNode()->getTypeSize(varInfo.baseTypeIndex);
    if(varInfo.pointerLevel!=0){
        sizeBytes = objInfo.CPU_BIT/8;
    }
    for(int i = 0;i<varInfo.arraySize.length();i++){
        sizeBytes *= varInfo.arraySize[i];
    }

    InitBlock initBlock(sizeBytes,objInfo.CPU_BIT,objInfo.ramEnd);

    //如果当前初始化的全局变量是一个基础的数据类型(整数,浮点数,指针,枚举)
    //那么initValue中存放该基础数据类型的初始值(结构体/共用体/数组就不被算作是基础数据类型)
    GlobalInitValueTmp initValue;

    while(places.length()!=0){
        InitValuePlace thisValuePlace = places[0];
        places.removeAt(0);
        if(thisValuePlace.isNoInitArray)continue;
        if(thisValuePlace.isStringArray){
            //存储常量字符串的字符数组
            QByteArray strByte = replaceTransChar(thisValuePlace.ext[0].text);
            initBlock.insertData(thisValuePlace.offset,strByte);
        }else if(thisValuePlace.isStringPtr){
            //指向一个常量字符串的指针
            initBlock.insertData(thisValuePlace.offset,thisValuePlace.ext[0].text,0,objInfo.CPU_BIT);

            initValue.valueType = ConstValueType_POINTER;
            initValue.factIsPointer = 1;
            initValue.pointerBytes = 1;
            initValue.pointerData_mark = thisValuePlace.ext[0].text;
            initValue.pointerData_offset = 0;
        }else{
            //一个普通 数值型常量 或 指针(非指向常量字符串的指针)
            bool su;
            bool isHaveGetAddress;
            QList<GlobalInitValueOperator> operators =
                analysisGlobalInitValueOperator(thisValuePlace.ext,objInfo,isHaveGetAddress,su);
            if(!su){
                return 0;
            }
            ConstValueType initValueType;
            DataBaseType baseType = objInfo.getDataTypeInfoNode()->getDataType(thisValuePlace.typeIndex)->baseType;
            if(thisValuePlace.pointerLevel != 0 || baseType==DataBaseType_FUNPTR){
                initValueType = ConstValueType_POINTER;
            }else if(baseType == DataBaseType_INT || baseType == DataBaseType_ENUM){
                initValueType = ConstValueType_INT32;
            }else if(baseType == DataBaseType_UINT){
                initValueType = ConstValueType_UINT32;
            }else if(baseType == DataBaseType_SHORT){
                initValueType = ConstValueType_INT16;
            }else if(baseType == DataBaseType_USHORT){
                initValueType = ConstValueType_UINT16;
            }else if(baseType == DataBaseType_CHAR){
                initValueType = ConstValueType_INT8;
            }else if(baseType == DataBaseType_UCHAR){
                initValueType = ConstValueType_UINT8;
            }else if(baseType == DataBaseType_LONG){
                initValueType = ConstValueType_INT64;
            }else if(baseType == DataBaseType_ULONG){
                initValueType = ConstValueType_UINT64;
            }else if(baseType == DataBaseType_FLOAT){
                initValueType = ConstValueType_FLOAT;
            }else if(baseType == DataBaseType_DOUBLE){
                initValueType = ConstValueType_DOUBLE;
            }else{
                initValueType = ConstValueType_ERROR;
            }
            initValue = runGlobalInitConstValueOperator(operators).converType(initValueType);
            if(initValue.valueType == ConstValueType_ERROR){
                return 0;
            }
            //将常量值的运算式结果转为psdl
            if(initValue.factIsPointer){
                //常量运算结果是一个指向某处的指针标记
                int pointerByte = 0;
                if(initValue.valueType == ConstValueType_POINTER){
                    pointerByte = objInfo.CPU_BIT/8;
                }else if(initValue.valueType == ConstValueType_INT16 || initValue.valueType == ConstValueType_UINT16){
                    pointerByte = 2;
                }else if(initValue.valueType == ConstValueType_INT32 || initValue.valueType == ConstValueType_UINT32){
                    pointerByte = 4;
                }else if(initValue.valueType == ConstValueType_INT8 || initValue.valueType == ConstValueType_UINT8){
                    pointerByte = 1;
                }else if(initValue.valueType == ConstValueType_INT64 || initValue.valueType == ConstValueType_UINT64){
                    pointerByte = 8;
                }
                initBlock.insertData(thisValuePlace.offset,
                                     initValue.pointerData_mark,
                                     initValue.pointerData_offset,
                                     pointerByte);
            }else if(initValue.valueType == ConstValueType_FLOAT ||
                     initValue.valueType == ConstValueType_DOUBLE){
                //常量运算结果是一个浮点数
                if(initValue.valueType == ConstValueType_DOUBLE){
                    initBlock.insertData(thisValuePlace.offset,initValue.floatData);
                }else if(initValue.valueType == ConstValueType_FLOAT){
                    initBlock.insertData(thisValuePlace.offset,(float)initValue.floatData);
                }
            }else{
                //常量运算结果是一个整数
                if(initValue.valueType==ConstValueType_INT8){
                    initBlock.insertData(thisValuePlace.offset,(int8_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_INT16){
                    initBlock.insertData(thisValuePlace.offset,(int16_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_INT32){
                    initBlock.insertData(thisValuePlace.offset,(int32_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_INT64){
                    initBlock.insertData(thisValuePlace.offset,(int64_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_UINT8){
                    initBlock.insertData(thisValuePlace.offset,(uint8_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_UINT16){
                    initBlock.insertData(thisValuePlace.offset,(uint16_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_UINT32){
                    initBlock.insertData(thisValuePlace.offset,(uint32_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_UINT64){
                    initBlock.insertData(thisValuePlace.offset,(uint64_t)initValue.intData);
                }else if(initValue.valueType==ConstValueType_POINTER){
                    if(objInfo.CPU_BIT == 8){
                        initBlock.insertData(thisValuePlace.offset,(uint8_t)initValue.intData);
                    }else if(objInfo.CPU_BIT == 16){
                        initBlock.insertData(thisValuePlace.offset,(uint16_t)initValue.intData);
                    }else if(objInfo.CPU_BIT == 32){
                        initBlock.insertData(thisValuePlace.offset,(uint32_t)initValue.intData);
                    }else if(objInfo.CPU_BIT == 64){
                        initBlock.insertData(thisValuePlace.offset,(uint64_t)initValue.intData);
                    }else{
                        return 0;
                    }
                }else{
                    return 0;
                }
            }


        }
    }

    //判断当前初始化的全局变量是否是C语言的基础数据类型,如果是,那就要把初始值注册到变量中
    DataTypeInfo * type = objInfo.getDataTypeInfoNode()->getDataType(varInfo.baseTypeIndex);
    if((varInfo.arraySize.length()==0)&&
        !(varInfo.pointerLevel==0 && (type->baseType==DataBaseType_STRUCT||type->baseType==DataBaseType_UNION))
       ){
        varInfo.globalVarIsInitValue = 1;
        varInfo.globalInitValue = initValue;
    }

    PsdlIR_Compiler::MiddleNode initPsdlNode = initBlock.toPSDLVarDefineNode(varInfo.psdlName,varInfo.isStatic,varInfo.isConst);
    initPsdlNode.atts.append(varInfo.extendAttributes);
    objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(
                initPsdlNode
              );
    return 1;
}

//解析扩展属性
static QList<PsdlIR_Compiler::MiddleNode_Att> analysisExtendAttribute(PhraseList &sentence,
                                SrcObjectInfo &objInfo,
                                bool &state){
    state = 0;
    QStringList attNames;

    QList<PsdlIR_Compiler::MiddleNode_Att> extendAtt;
    PhraseList replace;

    for(int i = 0;i<sentence.length();i++){
        if(i+1 < sentence.length() && sentence[i].isKeyWord("__attribute__") && sentence[i+1].isDelimiter("()")){
            PhraseList keyWordDefSentence = sentence[i+1].subPhraseList;
            QList<PhraseList> keyWordList = keyWordDefSentence.splitFromDelimiter(",");
            foreach(PhraseList keyWord,keyWordList){
                if(keyWord.length()!=1)return extendAtt;
                if(keyWord[0].text == "")return extendAtt;
                QString attName = keyWord[0].text;
                if(!attNames.contains(attName)){
                    attNames.append(attName);
                }
            }
            i++;
        }else{
            replace.append(sentence[i]);
        }
    }

    for(int i = 0;i<attNames.length();i++){
        if(!objInfo.allExterdAttributes.contains(attNames[i])){
            return extendAtt;
        }
        PsdlIR_Compiler::MiddleNode_Att att;
        att.attName = attNames[i];
        extendAtt.append(att);
    }

    sentence = replace;
    state = 1;
    return extendAtt;
}

//解析声明全局变量/函数语句,并将解析出的结果转为PSDL定义全局变量的MiddleNode
static bool analysisStatementGlobalVarOrFun(PhraseList &sentence,
                                     SrcObjectInfo &objInfo){

    sentence.removeLast();
    if(sentence.judgeIsHaveAnyKeyWord({"if",
                                      "else",
                                      "while",
                                      "for",
                                      "goto",
                                      "continue",
                                      "break",
                                      "return",
                                      "switch",
                                      "case",
                                      "default",
                                      "extern",
                                      "typedef"})){
        //语句中存在不合语法的关键字,出错
        objInfo.appendPrompt("语句中存在不合语法的关键字",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }

    //解析出定义的函数/变量信息
    QList<PhraseList> pls = sentence.splitFromDelimiter(",");
    if(pls.length()==0){
        objInfo.appendPrompt("定义语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col );
        return 0;
    }

    PhraseList defineVarTypePls;

    enum DefineVarType{
        DefineFun,
        DefineFunPtr,
        DefineVar
    };

    struct DefineVarPlsInfo{
        PhraseList pls;
        DefineVarType type;
    };
    QList<DefineVarPlsInfo> defineVarInfos;

    DefineVarPlsInfo tmp;
    if(pls[0].getDelimiterNum("=")){
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("=")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
                break;
            }
            tmp.pls.prepend(pls[0][i]);
            pls[0].removeLast();
        }
    }

    QList<Phrase> pl;
    if(pls[0].length()>=2 && pls[0][pls[0].length()-1].isDelimiter("()")&&
       pls[0][pls[0].length()-2].isDelimiter("()")){
        tmp.type = DefineFunPtr;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls.prepend(pl[1]);
        tmp.pls.prepend(pl[0]);
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isKeyWord("const") || pls[0][i].isDelimiter("*")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else if(pls[0].length()>=2 &&pls[0][pls[0].length()-1].isDelimiter("()")&&
             pls[0][pls[0].length()-2].isIdentifier()){
        tmp.type = DefineFun;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls.prepend(pl[1]);
        tmp.pls.prepend(pl[0]);
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isKeyWord("const") || pls[0][i].isDelimiter("*")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else{
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("[]")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(pls[0].length()>=1 && pls[0][pls[0].length()-1].isIdentifier()){
            if(pls[0].length()>=2 &&
                    !pls[0][pls[0].length()-2].isKeyWord("struct")&&
                    !pls[0][pls[0].length()-2].isKeyWord("union")&&
                    !pls[0][pls[0].length()-2].isKeyWord("enum")){
                tmp.pls.prepend(pls[0][pls[0].length()-1]);
                pls[0].removeLast();
            }
        }

        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isKeyWord("const") || pls[0][i].isDelimiter("*")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }

        if(tmp.pls.length()!=0){
            tmp.type = DefineVar;
            defineVarInfos.append(tmp);
        }
    }
    defineVarTypePls = pls[0];

    for(int i = 1;i<pls.length();i++){
        QList<Phrase> ext;
        if(pls[i].getDelimiterNum("=")){
            for(int j = pls[i].length()-1;j>=0;j--){
                if(pls[i][j].isDelimiter("=")){
                    ext.prepend(pls[i][j]);
                    pls[i].removeLast();
                    break;
                }
                ext.prepend(pls[i][j]);
                pls[i].removeLast();
            }
        }
        DefineVarPlsInfo tmp;
        if(pls[i].length()==2 && pls[i][pls[i].length()-1].isDelimiter("()")&&
           pls[i][pls[i].length()-2].isDelimiter("()")){
            tmp.type = DefineFunPtr;
        }else if(pls[i].length()>=2 &&pls[i][pls[i].length()-1].isDelimiter("()")&&
                 pls[i][pls[i].length()-2].isIdentifier()){
            tmp.type = DefineFun;
        }else{
            tmp.type = DefineVar;
        }
        tmp.pls = pls[i];
        tmp.pls.append(ext);
        defineVarInfos.append(tmp);
    }

    bool isConst = defineVarTypePls.getKeyWordNum("const");//是否是常量类型
    defineVarTypePls.removeKeyWord("const");
    if(defineVarTypePls.getKeyWordNum("static") && defineVarTypePls.getKeyWordNum("auto")){
        objInfo.appendPrompt("定义语法有误,static和auto属性不能同时定义",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }
    bool isStatic = defineVarTypePls.getKeyWordNum("static");//是否是静态变量
    defineVarTypePls.removeKeyWord("static");
    defineVarTypePls.removeKeyWord("auto");
    int pointerLevel = 0;//类型的基础指针级数
    IndexVN typeIndex;//类型索引号

    bool extendAttributeSuceess;
    QList<PsdlIR_Compiler::MiddleNode_Att> extendAtts =
            analysisExtendAttribute(defineVarTypePls,objInfo,extendAttributeSuceess);
    if(!extendAttributeSuceess){
        objInfo.appendPrompt("定义了不支持的扩展属性",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }

    bool isDefaultPointerConst = 0;
    //解析使用的类型
    if(defineVarTypePls[0].isKeyWord("struct") || defineVarTypePls[0].isKeyWord("union") || defineVarTypePls[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(defineVarTypePls.length()==2 && !defineVarTypePls[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!defineVarTypePls[1].isIdentifier()){
                objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            if(defineVarTypePls[0].isKeyWord("struct")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("union")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(defineVarTypePls[1].text);
            }
            if(typeIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(defineVarTypePls[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(defineVarTypePls[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(defineVarTypePls[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = defineVarTypePls[1].text;
                enumInfo.isOnlyStatement = 1;
                typeIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }

        }else if((defineVarTypePls.length()==2 && defineVarTypePls[1].isDelimiter("{}")) ||
                 (defineVarTypePls.length()==3 && defineVarTypePls[1].PhraseType==Phrase::Identifier && defineVarTypePls[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = analysisEnumTypeDef(defineVarTypePls,objInfo);
            }else{
                typeIndex = analysisStructUnionTypeDef(defineVarTypePls,objInfo);
            }
            if(typeIndex.isEmpty()){
                objInfo.appendPrompt("数据类型定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
        }else{
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!defineVarTypePls.isOnlyHaveKeyWordAndIdentifier()){
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }

        //被重定义类型的数据类型名
        QStringList oyName = defineVarTypePls.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        if(!status){
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
        typeIndex = map.index;
        isConst = isConst || map.isDefaultConst;
        isDefaultPointerConst = map.isDefaultPointerConst;
        pointerLevel = map.pointerLevel;
    }


    bool defVarIsHaveError = 0;



    for(int i = 0;i<defineVarInfos.length();i++){
        PhraseList extpls;//全局变量初始值运算表达式
        bool isHaveExtpls = 0;

        if(defineVarInfos[i].pls.length()==0){
            defVarIsHaveError = 1;
            objInfo.appendPrompt("定义语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            continue;
        }

        int thisLine = defineVarInfos[i].pls[0].line;
        int thisCol = defineVarInfos[i].pls[0].col;
        QString thisSrcPath = defineVarInfos[i].pls[0].srcPath;


        if(defineVarInfos[i].pls.getDelimiterNum("=")){
            isHaveExtpls = 1;
            for(int j = defineVarInfos[i].pls.length()-1;j>=0;j--){
                if(defineVarInfos[i].pls[j].isDelimiter("=")){
                    defineVarInfos[i].pls.removeLast();
                    break;
                }
                extpls.prepend(defineVarInfos[i].pls[j]);
                defineVarInfos[i].pls.removeLast();
            }
        }

        IndexVN varIndex;
        VarDefineInfo varInfo;
        varInfo.extendAttributes = extendAtts;



        if(defineVarInfos[i].type==DefineFun){
            if(isHaveExtpls){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("函数声明语法有误,存在=号",thisSrcPath,thisLine,thisCol);
                continue;
            }
            QString name;
            IndexVN funTypeIndex = analysisFunctionTypeDef(typeIndex,pointerLevel,defineVarInfos[i].pls,objInfo,name);
            if(funTypeIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("函数参数定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }


            FunctionInfo funInfo;
            funInfo.functionName = name;
            funInfo.baseTypeIndex = funTypeIndex;
            funInfo.isOnlyStatement = 1;
            funInfo.isStatic = 0;
            funInfo.extendAttributes = extendAtts;


            int index = objInfo.appendFunctionDefine(funInfo);
            if(index==-1){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("函数重复声明,且声明参数结构不同",thisSrcPath,thisLine,thisCol);
                continue;
            }
            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "EXTERN";
            node.args = QStringList({name});
            objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(node);

            continue;
        }else if(defineVarInfos[i].type==DefineFunPtr){
            QString name;
            bool isHaveName;
            int fun_pointerLevel;
            QList<int> fun_ptr_array_size;
            bool isAutoArrSize;
            bool isPointerConst;

            IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(typeIndex,
                                                                 pointerLevel,
                                                                 defineVarInfos[i].pls,
                                                                 objInfo,
                                                                 name,
                                                                 isHaveName,
                                                                 isPointerConst,
                                                                 fun_pointerLevel,
                                                                 fun_ptr_array_size,
                                                                 isAutoArrSize);



            if(funPtrTypeIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("函数参数定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(!objInfo.judegDefineVarNameIsCopce(name)){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }

            varInfo.varName = name;
            varInfo.isConst = 0;
            varInfo.isPointerConst = isDefaultPointerConst || isPointerConst;
            varInfo.isExtern = 0;
            varInfo.isStatic = isStatic;
            varInfo.arraySize = fun_ptr_array_size;
            varInfo.pointerLevel = fun_pointerLevel;
            varInfo.baseTypeIndex = funPtrTypeIndex;
            varInfo.isAutoArrSize = isAutoArrSize;

            varIndex = objInfo.defineVar(varInfo);
            if(varIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量重复定义",thisSrcPath,thisLine,thisCol);
                continue;
            }
        }else if(defineVarInfos[i].type==DefineVar){
            bool isHaveName;
            int var_pointerLevel;
            QList<int> var_array_size;
            bool status;
            bool isAutoArrSize;
            bool isHavePrefixConst;
            bool isHaveSuffixConst;
            QString name = analysisDefVarOrAtt_Name_Ptr_Arr(defineVarInfos[i].pls,objInfo,var_pointerLevel,var_array_size,isHaveName,isAutoArrSize,
                                                            isHavePrefixConst,isHaveSuffixConst,status);
            if(status==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("未定义变量名",thisSrcPath,thisLine,thisCol);
                continue;
            }

            if(!objInfo.judegDefineVarNameIsCopce(name)){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }

            varInfo.varName = name;
            varInfo.pointerLevel = var_pointerLevel + pointerLevel;
            if(varInfo.pointerLevel){
                varInfo.isConst = isConst || isHavePrefixConst;
                varInfo.isPointerConst = isHaveSuffixConst || isDefaultPointerConst;
            }else{
                varInfo.isConst = isConst || isHavePrefixConst || isDefaultPointerConst;
                varInfo.isPointerConst = 0;
            }
            varInfo.isExtern = 0;
            varInfo.isStatic = isStatic;
            varInfo.arraySize = var_array_size;
            varInfo.baseTypeIndex = typeIndex;
            varInfo.isAutoArrSize = isAutoArrSize;

            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->isOnlyStatement && varInfo.pointerLevel==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("所定义变量的数据类型还未完整定义,仅声明了",thisSrcPath,thisLine,thisCol);
                continue;
            }

            //void类型不能够作为数据类型,只能作为指针
            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->baseType == DataBaseType_VOID && varInfo.pointerLevel==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("void类型只能作为指针来定义一个变量",thisSrcPath,thisLine,thisCol);
                continue;
            }
            varIndex = objInfo.defineVar(varInfo);
            if(varIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量重复定义",thisSrcPath,thisLine,thisCol);
                continue;
            }
        }else{
            defVarIsHaveError = 1;
            objInfo.appendPrompt("变量定义语法有误",thisSrcPath,thisLine,thisCol);
            continue;
        }

        if(isHaveExtpls){
            if(analysisGlobalVarInitValue(varIndex,extpls,objInfo)==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量初始值定义有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(varInfo.isAutoArrSize){
                VarDefineInfo *ptrTmp = objInfo.getVarDefineInfoNode()->getVar(varIndex);
                varInfo.isAutoArrSize = 0;
                varInfo.arraySize[0] = ptrTmp->arraySize[0];
            }
        }else{


            if(varInfo.isAutoArrSize){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("非初始化数组无法自动计算其大小",thisSrcPath,thisLine,thisCol);
                continue;
            }




            //非初始化全局变量
            int baseTypeSize = objInfo.getRootDataTypeInfoNode()->getDataType(varInfo.baseTypeIndex)->dataBaseTypeBytes;
            if(varInfo.pointerLevel!=0){
                baseTypeSize = objInfo.CPU_BIT/8;
            }
            for(int j = 0;j<varInfo.arraySize.length();j++){
                baseTypeSize*=varInfo.arraySize[j];
            }


            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "ARRAY";
            node.args = QStringList({varInfo.psdlName,QString::number(baseTypeSize)+"D"});
            node.atts.append(varInfo.extendAttributes);

            objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(node);
        }
    }

    return !defVarIsHaveError;
}

//外部导入的全局变量,函数
static bool analysisExternGlobalVarOrFun(PhraseList &sentence,
                                  SrcObjectInfo &objInfo){

    sentence.removeLast();
    if(sentence.judgeIsHaveAnyKeyWord({"if",
                                      "else",
                                      "while",
                                      "for",
                                      "goto",
                                      "continue",
                                      "break",
                                      "return",
                                      "switch",
                                      "case",
                                      "default",
                                      "typedef",
                                      "static",
                                      "auto",
                                      "__attribute__"})){
        //语句中存在不合语法的关键字,出错
        objInfo.appendPrompt("语句中存在不合语法的关键字",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }
    //解析出定义的函数/变量信息
    QList<PhraseList> pls = sentence.splitFromDelimiter(",");
    if(pls.length()==0){
        objInfo.appendPrompt("外部导入变量/函数的语法不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }

    PhraseList defineVarTypePls;

    enum DefineVarType{
        DefineFun,
        DefineFunPtr,
        DefineVar
    };

    struct DefineVarPlsInfo{
        PhraseList pls;
        DefineVarType type;
    };
    QList<DefineVarPlsInfo> defineVarInfos;

    DefineVarPlsInfo tmp;
    QList<Phrase> pl;
    if(pls[0].length()>=2 && pls[0][pls[0].length()-1].isDelimiter("()")&&
       pls[0][pls[0].length()-2].isDelimiter("()")){
        tmp.type = DefineFunPtr;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*") || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else if(pls[0].length()>=2 &&pls[0][pls[0].length()-1].isDelimiter("()")&&
             pls[0][pls[0].length()-2].isIdentifier()){
        tmp.type = DefineFun;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls = pl;
        pls[0].removeLast();
        pls[0].removeLast();
        for(int j = pls[0].length()-1;j>=0;j--){
            if(pls[0][j].isDelimiter("*") || pls[0][j].isKeyWord("const")){
                tmp.pls.prepend(pls[0][j]);
                pls[0].removeLast();
            }
        }
        defineVarInfos.append(tmp);
    }else{
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("[]")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(pls[0].length()>=1 && pls[0][pls[0].length()-1].isIdentifier()){
            if(pls[0].length()>=2 &&
                    !pls[0][pls[0].length()-2].isKeyWord("struct")&&
                    !pls[0][pls[0].length()-2].isKeyWord("union")&&
                    !pls[0][pls[0].length()-2].isKeyWord("enum")){
                tmp.pls.prepend(pls[0][pls[0].length()-1]);
                pls[0].removeLast();
            }
        }
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*") || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }

        if(tmp.pls.length()!=0){
            tmp.type = DefineVar;
            defineVarInfos.append(tmp);
        }
    }
    defineVarTypePls = pls[0];
    for(int i = 1;i<pls.length();i++){
        DefineVarPlsInfo tmp;
        if(pls[i].length()==2 && pls[i][pls[i].length()-1].isDelimiter("()")&&
           pls[i][pls[i].length()-2].isDelimiter("()")){
            tmp.type = DefineFunPtr;
        }else if(pls[i].length()>=2 &&pls[i][pls[i].length()-1].isDelimiter("()")&&
                 pls[i][pls[i].length()-2].isIdentifier()){
            tmp.type = DefineFun;
        }else{
            tmp.type = DefineVar;
        }
        tmp.pls = pls[i];
        defineVarInfos.append(tmp);
    }
    defineVarTypePls.removeKeyWord("extern");

    bool isConst = defineVarTypePls.getKeyWordNum("const");//是否是常量类型
    int pointerLevel = 0;//类型的基础指针级数
    IndexVN typeIndex;//类型索引号
    defineVarTypePls.removeKeyWord("const");


    bool extendAttributeSuceess;
    QList<PsdlIR_Compiler::MiddleNode_Att> extendAtts =
            analysisExtendAttribute(defineVarTypePls,objInfo,extendAttributeSuceess);
    if(!extendAttributeSuceess){
        objInfo.appendPrompt("定义了不支持的扩展属性",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }


    bool isDefaultPointerConst = 0;
    //解析使用的类型
    if(defineVarTypePls[0].isKeyWord("struct") || defineVarTypePls[0].isKeyWord("union") || defineVarTypePls[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(defineVarTypePls.length()==2 && !defineVarTypePls[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!defineVarTypePls[1].isIdentifier()){
                objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            if(defineVarTypePls[0].isKeyWord("struct")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("union")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(defineVarTypePls[1].text);
            }
            if(typeIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(defineVarTypePls[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(defineVarTypePls[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(defineVarTypePls[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = defineVarTypePls[1].text;
                enumInfo.isOnlyStatement = 1;
                typeIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }
        }else if((defineVarTypePls.length()==2 && defineVarTypePls[1].isDelimiter("{}")) ||
                 (defineVarTypePls.length()==3 && defineVarTypePls[1].PhraseType==Phrase::Identifier && defineVarTypePls[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = analysisEnumTypeDef(defineVarTypePls,objInfo);
            }else{
                typeIndex = analysisStructUnionTypeDef(defineVarTypePls,objInfo);
            }
            if(typeIndex.isEmpty()){
                objInfo.appendPrompt("数据类型定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
        }else{
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!defineVarTypePls.isOnlyHaveKeyWordAndIdentifier()){
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }

        //被重定义类型的数据类型名
        QStringList oyName = defineVarTypePls.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        if(!status){
            objInfo.appendPrompt("数据类型名定义不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }
        typeIndex = map.index;
        isConst = isConst || map.isDefaultConst;
        isDefaultPointerConst = map.isDefaultPointerConst;
        pointerLevel = map.pointerLevel;
    }


    bool isHaveError = 0;
    for(int i = 0;i<defineVarInfos.length();i++){
        if(defineVarInfos[i].pls.length()==0){
            isHaveError = 1;
            objInfo.appendPrompt("外部导入变量定义失败",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            continue;
        }


        int thisLine = defineVarInfos[i].pls[0].line;
        int thisCol = defineVarInfos[i].pls[0].col;
        QString thisSrcPath = defineVarInfos[i].pls[0].srcPath;


        if(defineVarInfos[i].type==DefineFun){

            QString name;
            IndexVN funTypeIndex = analysisFunctionTypeDef(typeIndex,pointerLevel,defineVarInfos[i].pls,objInfo,name);
            if(funTypeIndex.isEmpty()){
                isHaveError = 1;
                objInfo.appendPrompt("函数参数定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }


            FunctionInfo funInfo;
            funInfo.functionName = name;
            funInfo.baseTypeIndex = funTypeIndex;
            funInfo.isOnlyStatement = 1;
            funInfo.isStatic = 0;
            funInfo.extendAttributes = extendAtts;


            int index = objInfo.appendFunctionDefine(funInfo);
            if(index==-1){
                isHaveError = 1;
                objInfo.appendPrompt("函数已被声明,且参数格式与当前声明的不同",thisSrcPath,thisLine,thisCol);
                continue;
            }


            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "EXTERN";
            node.args = QStringList({name});
            node.atts.append(extendAtts);
            objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(node);
        }else if(defineVarInfos[i].type==DefineFunPtr){
            QString name;
            bool isHaveName;
            int fun_pointerLevel;
            QList<int> fun_ptr_array_size;
            bool  isAutoArrSize;
            bool isPointerConst;
            IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(typeIndex,
                                                                 pointerLevel,
                                                                 defineVarInfos[i].pls,
                                                                 objInfo,
                                                                 name,
                                                                 isHaveName,
                                                                 isPointerConst,
                                                                 fun_pointerLevel,
                                                                 fun_ptr_array_size,
                                                                 isAutoArrSize);

            if(isAutoArrSize){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入数组不能够自动确定数组大小",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(!objInfo.judegDefineVarNameIsCopce(name)){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量名称定义不正确",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(funPtrTypeIndex.isEmpty()){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量定义失败",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量名称定义不正确",thisSrcPath,thisLine,thisCol);
                continue;
            }

            VarDefineInfo varInfo;
            varInfo.varName = name;
            varInfo.isConst = 0;
            varInfo.isPointerConst = isDefaultPointerConst || isPointerConst;
            varInfo.isExtern = 1;
            varInfo.isStatic = 0;
            varInfo.arraySize = fun_ptr_array_size;
            varInfo.pointerLevel = fun_pointerLevel;
            varInfo.baseTypeIndex = funPtrTypeIndex;
            if(objInfo.defineVar(varInfo).isEmpty()){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量定义失败",thisSrcPath,thisLine,thisCol);
                continue;
            }
            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "EXTERN";
            node.args = QStringList({name});
            node.atts.append(extendAtts);
            objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(node);
        }else if(defineVarInfos[i].type==DefineVar){
            bool isHaveName;
            int var_pointerLevel;
            QList<int> var_array_size;
            bool status;
            bool isAutoArrSize;
            bool isHavePrefixConst;
            bool isHaveSuffixConst;
            QString name = analysisDefVarOrAtt_Name_Ptr_Arr(defineVarInfos[i].pls,objInfo,var_pointerLevel,var_array_size,isHaveName,isAutoArrSize,isHavePrefixConst,isHaveSuffixConst,status);
            if(status==0){
                isHaveError = 1;
                objInfo.appendPrompt("导入变量定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }

            if(!objInfo.judegDefineVarNameIsCopce(name)){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量名称定义不正确",thisSrcPath,thisLine,thisCol);
                continue;
            }

            if(isAutoArrSize){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入数组不能够自动确定其大小",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                isHaveError = 1;
                objInfo.appendPrompt("外部导入变量名称定义不正确",thisSrcPath,thisLine,thisCol);
                continue;
            }

            VarDefineInfo varInfo;
            varInfo.varName = name;
            varInfo.pointerLevel = var_pointerLevel + pointerLevel;
            if(varInfo.pointerLevel){
                varInfo.isConst = isConst || isHavePrefixConst;
                varInfo.isPointerConst = isHaveSuffixConst || isDefaultPointerConst;
            }else{
                varInfo.isConst = isConst || isHavePrefixConst || isDefaultPointerConst;
                varInfo.isPointerConst = 0;
            }
            varInfo.isExtern = 1;
            varInfo.isStatic = 0;
            varInfo.arraySize = var_array_size;
            varInfo.baseTypeIndex = typeIndex;

            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->isOnlyStatement && varInfo.pointerLevel==0){
                isHaveError = 1;
                objInfo.appendPrompt("定义的外部导入变量的数据类型还未完整定义,仅进行了声明",thisSrcPath,thisLine,thisCol);
                continue;
            }

            //void类型不能够作为数据类型,只能作为指针
            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->baseType == DataBaseType_VOID && varInfo.pointerLevel==0){
                isHaveError = 1;
                objInfo.appendPrompt("void类型只能作为指针来定义变量",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(objInfo.defineVar(varInfo).isEmpty())return 0;
            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "EXTERN";
            node.args = QStringList({name});
            node.atts.append(extendAtts);
            objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(node);
        }else{
            isHaveError = 1;
            objInfo.appendPrompt("外部导入变量定义失败",thisSrcPath,thisLine,thisCol);
            continue;
        }
    }
    return !isHaveError;
}

//函数中变量的临时存储类型
enum FuncOperValueType{
    FuncOperValueType_Error,//出错
    FuncOperValueType_VoidVar,//空变量(void)

    FuncOperValueType_Operator,//运算符
    FuncOperValueType_ConverType,//强转数据类型
    FuncOperValueType_SubOper,//()子级运算表达式
    FuncOperValueType_IndexOper,//[]索引子表达式
    FuncOperValueType_Function,//指向一个函数
    FuncOperValueType_FuncParameters,//函数传参()
    FuncOperValueType_PtrAtt,//->指向一个结构体/共用体属性
    FuncOperValueType_Att,//.指向的一个结构体/共用体属性

    FuncOperValueType_RegData,//寄存器数据类型(varIndex指向的变量其必然是三大基础数据类型: 整型、浮点型、指针)可直接操作
    FuncOperValueType_QuoteData,//引用类型(varIndex指向的变量其必然是一个void*型)解引用后可操作
    //数组/结构体/共用体类型的动态变量以及所有类型的静态变量都会被转为引用类型去处理
    FuncOperValueType_ConstValue,//常量值类型
};
struct FuncOperValue;
typedef  QList<FuncOperValue> FuncOperToken;
//函数中变量的临时存储信息
struct FuncOperValue{
    ///////////////////////属性///////////////////////
    //公有属性
    FuncOperValueType funcOperValueType = FuncOperValueType_Error;
    //引用类型、寄存器类型、函数专有属性
    IndexVN varIndex;//变量在obj中的索引号(如果是函数,varIndex[0]存储函数的索引号)
    //引用类型/常量类型/强转数据类型专有属性
    IndexVN quoteOrConst_Type;//引用变量/常量的基本类型
    int quoteOrConst_PointerLevel;//引用变量/常量的指针级数
    //引用类型/寄存器类型专有属性
    int suffixIncrementValue = 0;//当前变量有需要后缀自增的值(只可能是-1 0 1)如果isDummyVar=1,该值无效。在变量下一轮运算前被自动加上该值
    //寄存器/应用类型专有属性
    bool isDummyVar = 0;//当前变量是否是一个运算处理过程中的临时虚拟伪变量(虚拟变量不能够作为++/--的变量，以及= += -= *= /= %=等赋值运算符的左值)
    //伪变量本身并不是一个变量，只是变量运算过程中的中间值，故不允许有任何赋值的操作。
    // +-*/(类型强转)<< >> ! || && ~ | & 函数调用(...) &取地址符 等运算符处理得出的变量都是临时变量
    // *解引用 将一个指针类型转为引用，非临时变量   ->取结构体中指针型属性指向数据的引用,非临时变量
    // .取结构体中子属性的引用,非临时变量

    //引用类型专有属性
    QList<int> quote_ArraySize;//引用变量的数组维度
    bool quote_IsConst;//引用的变量是否是常量
    bool quote_IsPointerConst;//引用的变量是否是指针常量
    //常量类型专有属性
    long long constValue_IntData;//如果常量值是整型、指针型，其数值存储的属性
    double constValue_FloatData;//如果常量值是浮点型，其数值存储的属性
    //运算符类型专有属性
    QString operatorType;//运算符类型
    //子表达式、索引表达式、函数传参括号专用属性
    QList<FuncOperToken> subOpers;
    //->或.指向的属性 类型 的专用属性
    QString attName;
    //得出当前临时变量的PSDL指令
    QList<PsdlIR_Compiler::MiddleNode> bodyAtt;//PSDL逻辑运算指令
    QList<PsdlIR_Compiler::MiddleNode> *varsAtt = NULL;//PSDL创建的临时变量的指令
    SrcObjectInfo *objInfo = NULL;
    //////////////////////////方法////////////////////////////
    //构建一个变量临时存储器
    FuncOperValue(IndexVN varIndex,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        IndexVN voidTypeIndex = objInfo.getDataTypeInfoNode()->searchDataType({"void"});//void类型的指针

        VarDefineInfo varInfo = *objInfo.getVarDefineInfoNode()->getVar(varIndex);


        IndexVN baseType = varInfo.baseTypeIndex;//基础数据类型 索引号
        DataTypeInfo* baseTypeInfo = objInfo.getDataTypeInfoNode()->getDataType(baseType);//被转换变量的 基础数据类型 信息
        int pointerLevel = varInfo.pointerLevel;//指针级数
        bool isArray = varInfo.arraySize.length();//是否是数组
        bool isStaticVar = !varInfo.isFuncVar;
        QString psdlName = varInfo.psdlName;//psdl名称


        enum DataClass{
            IntClass,
            FloatClass,
            PointerClass,
            ArrayClass,
            StructClass
        };

        DataClass dataClass;
        if(isArray){
            dataClass = ArrayClass;
        }else if(pointerLevel>0 || baseTypeInfo->baseType == DataBaseType_FUNPTR){
            dataClass = PointerClass;
        }else if(baseTypeInfo->baseType == DataBaseType_STRUCT ||
                 baseTypeInfo->baseType == DataBaseType_UNION){
            dataClass = StructClass;
        }else if(baseTypeInfo->baseType == DataBaseType_FLOAT ||
                 baseTypeInfo->baseType == DataBaseType_DOUBLE){
            dataClass = FloatClass;
        }else{
            dataClass = IntClass;
        }

        FuncOperValue &retValue = *this;
        if(isStaticVar || dataClass==StructClass || dataClass==ArrayClass){
            //静态变量 结构体 数组 类型需要被转为引用

            //设置返回值是引用类型
            retValue.funcOperValueType = FuncOperValueType_QuoteData;

            //设置引用的变量的数据类型
            retValue.quoteOrConst_Type = varInfo.baseTypeIndex;
            retValue.quote_ArraySize = varInfo.arraySize;
            retValue.quoteOrConst_PointerLevel =varInfo.pointerLevel;
            retValue.quote_IsConst = varInfo.isConst;
            retValue.quote_IsPointerConst = varInfo.isPointerConst;
            //创建一个临时变量,用于存储结构体引用的指针
            retValue.varIndex = objInfo.defineFuncTmpVar(voidTypeIndex,1,varsAtt);

            //读取数组的地址到一个临时指针型变量
            PsdlIR_Compiler::MiddleNode converNode;
            if(varInfo.isFuncVar){
                converNode.nodeType = "ADDRESS";
                QString tmpVarPsdlName = objInfo.getVarDefineInfoNode()->getVar(retValue.varIndex)->psdlName;
                QString varPsdlName = varInfo.psdlName;
                converNode.args = QStringList({tmpVarPsdlName,varPsdlName,"0D"});
            }else{
                converNode.nodeType = "MOV";
                QString tmpVarPsdlName = objInfo.getVarDefineInfoNode()->getVar(retValue.varIndex)->psdlName;
                QString varPsdlName = varInfo.psdlName;
                converNode.args = QStringList({tmpVarPsdlName,"["+varPsdlName+"]"});
            }
            bodyAtt.append(converNode);
            return;
        }
        retValue.funcOperValueType = FuncOperValueType_RegData;
        retValue.varIndex = varIndex;
    }

    //构建一个取属性引用的临时存储器
    FuncOperValue(bool isPointer,//是否是 -> 默认是 .
                  QString &attName,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = isPointer ? FuncOperValueType_PtrAtt : FuncOperValueType_Att;
        this->attName = attName;
    }

    //构建一个常量临时存储器
    FuncOperValue(double constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"double"});
        quoteOrConst_PointerLevel = 0;
        constValue_FloatData = constValue;
    }
    FuncOperValue(float constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;


        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"float"});
        quoteOrConst_PointerLevel = 0;
        constValue_FloatData = constValue;
    }

    FuncOperValue(uint8_t constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName({"unsigned","char"}));
        quoteOrConst_PointerLevel = 0;
        constValue_IntData = constValue;
    }

    FuncOperValue(int32_t constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName({"int"}));
        quoteOrConst_PointerLevel = 0;
        constValue_IntData = constValue;
    }
    FuncOperValue(uint32_t constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName({"unsigned","int"}));
        quoteOrConst_PointerLevel = 0;
        constValue_IntData = constValue;
    }
    FuncOperValue(uint64_t constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName({"unsigned","long","long"}));
        quoteOrConst_PointerLevel = 0;
        constValue_IntData = constValue;
    }
    FuncOperValue(int64_t constValue,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        funcOperValueType = FuncOperValueType_ConstValue;
        quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName({"long","long"}));
        quoteOrConst_PointerLevel = 0;
        constValue_IntData = constValue;
    }
    //构建一个运算符类型的临时存储器
    FuncOperValue(QString operatorName,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        funcOperValueType = FuncOperValueType_Operator;
        this->operatorType = operatorName;
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
    }
    //构建一个子表达式的临时存储器()
    FuncOperValue(QList<FuncOperToken> &subTokens,
                  bool isFuncFuncParameters,//是否是函数传参
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){

        funcOperValueType = isFuncFuncParameters ? FuncOperValueType_FuncParameters:FuncOperValueType_SubOper;
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        subOpers = subTokens;
    }
    //构建一个索引表达式的临时存储器[]
    FuncOperValue(FuncOperToken &subTokens,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        funcOperValueType = FuncOperValueType_IndexOper;
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        this->subOpers.clear();
        this->subOpers.append(subTokens);
    }
    //构建一个函数临时存放器
    FuncOperValue(QString funName,int funIndex,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        funName = QString();
        funcOperValueType = FuncOperValueType_Function;
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        varIndex.clear();
        varIndex.append(funIndex);
    }
    //构建一个强转类型的临时存放器
    FuncOperValue(IndexVN converType,
                  int pointerLevel,
                  QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                  SrcObjectInfo &objInfo){
        funcOperValueType = FuncOperValueType_ConverType;
        this->varsAtt = &varsAtt;
        this->objInfo = &objInfo;
        quoteOrConst_Type = converType;
        quoteOrConst_PointerLevel = pointerLevel;
    }

    FuncOperValue(){}

    ////////判断当前的临时存储器是哪种类型/////
    //是否存储的是一个变量值
    bool isValue(){
        return funcOperValueType==FuncOperValueType_RegData ||
                funcOperValueType==FuncOperValueType_ConstValue||
                funcOperValueType==FuncOperValueType_QuoteData||
                funcOperValueType==FuncOperValueType_Function;
    }
    //是否存储的是一个指定的运算功能
    //opearName = [] 是引用子表达式
    //opearName = () 是子表达式
    //opearName = (type) 是转数据类型
    //opearName = (*) 是解引用
    //opearName = (&) 是取地址
    //opearName = func() 是函数传参括号
    //其它运算符类型，与原运算符符号相同
    bool isOperator(QString opearName){
        if(opearName=="[]")return funcOperValueType==FuncOperValueType_IndexOper;
        else if(opearName=="()")return funcOperValueType==FuncOperValueType_SubOper;
        else if(opearName=="(type)")return funcOperValueType==FuncOperValueType_ConverType;
        else if(opearName=="func()")return funcOperValueType==FuncOperValueType_FuncParameters;
        else if(opearName==".")return funcOperValueType==FuncOperValueType_Att;
        else if(opearName=="->")return funcOperValueType==FuncOperValueType_PtrAtt;
        else if(funcOperValueType!=FuncOperValueType_Operator)return false;
        return this->operatorType==opearName;
    }
    //是否无效
    bool isError(){
        return funcOperValueType == FuncOperValueType_Error;
    }
    //是否是空变量
    bool isVoid(){
        return funcOperValueType==FuncOperValueType_VoidVar;
    }

    struct RegDataInfo{
        QString varPsdlName;//变量psdl名称
        IndexVN baseTypeIndex;//引用指向的变量的基类型索引号
        DataTypeInfo baseTypeInfo;//变量的基类型信息
        int pointerLevel;//变量指针层数
        int suffixIncrementValue;//是否需要自增
        bool isDummyVar;//是否是虚变量
        bool isConst;//变量是否是常量
        bool isPointerConst;//是否是指针常量

        QString toString(){
            QString txt = (isDummyVar ? "<DUVAR|" : "<VAR|")+baseTypeInfo.toString();
            txt+="-PTR:"+QString::number(pointerLevel);
            if(suffixIncrementValue!=0){
                txt+= "-SUFF:"+QString::number(suffixIncrementValue);
            }

            txt+= "-NAME:"+varPsdlName;

            if(isConst){
                txt += "-CONST";
            }
            if(isPointerConst){
                txt += "-PTRCONST";
            }
            return txt+">";
        }
    };
    bool getInfo_RegData(RegDataInfo &info){
        if(funcOperValueType!=FuncOperValueType_RegData)return 0;

        VarDefineInfo *var = objInfo->getVarDefineInfoNode()->getVar(varIndex);

        info.varPsdlName = var->psdlName;
        info.baseTypeInfo = *objInfo->getDataTypeInfoNode()->getDataType(var->baseTypeIndex);
        info.pointerLevel = var->pointerLevel;
        info.suffixIncrementValue = this->suffixIncrementValue;
        info.isDummyVar = this->isDummyVar;
        info.isConst = var->isConst;
        info.isPointerConst = var->isPointerConst;


        info.baseTypeIndex = var->baseTypeIndex;
        return 1;
    }

    struct ConstValueInfo{
        DataTypeInfo baseTypeInfo;//常量的基类型信息
        IndexVN baseTypeIndex;//引用指向的变量的基类型索引号
        int pointerLevel;//常量类型的指针层数
        bool constValueIsFloat;//常量是否是浮点值
        long long constIntValue;
        double constFloatValue;
        QString toString(){
            QString txt = "<CONSTVAR|"+baseTypeInfo.toString();
            txt+="-PTR:"+QString::number(pointerLevel);
            txt+= "-VALUE:"+(constValueIsFloat ? QString::number(constFloatValue) : QString::number(constIntValue));
            return txt+">";
        }
    };
    bool getInfo_ConstValue(ConstValueInfo &info){
        if(funcOperValueType!=FuncOperValueType_ConstValue)return 0;

        info.baseTypeInfo = *objInfo->getDataTypeInfoNode()->getDataType(quoteOrConst_Type);
        info.pointerLevel = quoteOrConst_PointerLevel;

        DataBaseType baseType = info.baseTypeInfo.baseType;
        info.constValueIsFloat = 0;
        info.baseTypeIndex = quoteOrConst_Type;
        if((baseType == DataBaseType_FLOAT ||
           baseType == DataBaseType_DOUBLE) && info.pointerLevel==0){
            info.constValueIsFloat = 1;
        }else if(info.pointerLevel==0 && (baseType==DataBaseType_STRUCT || baseType==DataBaseType_UNION)){
            return 0;
        }

        info.constIntValue = constValue_IntData;
        info.constFloatValue = constValue_FloatData;
        return 1;
    }

    struct QuoteDataInfo{
        QString varPsdlName;//引用指针的psdl名称
        IndexVN baseTypeIndex;//引用指向的变量的基类型索引号
        DataTypeInfo baseTypeInfo;//引用指向的变量的基类型信息
        int pointerLevel;//引用指向的变量指针层数
        QList<int> arraySize;//引用指向变量的数组维度
        int suffixIncrementValue;//是否需要自增
        bool isConst;//引用的变量是否是常量
        bool isPointerConst;//是否是指针常量
        bool isDummyVar;//是否是虚变量

        QString toString(){
            QString txt = "<QUOTE|"+baseTypeInfo.toString();
            txt+="-PTR:"+QString::number(pointerLevel);
            if(arraySize.length()>=1){
                txt+="-ARR:";
            }

            for(int i = 0;i<arraySize.length();i++){
                txt+= "["+QString::number(arraySize[i])+"]";
            }
            if(suffixIncrementValue!=0){
                txt+= "-SUFF:"+QString::number(suffixIncrementValue);
            }
            txt+= "-PTRNAME:"+varPsdlName;
            if(isConst){
                txt += "-CONST";
            }
            if(isPointerConst){
                txt += "-PTRCONST";
            }
            return txt+">";
        }
    };
    bool getInfo_QuoteData(QuoteDataInfo &info){
        if(funcOperValueType!=FuncOperValueType_QuoteData)return 0;
        info.baseTypeInfo = *objInfo->getDataTypeInfoNode()->getDataType(quoteOrConst_Type);
        info.pointerLevel = quoteOrConst_PointerLevel;
        info.arraySize = this->quote_ArraySize;
        VarDefineInfo *var = objInfo->getVarDefineInfoNode()->getVar(varIndex);
        info.varPsdlName = var->psdlName;
        info.suffixIncrementValue = this->suffixIncrementValue;
        info.isConst = quote_IsConst;
        info.isPointerConst = quote_IsPointerConst;
        info.baseTypeIndex =  quoteOrConst_Type;
        info.isDummyVar = isDummyVar;
        return 1;
    }
    bool getInfo_Operator(QString &operType){
        if(funcOperValueType!=FuncOperValueType_Operator)return 0;
        operType = operatorType;
        return 1;
    }
    bool getInfo_Att_PtrAtt(QString &attInfo){
        if(funcOperValueType!=FuncOperValueType_Att &&
           funcOperValueType!=FuncOperValueType_PtrAtt)return 0;
        attInfo = attName;
        return 1;
    }
    bool getInfo_SubOper_IndexOper_FuncParameters(QList<FuncOperToken> &subOpers){
        if(funcOperValueType!=FuncOperValueType_SubOper &&
           funcOperValueType!=FuncOperValueType_IndexOper&&
           funcOperValueType!=FuncOperValueType_FuncParameters)return 0;
        subOpers = this->subOpers;
        return 1;
    }
    bool getInfo_Function(QString &functionName,
                          IndexVN &funTypeIndex,
                          DataTypeInfo&funTypeInfo){
        if(funcOperValueType!=FuncOperValueType_Function)return 0;
        FunctionInfo *info = objInfo->getFunctionInfo(varIndex[0]);
        functionName = info->functionName;
        funTypeIndex = info->baseTypeIndex;
        funTypeInfo = *objInfo->getDataTypeInfoNode()->getDataType(info->baseTypeIndex);
        return 1;
    }
    bool getInfo_ConverType(IndexVN&typeIndex,//常量的基类型信息
                            int &pointerLevel){//常量类型的指针层数
        if(funcOperValueType!=FuncOperValueType_ConverType)return 0;
        typeIndex = quoteOrConst_Type;
        pointerLevel = quoteOrConst_PointerLevel;
        return 1;
    }

    //打印信息
    QString toString(){
        if(funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            getInfo_RegData(info);
            return info.toString();
        }else if(funcOperValueType==FuncOperValueType_ConstValue){
            ConstValueInfo info;
            getInfo_ConstValue(info);
            return info.toString();
        }else if(funcOperValueType==FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            getInfo_QuoteData(info);
            return info.toString();
        }else if(funcOperValueType==FuncOperValueType_Operator){
            return operatorType;
        }else if(funcOperValueType==FuncOperValueType_Att){
            return "."+attName;
        }else if(funcOperValueType==FuncOperValueType_PtrAtt){
            return "->"+attName;
        }else if(funcOperValueType==FuncOperValueType_SubOper){
            QStringList txt;
            QList<FuncOperToken> tokens;
            getInfo_SubOper_IndexOper_FuncParameters(tokens);
            for(int i = 0;i<tokens.length();i++){
                QString tmp;
                for(int j = 0;j<tokens[i].length();j++){
                    tmp += tokens[i][j].toString()+"\r\n";
                }
                txt.append(tmp);
            }
            return "(\r\n"+txt.join("\r\n,\r\n")+"\r\n)";
        }else if(funcOperValueType==FuncOperValueType_IndexOper){
            QStringList txt;
            QList<FuncOperToken> tokens;
            getInfo_SubOper_IndexOper_FuncParameters(tokens);
            for(int i = 0;i<tokens.length();i++){
                QString tmp;
                for(int j = 0;j<tokens[i].length();j++){
                    tmp += tokens[i][j].toString()+"\r\n";
                }
                txt.append(tmp);
            }
            return "[\r\n"+txt.join("\r\n,\r\n")+"\r\n]";
        }else if(funcOperValueType==FuncOperValueType_FuncParameters){
            QStringList txt;
            QList<FuncOperToken> tokens;
            getInfo_SubOper_IndexOper_FuncParameters(tokens);
            for(int i = 0;i<tokens.length();i++){
                QString tmp;
                for(int j = 0;j<tokens[i].length();j++){
                    tmp += tokens[i][j].toString()+"\r\n";
                }
                txt.append(tmp);
            }
            return "FUNC(\r\n"+txt.join("\r\n,\r\n")+"\r\n)";
        }else if(funcOperValueType==FuncOperValueType_Function){
            QString funName;
            DataTypeInfo type;
            IndexVN tmp;
            getInfo_Function(funName,tmp,type);
            return "<FUN|"+type.toString()+"-NAME:"+funName+">";
        }else if(funcOperValueType==FuncOperValueType_ConverType){
            DataTypeInfo type;
            IndexVN typeIndex;
            int pointerLevel;
            getInfo_ConverType(typeIndex,pointerLevel);
            type = *objInfo->getDataTypeInfoNode()->getDataType(typeIndex);
            return "<TYPE|"+type.toString()+"-PTR:"+QString::number(pointerLevel)+">";
        }else{
            return "<ERROR>";
        }
    }

    //生成当前变量自增自减的指令
    bool runSuffixIncrementOper(QList<PsdlIR_Compiler::MiddleNode> &codes,int selfIncreasing){
        if(selfIncreasing==0)return 0;
        if(funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            getInfo_RegData(info);
            if(info.isDummyVar || info.isConst)return 0;
            PsdlIR_Compiler::MiddleNode code;
            if(selfIncreasing>0){

                code.nodeType = "ADD";
            }else{
                code.nodeType = "SUB";
                selfIncreasing = - selfIncreasing;
            }

            int bytes;
            DataBaseType type = info.baseTypeInfo.baseType;
            if(info.pointerLevel==0 &&
               !(type==DataBaseType_VOID || type==DataBaseType_FUNPTR || type==DataBaseType_UNION || type==DataBaseType_STRUCT)){
                bytes = 1;
            }else if(info.pointerLevel==1){
                bytes = info.baseTypeInfo.dataBaseTypeBytes;
            }else if(info.pointerLevel>1){
                bytes = objInfo->CPU_BIT/8;
            }else return 0;

            FuncOperValue constValue((int)(bytes*selfIncreasing),*varsAtt,*objInfo);
            constValue = constValue.converType(info.baseTypeIndex,info.pointerLevel);



            code.args = QStringList({info.varPsdlName,info.varPsdlName,generateConstValueDefText(constValue)});
            codes.append(code);
        }else if(funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            getInfo_QuoteData(info);
            if(info.isConst || info.isDummyVar)return 0;

            PsdlIR_Compiler::MiddleNode loadCode;
            PsdlIR_Compiler::MiddleNode addCode;
            PsdlIR_Compiler::MiddleNode storeCode;

            if(selfIncreasing>0){
                addCode.nodeType = "ADD";
            }else{
                addCode.nodeType = "SUB";
                selfIncreasing = - selfIncreasing;
            }

            int bytes;
            DataBaseType type = info.baseTypeInfo.baseType;
            if(info.pointerLevel==0 &&
               !(type==DataBaseType_VOID || type==DataBaseType_FUNPTR || type==DataBaseType_UNION || type==DataBaseType_STRUCT)){
                bytes = 1;
            }else if(info.pointerLevel==1){
                bytes = info.baseTypeInfo.dataBaseTypeBytes;
            }else if(info.pointerLevel>1){
                bytes = objInfo->CPU_BIT/8;
            }else return 0;
            IndexVN tmpVarIndex = objInfo->defineFuncTmpVar(info.baseTypeIndex,info.pointerLevel,*varsAtt);

            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(tmpVarIndex)->psdlName;


            loadCode.nodeType = "LOAD";
            loadCode.args = QStringList({tmpVarPsdlName,info.varPsdlName});
            codes.append(loadCode);

            FuncOperValue constValue((int)(bytes*selfIncreasing),*varsAtt,*objInfo);
            constValue = constValue.converType(info.baseTypeIndex,info.pointerLevel);
            addCode.args = QStringList({tmpVarPsdlName,tmpVarPsdlName,generateConstValueDefText(constValue)});
            codes.append(addCode);


            storeCode.nodeType = "STORE";
            storeCode.args = QStringList({tmpVarPsdlName,info.varPsdlName});
            codes.append(storeCode);
        }else return 0;
        return 1;
    }

    ////////////指针相关运算//////////////
    //解指针的引用 C语法: *指针变量
    FuncOperValue dereference(){
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        runSuffixIncrementOper(suffixIncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;

        IndexVN voidType = objInfo->getDataTypeInfoNode()->searchDataType({"void"});
        FuncOperValue value;
        if(funcOperValueType == FuncOperValueType_Function){
            return *this;
        }else if(funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            if(getInfo_RegData(info)==0)return value;

            if(info.baseTypeInfo.baseType == DataBaseType_FUNPTR && info.pointerLevel==0){
                this->bodyAtt.append(suffixIncrementCode);
                return *this;
            }else if(info.pointerLevel==0)return value;

            //指针转为引用不需要任何操作
            value.varIndex = objInfo->defineFuncLinkVar(voidType,1,info.varPsdlName);


            value.quote_IsConst = info.isConst;//引用是否是常量取决于指针是否是常量
            value.quote_IsPointerConst = info.isPointerConst;
            value.isDummyVar = 0;//引用必然不是虚变量，可以赋值


            value.quote_ArraySize.clear();
            value.quoteOrConst_Type = info.baseTypeIndex;
            value.quoteOrConst_PointerLevel = info.pointerLevel-1;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_QuoteData;
        }else if(funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            if(getInfo_QuoteData(info)==0)return value;

            if(info.baseTypeInfo.baseType == DataBaseType_FUNPTR && info.pointerLevel==0 && info.arraySize.length()==0){
                this->bodyAtt.append(suffixIncrementCode);
                return *this;
            }else if(info.pointerLevel==0 && info.arraySize.length()==0)return value;

            if(info.arraySize.length() != 0){
                value.varIndex = objInfo->defineFuncLinkVar(voidType,1,info.varPsdlName);
                value.isDummyVar = info.isDummyVar;//引用是否是虚变量，取决于数组是否是虚变量
            }else{
                value.varIndex = objInfo->defineFuncTmpVar(voidType,1,*varsAtt);
                value.isDummyVar = 0;//引用必然不是虚变量，可以赋值
            }
            value.quote_IsConst = info.isConst;
            value.quote_IsPointerConst = info.isPointerConst;
            value.quote_ArraySize = info.arraySize;
            value.quoteOrConst_Type = info.baseTypeIndex;
            value.quoteOrConst_PointerLevel = info.pointerLevel;
            if(info.arraySize.length()!=0){
                value.quote_ArraySize.removeFirst();
            }else{
                value.quoteOrConst_PointerLevel-=1;
            }

            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_QuoteData;
            if(info.arraySize.length() == 0){
                QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
                //生成解引用指令
                PsdlIR_Compiler::MiddleNode code;
                code.nodeType = "LOAD";
                code.args = QStringList({tmpVarPsdlName,info.varPsdlName});
                value.bodyAtt.append(code);
            }
        }else if(funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            if(getInfo_ConstValue(info) == 0)return value;
            if(info.pointerLevel==0)return value;

            value.varIndex = objInfo->defineFuncTmpVar(voidType,1,*varsAtt);
            value.quote_IsConst = 0;//引用是否是常量取决于指针是否是常量
            value.quote_IsPointerConst = 0;
            value.isDummyVar = 0;//引用必然不是虚变量，可以赋值

            value.quote_ArraySize = QList<int>();
            value.quoteOrConst_Type = info.baseTypeIndex;
            value.quoteOrConst_PointerLevel = info.pointerLevel - 1;

            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_QuoteData;


            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            //生成解引用指令
            PsdlIR_Compiler::MiddleNode code;
            code.nodeType = "MOV";
            code.args = QStringList({tmpVarPsdlName,generateArgText(*this)});
            value.bodyAtt.append(code);
        }else return value;

        value.bodyAtt.append(suffixIncrementCode);
        return value;
    }

    //取变量地址 C语法: &被取地址变量
    FuncOperValue getAddress(){
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        runSuffixIncrementOper(suffixIncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        FuncOperValue value;
        if(funcOperValueType == FuncOperValueType_Function){
            return *this;
        }else if(funcOperValueType == FuncOperValueType_RegData){

            RegDataInfo info;
            if(getInfo_RegData(info)==0)return value;
            if(info.isDummyVar)return value;//虚变量不能被取地址

            value.varIndex = objInfo->defineFuncTmpVar(info.baseTypeIndex,info.pointerLevel+1,*varsAtt);
            value.isDummyVar = 1;//取地址得到的变量必然是一个虚拟变量
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_RegData;

            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            //生成取地址指令
            PsdlIR_Compiler::MiddleNode code;
            code.nodeType = "ADDRESS";
            code.args = QStringList({tmpVarPsdlName,info.varPsdlName,"0D"});
            value.bodyAtt.append(code);
        }else if(funcOperValueType == FuncOperValueType_QuoteData){
            //引用类型本质上就是一个指针。所以不需要任何指令。只要将引用转为指针即可
            QuoteDataInfo info;
            if(getInfo_QuoteData(info)==0)return value;
            if(info.arraySize.length() != 0){//如果是数组取地址，得到的是 数组 +1维 的引用
                value.varIndex = objInfo->defineFuncLinkVar(info.baseTypeIndex,1,info.varPsdlName);
                value.quote_IsConst = info.isConst;
                value.quote_IsPointerConst = info.isPointerConst;
                value.isDummyVar = info.isDummyVar;
                value.quote_ArraySize = info.arraySize;
                value.quote_ArraySize.prepend(1);
                value.quoteOrConst_Type = info.baseTypeIndex;
                value.quoteOrConst_PointerLevel = info.pointerLevel;
                value.objInfo = objInfo;
                value.varsAtt = varsAtt;
                value.bodyAtt = bodyAtt;
                value.funcOperValueType = FuncOperValueType_QuoteData;
                value.bodyAtt.append(suffixIncrementCode);
                return value;
            }


            value.varIndex = objInfo->defineFuncLinkVar(info.baseTypeIndex,info.pointerLevel+1,info.varPsdlName);
            value.isDummyVar = 1;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_RegData;
        }else return value;
        value.bodyAtt.append(suffixIncrementCode);
        return value;
    }
    ////////////获取引用相关运算////////////
    //获取结构体变量中属性的引用 C语法: 结构体变量.属性名
    FuncOperValue getStructAtt(QString attName){
        QuoteDataInfo info;
        FuncOperValue value;
        if(getInfo_QuoteData(info)==0)return value;

        if(info.pointerLevel || info.arraySize.length()!=0)return value;
        DataBaseType type = info.baseTypeInfo.baseType;
        if(type!=DataBaseType_STRUCT && type!=DataBaseType_UNION)return value;
        if(!info.baseTypeInfo.attInfos.contains(attName))return value;

        StructUnion_AttInfo att = info.baseTypeInfo.attInfos[attName];

        //如果是属性偏移值为0，直接将当前的引用输出出去，只是将当前引用的类型转为属性类型
        if(att.offset == 0){
            value = *this;
            value.quote_IsConst = info.isConst || att.isConst;
            value.quote_IsPointerConst = att.isPointerConst;
            value.quoteOrConst_Type = att.typeIndex;
            value.quote_ArraySize = att.arraySize;
            value.quoteOrConst_PointerLevel = att.pointerLevel;
            return value;
        }else{
            IndexVN voidType = objInfo->getDataTypeInfoNode()->searchDataType({"void"});
            value.varIndex = objInfo->defineFuncTmpVar(voidType,1,*varsAtt);
            value.quoteOrConst_Type = att.typeIndex;
            value.quote_ArraySize = att.arraySize;
            value.quote_IsConst = info.isConst || att.isConst;
            value.quote_IsPointerConst = info.isPointerConst;
            value.isDummyVar = info.isDummyVar;
            value.quoteOrConst_PointerLevel = att.pointerLevel;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_QuoteData;

            //生成psdl的指令
            PsdlIR_Compiler::MiddleNode getStructAttCode;
            getStructAttCode.nodeType = "ADD";
            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;

            QString offset = "<POINTER:"+QString::number(att.offset)+"D>";
            getStructAttCode.args = QStringList({tmpVarPsdlName,info.varPsdlName,offset});
            value.bodyAtt.append(getStructAttCode);
        }
        return value;
    }
    //获取结构体指针变量中属性的引用 C语法: 结构体指针->属性名
    FuncOperValue getPointerStructAtt(QString attName){
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCodes;
        runSuffixIncrementOper(suffixIncrementCodes,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        FuncOperValue value;
        DataBaseType type;
        StructUnion_AttInfo att;
        QString varPsdlName;
        long long constPtrValue;
        bool ptrIsConst; //指针是否是常量指针
        if(funcOperValueType == FuncOperValueType_RegData){
             RegDataInfo info;
             if(getInfo_RegData(info)==0)return value;
             if(info.pointerLevel != 1)return value;
             type = info.baseTypeInfo.baseType;
             if(type!=DataBaseType_STRUCT && type!=DataBaseType_UNION)return value;
             if(!info.baseTypeInfo.attInfos.contains(attName))return value;
             att = info.baseTypeInfo.attInfos[attName];
             varPsdlName = info.varPsdlName;
             ptrIsConst = info.isConst;
        }else if(funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            if(getInfo_QuoteData(info)==0)return value;
            if(info.pointerLevel != 1  || info.arraySize.length()!=0)return value;
            type = info.baseTypeInfo.baseType;
            if(type!=DataBaseType_STRUCT && type!=DataBaseType_UNION)return value;
            if(!info.baseTypeInfo.attInfos.contains(attName))return value;
            att = info.baseTypeInfo.attInfos[attName];
            varPsdlName = info.varPsdlName;
            ptrIsConst = info.isConst;
        }else if(funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            if(getInfo_ConstValue(info) == 0)return value;
            if(info.pointerLevel!=1)return value;
            type = info.baseTypeInfo.baseType;
            if(type!=DataBaseType_STRUCT && type!=DataBaseType_UNION)return value;
            if(!info.baseTypeInfo.attInfos.contains(attName))return value;
            att = info.baseTypeInfo.attInfos[attName];
            ptrIsConst = 0;
            constPtrValue = info.constIntValue;
        }else{
            return value;
        }
        IndexVN voidType = objInfo->getDataTypeInfoNode()->searchDataType({"void"});


        if(att.offset == 0 && funcOperValueType == FuncOperValueType_RegData){
            value.varIndex = objInfo->defineFuncLinkVar(voidType,1,varPsdlName);

        }else{
            value.varIndex = objInfo->defineFuncTmpVar(voidType,1,*varsAtt);
        }

        value.quoteOrConst_Type = att.typeIndex;
        value.quote_ArraySize = att.arraySize;
        value.quote_IsConst = ptrIsConst || att.isConst;//解得的属性引用是否是常量取决于指针是否是常量
        value.quote_IsPointerConst = att.isPointerConst;
        value.isDummyVar = 0;//属性引用必然非虚变量
        value.quoteOrConst_PointerLevel = att.pointerLevel;
        value.objInfo = objInfo;
        value.varsAtt = varsAtt;
        value.bodyAtt = bodyAtt;
        value.funcOperValueType = FuncOperValueType_QuoteData;


        if(att.offset != 0 && funcOperValueType == FuncOperValueType_RegData){
            //生成psdl的指令
            PsdlIR_Compiler::MiddleNode getStructAttCode;
            getStructAttCode.nodeType = "ADD";
            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            QString offset = "<POINTER:"+QString::number(att.offset)+"D>";
            getStructAttCode.args = QStringList({tmpVarPsdlName,varPsdlName,offset});
            value.bodyAtt.append(getStructAttCode);
        }else if(att.offset != 0 && funcOperValueType == FuncOperValueType_QuoteData){
            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            QString offset = "<POINTER:"+QString::number(att.offset)+"D>";

            //读取结构体指针的值到临时变量
            PsdlIR_Compiler::MiddleNode loadCode;
            loadCode.nodeType = "LOAD";
            loadCode.args = QStringList({tmpVarPsdlName,varPsdlName});
            value.bodyAtt.append(loadCode);

            //指针值+偏移值
            PsdlIR_Compiler::MiddleNode getStructAttCode;
            getStructAttCode.nodeType = "ADD";
            getStructAttCode.args = QStringList({tmpVarPsdlName,tmpVarPsdlName,offset});
            value.bodyAtt.append(getStructAttCode);
        }else if(att.offset == 0 && funcOperValueType == FuncOperValueType_QuoteData){
            //读取结构体指针的值到临时变量
            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            PsdlIR_Compiler::MiddleNode loadCode;
            loadCode.nodeType = "LOAD";
            loadCode.args = QStringList({tmpVarPsdlName,varPsdlName});
            value.bodyAtt.append(loadCode);
        }else if(funcOperValueType == FuncOperValueType_ConstValue){
            FuncOperValue ptrVar((uint64_t)constPtrValue + att.offset,*this->varsAtt,*this->objInfo);
            ConstValueInfo info;
            getInfo_ConstValue(info);
            ptrVar = ptrVar.converType(info.baseTypeIndex,info.pointerLevel);
            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            PsdlIR_Compiler::MiddleNode loadCode;
            loadCode.nodeType = "MOV";
            loadCode.args = QStringList({tmpVarPsdlName,generateConstValueDefText(ptrVar)});
            value.bodyAtt.append(loadCode);
        }
        value.bodyAtt.append(suffixIncrementCodes);

        return value;
    }
    //获取数组指针/引用变量的某个单元的引用  C语法: 数组/指针变量[索引号]
    FuncOperValue operator[](FuncOperValue &index){
        if(!varCanOperation(index,{CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY,CanOperationAddRequire_POINTER})){
            return FuncOperValue();
        }
        //让数组的地址 + 数组单元字节数*索引号
        int unitBytes;
        if((unitBytes = generateIndexSpan(*this)) == 0){
            return FuncOperValue();
        }
        //生成上一个后缀++ -- 的代码
        QList<PsdlIR_Compiler::MiddleNode> value_IncrementCode,index_IncrementCode;
        this->runSuffixIncrementOper(value_IncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        index.runSuffixIncrementOper(index_IncrementCode,index.suffixIncrementValue);
        index.suffixIncrementValue = 0;

        FuncOperValue value;
        if(judgeVarIsPointer(*this)){//指针的转换
            FuncOperValue tmp = this->operator+(index);//指针+索引号
            value = tmp.dereference();//解引用指针
        }else if(judgeVarIsArray(*this)){
            FuncOperValue tmp;
            if(unitBytes != 1){
                int powerToTwo = isPowerOfTwo(unitBytes);
                if(powerToTwo == -1){
                    FuncOperValue unitBytesValue(unitBytes,*varsAtt,*objInfo);
                    tmp = index*unitBytesValue;
                }else{
                    FuncOperValue powerToTwoValue(powerToTwo,*varsAtt,*objInfo);
                    tmp = index<<powerToTwoValue;
                }
            }else{
                tmp = index;
            }
            tmp = tmp.converType(objInfo->getDataTypeInfoNode()->searchDataType({"void"}),1);
            QuoteDataInfo info;
            getInfo_QuoteData(info);
            value.quote_IsConst = info.isConst;//元素是否为常量，取决于数组是否为常量
            value.quote_IsPointerConst = info.isPointerConst;
            value.isDummyVar = info.isDummyVar;

            value.varIndex = objInfo->defineFuncTmpVar(objInfo->getDataTypeInfoNode()->searchDataType({"void"}),1,*varsAtt);
            value.quote_ArraySize = info.arraySize;
            value.quote_ArraySize.removeLast();
            value.quoteOrConst_Type = info.baseTypeIndex;
            value.quoteOrConst_PointerLevel = info.pointerLevel;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt + tmp.bodyAtt;
            value.funcOperValueType = FuncOperValueType_QuoteData;
            QString tmpVarName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            if(tmp.funcOperValueType == FuncOperValueType_ConstValue){
                //偏移的值不可以是浮点数
                ConstValueInfo offsetInfo;
                tmp.getInfo_ConstValue(offsetInfo);
                if(offsetInfo.constValueIsFloat){
                    return FuncOperValue();
                }
                PsdlIR_Compiler::MiddleNode code;
                code.nodeType = "ADD";
                code.args = QStringList({tmpVarName,info.varPsdlName,generateArgText(tmp)});
                value.bodyAtt.append(code);
            }else if(tmp.funcOperValueType == FuncOperValueType_RegData){
                VarTypeInfo varInfo = getVarTypeInfo(tmp);
                if(varInfo != VarTypeInfo_INT_UINT8&&
                   varInfo != VarTypeInfo_INT_UINT16&&
                   varInfo != VarTypeInfo_INT_UINT32&&
                   varInfo != VarTypeInfo_INT_UINT64){
                    return FuncOperValue();
                }
                PsdlIR_Compiler::MiddleNode code;
                code.nodeType = "ADD";
                code.args = QStringList({tmpVarName,info.varPsdlName,generateArgText(tmp)});
                value.bodyAtt.append(code);
            }
        }else return FuncOperValue();
        value.bodyAtt.append(value_IncrementCode);
        value.bodyAtt.append(index_IncrementCode);
        return value;
    }

    ///////////赋值相关运算/////////////////
    //前置++ C语言语法:   ++变量
    FuncOperValue operator_PrefixAdd(){
        FuncOperValue v2 = FuncOperValue((long long)1,*varsAtt,*objInfo);
        return this->operator+=(v2);
    }
    //前置-- C语言语法:   --变量
    FuncOperValue operator_PrefixSub(){
        FuncOperValue v2 = FuncOperValue((long long)1,*varsAtt,*objInfo);
        return this->operator =(v2);
    }

    //后置 ++ 或 --
    FuncOperValue postPostAddOrSub(int s){
        FuncOperValue value;
        if(!isValue())return value;

        if(funcOperValueType==FuncOperValueType_RegData){
            RegDataInfo info;
            getInfo_RegData(info);
            if(info.isDummyVar)return value;

            DataBaseType type = info.baseTypeInfo.baseType;
            if((type == DataBaseType_FUNPTR ||
                type == DataBaseType_STRUCT ||
                type == DataBaseType_UNION) && info.pointerLevel==0)return value;

            value = *this;
            value.suffixIncrementValue+=s;
            return value;
        }else if(funcOperValueType==FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            getInfo_QuoteData(info);
            DataBaseType type = info.baseTypeInfo.baseType;
            if((type == DataBaseType_FUNPTR ||
                type == DataBaseType_STRUCT ||
                type == DataBaseType_UNION) && info.pointerLevel==0)return value;
            value = *this;
            value.suffixIncrementValue+=s;
            return value;
        }else if(funcOperValueType==FuncOperValueType_ConstValue){
            return value;
        }
        return value;
    }

    //后置++ C语言语法:   变量++
    FuncOperValue operator++(int){
        QList<PsdlIR_Compiler::MiddleNode> suffix;
        this->runSuffixIncrementOper(suffix,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;

        FuncOperValue v = postPostAddOrSub(1);
        v.bodyAtt.append(suffix);
        return v;
    }
    //后置-- C语言语法:   变量--
    FuncOperValue operator--(int){
        QList<PsdlIR_Compiler::MiddleNode> suffix;
        this->runSuffixIncrementOper(suffix,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        FuncOperValue v = postPostAddOrSub(-1);
        v.bodyAtt.append(suffix);
        return v;
    }

    struct FuncArgType{
        IndexVN typeIndex;
        int pointerLevel;
    };
    //获取一个变量的类型信息
    enum VarTypeInfo{
        VarTypeInfo_ERROR,
        VarTypeInfo_INT_UINT8,
        VarTypeInfo_INT_UINT16,
        VarTypeInfo_INT_UINT32,
        VarTypeInfo_INT_UINT64,
        VarTypeInfo_FLOAT,
        VarTypeInfo_DOUBLE,
        VarTypeInfo_STRUCT_UNION,
        VarTypeInfo_ARRAY,
    };
    static VarTypeInfo getVarTypeInfo(FuncOperValue value){
        DataBaseType type;
        int pointerLevel;
        QList<int> arraySize;
        if(value.funcOperValueType==FuncOperValueType_RegData){
            RegDataInfo info;
            value.getInfo_RegData(info);
            pointerLevel = info.pointerLevel;
            type = info.baseTypeInfo.baseType;
        }else if(value.funcOperValueType==FuncOperValueType_ConstValue){
            ConstValueInfo info;
            value.getInfo_ConstValue(info);
            pointerLevel = info.pointerLevel;
            type = info.baseTypeInfo.baseType;
        }else if(value.funcOperValueType==FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            value.getInfo_QuoteData(info);
            pointerLevel = info.pointerLevel;
            type = info.baseTypeInfo.baseType;
            arraySize = info.arraySize;
        }else if(value.funcOperValueType==FuncOperValueType_Function){
            DataTypeInfo info;
            QString funName;
            IndexVN typeIndex;

            value.getInfo_Function(funName,typeIndex,info);

            type = info.baseType;

            pointerLevel = 0;
            arraySize.clear();
        }else return VarTypeInfo_ERROR;


        if(arraySize.length()!=0)return VarTypeInfo_ARRAY;//数组不算变量，输出出错
        return getVarTypeInfo(type,pointerLevel,*value.objInfo);
    }
    static VarTypeInfo getVarTypeInfo(DataBaseType type,int pointerLevel,SrcObjectInfo &objInfo){
        if(pointerLevel>0 || type == DataBaseType_FUNPTR){
            if(objInfo.CPU_BIT == 8){
                return VarTypeInfo_INT_UINT8;
            }else if(objInfo.CPU_BIT == 16){
                return VarTypeInfo_INT_UINT16;
            }else if(objInfo.CPU_BIT == 32){
                return VarTypeInfo_INT_UINT32;
            }else if(objInfo.CPU_BIT == 64){
                return VarTypeInfo_INT_UINT64;
            }
            return VarTypeInfo_ERROR;
        }

        if(type == DataBaseType_INT || type == DataBaseType_UINT || type == DataBaseType_ENUM)return VarTypeInfo_INT_UINT32;
        else if(type == DataBaseType_SHORT || type == DataBaseType_USHORT)return VarTypeInfo_INT_UINT16;
        else if(type == DataBaseType_CHAR || type == DataBaseType_UCHAR)return VarTypeInfo_INT_UINT8;
        else if(type == DataBaseType_LONG || type == DataBaseType_ULONG)return VarTypeInfo_INT_UINT64;
        else if(type == DataBaseType_STRUCT || type == DataBaseType_UNION)return VarTypeInfo_STRUCT_UNION;
        else if(type == DataBaseType_FLOAT)return VarTypeInfo_FLOAT;
        else if(type == DataBaseType_DOUBLE)return VarTypeInfo_DOUBLE;
        return VarTypeInfo_ERROR;
    }
    static VarTypeInfo getVarTypeInfo(FuncArgType argType,SrcObjectInfo &objInfo){
        return getVarTypeInfo(objInfo.getDataTypeInfoNode()->getDataType(argType.typeIndex)->baseType,
                              argType.pointerLevel,
                              objInfo);
    }

    //=赋值 C语言语法: 变量=赋值
    FuncOperValue assignment(FuncOperValue &assignValue){

        if(judgeVarIsArray(*this))return FuncOperValue();
        QList<PsdlIR_Compiler::MiddleNode> value_IncrementCode,assignValue_IncrementCode;

        this->runSuffixIncrementOper(value_IncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        assignValue.runSuffixIncrementOper(assignValue_IncrementCode,assignValue.suffixIncrementValue);
        assignValue.suffixIncrementValue = 0;

        FuncOperValue retValue;
        if(assignValue.funcOperValueType==FuncOperValueType_Function){
            if(!judgeVarIsFuncPointer(*this) && !judgeVarIsPointer(*this)){
                return retValue;
            }

            QString funName;
            DataTypeInfo funTypeInfo;
            IndexVN typeIndex;
            assignValue.getInfo_Function(funName,typeIndex,funTypeInfo);

            //函数赋值
            if(funcOperValueType==FuncOperValueType_RegData){
                RegDataInfo valueInfo;
                this->getInfo_RegData(valueInfo);

                if(valueInfo.isDummyVar || valueInfo.isPointerConst){
                    return retValue;
                }

                PsdlIR_Compiler::MiddleNode assignCode;
                assignCode.nodeType = "MOV";
                assignCode.args = QStringList({valueInfo.varPsdlName,"["+funName+"]"});

                retValue = *this;
                retValue.bodyAtt.append(assignValue.bodyAtt);
                retValue.bodyAtt.append(assignCode);
            }else if(funcOperValueType==FuncOperValueType_QuoteData){
                QuoteDataInfo valueInfo;
                this->getInfo_QuoteData(valueInfo);
                if(valueInfo.isDummyVar || valueInfo.isPointerConst){
                    return retValue;
                }
                IndexVN tmpIndex = objInfo->defineFuncTmpVar(valueInfo.baseTypeIndex,valueInfo.pointerLevel,*varsAtt);

                QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(tmpIndex)->psdlName;
                PsdlIR_Compiler::MiddleNode getFunAddCode;
                getFunAddCode.nodeType = "MOV";
                getFunAddCode.args = QStringList({tmpVarPsdlName,"["+funName+"]"});

                PsdlIR_Compiler::MiddleNode assignCode;
                assignCode.nodeType = "STORE";
                assignCode.args = QStringList({tmpVarPsdlName,valueInfo.varPsdlName});

                retValue = *this;
                retValue.bodyAtt.append(assignValue.bodyAtt);
                retValue.bodyAtt.append(getFunAddCode);
                retValue.bodyAtt.append(assignCode);
            }else return retValue;
        }else if(funcOperValueType==FuncOperValueType_RegData){
            bool valueIsPointer =judgeVarIsPointer(*this,1) || judgeVarIsFuncPointer(*this);
            bool assignValueIsPointer = judgeVarIsPointer(assignValue) || judgeVarIsFuncPointer(assignValue);
            bool su;
            if(assignValueIsPointer){//赋值变量是指针
                //被赋值变量必须也要是指针
                su = valueIsPointer;
            }else if(valueIsPointer){//被赋值变量是指针
                //赋值变量不可是浮点数
                su = assignValueIsPointer || varCanOperation(assignValue,{CanOperationAddRequire_FLOAT});
            }else{//都不是指针，只要是数值就可以随意赋值
                su = varCanOperation(*this,{}) && varCanOperation(assignValue,{});
            }
            if(su==0)return retValue;

            RegDataInfo valueInfo;
            this->getInfo_RegData(valueInfo);
            if(valueInfo.isDummyVar || (!valueIsPointer && valueInfo.isConst) || (valueIsPointer && valueInfo.isPointerConst)){
                return retValue;
            }
            assignValue = generateLoadVarOper(assignValue);//如果v2是引用,读取引用的数据到一个堆栈临时变量。如果是数组型引用，解引用指针
            PsdlIR_Compiler::MiddleNode assignCode;
            assignCode.nodeType = "MOV";
            assignCode.args = QStringList({generateArgText(*this),generateArgText(assignValue)});

            retValue = *this;
            retValue.bodyAtt.append(assignValue.bodyAtt);
            retValue.bodyAtt.append(assignCode);
        }else if(funcOperValueType==FuncOperValueType_QuoteData &&
                 (assignValue.funcOperValueType==FuncOperValueType_RegData || assignValue.funcOperValueType==FuncOperValueType_ConstValue)){
            bool valueIsPointer =judgeVarIsPointer(*this,1) || judgeVarIsFuncPointer(*this);
            bool assignValueIsPointer = judgeVarIsPointer(assignValue) || judgeVarIsFuncPointer(assignValue);
            bool su;
            if(assignValueIsPointer){//赋值变量是指针
                //被赋值变量必须也要是指针
                su = valueIsPointer;
            }else if(valueIsPointer){//被赋值变量是指针
                //赋值变量不可是浮点数
                su = assignValueIsPointer || varCanOperation(assignValue,{CanOperationAddRequire_FLOAT});
            }else{//都不是指针，只要是数值就可以随意赋值
                su = varCanOperation(*this,{}) && varCanOperation(assignValue,{});
            }
            if(su==0)return retValue;
            QuoteDataInfo valueInfo;
            this->getInfo_QuoteData(valueInfo);
            if(valueInfo.isDummyVar || (!valueIsPointer && valueInfo.isConst)  || (valueIsPointer && valueInfo.isPointerConst)){
                return retValue;
            }
            //判断数据类型是否需要转换
            VarTypeInfo valueType = getVarTypeInfo(*this);
            VarTypeInfo assignValueType = getVarTypeInfo(assignValue);
            //数据类型不同，需要转换。如果是常赋值量，也必须先将常量存到一个寄存器
            if(valueType != assignValueType || assignValue.funcOperValueType == FuncOperValueType_ConstValue){
                IndexVN tmpVarIndex = objInfo->defineFuncTmpVar(valueInfo.baseTypeIndex,valueInfo.pointerLevel,*varsAtt);
                QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(tmpVarIndex)->psdlName;
                PsdlIR_Compiler::MiddleNode converTypeCode;
                converTypeCode.nodeType = "MOV";
                converTypeCode.args = QStringList({tmpVarPsdlName,generateArgText(assignValue)});
                PsdlIR_Compiler::MiddleNode assignCode;
                assignCode.nodeType = "STORE";
                assignCode.args = QStringList({tmpVarPsdlName,valueInfo.varPsdlName});
                retValue = *this;
                retValue.bodyAtt.append(assignValue.bodyAtt);
                retValue.bodyAtt.append(converTypeCode);
                retValue.bodyAtt.append(assignCode);
            }else{
                PsdlIR_Compiler::MiddleNode assignCode;
                assignCode.nodeType = "STORE";
                assignCode.args = QStringList({generateArgText(assignValue),valueInfo.varPsdlName});
                retValue = *this;
                retValue.bodyAtt.append(assignValue.bodyAtt);
                retValue.bodyAtt.append(assignCode);
            }
        }else if(funcOperValueType==FuncOperValueType_QuoteData && funcOperValueType==FuncOperValueType_QuoteData){
            //判断双方数据类型是否完全相同，完全相同可以直接拷贝数据。如果不同需要加载到寄存器进行转换
            //判断数据类型是否需要转换
            VarTypeInfo valueType = getVarTypeInfo(*this);
            VarTypeInfo assignValueType = getVarTypeInfo(assignValue);
            if(valueType==VarTypeInfo_ERROR ||
               assignValueType==VarTypeInfo_ERROR ||
               valueType == VarTypeInfo_ARRAY)return retValue;


            QuoteDataInfo valueInfo,assignValueInfo;
            assignValue.getInfo_QuoteData(assignValueInfo);
            this->getInfo_QuoteData(valueInfo);
            bool valueIsPointer =judgeVarIsPointer(*this,1) || judgeVarIsFuncPointer(*this);
            if(valueInfo.isDummyVar || (!valueIsPointer && valueInfo.isConst)  || (valueIsPointer && valueInfo.isPointerConst)){
                return retValue;
            }
            if(assignValueType == VarTypeInfo_ARRAY){
                //数组型的赋值
                IndexVN voidType = objInfo->getDataTypeInfoNode()->searchDataType({"void"});
                QuoteDataInfo info;
                assignValue.getInfo_QuoteData(info);
                //将数组转为void*指针
                FuncOperValue tmp;
                tmp.varIndex = assignValue.objInfo->defineFuncLinkVar(voidType,1,info.varPsdlName);
                tmp.isDummyVar = 1;
                tmp.objInfo = assignValue.objInfo;
                tmp.varsAtt = assignValue.varsAtt;
                tmp.bodyAtt = assignValue.bodyAtt;
                tmp.funcOperValueType = FuncOperValueType_RegData;

                //将数组的指针作为 数值 赋给 被赋值变量
                retValue = this->assignment(tmp);
            }else{

                int assignValueTypeBytes = valueInfo.pointerLevel ? objInfo->CPU_BIT/8 :valueInfo.baseTypeInfo.dataBaseTypeBytes;
                if(valueType==VarTypeInfo_STRUCT_UNION || (valueType == assignValueType && assignValueTypeBytes > objInfo->CPU_BIT/8)){
                    //相同数值型数据且字节数<=(CPU位数/8)的赋值 或者结构体变量的赋值
                    if(valueType==VarTypeInfo_STRUCT_UNION &&
                       valueInfo.baseTypeIndex != assignValueInfo.baseTypeIndex)return retValue;

                    int varBytes;
                    if(assignValueInfo.pointerLevel == 0){
                        varBytes = assignValueInfo.baseTypeInfo.dataBaseTypeBytes;
                    }else{
                        varBytes = objInfo->CPU_BIT/8;
                    }

                    //结构体变量的赋值
                    PsdlIR_Compiler::MiddleNode assignCode;
                    assignCode.nodeType = "ARRCOPY";

                    QString copyByteCountText = QString::number(varBytes)+"D";
                    assignCode.args = QStringList({valueInfo.varPsdlName,assignValueInfo.varPsdlName,copyByteCountText});
                    retValue = *this;
                    retValue.bodyAtt.append(assignValue.bodyAtt);
                    retValue.bodyAtt.append(assignCode);
                }else{//不同数值型数据的赋值

                    //创建2个临时变量
                    IndexVN assignValue_Tmp = objInfo->defineFuncTmpVar(assignValueInfo.baseTypeIndex,
                                                                        assignValueInfo.pointerLevel,
                                                                        *varsAtt);
                    IndexVN value_Tmp = objInfo->defineFuncTmpVar(valueInfo.baseTypeIndex,
                                                                  valueInfo.pointerLevel,
                                                                  *varsAtt);
                    QString assignValue_TmpName = objInfo->getVarDefineInfoNode()->getVar(assignValue_Tmp)->varName;
                    QString value_TmpName = objInfo->getVarDefineInfoNode()->getVar(value_Tmp)->varName;

                    //生成加载赋值的指令
                    PsdlIR_Compiler::MiddleNode loadCode;
                    loadCode.nodeType = "LOAD";
                    loadCode.args = QStringList({assignValue_TmpName,assignValueInfo.varPsdlName});

                    //生成转换数据类型的指令
                    PsdlIR_Compiler::MiddleNode converTypeCode;
                    converTypeCode.nodeType = "MOV";
                    converTypeCode.args = QStringList({value_TmpName,assignValue_TmpName});
                    //将转换后的数据回写到被赋值的指令
                    PsdlIR_Compiler::MiddleNode assignCode;
                    assignCode.nodeType = "STORE";
                    assignCode.args = QStringList({value_TmpName,valueInfo.varPsdlName});

                    retValue = *this;
                    retValue.bodyAtt.append(assignValue.bodyAtt);
                    retValue.bodyAtt.append(loadCode);
                    retValue.bodyAtt.append(converTypeCode);
                    retValue.bodyAtt.append(assignCode);
                }
            }
        }else{
            return retValue;
        }
        retValue.bodyAtt.append(value_IncrementCode);
        retValue.bodyAtt.append(assignValue_IncrementCode);
        return retValue;
    }

    //+=赋值 C语言语法: 变量+=赋值
    FuncOperValue operator+=(FuncOperValue &value){
        FuncOperValue tmp = (*this)+value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }
    //-=赋值 C语言语法: 变量-=赋值
    FuncOperValue operator-=(FuncOperValue &value){
        FuncOperValue tmp = (*this)-value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //*=赋值 C语言语法: 变量*=赋值
    FuncOperValue operator*=(FuncOperValue &value){
        FuncOperValue tmp = (*this)*value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }
    // /=赋值 C语言语法: 变量/=赋值
    FuncOperValue operator/=(FuncOperValue &value){
        FuncOperValue tmp = (*this)/value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }
    //%=赋值 C语言语法: 变量%=赋值
    FuncOperValue operator%=(FuncOperValue &value){
        FuncOperValue tmp = (*this)%value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //<<=赋值 C语言语法: 变量<<=赋值
    FuncOperValue operator<<=(FuncOperValue &value){
        FuncOperValue tmp = (*this)<<value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //>>=赋值 C语言语法: 变量>>=赋值
    FuncOperValue operator>>=(FuncOperValue &value){
        FuncOperValue tmp = (*this)>>value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //|=赋值 C语言语法: 变量|=赋值
    FuncOperValue operator|=(FuncOperValue &value){
        FuncOperValue tmp = (*this)|value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //&=赋值 C语言语法: 变量&=赋值
    FuncOperValue operator&=(FuncOperValue &value){
        FuncOperValue tmp = (*this)&value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //&=赋值 C语言语法: 变量^=赋值
    FuncOperValue operator^=(FuncOperValue &value){
        FuncOperValue tmp = (*this)^value;
        this->bodyAtt.clear();
        return this->assignment(tmp);
    }

    //函数功能:数据类型转换 C语法: (转换出的数据类型)被转换变量
    FuncOperValue converType(IndexVN converBaseType,
                             int converPointerLevel){
        DataTypeInfo * converBaseTypeInfo =  objInfo->getDataTypeInfoNode()->getDataType(converBaseType);
        DataBaseType converBaseTypeInfo_enum = converBaseTypeInfo->baseType;

        VarTypeInfo converType = getVarTypeInfo(converBaseTypeInfo->baseType,converPointerLevel,*objInfo);
        VarTypeInfo varType = getVarTypeInfo(*this);

        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        runSuffixIncrementOper(suffixIncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        if(varType == VarTypeInfo_ERROR || varType==VarTypeInfo_STRUCT_UNION ||
           converType == VarTypeInfo_ERROR || converType==VarTypeInfo_STRUCT_UNION)return FuncOperValue();
        if(funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            getInfo_QuoteData(info);
            PsdlIR_Compiler::MiddleNode loadCode;
            bool isUsingLoadCode = 0;//loadCode是否需要使用

            QString loadTmpVarName;//将引用的变量加载到寄存器指令
            IndexVN loadTmpVar;
            if(varType == VarTypeInfo_ARRAY){
                loadTmpVarName = info.varPsdlName;
                loadTmpVar = objInfo->defineFuncLinkVar(info.baseTypeIndex,1,loadTmpVarName);

                if(objInfo->CPU_BIT == 8){
                    varType = VarTypeInfo_INT_UINT8;
                }else if(objInfo->CPU_BIT == 16){
                    varType = VarTypeInfo_INT_UINT16;
                }else if(objInfo->CPU_BIT == 32){
                    varType = VarTypeInfo_INT_UINT32;
                }else if(objInfo->CPU_BIT == 64){
                    varType = VarTypeInfo_INT_UINT64;
                }else return FuncOperValue();
            }else{
                loadTmpVar = objInfo->defineFuncTmpVar(info.baseTypeIndex,info.pointerLevel,*varsAtt);
                loadTmpVarName = objInfo->getVarDefineInfoNode()->getVar(loadTmpVar)->psdlName;
                loadCode.nodeType = "LOAD";
                loadCode.args = QStringList({loadTmpVarName,info.varPsdlName});
                isUsingLoadCode = 1;
            }


            PsdlIR_Compiler::MiddleNode converCode;
            bool isUsingConverCode = 0;//loadCode是否需要使用

            QString converTmpVarName;//将引用的变量加载到寄存器指令
            IndexVN converTmpVar;
            if(varType == converType){
                converTmpVarName = loadTmpVarName;
                converTmpVar = objInfo->defineFuncLinkVar(converBaseType,converPointerLevel,converTmpVarName);
            }else{
                isUsingConverCode = 1;
                converTmpVar = objInfo->defineFuncTmpVar(converBaseType,converPointerLevel,*varsAtt);
                converTmpVarName = objInfo->getVarDefineInfoNode()->getVar(converTmpVar)->psdlName;
                converCode.nodeType = "MOV";
                converCode.args = QStringList({converTmpVarName,loadTmpVarName});
            }



            FuncOperValue value;
            value.varIndex = converTmpVar;
            value.isDummyVar = 1;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_RegData;

            if(isUsingLoadCode){
                value.bodyAtt.append(loadCode);
            }
            if(isUsingConverCode){
                value.bodyAtt.append(converCode);
            }
            value.bodyAtt.append(suffixIncrementCode);
            return value;
        }else if(funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            getInfo_RegData(info);
            if(varType == converType){
                FuncOperValue value;
                value.varIndex = objInfo->defineFuncLinkVar(converBaseType,converPointerLevel,info.varPsdlName);
                value.isDummyVar = 1;
                value.objInfo = objInfo;
                value.varsAtt = varsAtt;
                value.bodyAtt = bodyAtt;
                value.funcOperValueType = FuncOperValueType_RegData;
                return value;
            }
            FuncOperValue value;
            value.varIndex = objInfo->defineFuncTmpVar(converBaseType,converPointerLevel,*varsAtt);
            value.isDummyVar = 1;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_RegData;

            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            //生成取地址指令
            PsdlIR_Compiler::MiddleNode code;
            code.nodeType = "MOV";
            code.args = QStringList({tmpVarPsdlName,info.varPsdlName});
            value.bodyAtt.append(code);
            value.bodyAtt.append(suffixIncrementCode);
            return value;
        }else if(funcOperValueType == FuncOperValueType_ConstValue){
            GlobalInitValueTmp constVar = toGlobalInitValueTmp(*this);
            ConstValueType constConverType;

            if(converBaseTypeInfo_enum == DataBaseType_FUNPTR || converPointerLevel!= 0){
                constConverType = ConstValueType_POINTER;
            }else if(converBaseTypeInfo_enum == DataBaseType_INT || converBaseTypeInfo_enum == DataBaseType_ENUM){
                constConverType = ConstValueType_INT32;
            }else if(converBaseTypeInfo_enum == DataBaseType_UINT){
                constConverType = ConstValueType_UINT32;
            }else if(converBaseTypeInfo_enum == DataBaseType_SHORT){
                constConverType = ConstValueType_INT16;
            }else if(converBaseTypeInfo_enum == DataBaseType_USHORT){
                constConverType = ConstValueType_UINT16;
            }else if(converBaseTypeInfo_enum == DataBaseType_LONG){
                constConverType = ConstValueType_INT64;
            }else if(converBaseTypeInfo_enum == DataBaseType_ULONG){
                constConverType = ConstValueType_UINT64;
            }else if(converBaseTypeInfo_enum == DataBaseType_FLOAT){
                constConverType = ConstValueType_FLOAT;
            }else if(converBaseTypeInfo_enum == DataBaseType_DOUBLE){
                constConverType = ConstValueType_DOUBLE;
            }else if(converBaseTypeInfo_enum == DataBaseType_CHAR){
                constConverType = ConstValueType_INT8;
            }else if(converBaseTypeInfo_enum == DataBaseType_UCHAR){
                constConverType = ConstValueType_UINT8;
            }else{
                return FuncOperValue();
            }
            constVar = constVar.converType(constConverType);\
            FuncOperValue value = toFuncOperValue(constVar,*this->objInfo,*this->varsAtt,converBaseType,converPointerLevel);
            value.quoteOrConst_Type = converBaseType;
            value.quoteOrConst_PointerLevel = converPointerLevel;
            return value;
        }else if(funcOperValueType == FuncOperValueType_Function){
            DataTypeInfo info;
            QString funcName;
            IndexVN typeIndex;

            this->getInfo_Function(funcName,typeIndex,info);


            FuncOperValue value;
            value.varIndex = objInfo->defineFuncTmpVar(typeIndex,1,*varsAtt);
            value.isDummyVar = 1;
            value.objInfo = objInfo;
            value.varsAtt = varsAtt;
            value.bodyAtt = bodyAtt;
            value.funcOperValueType = FuncOperValueType_RegData;

            QString tmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(value.varIndex)->psdlName;
            //生成取地址指令
            PsdlIR_Compiler::MiddleNode code;
            code.nodeType = "MOV";
            code.args = QStringList({tmpVarPsdlName,"["+funcName+"]"});
            value.bodyAtt.append(code);
            if(converType == varType){
                FuncOperValue converValue;
                converValue.varIndex = objInfo->defineFuncLinkVar(converBaseType,converPointerLevel,tmpVarPsdlName);
                converValue.isDummyVar = 1;
                converValue.objInfo = objInfo;
                converValue.varsAtt = varsAtt;
                converValue.bodyAtt = value.bodyAtt;
                converValue.funcOperValueType = FuncOperValueType_RegData;
                return converValue;
            }else{
                FuncOperValue converValue;
                converValue.varIndex = objInfo->defineFuncTmpVar(converBaseType,converPointerLevel,*varsAtt);
                converValue.isDummyVar = 1;
                converValue.objInfo = objInfo;
                converValue.varsAtt = varsAtt;
                converValue.bodyAtt = value.bodyAtt;
                converValue.funcOperValueType = FuncOperValueType_RegData;
                QString converTmpVarPsdlName = objInfo->getVarDefineInfoNode()->getVar(converValue.varIndex)->psdlName;


                //生成转换类型指令
                PsdlIR_Compiler::MiddleNode converCode;
                converCode.nodeType = "MOV";
                converCode.args = QStringList({converTmpVarPsdlName,tmpVarPsdlName});
                converValue.bodyAtt.append(converCode);
                return converValue;
            }
        }else return FuncOperValue();
    }

    //////////数据运算相关运算///////////////
    //判断一个变量是否是可以直接被运算的。(至少不可被运算的有 结构体 共用体)
    enum CanOperationAddRequire{//附加要求
        CanOperationAddRequire_FLOAT,//不可是浮点数
        CanOperationAddRequire_POINTER,//不可是指针
        CanOperationAddRequire_ARRAY,//不可是数组
    };
    static bool varCanOperation(FuncOperValue &var,QList<CanOperationAddRequire> addRequires){

        DataBaseType baseType;
        int pointerLevel;
        QList<int> arraySize;


        if(var.funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            var.getInfo_QuoteData(info);
            baseType = info.baseTypeInfo.baseType;
            pointerLevel = info.pointerLevel;
            arraySize = info.arraySize;

        }else if(var.funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            var.getInfo_RegData(info);
            baseType = info.baseTypeInfo.baseType;
            pointerLevel = info.pointerLevel;
            arraySize.clear();

        }else if(var.funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            var.getInfo_ConstValue(info);
            baseType = info.baseTypeInfo.baseType;
            pointerLevel = info.pointerLevel;
            arraySize.clear();
        }else if(var.funcOperValueType == FuncOperValueType_Function){
            DataTypeInfo info;
            QString funName;
            IndexVN typeIndex;
            var.getInfo_Function(funName,typeIndex,info);
            baseType = info.baseType;
            pointerLevel = 0;
            arraySize.clear();
        }else return 0;

        if(arraySize.length() == 0 && pointerLevel==0 &&
                (baseType==DataBaseType_STRUCT ||
                 baseType==DataBaseType_UNION))return 0;


        for(int i = 0;i<addRequires.length();i++){
            if(addRequires[i] == CanOperationAddRequire_FLOAT &&
               (baseType==DataBaseType_FLOAT || baseType==DataBaseType_DOUBLE)){
                return 0;
            }else if(addRequires[i] == CanOperationAddRequire_POINTER &&
                     ((pointerLevel!=0 ||baseType==DataBaseType_FUNPTR)  && arraySize.length()==0)){
                return 0;
            }else if(addRequires[i] == CanOperationAddRequire_ARRAY && arraySize.length()!=0){
                return 0;
            }
        }



        return 1;
    }

    //判断变量是否是函数指针
    static bool judgeVarIsFuncPointer(FuncOperValue &var){
        if(var.funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            var.getInfo_QuoteData(info);
            if(info.pointerLevel!=0 || info.arraySize.length()!=0)return 0;
            return info.baseTypeInfo.baseType==DataBaseType_FUNPTR;
        }else if(var.funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            var.getInfo_RegData(info);
            if(info.pointerLevel!=0)return 0;
            return info.baseTypeInfo.baseType==DataBaseType_FUNPTR;
        }else if(var.funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            var.getInfo_ConstValue(info);
            if(info.pointerLevel!=0)return 0;
            return info.baseTypeInfo.baseType==DataBaseType_FUNPTR;
        }else return 0;
    }
    //判断变量是否是数组
    static bool judgeVarIsArray(FuncOperValue &var){
        if(var.funcOperValueType == FuncOperValueType_QuoteData){
            return var.quote_ArraySize.length() != 0;
        }else return 0;
    }

    //判断一个变量是否是指针
    //isLeftVar是否是赋值表达式的左值。表达式右值中数组的引用可被看作是指针来运算，赋值表达式右值依然是数组
    static bool judgeVarIsPointer(FuncOperValue &var,bool isLeftVar = 0){
        if(var.funcOperValueType == FuncOperValueType_QuoteData){
            if(isLeftVar && var.quote_ArraySize.length()!=0){
                return 0;
            }
            int pointerLevel = var.quoteOrConst_PointerLevel;
            return pointerLevel!=0;
        }else if(var.funcOperValueType == FuncOperValueType_RegData){
            return var.objInfo->getVarDefineInfoNode()->getVar(var.varIndex)->pointerLevel !=0;
        }else if(var.funcOperValueType == FuncOperValueType_ConstValue){
            return var.quoteOrConst_PointerLevel!=0;
        }else return 0;
    }

    //将一个常量类型的FuncOperValue转为GlobalInitValueTmp
    static GlobalInitValueTmp toGlobalInitValueTmp(FuncOperValue var){
        GlobalInitValueTmp tmp;
        ConstValueInfo info;
        if(!var.getInfo_ConstValue(info))return tmp;

        DataBaseType type = info.baseTypeInfo.baseType;
        if(info.pointerLevel >0){
            tmp.valueType = ConstValueType_POINTER;
        }else if(type==DataBaseType_INT || type==DataBaseType_ENUM){
            tmp.valueType = ConstValueType_INT32;
        }else if(type==DataBaseType_UINT){
            tmp.valueType = ConstValueType_UINT32;
        }else if(type==DataBaseType_SHORT){
            tmp.valueType = ConstValueType_INT16;
        }else if(type==DataBaseType_USHORT){
            tmp.valueType = ConstValueType_UINT16;
        }else if(type==DataBaseType_CHAR){
            tmp.valueType = ConstValueType_INT8;
        }else if(type==DataBaseType_UCHAR){
            tmp.valueType = ConstValueType_UINT8;
        }else if(type==DataBaseType_LONG){
            tmp.valueType = ConstValueType_INT64;
        }else if(type==DataBaseType_ULONG){
            tmp.valueType = ConstValueType_UINT64;
        }else if(type==DataBaseType_DOUBLE){
            tmp.valueType = ConstValueType_DOUBLE;
        }else if(type==DataBaseType_FLOAT){
            tmp.valueType = ConstValueType_FLOAT;
        }else{
            return tmp;
        }

        if(info.constValueIsFloat){
            tmp.floatData = info.constFloatValue;
        }else{
            tmp.intData = info.constIntValue;
        }

        if(info.pointerLevel > 1){
            tmp.pointerBytes = var.objInfo->CPU_BIT / 8;
        }else if(info.pointerLevel == 1){
            tmp.pointerBytes = info.baseTypeInfo.dataBaseTypeBytes;
        }
        return tmp;
    }


    //将GlobalInitValueTmp转为常量类型的FuncOperValue
    static FuncOperValue toFuncOperValue(GlobalInitValueTmp tmp,
                                         SrcObjectInfo &objInfo,
                                         QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                                          IndexVN pointerType = IndexVN(),//如果tmp是指针类型，指针的基类型
                                          int pointerLevel = 0
                                          ){//如果tmp是指针类型，指针层数
        FuncOperValue value;

        if(tmp.valueType == ConstValueType_INT8){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"char"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_UINT8){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"unsigned","char"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_INT16){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"short"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_UINT16){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"unsigned","short"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_INT32){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"int"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_UINT32){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"unsigned","int"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_INT64){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"long","long"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_UINT64){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"unsigned","long","long"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_IntData = tmp.intData;
        }else if(tmp.valueType == ConstValueType_FLOAT){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"float"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_FloatData = tmp.floatData;
        }else if(tmp.valueType == ConstValueType_DOUBLE){
            value.quoteOrConst_Type = objInfo.getDataTypeInfoNode()->searchDataType({"double"});
            value.quoteOrConst_PointerLevel = 0;
            value.constValue_FloatData = tmp.floatData;
        }else if(tmp.valueType == ConstValueType_POINTER){
            value.quoteOrConst_Type = pointerType;
            value.quoteOrConst_PointerLevel = pointerLevel;
            value.constValue_IntData = tmp.intData;
        }else{
            return value;
        }
        value.funcOperValueType = FuncOperValueType_ConstValue;
        value.objInfo = &objInfo;
        value.varsAtt = &varsAtt;
        return value;
    }

    //生成psdl常量值的定义文本
    static QString generateConstValueDefText(FuncOperValue v1){
        if(v1.funcOperValueType != FuncOperValueType_ConstValue){
            return "";
        }

        ConstValueInfo info;
        v1.getInfo_ConstValue(info);

        DataBaseType type = info.baseTypeInfo.baseType;

        QString typeName;
        QString number;
        if(info.pointerLevel >0){
            typeName = "POINTER";
            if(v1.objInfo->CPU_BIT == 8){
                number = QString::number((uint8_t)info.constIntValue)+"D";
            }else if(v1.objInfo->CPU_BIT == 16){
                number = QString::number((uint16_t)info.constIntValue)+"D";
            }else if(v1.objInfo->CPU_BIT == 32){
                number = QString::number((uint32_t)info.constIntValue)+"D";
            }else if(v1.objInfo->CPU_BIT == 64){
                number = QString::number((uint64_t)info.constIntValue)+"D";
            }else return "";

        }else if(type==DataBaseType_INT){
            typeName = "INT";
            number = QString::number((int32_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_UINT){
            typeName = "UINT";
            number = QString::number((uint32_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_SHORT){
            typeName = "SHORT";
            number = QString::number((int16_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_USHORT){
            typeName = "USHORT";
            number = QString::number((uint16_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_CHAR){
            typeName = "BYTE";
            number = QString::number((int8_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_UCHAR){
            typeName = "UBYTE";
            number = QString::number((uint8_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_LONG){
            typeName = "LONG";
            number = QString::number((int64_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_ULONG){
            typeName = "ULONG";
            number = QString::number((uint64_t)info.constIntValue)+"D";
        }else if(type==DataBaseType_DOUBLE){
            typeName = "DOUBLE";
            number = QString::number((double)info.constFloatValue)+"F";
        }else if(type==DataBaseType_FLOAT){
            typeName = "FLOAT";
            number = QString::number((float)info.constFloatValue)+"F";
        }else{
            return "";
        }
        return "<"+typeName+":"+number+">";
    }
    //生成psdl参数的文本
    static  QString generateArgText(FuncOperValue v1){
        if(v1.funcOperValueType == FuncOperValueType_ConstValue){
            return generateConstValueDefText(v1);
        }else if(v1.funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            v1.getInfo_RegData(info);
            return info.varPsdlName;
        }else return "";
    }

    //如果数值、指针变量是引用类型，生成将引用的变量数据加载到堆栈临时变量的代码
    //如果是数组的引用，将数组引用转为数组中元素的指针
    //如果是结构体的引用，保持不变
    static FuncOperValue generateLoadVarOper(FuncOperValue v1){
        FuncOperValue tmp;
        if(v1.funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            v1.getInfo_QuoteData(info);
            DataBaseType baseType = info.baseTypeInfo.baseType;

            if(info.arraySize.length()!=0){
                tmp.funcOperValueType = FuncOperValueType_RegData;
                tmp.varIndex = v1.objInfo->defineFuncLinkVar(info.baseTypeIndex,info.pointerLevel+info.arraySize.length(),info.varPsdlName);
                tmp.varsAtt = v1.varsAtt;
                tmp.bodyAtt = v1.bodyAtt;
                tmp.objInfo = v1.objInfo;
                tmp.isDummyVar = 1;
                return tmp;
            }else if(info.pointerLevel > 0 ||
                     (baseType != DataBaseType_STRUCT && baseType != DataBaseType_UNION)){
                tmp.funcOperValueType = FuncOperValueType_RegData;
                tmp.varIndex = v1.objInfo->defineFuncTmpVar(info.baseTypeIndex,info.pointerLevel,*v1.varsAtt);
                tmp.varsAtt = v1.varsAtt;
                tmp.bodyAtt = v1.bodyAtt;
                tmp.objInfo = v1.objInfo;
                tmp.isDummyVar = 1;

                QString tmpVarPsdlName = v1.objInfo->getVarDefineInfoNode()->getVar(tmp.varIndex)->psdlName;

                PsdlIR_Compiler::MiddleNode loadCode;
                loadCode.nodeType = "LOAD";
                loadCode.args = QStringList({tmpVarPsdlName,info.varPsdlName});
                tmp.bodyAtt.append(loadCode);
                return tmp;
            }
        }
        else if(v1.funcOperValueType == FuncOperValueType_Function){//转为同类型的函数指针

            QString funName;
            IndexVN typeIndex;
            DataTypeInfo typeInfo;
            v1.getInfo_Function(funName,typeIndex,typeInfo);


            tmp.funcOperValueType = FuncOperValueType_RegData;
            tmp.varIndex = v1.objInfo->defineFuncTmpVar(typeIndex,0,*v1.varsAtt);
            tmp.varsAtt = v1.varsAtt;
            tmp.bodyAtt = v1.bodyAtt;
            tmp.objInfo = v1.objInfo;
            tmp.isDummyVar = 1;

            QString tmpVarPsdlName = v1.objInfo->getVarDefineInfoNode()->getVar(tmp.varIndex)->psdlName;

            PsdlIR_Compiler::MiddleNode loadCode;
            loadCode.nodeType = "MOV";
            loadCode.args = QStringList({tmpVarPsdlName,"[funName]"});
            tmp.bodyAtt.append(loadCode);
            return tmp;
        }

        return v1;
    }

    //如果变量是指针、数组类型，判断指针/索引号+-时的字节跨度
    static int generateIndexSpan(FuncOperValue v1){

        if(v1.funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            v1.getInfo_QuoteData(info);
            int typeSize;

            if(info.arraySize.length()==0){
                if(info.pointerLevel==0){
                    return 0;
                }else if(info.pointerLevel==1 ){
                    return info.baseTypeInfo.dataBaseTypeBytes;
                }else{
                    return v1.objInfo->CPU_BIT/8;
                }
            }


            if(info.pointerLevel==0){
                typeSize = info.baseTypeInfo.dataBaseTypeBytes;
            }else{
                typeSize = v1.objInfo->CPU_BIT/8;
            }

            for(int i = 1;i<info.arraySize.length();i++){
                typeSize *= info.arraySize[i];
            }
            return typeSize;
        }else if(v1.funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            v1.getInfo_ConstValue(info);
            if(info.pointerLevel==0){
                return 0;
            }else if(info.pointerLevel==1){
                return info.baseTypeInfo.dataBaseTypeBytes;
            }else{
                return v1.objInfo->CPU_BIT/8;
            }

        }else if(v1.funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            v1.getInfo_RegData(info);
            if(info.pointerLevel==0){
                return 0;
            }else if(info.pointerLevel==1){
                return info.baseTypeInfo.dataBaseTypeBytes;
            }else{
                return v1.objInfo->CPU_BIT/8;
            }
        }else return 0;
    }


    //解析出双目运算符在运算前，要先转换的数据类型(参数2个数据类型必须统一)
    static bool analysisUnifyValueType(QList<FuncOperValue> args,
                                       IndexVN &typeBaseIndex,
                                       int &pointerLevel){
        if(args.length()==0){
            return 0;
        }

        //筛选出优先级最高的数据类型
        DataBaseType resultBaseType = DataBaseType_VOID;
        IndexVN resultType;
        int resultPointerLevel = 0;

        for(int i = 0;i<args.length();i++){
            DataBaseType argBaseType;
            IndexVN argType;
            int argPointerLevel;
            if(args[i].funcOperValueType == FuncOperValueType_RegData){
                RegDataInfo info;
                args[i].getInfo_RegData(info);
                argBaseType = info.baseTypeInfo.baseType;
                argType = info.baseTypeIndex;
                argPointerLevel = info.pointerLevel;
            }else if(args[i].funcOperValueType == FuncOperValueType_ConstValue){
                ConstValueInfo info;
                args[i].getInfo_ConstValue(info);
                argBaseType = info.baseTypeInfo.baseType;
                argType = info.baseTypeIndex;
                argPointerLevel = info.pointerLevel;
            }else if(args[i].funcOperValueType == FuncOperValueType_Function){
                DataTypeInfo info;
                IndexVN typeIndex;
                QString funName;
                args[i].getInfo_Function(funName,typeIndex,info);
                argBaseType = info.baseType;
                argType = typeIndex;
                argPointerLevel = 0;
            }else return 0;

            if((argPointerLevel==0 && resultPointerLevel==0 && argBaseType>resultBaseType) ||
                    (argPointerLevel!=0 && resultPointerLevel==0)){
                resultPointerLevel = argPointerLevel;
                resultType = argType;
                resultBaseType = argBaseType;
            }
        }
        typeBaseIndex = resultType;
        pointerLevel = resultPointerLevel;
        return 1;
    }

    //生成双目、三目运算符的结果临时变量
    static FuncOperValue generateResultValue(QList<FuncOperValue> args,
                                             QString &varName,
                                             bool isBoolCmp_Oper = 0){
        if(args.length()==0){
            return FuncOperValue();
        }

        //筛选出优先级最高的数据类型
        IndexVN resultType;
        int resultPointerLevel;
        if(isBoolCmp_Oper){
            resultType = args[0].objInfo->getDataTypeInfoNode()->searchDataType({"unsigned","char"});
            resultPointerLevel = 0;
        }else{
            if(!analysisUnifyValueType(args,resultType,resultPointerLevel)){
                return FuncOperValue();
            }
        }

        IndexVN resultValueIndex = args[0].objInfo->defineFuncTmpVar(resultType,resultPointerLevel,*args[0].varsAtt);
        FuncOperValue value(resultValueIndex,*args[0].varsAtt,*args[0].objInfo);
        value.isDummyVar = 1;

        if(value.funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo info;
            value.getInfo_RegData(info);

            varName = info.varPsdlName;
        }else if(value.funcOperValueType == FuncOperValueType_QuoteData){
            QuoteDataInfo info;
            value.getInfo_QuoteData(info);

            varName = info.varPsdlName;
        }
        return value;
    }

    //生成普通双目、单目运算后结果保存的代码
    static FuncOperValue generateSaveTempResultVar(QString operTypeName,
                                            QList<FuncOperValue> args,
                                            bool isCmpBool_Oper = 0){//是否是比较运算或者布尔运算
        QString varName;
        FuncOperValue value = generateResultValue(args,varName,isCmpBool_Oper);

        IndexVN unifyValueType;
        int unifyValuePointerLevel;
        if(isCmpBool_Oper){
            if(!analysisUnifyValueType(args,unifyValueType,unifyValuePointerLevel)){
                return FuncOperValue();
            }
        }else{
            if(value.funcOperValueType == FuncOperValueType_RegData){
                RegDataInfo info;
                value.getInfo_RegData(info);

                unifyValueType = info.baseTypeIndex;
                unifyValuePointerLevel = info.pointerLevel;
            }else if(value.funcOperValueType == FuncOperValueType_QuoteData){
                QuoteDataInfo info;
                value.getInfo_QuoteData(info);

                unifyValueType = info.baseTypeIndex;
                unifyValuePointerLevel = info.pointerLevel;
            }else return FuncOperValue();
        }


        //生成psdl的指令
        PsdlIR_Compiler::MiddleNode code;
        code.nodeType = operTypeName;

        QStringList argsText = {varName};
        for(int i = 0;i<args.length();i++){
            args[i] = args[i].converType(unifyValueType,unifyValuePointerLevel);
            argsText.append(generateArgText(args[i]));
            value.bodyAtt.append(args[i].bodyAtt);
        }

        code.args = argsText;
        value.bodyAtt.append(code);
        return value;
    }

    //单目运算符
    static FuncOperValue generateSingleOper(FuncOperValue v1,QString typeName){
        //判断运算变量是否符合要求
        if(judgeVarIsArray(v1) || judgeVarIsPointer(v1) || judgeVarIsFuncPointer(v1))return FuncOperValue();


        if(v1.funcOperValueType == FuncOperValueType_ConstValue){
            //常量运算
            GlobalInitValueTmp constVar = toGlobalInitValueTmp(v1);
            GlobalInitValueTmp retConstVar;
            if(typeName == "NOT"){
                retConstVar = ~constVar;
            }else{
                retConstVar = !constVar;
            }
            FuncOperValue value = toFuncOperValue(retConstVar,*v1.objInfo,*v1.varsAtt);
            value.bodyAtt.append(v1.bodyAtt);
            return value;
        }
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        v1.runSuffixIncrementOper(suffixIncrementCode,v1.suffixIncrementValue);
        v1.suffixIncrementValue = 0;

        v1 = generateLoadVarOper(v1);
        QString varName;
        FuncOperValue value = generateResultValue({v1},varName,typeName=="BNOT");
        //生成psdl的指令
        PsdlIR_Compiler::MiddleNode code;
        code.nodeType = typeName;
        code.args = QStringList({varName,generateArgText(v1)});
        value.bodyAtt.append(v1.bodyAtt);
        value.bodyAtt.append(code);
        value.bodyAtt.append(suffixIncrementCode);
        return value;
    }
    //生成双目运算代码(+-*/%)
    static FuncOperValue generateQuaternionOper(FuncOperValue v1,FuncOperValue v2,QString typeName){

        QList<PsdlIR_Compiler::MiddleNode> v1_IncrementCode,v2_IncrementCode;
        v1.runSuffixIncrementOper(v1_IncrementCode,v1.suffixIncrementValue);
        v1.suffixIncrementValue = 0;
        v2.runSuffixIncrementOper(v2_IncrementCode,v2.suffixIncrementValue);
        v2.suffixIncrementValue = 0;
        FuncOperValue retValue;
        bool operTypeSuceess = 0;
        static const QStringList connot_be_pointer_Oper =//不能是指针的运算类型
                    {"MUL","DIV","BOR","BAND"};
        static const QStringList connot_be_pointer_or_float_Oper =//不能是指针或浮点数的运算类型
                    {"REM","OR","AND","XOR","SAL","SAR"};
        static const QStringList can_be_pointer_Oper = {"CMEC","CMNEC","CMLC","CMLEC","CMMC","CMMEC"};//2个参数任意都可以是指针的运算类型

        bool isPointerOper = 0;
        bool isArrayOper = 0;
        //判断v1/v2是否可以被运算
        if(typeName=="ADD"){
            if((isPointerOper = (judgeVarIsPointer(v1)||judgeVarIsFuncPointer(v1))) ||
               (isArrayOper = judgeVarIsArray(v1))){//如果v1是指针或数组
                operTypeSuceess = varCanOperation(v2,{CanOperationAddRequire_POINTER,CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY})&&
                                  varCanOperation(v1,{});
                //v2不可为浮点数或指针数组
            }else if((isPointerOper = (judgeVarIsPointer(v2)||judgeVarIsFuncPointer(v2)))  ||
                     (isArrayOper = judgeVarIsArray(v2))){//如果v2是指针或数组
                operTypeSuceess = varCanOperation(v1,{CanOperationAddRequire_POINTER,CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY})&&
                                  varCanOperation(v2,{});
                //v1不可为浮点数或指针数组

                //交换v1和v2的值,方便运算
                FuncOperValue tmp = v1;
                v1 = v2;
                v2 = tmp;
            }else{
                //v1和v2都不是指针
                //则v1和v2可以是任意数值型
                operTypeSuceess = varCanOperation(v1,{})&&
                                  varCanOperation(v2,{});
            }
        }else if(typeName=="SUB"){
            operTypeSuceess = varCanOperation(v1,{});
            if(!operTypeSuceess)return retValue;
            if((isPointerOper = (judgeVarIsPointer(v1)||judgeVarIsFuncPointer(v1))) ||
                    (isArrayOper = judgeVarIsArray(v1))){
                isPointerOper = 1;
                //v2不可为指针和浮点数
                operTypeSuceess = varCanOperation(v2,{CanOperationAddRequire_POINTER,CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY});
            }else{
                //v2不可为指针
                operTypeSuceess = varCanOperation(v2,{CanOperationAddRequire_POINTER,CanOperationAddRequire_ARRAY});
            }
        }else if(can_be_pointer_Oper.contains(typeName)){
            operTypeSuceess = varCanOperation(v1,{}) &&
                              varCanOperation(v2,{});
        }else if(connot_be_pointer_Oper.contains(typeName)){
            //不可是指针
            operTypeSuceess = varCanOperation(v1,{CanOperationAddRequire_POINTER,CanOperationAddRequire_ARRAY}) &&
                              varCanOperation(v2,{CanOperationAddRequire_POINTER,CanOperationAddRequire_ARRAY});
        }else if(connot_be_pointer_or_float_Oper.contains(typeName)){
            //不可是指针和浮点数
            operTypeSuceess = varCanOperation(v1,{CanOperationAddRequire_POINTER,CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY}) &&
                              varCanOperation(v2,{CanOperationAddRequire_POINTER,CanOperationAddRequire_FLOAT,CanOperationAddRequire_ARRAY});
        }else{
            return retValue;
        }
        if(!operTypeSuceess)return retValue;
        //判断2个参数是否都是常量，如果是常量启用常量运算逻辑
        if(v1.funcOperValueType == FuncOperValueType_ConstValue && v2.funcOperValueType==FuncOperValueType_ConstValue){
            if(isPointerOper){
                int bytes = generateIndexSpan(v1);
                if(bytes>1){
                    FuncOperValue bytesVar((int)bytes,*v1.varsAtt,*v1.objInfo);
                    v2 = v2*bytesVar;
                }else if(bytes==0){
                    return retValue;
                }
            }
            GlobalInitValueTmp constVar1 = toGlobalInitValueTmp(v1);
            GlobalInitValueTmp constVar2 = toGlobalInitValueTmp(v2);

            GlobalInitValueTmp constVarRet;
            ConstValueInfo info;
            v1.getInfo_ConstValue(info);

            if(typeName=="ADD"){
                constVarRet = constVar1+constVar2;
            }else if(typeName=="SUB"){
                constVarRet = constVar1-constVar2;
            }else if(typeName=="MUL")constVarRet = constVar1*constVar2;
            else if(typeName=="DIV")constVarRet = constVar1/constVar2;
            else if(typeName=="REM")constVarRet = constVar1%constVar2;
            else if(typeName=="AND")constVarRet = constVar1&constVar2;
            else if(typeName=="OR")constVarRet = constVar1|constVar2;
            else if(typeName=="XOR")constVarRet = constVar1^constVar2;
            else if(typeName=="BAND")constVarRet = constVar1&&constVar2;
            else if(typeName=="BOR")constVarRet = constVar1||constVar2;
            else if(typeName=="CMEC")constVarRet = constVar1==constVar2;
            else if(typeName=="CMNEC")constVarRet = constVar1!=constVar2;
            else if(typeName=="CMLC")constVarRet = constVar1<constVar2;
            else if(typeName=="CMLEC")constVarRet = constVar1<=constVar2;
            else if(typeName=="CMMC")constVarRet = constVar1>constVar2;
            else if(typeName=="CMMEC")constVarRet = constVar1>=constVar2;
            else if(typeName=="SAL")constVarRet = constVar1<<constVar2;
            else if(typeName=="SAR")constVarRet = constVar1>>constVar2;
            else return retValue;
            retValue = toFuncOperValue(constVarRet,*v1.objInfo,*v1.varsAtt,info.baseTypeIndex,info.pointerLevel);
            retValue.bodyAtt.append(v1.bodyAtt + v2.bodyAtt);
            return retValue;
        }else if(isArrayOper){//如果是数组运算，启用数组运算的逻辑
            //数组+-运算，将引用指向的地址偏移
            long long unitBytes = generateIndexSpan(v1);
            FuncOperValue unitBytesValue(unitBytes,*v1.varsAtt,*v1.objInfo);
            FuncOperValue tmp = v2*unitBytesValue;
            QuoteDataInfo info;
            v1.getInfo_QuoteData(info);
            retValue.varIndex = v1.objInfo->defineFuncTmpVar(v1.objInfo->getDataTypeInfoNode()->searchDataType({"void"}),1,*v1.varsAtt);
            retValue.quote_IsConst = info.isConst;
            retValue.isDummyVar = info.isDummyVar;
            retValue.quote_ArraySize = info.arraySize;
            retValue.quote_ArraySize[retValue.quote_ArraySize.length()-1] = 1;
            retValue.quoteOrConst_Type = info.baseTypeIndex;
            retValue.quoteOrConst_PointerLevel = info.pointerLevel;
            retValue.objInfo = v1.objInfo;
            retValue.varsAtt = v1.varsAtt;
            retValue.bodyAtt = v1.bodyAtt + tmp.bodyAtt;
            retValue.funcOperValueType = FuncOperValueType_QuoteData;

            QString tmpVarName = v1.objInfo->getVarDefineInfoNode()->getVar(retValue.varIndex)->psdlName;
            if(tmp.funcOperValueType == FuncOperValueType_ConstValue){
                //偏移的值不可以是浮点数
                ConstValueInfo offsetInfo;
                tmp.getInfo_ConstValue(offsetInfo);
                if(offsetInfo.constValueIsFloat){
                    return FuncOperValue();
                }

                PsdlIR_Compiler::MiddleNode code;
                code.nodeType = "ADD";
                code.args = QStringList({tmpVarName,info.varPsdlName,generateArgText(tmp)});
                retValue.bodyAtt.append(code);
            }else if(tmp.funcOperValueType == FuncOperValueType_RegData){
                VarTypeInfo varInfo = getVarTypeInfo(tmp);
                if(varInfo != VarTypeInfo_INT_UINT8&&
                   varInfo != VarTypeInfo_INT_UINT16&&
                   varInfo != VarTypeInfo_INT_UINT32&&
                   varInfo != VarTypeInfo_INT_UINT64){
                    return FuncOperValue();
                }

                PsdlIR_Compiler::MiddleNode code;
                code.nodeType = "ADD";
                code.args = QStringList({tmpVarName,info.varPsdlName,generateArgText(tmp)});
                retValue.bodyAtt.append(code);
            }
            retValue.bodyAtt.append(v1_IncrementCode+v2_IncrementCode);
            return retValue;
        }
        v1 = generateLoadVarOper(v1);
        v2 = generateLoadVarOper(v2);

        if(isPointerOper){//指针+-运算
            //判断如果v1指针指向的数据类型字节数是否不为1,如果不为1，要让v2先乘以字节数
            int bytes = generateIndexSpan(v1);
            if(bytes>1){
                FuncOperValue bytesVar((int)bytes,*v1.varsAtt,*v1.objInfo);
                v2 = v2*bytesVar;
            }else if(bytes==0){
                return retValue;
            }
            retValue = generateSaveTempResultVar(typeName,{v1,v2});
        }else if(typeName=="BOR" || typeName=="BAND"){
            //布尔运算

            //判断布尔运算是使用哪种运算方式:
            //方式1:直接通过指令BOR / BAND
            //方式2: 通过流程块
            //OR：   res = v1;
            /*BOR:
             *      res = v1;
             *      if(!res)res = v2;
             *
             * BAND:
             *      res = v1;
             *      if(res)res = v2;
             *
             * 如果v2是常量 或者 没有mov/store指令arrcopy这类赋值指令，就选用方式1。否则用方式2
             */

            bool isUseFunc2 = 0;
            for(int i = 0;i<v2.bodyAtt.length();i++){
                QString codeType = v2.bodyAtt[i].nodeType;
                if(codeType == "MOV" ||
                   codeType == "STORE" ||
                   codeType == "ARRCOPY"||
                   codeType == "CALL"){
                    isUseFunc2 = 1;
                    break;
                }
            }

            if(isUseFunc2){
                if(v1.funcOperValueType == FuncOperValueType_ConstValue){
                    ConstValueInfo info;
                    v1.getInfo_ConstValue(info);

                    bool v1Value = info.constValueIsFloat ? info.constFloatValue : info.constIntValue;
                    if((typeName=="BOR" && v1Value) || (typeName=="BAND" && !v1Value)){
                        return v1;
                    }


                    FuncOperValue value = v2;
                    value.isDummyVar = 1;
                    value.bodyAtt.append(v1.bodyAtt);
                    value.bodyAtt.append(v2_IncrementCode);
                    retValue = value;
                    return retValue;
                }
                IndexVN resultVarIndex = v1.objInfo->defineFuncTmpVar(v1.objInfo->getDataTypeInfoNode()->searchDataType({"unsigned","char"}),
                                                                      0,*v1.varsAtt);
                QString resultVarName = v1.objInfo->getVarDefineInfoNode()->getVar(resultVarIndex)->psdlName;
                PsdlIR_Compiler::MiddleNode v1MovCode;
                v1MovCode.nodeType = "MOV";
                v1MovCode.args = QStringList({resultVarName,generateArgText(v1)});

                PsdlIR_Compiler::MiddleNode v2MovCode;
                v2MovCode.nodeType = "MOV";
                v2MovCode.args = QStringList({resultVarName,generateArgText(v2)});

                PsdlIR_Compiler::MiddleNode processCode;
                processCode.nodeType = "PROCESS";
                processCode.args = QStringList({typeName=="BOR" ? "EC" : "NEC",
                                                resultVarName,
                                                "<UBYTE:0D>"});
                PsdlIR_Compiler::MiddleNode_Att ifAtt;
                ifAtt.attName = "IF";
                ifAtt.subNodes = v2.bodyAtt;
                ifAtt.subNodes += v2MovCode;
                processCode.atts.append(ifAtt);


                FuncOperValue value(resultVarIndex,*v1.varsAtt,*v1.objInfo);
                value.isDummyVar = 1;
                value.bodyAtt.clear();
                value.bodyAtt += v1.bodyAtt;
                value.bodyAtt += v1MovCode;
                value.bodyAtt += processCode;
                value.bodyAtt.append(v1_IncrementCode+v2_IncrementCode);
                retValue = value;
            }else{
                retValue = generateSaveTempResultVar(typeName,{v1,v2},1);
            }
        }else{//普通算数、比较运算
            const QStringList allCmpOper = {
                 "CMEC","CMNEC","CMLC","CMLEC","CMMC","CMMEC"
            };
            bool isCmpOper = allCmpOper.contains(typeName);
            //四则运算、位运算的返回值数据类型是取a/b两参数中较大的，而比较运算固定返回uchar型
            retValue = generateSaveTempResultVar(typeName,{v1,v2},isCmpOper);
        }
        retValue.bodyAtt.append(v1_IncrementCode+v2_IncrementCode);
        return retValue;
    }

    FuncOperValue operator~(){
        return generateSingleOper(*this,"NOT");
    }
    FuncOperValue operator!(){
        return generateSingleOper(*this,"BNOT");
    }
    FuncOperValue operator+(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"ADD");
    }
    FuncOperValue operator-(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"SUB");
    }
    FuncOperValue operator*(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"MUL");
    }
    FuncOperValue operator/(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"DIV");
    }
    FuncOperValue operator%(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"REM");
    }
    FuncOperValue operator>(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMMC");
    }
    FuncOperValue operator>=(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMMEC");
    }
    FuncOperValue operator==(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMEC");
    }
    FuncOperValue operator!=(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMNEC");
    }
    FuncOperValue operator<(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMLC");
    }
    FuncOperValue operator<=(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"CMLEC");
    }
    FuncOperValue operator|(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"OR");
    }
    FuncOperValue operator&(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"AND");
    }
    FuncOperValue operator^(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"XOR");
    }
    FuncOperValue operator||(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"BOR");
    }
    FuncOperValue operator&&(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"BAND");
    }
    FuncOperValue operator<<(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"SAL");
    }
    FuncOperValue operator>>(FuncOperValue &value){
        return generateQuaternionOper(*this,value,"SAR");
    }
    // 运算,号
    static FuncOperValue mergeExp(QList<FuncOperValue> values){
        if(values.length()==0)return FuncOperValue();

        QList<PsdlIR_Compiler::MiddleNode> orders;
        for(int i = 0; i<values.length() ;i++){
            if(values[i].isError()){
                return FuncOperValue();
            }
            QList<PsdlIR_Compiler::MiddleNode> code;
            values[i].runSuffixIncrementOper(code,values[i].suffixIncrementValue);
            values[i].suffixIncrementValue = 0;
            orders.append(values[i].bodyAtt + code);
        }
        values[values.length()-1].bodyAtt = orders;
        return values[values.length()-1];
    }

    //将变量转为可以被直接作为参数传入的值，期间判断函数参数是否匹配，以及是否可以隐式转换。
    static FuncOperValue generateArgVar(FuncOperValue value,bool isValist,FuncArgType argType = FuncArgType()){
        //数组引用类型转为数组首元素指针变量，数值/指针的引用转为同类型变量，结构体引用保持不变
        value = generateLoadVarOper(value);
        VarTypeInfo varTypeInfo = getVarTypeInfo(value);

        if(varTypeInfo == VarTypeInfo_ERROR){
            return FuncOperValue();
        }
        if(isValist){//stdcall函数的可变参数函数不检查数据类型。直接传入
            return value;
        }
        VarTypeInfo argTypeInfo = getVarTypeInfo(argType,*value.objInfo);
        if(argTypeInfo==VarTypeInfo_ERROR ||
                argTypeInfo==VarTypeInfo_ARRAY){
            return FuncOperValue();
        }
        if(varTypeInfo == VarTypeInfo_STRUCT_UNION){//结构体型变量，判断是否与参数类型完全相同。必须完全相同才能传参
            if(argTypeInfo != VarTypeInfo_STRUCT_UNION)return FuncOperValue();
            QuoteDataInfo quoteInfo;
            value.getInfo_QuoteData(quoteInfo);
            if(quoteInfo.baseTypeIndex != argType.typeIndex)return FuncOperValue();
            return value;
        }else{//数值/指针型变量尝试隐式转换数据类型
            return value.converType(argType.typeIndex,argType.pointerLevel);
        }
    }

    //调用函数运算
    FuncOperValue callFunction(QList<FuncOperValue> value){
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        this->runSuffixIncrementOper(suffixIncrementCode,this->suffixIncrementValue);
        this->suffixIncrementValue = 0;
        for(int i = 0;i<value.length();i++){
            QList<PsdlIR_Compiler::MiddleNode> argSuffixIncrementCode;
            value[i].runSuffixIncrementOper(argSuffixIncrementCode,value[i].suffixIncrementValue);
            value[i].suffixIncrementValue = 0;
            suffixIncrementCode.append(argSuffixIncrementCode);
        }

        bool isFuncPtr = 0;//是否是函数指针调用
        QString funPsdlName;//函数调用的名称

        bool isHaveRetVar = 0;//函数是否有返回值
        FuncArgType retTypes;//返回值数据类型
        QList<FuncArgType> argTypes;//函数各参数的类型
        bool isStdCall;//函数是否是stdcall的可变参数函数


        FuncOperValue ptrValue;
        DataTypeInfo funTypeInfo;
        if(funcOperValueType == FuncOperValueType_Function){
            IndexVN typeIndex;
            getInfo_Function(funPsdlName,typeIndex,funTypeInfo);
            ptrValue = *this;
        }else if((funcOperValueType == FuncOperValueType_RegData ||
                 funcOperValueType == FuncOperValueType_QuoteData)&&
                 judgeVarIsFuncPointer(*this)){
            isFuncPtr = 1;
            ptrValue = generateLoadVarOper(*this);

            RegDataInfo info;
            ptrValue.getInfo_RegData(info);

            funTypeInfo = info.baseTypeInfo;
            funPsdlName = info.varPsdlName;
        }else return FuncOperValue();
        isStdCall = funTypeInfo.isStdcallFun;
        for(int i = 0;i<funTypeInfo.funArgInfos.length();i++){
            StructUnion_AttInfo attInfo = funTypeInfo.funArgInfos[i];

            FuncArgType argType;
            argType.typeIndex = attInfo.typeIndex;
            argType.pointerLevel = attInfo.pointerLevel;

            if(i==0){
                retTypes = argType;

                bool isVoid = objInfo->getDataTypeInfoNode()->getDataType(attInfo.typeIndex)->baseType == DataBaseType_VOID;
                isHaveRetVar = !(attInfo.pointerLevel == 0 && isVoid);
            }else{
                argTypes.append(argType);
            }
        }

        //检查函数传参数量是否正确
        if((value.length() < argTypes.length())||
            (!isStdCall && (value.length() > argTypes.length()))){
            return FuncOperValue();
        }
        for(int i = 0;i<value.length();i++){
            if(i>=argTypes.length()){
                value[i] = generateArgVar(value[i],1);
            }else{
                value[i] = generateArgVar(value[i],0,argTypes[i]);
            }
            if(value[i].isError()){
                return FuncOperValue();
            }
        }

        //生成最终函数调用代码
        PsdlIR_Compiler::MiddleNode callFun;
        callFun.nodeType = "CALL";

        QString callAdd = isFuncPtr ? funPsdlName : "["+funPsdlName+"]";
        callFun.args.append(callAdd);

        if(isStdCall){
            PsdlIR_Compiler::MiddleNode_Att stdcallAtt;
            stdcallAtt.attName = "STDCALL";
            callFun.atts.append(stdcallAtt);
        }

        //函数的返回值变量的引用
        FuncOperValue retValue;
        FuncOperValue resultValue;
        if(isHaveRetVar){
            IndexVN tmpVar = objInfo->defineFuncTmpVar(retTypes.typeIndex,retTypes.pointerLevel,*ptrValue.varsAtt);
            retValue = FuncOperValue(tmpVar,*ptrValue.varsAtt,*ptrValue.objInfo);
            retValue.bodyAtt = ptrValue.bodyAtt + retValue.bodyAtt;
            resultValue = retValue;

            retValue = retValue.getAddress();
            RegDataInfo info;
            retValue.getInfo_RegData(info);
            callFun.args.append(info.varPsdlName);
        }else{
            retValue.funcOperValueType = FuncOperValueType_VoidVar;
            retValue.bodyAtt = ptrValue.bodyAtt;
            retValue.varsAtt = ptrValue.varsAtt;
            retValue.objInfo = ptrValue.objInfo;
            resultValue = retValue;
        }


        //生成函数传参的指令
        for(int i = 0;i<value.length();i++){
            QString argText;
            if(value[i].funcOperValueType == FuncOperValueType_RegData ||
               value[i].funcOperValueType == FuncOperValueType_ConstValue){
                argText = generateArgText(value[i]);
            }else if(value[i].funcOperValueType == FuncOperValueType_QuoteData){
                QuoteDataInfo info;
                value[i].getInfo_QuoteData(info);
                QString bytes = QString::number(info.baseTypeInfo.dataBaseTypeBytes)+"D";
                argText = "|"+info.varPsdlName+":"+bytes+"|";
            }
            retValue.bodyAtt += value[i].bodyAtt;
            callFun.args.append(argText);
        }
        retValue.bodyAtt += callFun;

        resultValue.bodyAtt = retValue.bodyAtt;
        resultValue.quote_IsConst = 1;
        resultValue.isDummyVar = 1;
        resultValue.bodyAtt.append(suffixIncrementCode);
        return resultValue;
    }

    //运算?:三目运算
    static FuncOperValue ifelseOper(FuncOperValue judge,FuncOperValue ifValue,FuncOperValue elseValue){
        QList<PsdlIR_Compiler::MiddleNode> judgeSuffixIncrementCode,ifValueSuffixIncrementCode,elseValueSuffixIncrementCode;
        judge.runSuffixIncrementOper(judgeSuffixIncrementCode,judge.suffixIncrementValue);
        judge.suffixIncrementValue = 0;
        ifValue.runSuffixIncrementOper(ifValueSuffixIncrementCode,ifValue.suffixIncrementValue);
        ifValue.suffixIncrementValue = 0;
        elseValue.runSuffixIncrementOper(elseValueSuffixIncrementCode,elseValue.suffixIncrementValue);
        elseValue.suffixIncrementValue = 0;
        ifValue = generateLoadVarOper(ifValue);
        elseValue = generateLoadVarOper(elseValue);
        bool isStructOper = 0;
        int structBytes;//如果是结构体，结构体的字节数
        QString ifVarOperText,elseVarOperText;//变量的信息
        VarTypeInfo ifValueInfo = getVarTypeInfo(ifValue);
        VarTypeInfo elseValueInfo = getVarTypeInfo(elseValue);
        if(ifValueInfo == VarTypeInfo_ERROR ||
           elseValueInfo == VarTypeInfo_ERROR){
            return FuncOperValue();
        }
        if(ifValueInfo == VarTypeInfo_STRUCT_UNION || elseValueInfo==VarTypeInfo_STRUCT_UNION){
            if(ifValueInfo!=elseValueInfo)return FuncOperValue();
            QuoteDataInfo info;
            ifValue.getInfo_QuoteData(info);
            ifVarOperText = info.varPsdlName;
            structBytes = info.baseTypeInfo.dataBaseTypeBytes;
            IndexVN ifValueBaseType = info.baseTypeIndex;
            elseValue.getInfo_QuoteData(info);
            elseVarOperText = info.varPsdlName;
            IndexVN elseValueBaseType = info.baseTypeIndex;
            if(ifValueBaseType != elseValueBaseType)return FuncOperValue();
            isStructOper = 1;
        }else{
            ifVarOperText = generateArgText(ifValue);
            elseVarOperText = generateArgText(elseValue);
        }
        //返回的变量
        QString resultValueName;//名称
        FuncOperValue resultValue = generateResultValue({ifValue,elseValue},resultValueName);//对象

        //生成ifValue和elseValue的赋值指令
        PsdlIR_Compiler::MiddleNode ifValueCode,elseValueCode;
        if(isStructOper){
            ifValueCode.nodeType = "ARRCOPY";
            elseValueCode.nodeType = "ARRCOPY";
            ifValueCode.args = QStringList({resultValueName,ifVarOperText,QString::number(structBytes)+"D"});
            elseValueCode.args = QStringList({resultValueName,elseVarOperText,QString::number(structBytes)+"D"});
        }else{
            ifValueCode.nodeType = "MOV";
            elseValueCode.nodeType = "MOV";
            ifValueCode.args = QStringList({resultValueName,ifVarOperText});
            elseValueCode.args = QStringList({resultValueName,elseVarOperText});
        }

        if(judge.funcOperValueType == FuncOperValueType_RegData ||
           judge.funcOperValueType == FuncOperValueType_QuoteData){
            VarTypeInfo varInfo = getVarTypeInfo(judge);
            QString cmpNameText;
            if(varInfo == VarTypeInfo_INT_UINT8){
                cmpNameText = "<BYTE:0D>";
            }else if(varInfo == VarTypeInfo_INT_UINT16){
                cmpNameText = "<SHORT:0D>";
            }else if(varInfo == VarTypeInfo_INT_UINT32){
                cmpNameText = "<INT:0D>";
            }else if(varInfo == VarTypeInfo_INT_UINT64){
                cmpNameText = "<LONG:0D>";
            }else if(varInfo == VarTypeInfo_FLOAT){
                cmpNameText = "<FLOAT:0F>";
            }else if(varInfo == VarTypeInfo_DOUBLE){
                cmpNameText = "<DOUBLE:0F>";
            }else if(varInfo == VarTypeInfo_ARRAY){
                int cpuBytes = judge.objInfo->CPU_BIT;
                if(cpuBytes==8){
                    cmpNameText = "<BYTE:0D>";
                }else if(cpuBytes==16){
                    cmpNameText = "<SHORT:0D>";
                }else if(cpuBytes==32){
                    cmpNameText = "<INT:0D>";
                }else if(cpuBytes==64){
                    cmpNameText = "<LONG:0D>";
                }else return FuncOperValue();
            }
            PsdlIR_Compiler::MiddleNode processCode;
            processCode.nodeType = "PROCESS";
            judge = generateLoadVarOper(judge);//如果是引用变量，加载引用的变量到相同类型临时局部局部变量
            //如果是数组的引用，读取引用的地址到指针型临时局部变量
            processCode.args = QStringList({"NEC",generateArgText(judge),cmpNameText});
            PsdlIR_Compiler::MiddleNode_Att ifCode;
            ifCode.attName = "IF";
            ifCode.subNodes.append(ifValue.bodyAtt);
            ifCode.subNodes.append(ifValueCode);
            processCode.atts.append(ifCode);
            PsdlIR_Compiler::MiddleNode_Att elseifCode;
            elseifCode.attName = "ELSE";
            elseifCode.subNodes.append(elseValue.bodyAtt);
            elseifCode.subNodes.append(elseValueCode);
            processCode.atts.append(elseifCode);

            resultValue.bodyAtt.append(judge.bodyAtt);
            resultValue.bodyAtt.append(processCode);
        }else if(judge.funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            judge.getInfo_ConstValue(info);
            resultValue.bodyAtt.append(judge.bodyAtt);
            if(info.constValueIsFloat ? info.constFloatValue : info.constIntValue){
                //输出if的变量
                resultValue.bodyAtt.append(ifValue.bodyAtt);
                resultValue.bodyAtt.append(ifValueCode);
            }else{
                //输出else的变量
                resultValue.bodyAtt.append(elseValue.bodyAtt);
                resultValue.bodyAtt.append(elseValueCode);
            }
        }else return FuncOperValue();
        resultValue.bodyAtt.append(judgeSuffixIncrementCode);
        resultValue.bodyAtt.append(ifValueSuffixIncrementCode);
        resultValue.bodyAtt.append(elseValueSuffixIncrementCode);
        return resultValue;
    }

    //生成judge语句的指令 if(judge)  while(judge) ....
    //返回值:输出judge语句的指令
    struct JudgeOperInfo{
        enum JudgeResult{
            JudgeResult_ERROR,  //出错
            JudgeResult_True,   //必定为真
            JudgeResult_False,  //必定为假
            JudgeResult_Indefinite//不一定，是一个变量
        }judgeResultType = JudgeResult_ERROR;
        QList<PsdlIR_Compiler::MiddleNode> operCode;//judge语句生成的指令
        //judge语句与比较值比较的指令(judgeResultType==JudgeResult_Indefinite才有效)
        PsdlIR_Compiler::MiddleNode judgeProcessCode;//一个没有IF/ELSE的PROCESS语句
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;//如果judge语句有自增减语句，自增减的指令
    };

    static JudgeOperInfo generateJudge(QList<FuncOperValue> values){
        if(values.length()==0)return JudgeOperInfo();

        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        for(int i = 0; i<values.length() ;i++){
            QList<PsdlIR_Compiler::MiddleNode> code;
            if(values[i].isError()){
                return JudgeOperInfo();
            }
            values[i].runSuffixIncrementOper(code,values[i].suffixIncrementValue);
            values[i].suffixIncrementValue = 0;
            suffixIncrementCode.append(code);
        }

        FuncOperValue judgeVar = values[values.length()-1];
        JudgeOperInfo retInfo;
        if(judgeVar.funcOperValueType == FuncOperValueType_ConstValue){
            ConstValueInfo info;
            judgeVar.getInfo_ConstValue(info);
            bool state;
            state = info.constValueIsFloat ? info.constFloatValue!=0 : info.constIntValue!=0;
            retInfo.judgeResultType = state ? JudgeOperInfo::JudgeResult_True : JudgeOperInfo::JudgeResult_False;
        }else if(judgeVar.funcOperValueType == FuncOperValueType_RegData ||
                 judgeVar.funcOperValueType == FuncOperValueType_QuoteData){
            judgeVar = generateLoadVarOper(judgeVar);
            if(judgeVar.funcOperValueType == FuncOperValueType_QuoteData)return JudgeOperInfo();

            QString cmpType;
            cmpType = "NEC";

            FuncOperValue judgeConstVar((int)0,*values[0].varsAtt,*values[0].objInfo);

            RegDataInfo info;
            judgeVar.getInfo_RegData(info);
            judgeConstVar = judgeConstVar.converType(info.baseTypeIndex,info.pointerLevel);


            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "PROCESS";
            QString constVarText = generateArgText(judgeConstVar);
            node.args = QStringList({cmpType,info.varPsdlName,constVarText});

            retInfo.judgeResultType = JudgeOperInfo::JudgeResult_Indefinite;
            retInfo.judgeProcessCode = node;
            values[values.length()-1] = judgeVar;
        }else return JudgeOperInfo();


        QList<PsdlIR_Compiler::MiddleNode> operCode;
        for(int i = 0; i<values.length() ;i++){
            operCode.append(values[i].bodyAtt);
        }
        retInfo.operCode = operCode;
        retInfo.suffixIncrementCode = suffixIncrementCode;

        return retInfo;
    }

    //生成swicth语句用于case比较的变量
    //suffixIncrementCode:返回自增自减指令
    static FuncOperValue generateSwitchJudge(QList<FuncOperValue> values){
        QList<PsdlIR_Compiler::MiddleNode> suffixIncrementCode;
        if(values.length()==0)return FuncOperValue();
        QList<PsdlIR_Compiler::MiddleNode> orders;
        bool isHaveSuffixIncrement = values[values.length()-1].suffixIncrementValue;
        for(int i = 0; i<values.length() ;i++){
            if(values[i].isError()){
                return FuncOperValue();
            }
            QList<PsdlIR_Compiler::MiddleNode> code;
            values[i].runSuffixIncrementOper(code,values[i].suffixIncrementValue);
            values[i].suffixIncrementValue = 0;
            orders.append(values[i].bodyAtt);
            suffixIncrementCode.append(code);
        }
        values[values.length()-1].bodyAtt = orders;
        VarTypeInfo info = getVarTypeInfo(values[values.length()-1]);
        if(info == VarTypeInfo_ERROR ||
           info == VarTypeInfo_FLOAT ||
           info == VarTypeInfo_DOUBLE ||
           info == VarTypeInfo_STRUCT_UNION){
            return FuncOperValue();
        }
        if(values[values.length()-1].funcOperValueType == FuncOperValueType_QuoteData){
            FuncOperValue retValue = generateLoadVarOper(values[values.length()-1]);
            retValue.bodyAtt += suffixIncrementCode;
            return retValue;
        }else if(isHaveSuffixIncrement && values[values.length()-1].funcOperValueType == FuncOperValueType_RegData){
            RegDataInfo varRegInfo;
            values[values.length()-1].getInfo_RegData(varRegInfo);


            IndexVN tmpVarIndex =
            //如果需要累加累减,就创建一个副本,返回变量的副本
            values[values.length()-1].objInfo->defineFuncTmpVar(varRegInfo.baseTypeIndex,varRegInfo.pointerLevel,
                                                                *values[values.length()-1].varsAtt);

            FuncOperValue retValue(tmpVarIndex,*values[values.length()-1].varsAtt,
                                    *values[values.length()-1].objInfo);
            retValue.isDummyVar = 1;

            retValue.bodyAtt += values[values.length()-1].bodyAtt;

            QString tmpVarName = values[values.length()-1].objInfo->getVarDefineInfoNode()->getVar(tmpVarIndex)->psdlName;


            PsdlIR_Compiler::MiddleNode movCode;
            movCode.nodeType = "MOV";
            movCode.args = QStringList({tmpVarName,varRegInfo.varPsdlName});
            retValue.bodyAtt += movCode;
            retValue.bodyAtt += suffixIncrementCode;
            return retValue;
        }




        return values[values.length()-1];

    }
};


//运行token
static FuncOperValue opertionFuncOperToken(FuncOperToken tokens){
    FuncOperValue retValue;

    ///////////////////优先级1运算符:从左至右/////////////////////
    //运算() . -> [] func()  i++ i--  (已完成. -> i++ i-- [] ()  )
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length();i++){
            if(tokens[i].isOperator("()")){
                operCount++;
                QList<FuncOperToken> subTokens;
                tokens[i].getInfo_SubOper_IndexOper_FuncParameters(subTokens);
                QList<FuncOperValue> values;
                for(int i = 0;i<subTokens.length();i++){
                    FuncOperValue tmp = opertionFuncOperToken(subTokens[i]);

                    values.append(tmp);
                }
                if(values.length()!=0){
                    tokens[i] = FuncOperValue::mergeExp(values);
                    if(tokens[i].isError())return retValue;
                }else{
                    tokens.removeAt(i);
                    i-=1;
                }
            }if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("[]")){
                operCount++;

                QList<FuncOperToken> subTokens;
                tokens[i].getInfo_SubOper_IndexOper_FuncParameters(subTokens);
                QList<FuncOperValue> values;
                for(int i = 0;i<subTokens.length();i++){
                    FuncOperValue tmp = opertionFuncOperToken(subTokens[i]);
                    values.append(tmp);
                }

                if(values.length()>=1){
                    FuncOperValue indexValue = FuncOperValue::mergeExp(values);
                    tokens[i-1] = tokens[i-1][indexValue];
                    if(tokens[i-1].isError())return retValue;
                    tokens.removeAt(i);
                    i-=1;
                }else return retValue;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("func()")){
                operCount++;
                QList<FuncOperToken> subTokens;
                tokens[i].getInfo_SubOper_IndexOper_FuncParameters(subTokens);
                QList<FuncOperValue> values;
                for(int i = 0;i<subTokens.length();i++){
                    FuncOperValue tmp = opertionFuncOperToken(subTokens[i]);

                    values.append(tmp);
                }


                tokens[i-1] = tokens[i-1].callFunction(values);
                if(tokens[i-1].isError())return retValue;
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator(".")){

                operCount++;
                QString attName;
                tokens[i].getInfo_Att_PtrAtt(attName);
                tokens[i-1] = tokens[i-1].getStructAtt(attName);
                if(tokens[i-1].isError())return retValue;
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("->")){
                operCount++;
                QString attName;
                tokens[i].getInfo_Att_PtrAtt(attName);
                tokens[i-1] = tokens[i-1].getPointerStructAtt(attName);
                if(tokens[i-1].isError())return retValue;
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("++")){
                operCount++;
                tokens[i-1] = tokens[i-1]++;
                if(tokens[i-1].isError())return retValue;
                tokens.removeAt(i);
                i-=1;
            }else if(i!=0 && tokens[i-1].isValue() &&tokens[i].isOperator("--")){
                operCount++;
                operCount++;
                tokens[i-1] = tokens[i-1]--;
                if(tokens[i-1].isError())return retValue;
                tokens.removeAt(i);
                i-=1;
            }
        }
        if(operCount==0)break;
    }

    ///////////////////优先级2运算符:从右至左/////////////////////
    //运算++i  --i  (*)  (&)  ~  !
    while(1){
        int operCount = 0;
        for(int i = tokens.length()-1;i>=0;i--){
            if(!tokens[i].isValue())continue;
            else if(i<=0)break;
            if(tokens[i-1].isOperator("++")){
                tokens[i-1] = tokens[i].operator_PrefixAdd();
            }else if(tokens[i-1].isOperator("--")){
                tokens[i-1] = tokens[i].operator_PrefixSub();
            }else if(tokens[i-1].isOperator("(*)")){
                tokens[i-1] = tokens[i].dereference();
            }else if(tokens[i-1].isOperator("(&)")){
                tokens[i-1] = tokens[i].getAddress();
            }else if(tokens[i-1].isOperator("~")){
                tokens[i-1] = ~tokens[i];
            }else if(tokens[i-1].isOperator("!")){
                tokens[i-1] = !tokens[i];
            }else if(tokens[i-1].isOperator("+") &&
                     (i-1 == 0 || !tokens[i-2].isValue())){
                FuncOperValue v((int)0,*tokens[0].varsAtt,*tokens[0].objInfo);
                tokens[i-1] = v + tokens[i];
            }else if(tokens[i-1].isOperator("-") &&
                     (i-1 == 0 || !tokens[i-2].isValue())){
                FuncOperValue v((int)0,*tokens[0].varsAtt,*tokens[0].objInfo);
                tokens[i-1] = v - tokens[i];
            }else{
                continue;
            }
            if(tokens[i-1].isError())return retValue;
            tokens.removeAt(i);
            i--;
            operCount++;
        }
        if(operCount==0)break;
    }

    ///////////////////优先级3运算符:从右至左/////////////////////
    //运算(type)
    while(1){
        int operCount = 0;
        for(int i = tokens.length()-1;i>=1;i--){
            if(!tokens[i].isValue() || !tokens[i-1].isOperator("(type)"))continue;

            IndexVN baseTypeIndex;
            int pointerLevel;
            tokens[i-1].getInfo_ConverType(baseTypeIndex,//常量的基类型信息
                                            pointerLevel);
            tokens[i-1] = tokens[i].converType(baseTypeIndex,pointerLevel);
            if(tokens[i-1].isError())return retValue;
            tokens.removeAt(i);
            i--;
            operCount++;
        }
        if(operCount==0)break;
    }

    ///////////////////优先级4运算符:从左至右/////////////////////
    //运算* / %
    //运算*/%
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("*")){
                isOper = 1;
                tmp = tokens[i]*tokens[i+2];
            }else if(tokens[i+1].isOperator("/")){
                isOper = 1;
                tmp = tokens[i]/tokens[i+2];
            }else if(tokens[i+1].isOperator("%")){
                isOper = 1;
                tmp = tokens[i]%tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级5运算符:从左至右/////////////////////
    //运算+ -
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("+")){
                isOper = 1;
                tmp = tokens[i]+tokens[i+2];
            }else if(tokens[i+1].isOperator("-")){
                isOper = 1;
                tmp = tokens[i]-tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级6运算符:从左至右/////////////////////
    //运算<< >>
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("<<")){
                isOper = 1;
                tmp = tokens[i]<<tokens[i+2];
            }else if(tokens[i+1].isOperator(">>")){
                isOper = 1;
                tmp = tokens[i]>>tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级7运算符:从左至右/////////////////////
    //运算<  >  <=  >=
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("<")){
                isOper = 1;
                tmp = tokens[i]<tokens[i+2];
            }else if(tokens[i+1].isOperator(">")){
                isOper = 1;
                tmp = tokens[i]>tokens[i+2];
            }else if(tokens[i+1].isOperator(">=")){
                isOper = 1;
                tmp = tokens[i]>=tokens[i+2];
            }else if(tokens[i+1].isOperator("<=")){
                isOper = 1;
                tmp = tokens[i]<=tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级8运算符:从左至右/////////////////////
    //运算==  !=
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("==")){
                isOper = 1;
                tmp = tokens[i]==tokens[i+2];
            }else if(tokens[i+1].isOperator("!=")){
                isOper = 1;
                tmp = tokens[i]==tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从左至右/////////////////////
    //运算&
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("&")){
                isOper = 1;
                tmp = tokens[i]&tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从左至右/////////////////////
    //运算^
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("^")){
                isOper = 1;
                tmp = tokens[i]^tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从左至右/////////////////////
    //运算|
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("|")){
                isOper = 1;
                tmp = tokens[i]|tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从左至右/////////////////////
    //运算&&
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("&&")){
                isOper = 1;
                tmp = tokens[i]&&tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从左至右/////////////////////
    //运算||
    while(1){
        int operCount = 0;
        for(int i = 0;i<tokens.length()-2;i++){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("||")){
                isOper = 1;
                tmp = tokens[i]||tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    ///////////////////优先级9运算符:从右至左/////////////////////
    //运算 a ? b : c
    while(1){
        int operCount = 0;
        for(int i = tokens.length()-5;i>=0;i--){
            if(!tokens[i].isValue() ||
               !tokens[i+1].isOperator("?")||
               !tokens[i+2].isValue()||
               !tokens[i+3].isOperator(":")||
               !tokens[i+4].isValue()){
                continue;
            }
            tokens[i] = FuncOperValue::ifelseOper(tokens[i],tokens[i+2],tokens[i+4]);
            if(tokens[i].isError()){
                return retValue;
            }
            tokens.removeAt(i+1);
            tokens.removeAt(i+1);
            tokens.removeAt(i+1);
            tokens.removeAt(i+1);
            operCount++;
        }
        if(operCount==0){
            break;
        }
    }
    ///////////////////优先级9运算符:从右至左/////////////////////
    //运算 =  *=  /=  %=  +=  -=  <<= >>= &=  ^=  |=
    while(1){
        int operCount = 0;
        for(int i = tokens.length()-3;i>=0;i--){
            if(!tokens[i].isValue() ||
               !tokens[i+2].isValue()){
                continue;
            }

            bool isOper = 0;
            FuncOperValue tmp;

            if(tokens[i+1].isOperator("=")){
                isOper = 1;
                tmp = tokens[i].assignment(tokens[i+2]);
            }else if(tokens[i+1].isOperator("+=")){
                isOper = 1;
                tmp = tokens[i]+=tokens[i+2];
            }else if(tokens[i+1].isOperator("-=")){
                isOper = 1;
                tmp = tokens[i]-=tokens[i+2];
            }else if(tokens[i+1].isOperator("*=")){
                isOper = 1;
                tmp = tokens[i]*=tokens[i+2];
            }else if(tokens[i+1].isOperator("/=")){
                isOper = 1;
                tmp = tokens[i]/=tokens[i+2];
            }else if(tokens[i+1].isOperator("%=")){
                isOper = 1;
                tmp = tokens[i]%=tokens[i+2];
            }else if(tokens[i+1].isOperator("<<=")){
                isOper = 1;
                tmp = tokens[i]<<=tokens[i+2];
            }else if(tokens[i+1].isOperator(">>=")){
                isOper = 1;
                tmp = tokens[i]>>=tokens[i+2];
            }else if(tokens[i+1].isOperator("&=")){
                isOper = 1;
                tmp = tokens[i]&=tokens[i+2];
            }else if(tokens[i+1].isOperator("^=")){
                isOper = 1;
                tmp = tokens[i]^=tokens[i+2];
            }else if(tokens[i+1].isOperator("|=")){
                isOper = 1;
                tmp = tokens[i]|=tokens[i+2];
            }

            if(isOper){
                if(tmp.isError()){
                    return retValue;
                }
                tokens[i] = tmp;
                tokens.removeAt(i+1);
                tokens.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    if(tokens.length()!=1)return retValue;

    return tokens[0];
}


//解析sentence强转的数据类型
static FuncOperValue analysisConverTypeToken(PhraseList &sentence,
                                      QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                                      SrcObjectInfo &objInfo){

    FuncOperValue conver;
    int pointerLevel = 0;
    IndexVN baseTypeIndex;
    if(sentence.length()==0){
        return conver;
    }

    bool isFunPtr = 0;
    if(sentence.length()>=3){
        isFunPtr = sentence[sentence.length()-1].isDelimiter("()") &&
                            sentence[sentence.length()-2].isDelimiter("()");
    }
    PhraseList funArgPhrs;
    if(isFunPtr){
        funArgPhrs.append(sentence[sentence.length()-2]);
        funArgPhrs.append(sentence[sentence.length()-1]);
        sentence.removeLast();
        sentence.removeLast();
    }


    for(int i = sentence.length()-1;i>=0;i--){
        if(sentence[i].isDelimiter("*")){
            pointerLevel++;
            sentence.removeLast();
        }else{
            break;
        }
    }
    if(sentence.length()==0)return conver;


    if(sentence.length()==2 && sentence[0].isKeyWord("struct") && sentence[1].isIdentifier()){
        baseTypeIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[1].text);
    }else if(sentence.length()==2 && sentence[0].isKeyWord("union") && sentence[1].isIdentifier()){
        baseTypeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(sentence[1].text);
    }else if(sentence.length()==2 && sentence[0].isKeyWord("enum") && sentence[1].isIdentifier()){
        baseTypeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(sentence[1].text);
    }else if(sentence.isOnlyHaveKeyWordAndIdentifier()){
        baseTypeIndex = objInfo.getDataTypeInfoNode()->searchDataType(stdFormatBaseTypeName(sentence.getKeyWordAndIdentifierText()));
    }else{
        return conver;
    }

    if(isFunPtr){
        QString ptrName;
        bool isHavePtrName;
        int funPtrPointerLevel;
        QList<int> arraySize;
        bool isAutoArraySize;
        bool isPointerConst;
        IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(baseTypeIndex,
                                                             pointerLevel,
                                                             funArgPhrs,
                                                             objInfo,
                                                             ptrName,
                                                             isHavePtrName,
                                                             isPointerConst,
                                                             funPtrPointerLevel,
                                                             arraySize,
                                                             isAutoArraySize);
        if(arraySize.length()!=0 || isAutoArraySize || isHavePtrName){
            return conver;
        }

        baseTypeIndex = funPtrTypeIndex;
        pointerLevel = funPtrPointerLevel;
    }


    if(baseTypeIndex.isEmpty()){
        return conver;
    }

    return FuncOperValue(baseTypeIndex,pointerLevel,varsAtt,objInfo);
}


//将表达式语句转为方便算数运算token
static QList<FuncOperToken> analysisFuncOperToken(PhraseList &sentence,//当前函数体/流程块体内的所有语句
                                    SrcObjectInfo &objInfo,
                                    QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                                    bool &suceess){

    QList<FuncOperToken> opers;
    suceess = 0;
    FuncOperToken tmp;
    bool lastIsValue = 0;
    for(int i = 0;i<sentence.length();i++){

        bool isConverType = 0;
        bool converIsValue = 0;
        FuncOperValue converTypeToken;
        if(sentence[i].isDelimiter("()")){
            PhraseList sub = sentence[i].subPhraseList;
            converTypeToken = analysisConverTypeToken(sub,varsAtt,objInfo);
            isConverType = converTypeToken.funcOperValueType == FuncOperValueType_ConverType;
        }

        if(sentence[i].isKeyWord("sizeof") && i+1<sentence.length() &&
           sentence[i+1].isDelimiter("()")){
            //sizeof运算 转为int的常量
            PhraseList phr = sentence[i+1].subPhraseList;
            bool su;
            int sizeofValue = sizeofOperator(phr,objInfo,su);
            if(!su){
                return opers;
            }
            FuncOperValue value((int32_t)sizeofValue,varsAtt,objInfo);
            tmp.append(value);
            i+=1;
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT32){
            FuncOperValue value((int32_t)sentence[i].text.toInt(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT32){
            FuncOperValue value((uint32_t)sentence[i].text.toUInt(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_UINT64){
            FuncOperValue value((uint64_t)sentence[i].text.toULongLong(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_INT64){
            FuncOperValue value((int64_t)sentence[i].text.toLongLong(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT32){
            //浮点型常量 转为float32的常量
            FuncOperValue value((float)sentence[i].text.toFloat(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_FLOAT64){
            //浮点型常量 转为float64的常量
            FuncOperValue value((double)sentence[i].text.toDouble(),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_CHAR){
            //字符常量 转为uint8的常量
            bool su;
            uint8_t cv = analysisCharConstValue(sentence[i].text,su);
            FuncOperValue value((uint8_t)cv,varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::ConstValue_STRING){
            //字符串常量 转为一个char[]的引用
            int size;
            QString psdlName = objInfo.appendAnonyConstStrValue(sentence[i].text,size);
            IndexVN charType = objInfo.getDataTypeInfoNode()->searchDataType({"char"});

            VarDefineInfo info;
            info.baseTypeIndex = charType;
            info.varName = psdlName;
            info.isStatic = 1;
            info.isConst = 1;
            info.psdlName = psdlName;
            info.arraySize = QList<int>({size});
            FuncOperValue value(objInfo.getVarDefineInfoNode()->defineVar(info),varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(isConverType){
            //类型强转表达式
            tmp.append(converTypeToken);
        }else if(sentence[i].isDelimiter("()")){
            //子表达式
            PhraseList sub = sentence[i].subPhraseList;
            bool su;

            QList<FuncOperToken> subToken = analysisFuncOperToken(sub,objInfo,varsAtt,su);

            if(!su){
                return opers;
            }

            FuncOperValue value(subToken,lastIsValue,varsAtt,objInfo);

            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].isDelimiter("[]")){
            //索引子表达式
            PhraseList sub = sentence[i].subPhraseList;
            bool su;
            QList<FuncOperToken> subToken = analysisFuncOperToken(sub,objInfo,varsAtt,su);

            if(!su || subToken.length()!=1){
                return opers;
            }

            FuncOperValue value(subToken[0],varsAtt,objInfo);
            tmp.append(value);
            converIsValue = 1;
        }else if(sentence[i].isIdentifier() || (sentence[i].isKeyWord("__valist__"))){
            //判断是否是一个变量
            IndexVN varIndex = objInfo.getVarDefineInfoNode()->searchVar(sentence[i].text);

            bool isEnumConstValue;
            int enumValue = objInfo.getDataTypeInfoNode()->getEnumConstValue(sentence[i].text,&isEnumConstValue);
            //判断是否是一个函数
            int funIndex = objInfo.serachFunction(sentence[i].text);


            if(!varIndex.isEmpty()){

                //是一个变量
                FuncOperValue value(varIndex,varsAtt,objInfo);
                tmp.append(value);
            }else if(isEnumConstValue){
                //是一个枚举常量
                FuncOperValue value((long long)enumValue,varsAtt,objInfo);
                tmp.append(value);
            }else if(funIndex!=-1){
                //是一个函数
                FuncOperValue value(sentence[i].text,funIndex,varsAtt,objInfo);
                tmp.append(value);
            }else{

                return opers;
            }
            converIsValue = 1;
        }else if(sentence[i].isDelimiter("&") && !lastIsValue){
            //取地址
            FuncOperValue value("(&)",varsAtt,objInfo);
            tmp.append(value);
        }else if(sentence[i].isDelimiter("*") && !lastIsValue){
            //解引用
            FuncOperValue value("(*)",varsAtt,objInfo);
            tmp.append(value);
        }else if(sentence[i].isDelimiter(",")){
            //表达式分段
            opers.append(tmp);
            tmp.clear();
        }else if(i+1<sentence.length() &&
                 sentence[i].isDelimiter("->")&&
                 sentence[i+1].isIdentifier()){
            FuncOperValue value(true,sentence[i+1].text,varsAtt,objInfo);
            tmp.append(value);
            i+=1;
            converIsValue = 1;
        }else if(i+1<sentence.length() &&
                 sentence[i].isDelimiter(".")&&
                 sentence[i+1].isIdentifier()){

            FuncOperValue value(false,sentence[i+1].text,varsAtt,objInfo);
            tmp.append(value);
            i+=1;
            converIsValue = 1;
        }else if(sentence[i].PhraseType == Phrase::Delimiter){
            //运算符
            QString delText = sentence[i].text;

            //所有常量运算表达式所支持的运算符
            const static QStringList allDel ={
                "+","-","*","/","%",
                "|","&","~","^",
                "||","&&","!",
                ">",">=","<","<=","==","!=",
                "<<",">>",
                "?",":",
                "+=","-=","*=","/=","%=",
                "|=","&=","^=",
                "<<=",">>=",
                "=",
                "++","--",
                ".","->"
            };
            if(!allDel.contains(delText)){
                return opers;
            }
            FuncOperValue value(delText,varsAtt,objInfo);
            tmp.append(value);
        }else{
            return opers;
        }
        lastIsValue = converIsValue;
    }

    if(tmp.length()!=0){
        opers.append(tmp);
    }
    suceess = 1;
    return opers;
}


//解析函数中变量运算的语句
//返回值是运算语句运算结束后结果存储的变量索引号
static QList<FuncOperValue> analysisFunVarOpertion(PhraseList &sentence,//当前函数体/流程块体内的所有语句
                            SrcObjectInfo &objInfo,
                            //QList<PsdlIR_Compiler::MiddleNode> &bodyAtt,
                            QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                            int &operUsePhraseCount){
    PhraseList oper = sentence.splitFromDelimiter(";")[0];
    operUsePhraseCount = oper.length()+1;
    QList<FuncOperValue> operTmpVar;
    bool suceess;
    QList<FuncOperToken> tokens = analysisFuncOperToken(oper,objInfo,varsAtt,suceess);
    if(suceess==0){

        return operTmpVar;
    }
    for(int i = 0;i<tokens.length();i++){
        FuncOperValue tmp = opertionFuncOperToken(tokens[i]);
        operTmpVar.append(tmp);
    }
    return operTmpVar;
}

//解析函数中定义的局部变量的初始化语句
static bool analysisFunDefineVarInitValueOrder(IndexVN varIndex,//传入要初始化的变量索引号
                                       PhraseList &sentence,//传入初始化的语句
                                       SrcObjectInfo &objInfo,
                                       QList<PsdlIR_Compiler::MiddleNode> &bodyAtt,
                                       QList<PsdlIR_Compiler::MiddleNode> &varsAtt){
    if(sentence.length()==0)return 0;
    VarDefineInfo &varInfo = *objInfo.getVarDefineInfoNode()->getVar(varIndex);

    bool isConstValue = varInfo.isConst;
    bool isPointerConstValue = varInfo.isPointerConst;
    varInfo.isConst = 0;
    varInfo.isPointerConst = 0;
    bool isDirectInit = !(sentence.length() == 1 && sentence[0].isDelimiter("{}")) && varInfo.arraySize.length()==0;
    if(isDirectInit){
        //基础数据类型的初始化
        Phrase varNamePhrase;
        varNamePhrase.PhraseType = Phrase::Identifier;
        varNamePhrase.text = varInfo.varName;

        Phrase assignmentPhrase;
        assignmentPhrase.PhraseType = Phrase::Delimiter;
        assignmentPhrase.text = "=";

        Phrase semicolonPhrase;
        semicolonPhrase.PhraseType = Phrase::Delimiter;
        semicolonPhrase.text = ";";

        PhraseList assignmentOper;
        assignmentOper.append(varNamePhrase);
        assignmentOper.append(assignmentPhrase);
        assignmentOper.append(sentence);
        assignmentOper.append(semicolonPhrase);

        int assignmentOperCount = assignmentOper.length();
        int operUsePhraseCount;
        QList<FuncOperValue> initValues = analysisFunVarOpertion(assignmentOper,//当前函数体/流程块体内的所有语句
                                            objInfo,
                                            varsAtt,
                                            operUsePhraseCount);
        FuncOperValue retValue = FuncOperValue::mergeExp(initValues);
        if(retValue.isError() || operUsePhraseCount!=assignmentOperCount){
            return 0;
        }
        bodyAtt.append(retValue.bodyAtt);
        varInfo.isConst = isConstValue;
        varInfo.isPointerConst = isPointerConstValue;
        return 1;
    }

    //解析出全局变量中各个初始值存储的地址
    int retAutoArrSize;

    QList<InitValuePlace> places =
                          analysisGlobalVarInitValuePlace(varInfo.baseTypeIndex,
                                                          varInfo.pointerLevel,
                                                          0,
                                                          varInfo.arraySize,
                                                          //c语言定义数组型变量时,支持最高维度的数组的单元数可不写,通过写入的元素来自动求出总单元数,如果要用该功能isAutoArrSize==1
                                                          varInfo.isAutoArrSize,
                                                          sentence,
                                                          objInfo,
                                                          retAutoArrSize);
    if(places.length()==0){
        return 0;
    }
    if(varInfo.isAutoArrSize){
        varInfo.arraySize[0] = retAutoArrSize;
        varInfo.isAutoArrSize = 0;
    }
    QString varPsdlName = varInfo.psdlName;
    while(places.length()!=0){

        InitValuePlace thisValuePlace = places[0];
        if(thisValuePlace.typeIndex.isEmpty())return 0;
        places.removeAt(0);

        if(thisValuePlace.isNoInitArray)continue;
        int unitOffset = thisValuePlace.offset;


        if(thisValuePlace.isStringArray){
            if(thisValuePlace.ext.length() != 1)return 0;
            //存储常量字符串的字符数组
            QByteArray strByte = replaceTransChar(thisValuePlace.ext[0].text);//字符串的初始值

            if(strByte.length() > 10){//如果字符数>20个，创建一个静态字符串常量数组，通过ARRCOPY指令拷贝到数组中
                int strByteSize;
                QString constStrValueName = objInfo.appendAnonyConstStrValue(thisValuePlace.ext[0].text,strByteSize);

                IndexVN tmpVar = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,
                                                          thisValuePlace.pointerLevel,
                                                          varsAtt);
                QString tmpVarName = objInfo.getVarDefineInfoNode()->getVar(tmpVar)->psdlName;


                //读取被赋值变量的地址到临时变量2
                IndexVN tmpVar2 = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,
                                                          thisValuePlace.pointerLevel + 1,
                                                          varsAtt);
                QString tmpVar2Name = objInfo.getVarDefineInfoNode()->getVar(tmpVar2)->psdlName;
                PsdlIR_Compiler::MiddleNode getPtrAddress;
                getPtrAddress.nodeType = "ADDRESS";
                getPtrAddress.args = QStringList({tmpVar2Name,varPsdlName,QString::number(unitOffset)+"D"});
                bodyAtt.append(getPtrAddress);


                //加载常量字符串地址到临时变量
                PsdlIR_Compiler::MiddleNode getConstStingAddCode;
                getConstStingAddCode.nodeType = "MOV";
                getConstStingAddCode.args = QStringList({tmpVarName,"["+constStrValueName+"]"});
                bodyAtt.append(getConstStingAddCode);

                //将临时变量的值存入指针
                PsdlIR_Compiler::MiddleNode storeValueCode;
                storeValueCode.nodeType = "ARRCOPY";
                storeValueCode.args = QStringList({tmpVar2Name,tmpVarName,QString::number(strByteSize)+"D"});
                bodyAtt.append(storeValueCode);
            }else{//如果字符数<=10个，直接赋值

                //创建一个临时变量,暂存字符值
                IndexVN tmpVar = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,0,
                                                          varsAtt);
                QString tmpVarName = objInfo.getVarDefineInfoNode()->getVar(tmpVar)->psdlName;


                //读取被赋值变量的地址到临时变量2
                IndexVN tmpVar2 = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,
                                                          thisValuePlace.pointerLevel + 1,
                                                          varsAtt);
                QString tmpVar2Name = objInfo.getVarDefineInfoNode()->getVar(tmpVar2)->psdlName;
                PsdlIR_Compiler::MiddleNode getPtrAddress;
                getPtrAddress.nodeType = "ADDRESS";
                getPtrAddress.args = QStringList({tmpVar2Name,varPsdlName,QString::number(unitOffset)+"D"});
                bodyAtt.append(getPtrAddress);

                //将tmpVar2向后移动一位
                PsdlIR_Compiler::MiddleNode offsetPtrAddress;
                offsetPtrAddress.nodeType = "ADD";
                offsetPtrAddress.args = QStringList({tmpVar2Name,"1D"});

                //将临时变量1存入到临时变量2指向的数组元素位置
                PsdlIR_Compiler::MiddleNode storeValue;
                storeValue.nodeType = "STORE";
                storeValue.args = QStringList({tmpVarName,tmpVar2Name});

                for(int i = 0;i<strByte.length();i++){
                    QString initValueText = QString::number(strByte[i],16);
                    while(initValueText.length()<2){
                        initValueText.prepend("0");
                    }
                    initValueText = initValueText.right(2);
                    initValueText = "<UBYTE:"+initValueText+"H>";

                    //将字符值初始化到临时变量1
                    PsdlIR_Compiler::MiddleNode initValueCode;
                    initValueCode.nodeType = "MOV";
                    initValueCode.args = QStringList({tmpVarName,initValueText});
                    bodyAtt.append(initValueCode);

                    //将临时变量1存入到临时变量2指向的数组元素位置
                    bodyAtt.append(storeValue);


                    //将tmpVar2向后移动一位
                    if(i!=strByte.length()-1){
                        bodyAtt.append(offsetPtrAddress);
                    }
                }
            }
        }else if(thisValuePlace.isStringPtr){//指向一个常量字符串的指针
            if(thisValuePlace.ext.length() != 1)return 0;
            //创建一个临时变量
            IndexVN tmpVar = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,
                                                      thisValuePlace.pointerLevel,
                                                      varsAtt);
            QString tmpVarName = objInfo.getVarDefineInfoNode()->getVar(tmpVar)->psdlName;

            //加载常量字符串地址到临时变量
            PsdlIR_Compiler::MiddleNode getConstStingAddCode;
            getConstStingAddCode.nodeType = "MOV";
            getConstStingAddCode.args = QStringList({tmpVarName,"["+thisValuePlace.ext[0].text+"]"});
            bodyAtt.append(getConstStingAddCode);

            //读取被赋值变量的地址到临时变量2
            IndexVN tmpVar2 = objInfo.defineFuncTmpVar(thisValuePlace.typeIndex,
                                                      thisValuePlace.pointerLevel + 1,
                                                      varsAtt);
            QString tmpVar2Name = objInfo.getVarDefineInfoNode()->getVar(tmpVar2)->psdlName;
            PsdlIR_Compiler::MiddleNode getPtrAddress;
            getPtrAddress.nodeType = "ADDRESS";
            getPtrAddress.args = QStringList({tmpVar2Name,varPsdlName,QString::number(unitOffset)+"D"});
            bodyAtt.append(getPtrAddress);

            //将临时变量的值存入指针
            PsdlIR_Compiler::MiddleNode storeValueCode;
            storeValueCode.nodeType = "STORE";
            storeValueCode.args = QStringList({tmpVarName,tmpVar2Name});
            bodyAtt.append(storeValueCode);
        }else{//普通数值类型
            IndexVN tmpVar = objInfo.defineFuncTmpVar(objInfo.getDataTypeInfoNode()->searchDataType({"void"}),
                                                      1,
                                                      varsAtt);
            QString tmpVarName = objInfo.getVarDefineInfoNode()->getVar(tmpVar)->psdlName;
            PsdlIR_Compiler::MiddleNode getPtrAddress;
            getPtrAddress.nodeType = "ADDRESS";
            getPtrAddress.args = QStringList({tmpVarName,varPsdlName,QString::number(unitOffset)+"D"});

            FuncOperValue unitPtr;
            unitPtr.funcOperValueType = FuncOperValueType_QuoteData;
            unitPtr.varIndex = tmpVar;

            unitPtr.quote_IsConst = 0;
            unitPtr.quote_ArraySize.clear();
            unitPtr.quoteOrConst_Type = thisValuePlace.typeIndex;
            unitPtr.quoteOrConst_PointerLevel = thisValuePlace.pointerLevel;
            unitPtr.objInfo = &objInfo;
            unitPtr.varsAtt = &varsAtt;


            unitPtr.bodyAtt.append(getPtrAddress);



            FuncOperValue assignmentOper("=",varsAtt,objInfo);


            bool suceess;
            QList<FuncOperToken> operToken =analysisFuncOperToken(thisValuePlace.ext,//当前函数体/流程块体内的所有语句
                                                                    objInfo,
                                                                    varsAtt,
                                                                    suceess);

            if(operToken.length()!=1 || !suceess)return 0;

            operToken[0].prepend(assignmentOper);
            operToken[0].prepend(unitPtr);


            FuncOperValue result = opertionFuncOperToken(operToken[0]);
            result = FuncOperValue::mergeExp({result});
            if(result.isError())return 0;
            bodyAtt.append(result.bodyAtt);
        }
    }
    varInfo.isConst = isConstValue;
    varInfo.isPointerConst = isPointerConstValue;
    return 1;
}

//解析函数内定义局部变量的语句
static bool analysisFunDefineVarOrder(PhraseList &sentence,//当前函数体/流程块体内的所有语句
                               SrcObjectInfo &objInfo,
                               QList<PsdlIR_Compiler::MiddleNode> &bodyAtt,
                               QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                               int &operUsePhraseCount,
                               bool &isDefineVar){//是否是定义变量的语法
    isDefineVar = 1;
    if(sentence.getDelimiterNum(";") ==0)return false;
    PhraseList oper = sentence.splitFromDelimiter(";")[0];
    if(oper.length()==0){
        return 0;
    }
    operUsePhraseCount = oper.length()+1;

    Phrase sem;
    sem.PhraseType = sem.Delimiter;
    sem.text = ";";
    oper.append(sem);

    if(oper.getKeyWordNum("typedef")){
        //解析数据类型的定义
        isDefineVar = 0;
        return analysisTypeDefineSentence(oper,objInfo);
    }else if(oper.getKeyWordNum("static")){
        //解析函数中静态变量的定义
        return analysisStatementGlobalVarOrFun(oper,objInfo);
    }else if((oper[0].PhraseType != Phrase::KeyWord || oper[0].isKeyWord("sizeof") ||oper[0].isKeyWord("__valist__")) &&
            !(oper[0].isIdentifier() &&
             !objInfo.getDataTypeInfoNode()->searchDataType(QStringList({oper[0].text})).isEmpty())){
        isDefineVar = 0;
        return 0;
    }



    //解析函数中的局部变量定义
    oper.removeLast();
    if(oper.judgeIsHaveAnyKeyWord({"if",
                                      "else",
                                      "while",
                                      "for",
                                      "goto",
                                      "continue",
                                      "break",
                                      "return",
                                      "switch",
                                      "case",
                                      "default",
                                      "extern",
                                      "typedef"})){
        //语句中存在不合语法的关键字,出错
        objInfo.appendPrompt("语句中存在不合语法的关键字",oper[0].srcPath,oper[0].line,oper[0].col);
        return 0;
    }

    //解析出定义的函数/变量信息
    QList<PhraseList> pls = oper.splitFromDelimiter(",");
    if(pls.length()==0){
        objInfo.appendPrompt("定义语法有误",oper[0].srcPath,oper[0].line,oper[0].col);
        return 0;
    }
    PhraseList defineVarTypePls;
    enum DefineVarType{
        DefineFunPtr,
        DefineVar
    };

    struct DefineVarPlsInfo{
        PhraseList pls;
        DefineVarType type;
    };
    QList<DefineVarPlsInfo> defineVarInfos;

    DefineVarPlsInfo tmp;
    if(pls[0].getDelimiterNum("=")){
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("=")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
                break;
            }
            tmp.pls.prepend(pls[0][i]);
            pls[0].removeLast();
        }
    }

    QList<Phrase> pl;
    if(pls[0].length()>=2 && pls[0][pls[0].length()-1].isDelimiter("()")&&
       pls[0][pls[0].length()-2].isDelimiter("()")){
        tmp.type = DefineFunPtr;
        pl = pls[0].mid(pls[0].length()-2,2);
        tmp.pls.prepend(pl[1]);
        tmp.pls.prepend(pl[0]);
        pls[0].removeLast();
        pls[0].removeLast();
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*") || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        defineVarInfos.append(tmp);
    }else{
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("[]")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }
        if(pls[0].length()>=1 && pls[0][pls[0].length()-1].isIdentifier()){
            if(pls[0].length()>=2 &&
                    !pls[0][pls[0].length()-2].isKeyWord("struct")&&
                    !pls[0][pls[0].length()-2].isKeyWord("union")&&
                    !pls[0][pls[0].length()-2].isKeyWord("enum")){
                tmp.pls.prepend(pls[0][pls[0].length()-1]);
                pls[0].removeLast();
            }
        }
        for(int i = pls[0].length()-1;i>=0;i--){
            if(pls[0][i].isDelimiter("*") || pls[0][i].isKeyWord("const")){
                tmp.pls.prepend(pls[0][i]);
                pls[0].removeLast();
            }else{
                break;
            }
        }

        if(tmp.pls.length()!=0){
            tmp.type = DefineVar;
            defineVarInfos.append(tmp);
        }
    }
    defineVarTypePls = pls[0];

    for(int i = 1;i<pls.length();i++){
        QList<Phrase> ext;
        if(pls[i].getDelimiterNum("=")){
            for(int j = pls[i].length()-1;j>=0;j--){
                if(pls[i][j].isDelimiter("=")){
                    ext.prepend(pls[i][j]);
                    pls[i].removeLast();
                    break;
                }
                ext.prepend(pls[i][j]);
                pls[i].removeLast();
            }
        }
        DefineVarPlsInfo tmp;
        if(pls[i].length()==2 && pls[i][pls[i].length()-1].isDelimiter("()")&&
           pls[i][pls[i].length()-2].isDelimiter("()")){
            tmp.type = DefineFunPtr;
        }else{
            tmp.type = DefineVar;
        }
        tmp.pls = pls[i];
        tmp.pls.append(ext);
        defineVarInfos.append(tmp);
    }

    bool isConst = defineVarTypePls.getKeyWordNum("const");//是否是常量类型
    defineVarTypePls.removeKeyWord("const");
    defineVarTypePls.removeKeyWord("auto");
    int pointerLevel = 0;//类型的基础指针级数
    IndexVN typeIndex;//类型索引号

    bool extendAttributeSuceess;
    QList<PsdlIR_Compiler::MiddleNode_Att> extendAtts =
            analysisExtendAttribute(defineVarTypePls,objInfo,extendAttributeSuceess);
    if(!extendAttributeSuceess){
        objInfo.appendPrompt("定义了不支持的扩展属性",oper[0].srcPath,oper[0].line,oper[0].col);
        return 0;
    }

    bool isDefaultPointerConst = 0;
    //解析使用的类型
    if(defineVarTypePls[0].isKeyWord("struct") || defineVarTypePls[0].isKeyWord("union") || defineVarTypePls[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(defineVarTypePls.length()==2 && !defineVarTypePls[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!defineVarTypePls[1].isIdentifier()){
                objInfo.appendPrompt("数据类型名定义不正确",oper[0].srcPath,oper[0].line,oper[0].col);
                return 0;
            }
            if(defineVarTypePls[0].isKeyWord("struct")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchStructType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("union")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchUnionType(defineVarTypePls[1].text);
            }else if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = objInfo.getDataTypeInfoNode()->searchEnumType(defineVarTypePls[1].text);
            }
            if(typeIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(defineVarTypePls[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(defineVarTypePls[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(defineVarTypePls[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = defineVarTypePls[1].text;
                enumInfo.isOnlyStatement = 1;
                typeIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }
        }else if((defineVarTypePls.length()==2 && defineVarTypePls[1].isDelimiter("{}")) ||
                 (defineVarTypePls.length()==3 && defineVarTypePls[1].PhraseType==Phrase::Identifier && defineVarTypePls[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(defineVarTypePls[0].isKeyWord("enum")){
                typeIndex = analysisEnumTypeDef(defineVarTypePls,objInfo);
            }else{
                typeIndex = analysisStructUnionTypeDef(defineVarTypePls,objInfo);
            }
            if(typeIndex.isEmpty()){
                objInfo.appendPrompt("数据类型定义不正确",oper[0].srcPath,oper[0].line,oper[0].col);
                return 0;
            }
        }else{
            objInfo.appendPrompt("数据类型名定义不正确",oper[0].srcPath,oper[0].line,oper[0].col);
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!defineVarTypePls.isOnlyHaveKeyWordAndIdentifier()){
            objInfo.appendPrompt("数据类型名定义不正确",oper[0].srcPath,oper[0].line,oper[0].col);
            return 0;
        }

        //被重定义类型的数据类型名
        QStringList oyName = defineVarTypePls.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        if(!status){
            objInfo.appendPrompt("数据类型名定义不正确",oper[0].srcPath,oper[0].line,oper[0].col);
            return 0;
        }
        typeIndex = map.index;
        isConst = isConst || map.isDefaultConst;
        isDefaultPointerConst = map.isDefaultPointerConst;
        pointerLevel = map.pointerLevel;
    }

    bool defVarIsHaveError = 0;
    for(int i = 0;i<defineVarInfos.length();i++){
        PhraseList extpls;//变量初始值运算表达式
        bool isHaveExtpls = 0;

        if(defineVarInfos[i].pls.length()==0){
            defVarIsHaveError = 1;
            objInfo.appendPrompt("定义语法有误",oper[0].srcPath,oper[0].line,oper[0].col);
            continue;
        }
        int thisLine = defineVarInfos[i].pls[0].line;
        int thisCol = defineVarInfos[i].pls[0].col;
        QString thisSrcPath = defineVarInfos[i].pls[0].srcPath;

        if(defineVarInfos[i].pls.getDelimiterNum("=")){
            isHaveExtpls = 1;
            for(int j = defineVarInfos[i].pls.length()-1;j>=0;j--){
                if(defineVarInfos[i].pls[j].isDelimiter("=")){
                    defineVarInfos[i].pls.removeLast();
                    break;
                }
                extpls.prepend(defineVarInfos[i].pls[j]);
                defineVarInfos[i].pls.removeLast();
            }
        }

        IndexVN varIndex;
        VarDefineInfo varInfo;
        varInfo.extendAttributes = extendAtts;
        varInfo.isFuncVar = 1;
        if(defineVarInfos[i].type==DefineFunPtr){
            QString name;
            bool isHaveName;
            int fun_pointerLevel;
            QList<int> fun_ptr_array_size;
            bool isAutoArrSize;
            bool isPointerConst;
            IndexVN funPtrTypeIndex = analysisFunctionPtrTypeDef(typeIndex,
                                                                 pointerLevel,
                                                                 defineVarInfos[i].pls,
                                                                 objInfo,
                                                                 name,
                                                                 isHaveName,
                                                                 isPointerConst,
                                                                 fun_pointerLevel,
                                                                 fun_ptr_array_size,
                                                                 isAutoArrSize);



            if(funPtrTypeIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("函数指针定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(!objInfo.judegDefineVarNameIsCopce(name)){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }

            varInfo.varName = name;
            varInfo.isConst = 0;
            varInfo.isPointerConst = isPointerConst || isDefaultPointerConst;
            varInfo.isExtern = 0;
            varInfo.isStatic = 0;
            varInfo.arraySize = fun_ptr_array_size;
            varInfo.pointerLevel = fun_pointerLevel;
            varInfo.baseTypeIndex = funPtrTypeIndex;
            varInfo.isAutoArrSize = isAutoArrSize;

            varIndex = objInfo.defineVar(varInfo);
            if(varIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量重复定义",thisSrcPath,thisLine,thisCol);
                continue;
            }
        }else if(defineVarInfos[i].type==DefineVar){
            bool isHaveName;
            int var_pointerLevel;
            QList<int> var_array_size;
            bool status;
            bool isAutoArrSize;
            bool isHavePrefixConst;
            bool isHaveSuffixConst;
            QString name = analysisDefVarOrAtt_Name_Ptr_Arr(defineVarInfos[i].pls,objInfo,var_pointerLevel,var_array_size,isHaveName,isAutoArrSize,isHavePrefixConst,isHaveSuffixConst,status);
            if(status==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量定义语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            if(isHaveName==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("未定义变量名",thisSrcPath,thisLine,thisCol);
                continue;
            }


            if(!objInfo.judegDefineVarNameIsCopce(name)){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量名不符合要求",thisSrcPath,thisLine,thisCol);
                continue;
            }
            varInfo.varName = name;
            varInfo.pointerLevel = var_pointerLevel + pointerLevel;
            if(varInfo.pointerLevel){
                varInfo.isConst = isConst || isHavePrefixConst;
                varInfo.isPointerConst = isHaveSuffixConst || isDefaultPointerConst;
            }else{
                varInfo.isConst = isConst || isHavePrefixConst || isDefaultPointerConst;
                varInfo.isPointerConst = 0;
            }
            varInfo.isExtern = 0;
            varInfo.isStatic = 0;
            varInfo.arraySize = var_array_size;
            varInfo.baseTypeIndex = typeIndex;
            varInfo.isAutoArrSize = isAutoArrSize;



            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->isOnlyStatement && varInfo.pointerLevel==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("所定义变量的数据类型还未完整定义,仅声明了",thisSrcPath,thisLine,thisCol);
                continue;
            }

            //void类型不能够作为数据类型,只能作为指针
            if(objInfo.getRootDataTypeInfoNode()->getDataType(typeIndex)->baseType == DataBaseType_VOID && varInfo.pointerLevel==0){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("void类型只能作为指针来定义一个变量",thisSrcPath,thisLine,thisCol);
                continue;
            }
            varIndex = objInfo.defineVar(varInfo);
            if(varIndex.isEmpty()){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("变量重复定义",thisSrcPath,thisLine,thisCol);
                continue;
            }
        }else{
            defVarIsHaveError = 1;
            objInfo.appendPrompt("变量定义语法有误",thisSrcPath,thisLine,thisCol);
            continue;
        }



        if(isHaveExtpls){
            //初始化变量
            if(!analysisFunDefineVarInitValueOrder(varIndex,
                                               extpls,
                                               objInfo,
                                               bodyAtt,
                                               varsAtt)){
                defVarIsHaveError = 1;
                objInfo.appendPrompt("初始化表达式语法有误",thisSrcPath,thisLine,thisCol);
                continue;
            }
            varInfo = *objInfo.getVarDefineInfoNode()->getVar(varIndex);
        }

        //生成变量定义的指令
        PsdlIR_Compiler::MiddleNode defVarNode;
        if(varInfo.isAutoArrSize){
            defVarIsHaveError = 1;
            objInfo.appendPrompt("非初始化数组无法自动计算其大小",thisSrcPath,thisLine,thisCol);
            continue;
        }
        //非初始化局部变量
        DataTypeInfo * typeInfo = objInfo.getRootDataTypeInfoNode()->getDataType(varInfo.baseTypeIndex);
        DataBaseType baseType = typeInfo->baseType;
        if(varInfo.pointerLevel==0 &&
           varInfo.arraySize.length() == 0 &&
           baseType != DataBaseType_VOID &&
           baseType != DataBaseType_FUNPTR &&
           baseType != DataBaseType_STRUCT &&
           baseType != DataBaseType_UNION){
            QString dataType;
            //局部变量是基础数据类型
            if(baseType==DataBaseType_INT || baseType==DataBaseType_ENUM){
                dataType = "INT";
            }else if(baseType==DataBaseType_UINT){
                dataType = "UINT";
            }else if(baseType==DataBaseType_SHORT){
                dataType = "SHORT";
            }else if(baseType==DataBaseType_USHORT){
                dataType = "USHORT";
            }else if(baseType==DataBaseType_CHAR){
                dataType = "BYTE";
            }else if(baseType==DataBaseType_UCHAR){
                dataType = "UBYTE";
            }else if(baseType==DataBaseType_LONG){
                dataType = "LONG";
            }else if(baseType==DataBaseType_ULONG){
                dataType ="ULONG";
            }else if(baseType==DataBaseType_DOUBLE){
                dataType = "DOUBLE";
            }else if(baseType==DataBaseType_FLOAT){
                dataType = "FLOAT";
            }
            defVarNode.nodeType = "DATA";
            defVarNode.args = QStringList({dataType,varInfo.psdlName});
        }else if(varInfo.arraySize.length() == 0 && (varInfo.pointerLevel>0 || baseType==DataBaseType_FUNPTR)){
            //指针类型
            defVarNode.nodeType = "DATA";
            defVarNode.args = QStringList({"POINTER",varInfo.psdlName});
        }else{
            //复杂数据类型结构
            int baseTypeSize = typeInfo->dataBaseTypeBytes;
            if(varInfo.pointerLevel!=0){
                baseTypeSize = objInfo.CPU_BIT/8;
            }
            for(int j = 0;j<varInfo.arraySize.length();j++){
                baseTypeSize*=varInfo.arraySize[j];
            }

            defVarNode.nodeType = "ARRAY";
            defVarNode.args = QStringList({varInfo.psdlName,QString::number(baseTypeSize)+"D"});
            defVarNode.atts.append(varInfo.extendAttributes);
        }

        varsAtt.append(defVarNode);
    }

    return !defVarIsHaveError;
}

//获取ifelse中else的所有token并返回。
//同时，删除allPhrs中相应的部分
static PhraseList getElsePhrases(PhraseList &allPhrs,bool &status){
    Phrase tmp;
    tmp.text = ";";
    tmp.PhraseType = Phrase::Delimiter;


    PhraseList elsePhrases;
    status = 0;
    while(allPhrs.length() != 0){
        if(allPhrs[0].isKeyWord("if")){
            if(allPhrs.length()<3 || !allPhrs[1].isDelimiter("()"))return elsePhrases;
            elsePhrases.append(allPhrs[0]);
            allPhrs.removeFirst();
            elsePhrases.append(allPhrs[0]);
            allPhrs.removeFirst();

            if(allPhrs[0].isDelimiter("{}")){
                elsePhrases.append(allPhrs[0]);
                allPhrs.removeFirst();
            }else if(allPhrs.getDelimiterNum(";")){
                PhraseList oper = allPhrs.splitFromDelimiter(";")[0];
                oper.append(tmp);
                elsePhrases.append(oper);

                for(int i = 0;i<oper.length();i++){
                    allPhrs.removeFirst();
                }
            }else return elsePhrases;

            if(allPhrs.length()!=0 && allPhrs[0].isKeyWord("else")){
                elsePhrases.append(allPhrs[0]);
                allPhrs.removeFirst();
            }

        }else if(allPhrs[0].isDelimiter("{}")){
            elsePhrases.append(allPhrs[0]);
            allPhrs.removeFirst();
            break;
        }else if(allPhrs.getDelimiterNum(";")){
            PhraseList oper = allPhrs.splitFromDelimiter(";")[0];
            oper.append(tmp);
            elsePhrases.append(oper);
            for(int i = 0;i<oper.length();i++){
                allPhrs.removeFirst();
            }
            break;
        }else return elsePhrases;
    }
    status =1;
    return elsePhrases;
}

//解析函数内指令
static bool analysisFunOrder(int thisFunctionIndex,//当前函数的索引号
                      IndexVN funcIsHaveReturnValue,//函数是否有返回值(如果有，就是返回值变量的索引号)
                      PhraseList &sentence,//当前函数体/流程块体内的所有语句
                      QList<PsdlIR_Compiler::MiddleNode> &bodyAtt,
                      QList<PsdlIR_Compiler::MiddleNode> &varsAtt,
                      SrcObjectInfo &objInfo){

    while(sentence.length()!=0){
        int operUsePhraseCount = 0;
        if(sentence.length()>=3 &&
           sentence[0].isKeyWord("if")&&
           sentence[1].isDelimiter("()")){//if-else选择跳转语句
            objInfo.startAnalysisProcess(ifelseProcess);
            PhraseList judgeSubPhr = sentence[1].subPhraseList;
            if(judgeSubPhr.getDelimiterNum(";") != 0){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            Phrase tmp;
            tmp.text = ";";
            tmp.PhraseType = Phrase::Delimiter;
            judgeSubPhr.append(tmp);
            int judgeOperUsePhraseCount;
            QList<FuncOperValue> judgeValues = analysisFunVarOpertion(judgeSubPhr,
                                                                      objInfo,
                                                                      varsAtt,
                                                                      judgeOperUsePhraseCount);
            FuncOperValue::JudgeOperInfo judgeInfo = FuncOperValue::generateJudge(judgeValues);
            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_ERROR){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            PhraseList ifSubBodyPhr;
            QList<PsdlIR_Compiler::MiddleNode> ifSubOperBodyAtts;//if中的指令
            if(sentence[2].isDelimiter("{}")){
                ifSubBodyPhr = sentence[2].subPhraseList;
                operUsePhraseCount = 3;
            }else{

                if(sentence.getDelimiterNum(";") == 0){
                    objInfo.appendPrompt("流程块没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                ifSubBodyPhr = sentence.splitFromDelimiter(";")[0];
                ifSubBodyPhr.removeFirst();
                ifSubBodyPhr.removeFirst();
                ifSubBodyPhr.append(tmp);
                operUsePhraseCount = 2+ifSubBodyPhr.length();
            }

            if(!analysisFunOrder(thisFunctionIndex,
                                funcIsHaveReturnValue,
                                ifSubBodyPhr,
                                ifSubOperBodyAtts,
                                varsAtt,
                                objInfo)){
                return 0;
            }



            for(int i = 0;i<operUsePhraseCount;i++){
                sentence.removeFirst();
            }


            operUsePhraseCount = 0;
            objInfo.endAnalysisProcess();


            QList<PsdlIR_Compiler::MiddleNode> elseSubOperBodyAtts;//else中的指令
            bool isHaveElseOper = 0;
            if(sentence.length()!=0 && sentence[0].isKeyWord("else")){
                isHaveElseOper = 1;
                sentence.removeFirst();
                bool stats;
                PhraseList elseSubBodyPhr = getElsePhrases(sentence,stats);
                if(!stats){
                    objInfo.appendPrompt("流程块定义语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }

                objInfo.startAnalysisProcess(subProcess);
                if(!analysisFunOrder(thisFunctionIndex,
                                    funcIsHaveReturnValue,
                                    elseSubBodyPhr,
                                    elseSubOperBodyAtts,
                                    varsAtt,
                                    objInfo)){
                    return 0;
                }
                objInfo.endAnalysisProcess();
            }

            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_True){
                QString anonyMark = objInfo.defineAnonyMark();
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令



                bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                bodyAtt.append(ifSubOperBodyAtts);

                PsdlIR_Compiler::MiddleNode gotoMarkCode;
                gotoMarkCode.nodeType = "GOTO";
                gotoMarkCode.args = QStringList({anonyMark});
                bodyAtt.append(gotoMarkCode);

                if(isHaveElseOper){
                    bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                    bodyAtt.append(elseSubOperBodyAtts);
                }

                PsdlIR_Compiler::MiddleNode defMarkCode;
                defMarkCode.nodeType = "LABEL";
                defMarkCode.args = QStringList({anonyMark});
                bodyAtt.append(defMarkCode);

            }else if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_False){
                QString anonyMark = objInfo.defineAnonyMark();
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令

                PsdlIR_Compiler::MiddleNode gotoMarkCode;
                gotoMarkCode.nodeType = "GOTO";
                gotoMarkCode.args = QStringList({anonyMark});
                bodyAtt.append(gotoMarkCode);


                bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                bodyAtt.append(ifSubOperBodyAtts);

                PsdlIR_Compiler::MiddleNode defMarkCode;
                defMarkCode.nodeType = "LABEL";
                defMarkCode.args = QStringList({anonyMark});

                bodyAtt.append(defMarkCode);
                if(isHaveElseOper){
                    bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                    bodyAtt.append(elseSubOperBodyAtts);
                }

            }else{
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令

                //如果比较结果为真，此次循环需要执行的指令
                PsdlIR_Compiler::MiddleNode_Att ifBodyAtt;
                ifBodyAtt.attName = "IF";
                ifBodyAtt.subNodes.append(judgeInfo.suffixIncrementCode);
                ifBodyAtt.subNodes.append(ifSubOperBodyAtts);
                judgeInfo.judgeProcessCode.atts.append(ifBodyAtt);
                if(isHaveElseOper){
                    PsdlIR_Compiler::MiddleNode_Att elseBodyAtt;
                    elseBodyAtt.attName = "ELSE";
                    elseBodyAtt.subNodes.append(judgeInfo.suffixIncrementCode);
                    elseBodyAtt.subNodes.append(elseSubOperBodyAtts);
                    judgeInfo.judgeProcessCode.atts.append(elseBodyAtt);
                }
                bodyAtt.append(judgeInfo.judgeProcessCode);
            }

        }else if(sentence.length()>=3 &&
                 sentence[0].isKeyWord("while")&&
                 sentence[1].isDelimiter("()")){//while循环语句
            objInfo.startAnalysisProcess(whileProcess);
            PhraseList judgeSubPhr = sentence[1].subPhraseList;
            if(judgeSubPhr.getDelimiterNum(";") != 0){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            Phrase tmp;
            tmp.text = ";";
            tmp.PhraseType = Phrase::Delimiter;
            judgeSubPhr.append(tmp);
            int judgeOperUsePhraseCount;
            QList<FuncOperValue> judgeValues = analysisFunVarOpertion(judgeSubPhr,
                                                                      objInfo,
                                                                      varsAtt,
                                                                      judgeOperUsePhraseCount);
            FuncOperValue::JudgeOperInfo judgeInfo = FuncOperValue::generateJudge(judgeValues);
            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_ERROR){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }


            PhraseList subBodyPhr;
            QList<PsdlIR_Compiler::MiddleNode> subOperBodyAtts;
            if(sentence[2].isDelimiter("{}")){
                subBodyPhr = sentence[2].subPhraseList;
                operUsePhraseCount = 3;
            }else{
                if(sentence.getDelimiterNum(";") == 0){
                    objInfo.appendPrompt("流程块没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                subBodyPhr = sentence.splitFromDelimiter(";")[0];
                subBodyPhr.removeFirst();
                subBodyPhr.removeFirst();
                subBodyPhr.append(tmp);
                operUsePhraseCount = 2+subBodyPhr.length();
            }





            if(!analysisFunOrder(thisFunctionIndex,
                                funcIsHaveReturnValue,
                                subBodyPhr,
                                subOperBodyAtts,
                                varsAtt,
                                objInfo)){
                return 0;
            }
            WhileBlockInfo whileMark = objInfo.funcWhileBlockInfo.last();


            PsdlIR_Compiler::MiddleNode whileBeginMarkCode;//循环开始处标记点
            whileBeginMarkCode.nodeType = "LABEL";
            whileBeginMarkCode.args = QStringList({whileMark.beginMark});

            PsdlIR_Compiler::MiddleNode whileEndMarkCode;//循环结束处标记点
            whileEndMarkCode.nodeType = "LABEL";
            whileEndMarkCode.args = QStringList({whileMark.endMark});

            PsdlIR_Compiler::MiddleNode gotoWhileBeginCode;//跳转回循环起始处指令
            gotoWhileBeginCode.nodeType = "GOTO";
            gotoWhileBeginCode.args = QStringList({whileMark.beginMark});

            PsdlIR_Compiler::MiddleNode gotoWhileEndCode;//跳转回循环结束处指令
            gotoWhileEndCode.nodeType = "GOTO";
            gotoWhileEndCode.args = QStringList({whileMark.endMark});


            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_True){
                bodyAtt.append(whileBeginMarkCode); //while起始标记点
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令
                bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                bodyAtt.append(subOperBodyAtts);    //while主体程序的指令
                bodyAtt.append(gotoWhileBeginCode); //跳转会while起始点指令
                bodyAtt.append(whileEndMarkCode);   //while结束处标记点
            }else if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_False){
                bodyAtt.append(whileBeginMarkCode); //while起始标记点
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令
                bodyAtt.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                bodyAtt.append(gotoWhileEndCode);   //跳转到while结束点指令
                bodyAtt.append(subOperBodyAtts);    //while主体程序的指令
                bodyAtt.append(gotoWhileBeginCode); //跳转回while起始点指令
                bodyAtt.append(whileEndMarkCode);   //while结束处标记点
            }else{
                bodyAtt.append(whileBeginMarkCode); //while起始标记点
                bodyAtt.append(judgeInfo.operCode); //while判断是否执行循环体内程序的指令

                //如果比较结果为真，此次循环需要执行的指令
                PsdlIR_Compiler::MiddleNode_Att whileBodyAtt;
                whileBodyAtt.attName = "IF";
                whileBodyAtt.subNodes.append(judgeInfo.suffixIncrementCode);//while判断是否执行循环体内程序的自增自减指令
                whileBodyAtt.subNodes.append(subOperBodyAtts);//while主体程序的指令
                whileBodyAtt.subNodes.append(gotoWhileBeginCode); //跳转回while起始点指令
                judgeInfo.judgeProcessCode.atts.append(whileBodyAtt);
                bodyAtt.append(judgeInfo.judgeProcessCode);

                bodyAtt.append(whileEndMarkCode);   //while结束处标记点
            }
            objInfo.endAnalysisProcess();
        }else if(sentence.length()>=5 &&
                 sentence[0].isKeyWord("do")){//do-while循环语句
            objInfo.startAnalysisProcess(whileProcess);
            Phrase tmp;
            tmp.text = ";";
            tmp.PhraseType = Phrase::Delimiter;
            PhraseList subBodyPhr;
            QList<PsdlIR_Compiler::MiddleNode> subOperBodyAtts;
            if(sentence[1].isDelimiter("{}")){
                subBodyPhr = sentence[1].subPhraseList;
                operUsePhraseCount = 2;
            }else{

                if(sentence.getDelimiterNum(";") == 0){
                    objInfo.appendPrompt("流程块没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                subBodyPhr = sentence.splitFromDelimiter(";")[0];
                subBodyPhr.removeFirst();
                subBodyPhr.append(tmp);
                operUsePhraseCount = 1+subBodyPhr.length();
            }

            for(int i = 0;i<operUsePhraseCount;i++){
                sentence.removeFirst();
            }



            if(!analysisFunOrder(thisFunctionIndex,
                                funcIsHaveReturnValue,
                                subBodyPhr,
                                subOperBodyAtts,
                                varsAtt,
                                objInfo)){
                return 0;
            }

            operUsePhraseCount = 3;
            if(sentence.length()<3 ||
               !sentence[0].isKeyWord("while") ||
               !sentence[1].isDelimiter("()")||
               !sentence[2].isDelimiter(";")){
                objInfo.appendPrompt("do-while流程块语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }



            PhraseList judgeSubPhr = sentence[1].subPhraseList;
            judgeSubPhr.append(tmp);
            int judgeOperUsePhraseCount;
            QList<FuncOperValue> judgeValues = analysisFunVarOpertion(judgeSubPhr,
                                                                      objInfo,
                                                                      varsAtt,
                                                                      judgeOperUsePhraseCount);
            FuncOperValue::JudgeOperInfo judgeInfo = FuncOperValue::generateJudge(judgeValues);
            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_ERROR){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            WhileBlockInfo whileMark = objInfo.funcWhileBlockInfo.last();


            PsdlIR_Compiler::MiddleNode whileBeginMarkCode;//循环开始处标记点
            whileBeginMarkCode.nodeType = "LABEL";
            whileBeginMarkCode.args = QStringList({whileMark.beginMark});

            PsdlIR_Compiler::MiddleNode whileEndMarkCode;//循环结束处标记点
            whileEndMarkCode.nodeType = "LABEL";
            whileEndMarkCode.args = QStringList({whileMark.endMark});

            PsdlIR_Compiler::MiddleNode gotoWhileBeginCode;//跳转回循环起始处指令
            gotoWhileBeginCode.nodeType = "GOTO";
            gotoWhileBeginCode.args = QStringList({whileMark.beginMark});

            PsdlIR_Compiler::MiddleNode gotoWhileEndCode;//跳转回循环结束处指令
            gotoWhileEndCode.nodeType = "GOTO";
            gotoWhileEndCode.args = QStringList({whileMark.endMark});


            if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_True){
                bodyAtt.append(whileBeginMarkCode); //do-while标记点
                bodyAtt.append(subOperBodyAtts);    //do-while的主体指令
                bodyAtt.append(judgeInfo.operCode); //do-while中比较判断是否有下次循环的指令
                bodyAtt.append(judgeInfo.suffixIncrementCode);//比较判断过程中后缀++--的指令
                bodyAtt.append(gotoWhileBeginCode); //跳转回do-while起始点指令
                bodyAtt.append(whileEndMarkCode);   //do-while结束标记点
            }else if(judgeInfo.judgeResultType == judgeInfo.JudgeResult_False){
                bodyAtt.append(whileBeginMarkCode); //do-while标记点
                bodyAtt.append(subOperBodyAtts);    //do-while的主体指令
                bodyAtt.append(judgeInfo.operCode); //do-while中比较判断是否有下次循环的指令
                bodyAtt.append(judgeInfo.suffixIncrementCode);//比较判断过程中后缀++--的指令
                bodyAtt.append(whileEndMarkCode);   //do-while结束标记点
            }else{
                bodyAtt.append(whileBeginMarkCode); //do-while标记点
                bodyAtt.append(subOperBodyAtts);    //do-while的主体指令
                bodyAtt.append(judgeInfo.operCode); //do-while中比较判断是否有下次循环的指令

                //根据比较判断的结果，条件跳转至do-while起始点的指令
                PsdlIR_Compiler::MiddleNode_Att whileJudgeAtt;
                whileJudgeAtt.attName = "IF";
                whileJudgeAtt.subNodes.append(judgeInfo.suffixIncrementCode);//如果为真，执行比较判断过程中后缀++--的指令
                whileJudgeAtt.subNodes.append(gotoWhileBeginCode);           //如果为真，执行跳转回do-while起始点指令

                if(judgeInfo.suffixIncrementCode.length()!=0){
                    PsdlIR_Compiler::MiddleNode_Att whileJudgeElseAtt;
                    whileJudgeElseAtt.attName = "ELSE";
                    whileJudgeElseAtt.subNodes.append(judgeInfo.suffixIncrementCode);//如果为假，执行比较判断过程中后缀++--的指令
                    judgeInfo.judgeProcessCode.atts.append(whileJudgeElseAtt);
                }


                judgeInfo.judgeProcessCode.atts.append(whileJudgeAtt);
                bodyAtt.append(judgeInfo.judgeProcessCode);

                bodyAtt.append(whileEndMarkCode);   //do-while结束标记点
            }
            objInfo.endAnalysisProcess();
        }else if(sentence.length()>=3 &&
                 sentence[0].isKeyWord("for")&&
                 sentence[1].isDelimiter("()")){//for循环语句
            objInfo.startAnalysisProcess(ProcessType::subProcess);
            PhraseList judgeSubPhr = sentence[1].subPhraseList;

            int line = sentence[0].line;
            int col = sentence[0].col;
            QString srcPath = sentence[0].srcPath;

            sentence.removeFirst();
            sentence.removeFirst();

            QList<PhraseList> forOperPhr = judgeSubPhr.splitFromDelimiter(";");
            if(forOperPhr.length()!=3){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            Phrase tmp;
            tmp.text = ";";
            tmp.PhraseType = Phrase::Delimiter;

            /////////将for语句转为while语句去处理////////////


            //初始表达式
            PhraseList initOper = forOperPhr[0];
            initOper.append(tmp);


            //循环体表达式
            PhraseList bodyOper;
            if(sentence[0].isDelimiter("{}")){
                bodyOper = sentence[0].subPhraseList;
                operUsePhraseCount = 1;
            }else{
                if(sentence.getDelimiterNum(";")==0){
                    objInfo.appendPrompt("流程块没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                bodyOper = sentence.splitFromDelimiter(";")[0];
                bodyOper.append(tmp);
                operUsePhraseCount = bodyOper.length();
            }
            //循环每次即将结束前执行的表达式
            PhraseList endOper = forOperPhr[2];
            endOper.append(tmp);
            bodyOper.append(endOper);


            //转换为while语句
            Phrase whilePhr;
            whilePhr.PhraseType = Phrase::KeyWord;
            whilePhr.text = "while";
            whilePhr.col = col;
            whilePhr.line = line;
            whilePhr.srcPath = srcPath;

            Phrase whileCmpPhr;
            whileCmpPhr.PhraseType = Phrase::SubExp_PAR;
            whileCmpPhr.subPhraseList.append(forOperPhr[1]);
            whileCmpPhr.col = col;
            whileCmpPhr.line = line;
            whileCmpPhr.srcPath = srcPath;


            Phrase whileBodyPhr;
            whileBodyPhr.PhraseType = Phrase::SubExp_BRACE;
            whileBodyPhr.subPhraseList = bodyOper;
            whileBodyPhr.col = col;
            whileBodyPhr.line = line;
            whileBodyPhr.srcPath = srcPath;

            PhraseList operPhrase;
            operPhrase.append(initOper);
            operPhrase.append(whilePhr);
            operPhrase.append(whileCmpPhr);
            operPhrase.append(whileBodyPhr);

            QList<PsdlIR_Compiler::MiddleNode> forCodes;
            if(!analysisFunOrder(thisFunctionIndex,
                                 funcIsHaveReturnValue,
                                 operPhrase,
                                 forCodes,
                                 varsAtt,
                                 objInfo)){
                return 0;
            }
            bodyAtt.append(forCodes);

            objInfo.endAnalysisProcess();
        }else if(sentence.length()>=3 &&
                 sentence[0].isKeyWord("switch")&&
                 sentence[1].isDelimiter("()")){//switch-case选择跳转语句
            objInfo.startAnalysisProcess(switchProcess);
            PhraseList judgeSubPhr = sentence[1].subPhraseList;
            if(judgeSubPhr.getDelimiterNum(";") != 0){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            Phrase tmp;
            tmp.text = ";";
            tmp.PhraseType = Phrase::Delimiter;
            judgeSubPhr.append(tmp);
            int judgeOperUsePhraseCount;
            FuncOperValue judgeValue = FuncOperValue::generateSwitchJudge(analysisFunVarOpertion(judgeSubPhr,
                                                                                                 objInfo,
                                                                                                 varsAtt,
                                                                                                 judgeOperUsePhraseCount));
            if(judgeValue.isError()){
                objInfo.appendPrompt("流程块语句()内表达式有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            PhraseList switchBodyPhr;//switch块内的语句
            if(sentence[2].isDelimiter("{}")){
                switchBodyPhr = sentence[2].subPhraseList;
                operUsePhraseCount = 3;
            }else{
                if(sentence.getDelimiterNum(";") == 0){
                    objInfo.appendPrompt("流程块没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                switchBodyPhr = sentence.splitFromDelimiter(";")[0];
                switchBodyPhr.removeFirst();
                switchBodyPhr.removeFirst();
                switchBodyPhr.append(tmp);
                operUsePhraseCount = 2+switchBodyPhr.length();
            }

            //解析得到的switch中各个分支的信息
            struct SwitchCaseInfo{
                QList<PsdlIR_Compiler::MiddleNode> caseCodes;//分支中的指令
                bool isDefault = 0;//是否是default语句
                int caseConstValue;//是case语句,case为真的取值

                QString caseMarkName;//case的入口跳转标记点名称
            };
            bool isDefinedDefaultCase = 0;
            QList<SwitchCaseInfo> allCase;
            //解析switch中的分支
            while(switchBodyPhr.length() != 0){
                SwitchCaseInfo tmpCase;
                if(switchBodyPhr[0].isKeyWord("case")){

                    switchBodyPhr.removeFirst();
                    if(switchBodyPhr.length() < 3 || switchBodyPhr.getDelimiterNum(":")==0){
                        objInfo.appendPrompt("流程块中语法有误",switchBodyPhr[0].srcPath,switchBodyPhr[0].line,switchBodyPhr[0].col);
                        return 0;
                    }

                    PhraseList constValuePhr = switchBodyPhr.splitFromDelimiter(":")[0];
                    int removCount = constValuePhr.length()+1;
                    for(int i = 0;i<removCount;i++){
                        switchBodyPhr.removeFirst();
                    }

                    bool state;
                    int value = analysisConstValueOperator(constValuePhr,objInfo,state);
                    if(!state){
                        objInfo.appendPrompt("分支条件语句语法有误",switchBodyPhr[0].srcPath,switchBodyPhr[0].line,switchBodyPhr[0].col);
                        return 0;
                    }
                    tmpCase.caseConstValue = value;

                }else if(switchBodyPhr[0].isKeyWord("default")){
                    if(isDefinedDefaultCase){
                        objInfo.appendPrompt("重复定义了default分支",switchBodyPhr[0].srcPath,switchBodyPhr[0].line,switchBodyPhr[0].col);
                        return 0;
                    }
                    tmpCase.isDefault = 1;
                    isDefinedDefaultCase = 1;

                    switchBodyPhr.removeFirst();
                    if(switchBodyPhr.length() < 2 || !switchBodyPhr[0].isDelimiter(":")){
                        objInfo.appendPrompt("流程块中语法有误",switchBodyPhr[0].srcPath,switchBodyPhr[0].line,switchBodyPhr[0].col);
                        return 0;
                    }
                    switchBodyPhr.removeFirst();
                }else{
                    objInfo.appendPrompt("流程块中语法有误",switchBodyPhr[0].srcPath,switchBodyPhr[0].line,switchBodyPhr[0].col);
                    return 0;
                }


                PhraseList caseBodyPhr;
                while(switchBodyPhr.length() != 0){
                    if(switchBodyPhr[0].isKeyWord("case") || switchBodyPhr[0].isKeyWord("default"))break;
                    caseBodyPhr.append(switchBodyPhr[0]);
                    switchBodyPhr.removeFirst();
                }
                QList<PsdlIR_Compiler::MiddleNode> caseBodyCodes;
                if(!analysisFunOrder(thisFunctionIndex,
                                    funcIsHaveReturnValue,
                                    caseBodyPhr,
                                    caseBodyCodes,
                                    varsAtt,
                                    objInfo)){
                    return 0;
                }
                tmpCase.caseCodes = caseBodyCodes;

                tmpCase.caseMarkName = objInfo.defineAnonyMark();
                allCase.append(tmpCase);
            }
            //生成根据swicth比较值，选择跳转到哪个分支中的指令
            bodyAtt.append(judgeValue.bodyAtt);

            bool isAlwaysJump = 0;
            for(int i = 0;i<allCase.length();i++){
                PsdlIR_Compiler::MiddleNode gotoCaseCode;
                gotoCaseCode.nodeType = "GOTO";
                gotoCaseCode.args = QStringList({allCase[i].caseMarkName});

                if(allCase[i].isDefault){
                    bodyAtt.append(gotoCaseCode);
                    isAlwaysJump = 1;
                    break;
                }else if(judgeValue.funcOperValueType == FuncOperValueType_RegData){
                    FuncOperValue::RegDataInfo info;
                    judgeValue.getInfo_RegData(info);
                    FuncOperValue caseConstVar(allCase[i].caseConstValue,varsAtt,objInfo);
                    caseConstVar = caseConstVar.converType(info.baseTypeIndex,info.pointerLevel);


                    PsdlIR_Compiler::MiddleNode processCode;
                    processCode.nodeType = "PROCESS";
                    processCode.args = QStringList({"EC",
                                                   FuncOperValue::generateArgText(judgeValue),
                                                   FuncOperValue::generateArgText(caseConstVar)});
                    PsdlIR_Compiler::MiddleNode_Att ifAtt;
                    ifAtt.attName = "IF";
                    ifAtt.subNodes +=gotoCaseCode;
                    processCode.atts.append(ifAtt);
                    bodyAtt.append(processCode);
                }else if(judgeValue.funcOperValueType == FuncOperValueType_ConstValue){
                    FuncOperValue::ConstValueInfo info;
                    judgeValue.getInfo_ConstValue(info);
                    if(info.constIntValue == allCase[i].caseConstValue){
                        bodyAtt.append(gotoCaseCode);
                        isAlwaysJump = 1;
                        break;
                    }
                }else{
                    objInfo.appendPrompt("流程块语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
            }
            //如果未能进入任何分支，跳转退出switch
            WhileBlockInfo whileMark = objInfo.funcWhileBlockInfo.last();
            if(!isAlwaysJump){
                PsdlIR_Compiler::MiddleNode switchGotoEndMarkCode;
                switchGotoEndMarkCode.nodeType = "GOTO";
                switchGotoEndMarkCode.args = QStringList({whileMark.endMark});
                bodyAtt.append(switchGotoEndMarkCode);
            }

            //生成switch中各个分支的指令
            for(int i = 0;i<allCase.length();i++){
                PsdlIR_Compiler::MiddleNode markBeginMarkCode;
                markBeginMarkCode.nodeType = "LABEL";
                markBeginMarkCode.args = QStringList({allCase[i].caseMarkName});
                bodyAtt.append(markBeginMarkCode);
                if(allCase[i].caseCodes.length()==0)continue;
                bodyAtt.append(allCase[i].caseCodes);
            }

            PsdlIR_Compiler::MiddleNode switchEndMarkCode;
            switchEndMarkCode.nodeType = "LABEL";
            switchEndMarkCode.args = QStringList({whileMark.endMark});
            bodyAtt.append(switchEndMarkCode);
            objInfo.endAnalysisProcess();
        }else if(sentence.length()>=2 &&
                 sentence[0].isKeyWord("return")){//函数退出语句
            if(!funcIsHaveReturnValue.isEmpty()){
                if(sentence[1].isDelimiter(";")){
                    objInfo.appendPrompt("函数必须有一个返回值",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }else if(sentence.getDelimiterNum(";")==0){
                    objInfo.appendPrompt("语句没有结束符",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                sentence.removeFirst();
                FuncOperValue retValue(funcIsHaveReturnValue,varsAtt,objInfo);
                FuncOperValue dereference("(*)",varsAtt,objInfo);
                FuncOperValue assignment("=",varsAtt,objInfo);

                FuncOperToken retValueOper = analysisFunVarOpertion(sentence,
                                                        objInfo,
                                                        varsAtt,
                                                        operUsePhraseCount);

                FuncOperValue tempVar = FuncOperValue::mergeExp(retValueOper);

                if(tempVar.funcOperValueType == FuncOperValueType_Error){
                    objInfo.appendPrompt("算术表达式语句语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                FuncOperToken retOper = {dereference,retValue,assignment,tempVar};
                tempVar = opertionFuncOperToken(retOper);
                if(tempVar.funcOperValueType == FuncOperValueType_Error){
                    objInfo.appendPrompt("返回的数据不正确",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }
                //将返回值结构
                bodyAtt.append(tempVar.bodyAtt);

                PsdlIR_Compiler::MiddleNode node;
                node.nodeType = "RETURN";
                node.args = QStringList();
                bodyAtt.append(node);
            }else{
                if(!sentence[1].isDelimiter(";")){
                    objInfo.appendPrompt("该函数return不可返回任何数据",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                    return 0;
                }

                PsdlIR_Compiler::MiddleNode node;
                node.nodeType = "RETURN";
                node.args = QStringList();
                bodyAtt.append(node);
                operUsePhraseCount = 2;
            }
        }else if(sentence.length()>=2 &&
                 sentence[0].isKeyWord("break")&&
                 sentence[1].isDelimiter(";")){//break退出循环语句
            if(objInfo.funcWhileBlockInfo.length()==0){
                objInfo.appendPrompt("此处非循环流程块中,不可使用break",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "GOTO";
            node.args = QStringList({objInfo.funcWhileBlockInfo.last().endMark});
            bodyAtt.append(node);
            operUsePhraseCount = 2;
        }else if(sentence.length()>=2 &&
                 sentence[0].isKeyWord("continue")&&
                 sentence[1].isDelimiter(";")){//continue进入下一循环语句
            if(objInfo.funcWhileBlockInfo.length()==0){
                objInfo.appendPrompt("此处非循环流程块中,不可使用continue",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }

            QString gotoMark;
            bool isHaveWhile = 0;
            for(int i = objInfo.funcWhileBlockInfo.length()-1;i>=0;i--){
                if(!objInfo.funcWhileBlockInfo[i].isSwitch){
                    isHaveWhile = 1;
                    gotoMark = objInfo.funcWhileBlockInfo[i].beginMark;
                }
            }
            if(!isHaveWhile){
                objInfo.appendPrompt("此处非循环流程块中,不可使用continue",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "GOTO";
            node.args = QStringList({gotoMark});
            bodyAtt.append(node);
            operUsePhraseCount = 2;
        }else if( sentence.length()>=3 &&
                  sentence[0].isKeyWord("goto") &&
                  sentence[1].isIdentifier()&&
                  sentence[2].isDelimiter(";")){//无条件跳转起跳点
            QString markName = sentence[1].text;
            JumpMarkInfo info;
            info.markName = markName;
            info.srcPath = sentence[0].srcPath;
            info.line = sentence[0].line;
            info.col = sentence[0].col;
            objInfo.jumpMarks.append(info);

            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "GOTO";
            node.args = QStringList({markName});
            bodyAtt.append(node);


            operUsePhraseCount = 3;
        }else if(sentence.length()>=2 &&
                 sentence.getDelimiterNum(":",";")&&
                 sentence[0].isIdentifier()&&
                 sentence[1].isDelimiter(":")){//无条件跳转标记点
            QString markName = sentence[0].text;
            if(objInfo.defMarks.contains(markName)){
                objInfo.appendPrompt("跳转标记点重复定义",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                return 0;
            }
            objInfo.defMarks.append(markName);
            PsdlIR_Compiler::MiddleNode node;
            node.nodeType = "LABEL";
            node.args = QStringList({markName});
            bodyAtt.append(node);

            operUsePhraseCount = 2;
        }else if(sentence[0].isDelimiter("{}")){//无任何特殊功能的子流程块
            operUsePhraseCount = 1;
            PhraseList subPhrase = sentence[0].subPhraseList;
            objInfo.startAnalysisProcess(subProcess);
            if(!analysisFunOrder(thisFunctionIndex,funcIsHaveReturnValue,subPhrase,bodyAtt,varsAtt,objInfo)){
                return 0;
            }
            objInfo.endAnalysisProcess();
        }else if(sentence[0].isDelimiter(";")){
            operUsePhraseCount = 1;
        }else if(sentence.getDelimiterNum(";")){//变量定义运算、内嵌汇编语句
            if(sentence[0].isKeyWord("__asm__") && sentence[1].isDelimiter("()") && sentence[2].isDelimiter(";")){

                //内嵌汇编语句:  语法 __asm__("汇编语句1","汇编语句2","汇编语句3",.....);

                operUsePhraseCount = 3;
                PhraseList subPhrase = sentence[1].subPhraseList;
                QList<PhraseList> codes = subPhrase.splitFromDelimiter(",");
                for(int i = 0;i<codes.length();i++){
                    if(codes[i].length() != 1 || codes[i][0].PhraseType != Phrase::ConstValue_STRING){
                        objInfo.appendPrompt("内嵌汇编语法有误",
                                             sentence[0].srcPath,
                                             sentence[0].line,
                                             sentence[0].col);
                        return 0;
                    }

                    //解析内嵌的汇编指令
                    QString asmCodeText = codes[i][0].text;
                    asmCodeText.removeFirst();
                    asmCodeText.removeLast();
                    asmCodeText = dislodgeSpace(asmCodeText);

                    PsdlIR_Compiler::MiddleNode node;
                    node.nodeType = "ASM";

                    QStringList tmp = asmCodeText.split(" ");
                    QString type = tmp[0];


                    QStringList args;
                    if(tmp.length() > 1){
                        tmp.removeFirst();
                        asmCodeText = tmp.join("");
                        args = asmCodeText.split(",");
                        //如果汇编代码中使用到了局部变量，将局部变量名称替换为局部变量转位PSDL后的名称
                        for(int j = 0;j<args.length();j++){
                            IndexVN varIndex = objInfo.getVarDefineInfoNode()->searchVar(args[j]);
                            if(!varIndex.isEmpty()){
                                args[j] = objInfo.getVarDefineInfoNode()->getVar(varIndex)->psdlName;
                            }
                        }
                    }
                    args.prepend(type);
                    node.args = args;




                    bodyAtt.append(node);
                }

            }else{
                //解析是否是定义变量语句
                bool isDefineVar;
                bool su = analysisFunDefineVarOrder(sentence,
                                                     objInfo,
                                                     bodyAtt,varsAtt,
                                                     operUsePhraseCount,
                                                     isDefineVar);
                if(!isDefineVar){
                    //如果不是定义变量的语句,解析判断是否是运算语句
                    FuncOperValue tempVar = FuncOperValue::mergeExp(analysisFunVarOpertion(sentence,
                                                                   objInfo,
                                                                   varsAtt,
                                                                   operUsePhraseCount));
                    if(tempVar.funcOperValueType == FuncOperValueType_Error){
                        objInfo.appendPrompt("算术表达式语句语法有误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
                        return 0;
                    }
                    bodyAtt.append(tempVar.bodyAtt);
                }else if(!su){
                    objInfo.appendPrompt("定义语句语法有误",
                                         sentence[0].srcPath,
                                         sentence[0].line,
                                         sentence[0].col);
                    return 0;
                }
            }
        }else{//错误语句
            objInfo.appendPrompt("语法错误",sentence[0].srcPath,sentence[0].line,sentence[0].col);
            return 0;
        }

        for(int i = 0;i<operUsePhraseCount;i++){
            sentence.removeFirst();
        }

    }
    return 1;
}

//解析函数定义
static bool analysisDefineFunction(PhraseList &sentence,
                            SrcObjectInfo &objInfo){

    if(sentence.length()<4)return 0;
    if(!sentence[sentence.length()-1].isDelimiter("{}"))return 0;
    PhraseList codeDef = sentence[sentence.length()-1].subPhraseList;
    sentence.removeLast();
    QStringList funExAtts;
    for(int i = sentence.length()-1;i>=0;i--){
        if(sentence[i].PhraseType == Phrase::KeyWord){
            funExAtts.append(sentence[i].text);
            sentence.removeLast();
        }else{
            break;
        }
    }
    if(!sentence[sentence.length()-1].isDelimiter("()") ||
       !sentence[sentence.length()-2].isIdentifier())return 0;

    QString funName = sentence[sentence.length()-2].text;
    PhraseList argsDef = sentence[sentence.length()-1].subPhraseList;

    sentence.removeLast();
    sentence.removeLast();
    if(sentence.judgeIsHaveAnyKeyWord({"if",
                                      "else",
                                      "while",
                                      "for",
                                      "goto",
                                      "continue",
                                      "break",
                                      "return",
                                      "switch",
                                      "case",
                                      "default",
                                      "typedef",
                                      "const",
                                      "auto"})){
        //语句中存在不合语法的关键字,出错
        return 0;
    }

    int retPointerLevel = 0;
    for(int i = sentence.length()-1;i>=0;i--){
        if(sentence[i].isDelimiter("*")){
            retPointerLevel ++;
            sentence.removeLast();
        }else{
            break;
        }
    }

    bool isStatic = sentence.getKeyWordNum("static");//是否是静态函数
    if(sentence.getKeyWordNum("static") && sentence.getKeyWordNum("auto")){
        return 0;
    }

    sentence.removeKeyWord("static");
    sentence.removeKeyWord("auto");

    bool extendAttributeSuceess;
    QList<PsdlIR_Compiler::MiddleNode_Att> extendAtts =
            analysisExtendAttribute(sentence,objInfo,extendAttributeSuceess);
    if(!extendAttributeSuceess){
        objInfo.appendPrompt("定义了不支持的扩展属性",sentence[0].srcPath,sentence[0].line,sentence[0].col);
        return 0;
    }

    IndexVN retIndex;
    //解析使用的类型
    if(sentence[0].isKeyWord("struct") || sentence[0].isKeyWord("union") || sentence[0].isKeyWord("enum")){
        //增加一个自定义的枚举/结构体/共用体类型
        if(sentence.length()==2 && !sentence[1].isDelimiter("{}")){
            //或者为此前创建的枚举/结构体/类型重定义一个数据类型名
            if(!sentence[1].isIdentifier())return 0;
            if(sentence[0].isKeyWord("struct")){
                retIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[1].text);
            }else if(sentence[0].isKeyWord("union")){
                retIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[1].text);
            }else if(sentence[0].isKeyWord("enum")){
                retIndex = objInfo.getDataTypeInfoNode()->searchStructType(sentence[1].text);
            }
            if(retIndex.isEmpty()){
                //如果为空,声明该struct/union/enum
                DataTypeInfo enumInfo;
                if(sentence[0].isKeyWord("struct")){
                    enumInfo.baseType = DataBaseType_STRUCT;
                }else if(sentence[0].isKeyWord("union")){
                    enumInfo.baseType = DataBaseType_UNION;
                }else if(sentence[0].isKeyWord("enum")){
                    enumInfo.baseType = DataBaseType_ENUM;
                }
                enumInfo.isAnnony = 0;
                enumInfo.structName = sentence[1].text;
                enumInfo.isOnlyStatement = 1;
                retIndex = objInfo.getDataTypeInfoNode()->appendCustomSturctOrUnionOrEnum(enumInfo);
            }

        }else if((sentence.length()==2 && sentence[1].isDelimiter("{}")) ||
                 (sentence.length()==3 && sentence[1].PhraseType==Phrase::Identifier && sentence[2].isDelimiter("{}"))){
            //增加一个自定义的枚举/结构体/共用体类型
            if(sentence[0].isKeyWord("enum")){
                retIndex = analysisEnumTypeDef(sentence,objInfo);
            }else{
                retIndex = analysisStructUnionTypeDef(sentence,objInfo);
            }
            if(retIndex.isEmpty())return 0;
        }else{
            return 0;
        }
    }else{
        //对于c基础数据类型重定义一个数据类型名
        //或者对于此前自定义的数据类型再增加一个数据类型名
        if(!sentence.isOnlyHaveKeyWordAndIdentifier())return 0;

        //被重定义类型的数据类型名
        QStringList oyName = sentence.getKeyWordAndIdentifierText();
        oyName = stdFormatBaseTypeName(oyName);
        bool status;
        MappingInfo map = objInfo.getDataTypeInfoNode()->searchMappingInfo(oyName,status);
        retIndex = map.index;
        retPointerLevel += map.pointerLevel;
    }

    bool isStdcall;
    bool status;
    QList<FunArgDefInfo> funAttInfo = analysisFunctionArgDef(argsDef,
                                                            objInfo,
                                                            isStdcall,
                                                            status);
    if(status==0)return 0;

    //函数对应的函数指针类型注册
    DataTypeInfo functionInfo;
    functionInfo.baseType = DataBaseType_FUNPTR;
    functionInfo.dataBaseTypeBytes = objInfo.CPU_BIT/8;
    functionInfo.isStdcallFun = isStdcall;

    //返回值类型信息
    StructUnion_AttInfo attTmp;
    attTmp.typeIndex = retIndex;
    attTmp.isConst = 0;//函数返回值不可能是一个const型
    attTmp.arraySize.clear();
    attTmp.pointerLevel = retPointerLevel;
    functionInfo.funArgInfos.append(attTmp);

    for(int i = 0;i<funAttInfo.length();i++){
        if(funAttInfo[i].isAnnony)return 0;
        functionInfo.funArgInfos.append(funAttInfo[i].info);
    }
    IndexVN funTypeInfo = objInfo.getRootDataTypeInfoNode()->appendFunctionType(functionInfo);
    if(funTypeInfo.isEmpty())return 0;

    //将函数信息注册
    FunctionInfo funInfo;
    funInfo.functionName = funName;
    funInfo.baseTypeIndex = funTypeInfo;
    funInfo.isOnlyStatement = 0;
    funInfo.isStatic = isStatic;
    int functionIndex = objInfo.appendFunctionDefine(funInfo);
    if(functionIndex==-1)return 0;

    objInfo.startAnalysisFunction();

    //////////生成函数PSDL描述节点/////////////
    PsdlIR_Compiler::MiddleNode funDefNode;
    funDefNode.nodeType = "FUN";
    funDefNode.args = QStringList(funName);

    if(!isStatic){
        PsdlIR_Compiler::MiddleNode_Att externNode;
        externNode.attName = "EXPORT";
        funDefNode.atts.append(externNode);
    }
    if(isStdcall){
        PsdlIR_Compiler::MiddleNode_Att stdcall;
        stdcall.attName = "STDCALL";
        funDefNode.atts.append(stdcall);
    }
    for(int i = 0;i<funExAtts.length();i++){
        PsdlIR_Compiler::MiddleNode_Att exAtt;
        exAtt.attName = funExAtts[i];
        exAtt.attName = exAtt.attName.toUpper();
        funDefNode.atts.append(exAtt);
    }

    //////////生成函数参数定义节////////////
    PsdlIR_Compiler::MiddleNode_Att funArgAtt;
    funArgAtt.attName = "ARGS";

    IndexVN funcIsHaveReturnValue;//函数是否有返回值
    DataTypeInfo *retVarType = objInfo.getRootDataTypeInfoNode()->getDataType(attTmp.typeIndex);
    int retVarPointerLevel = attTmp.pointerLevel;
    if(retVarType->baseType != DataBaseType_VOID || retVarPointerLevel!=0){
        PsdlIR_Compiler::MiddleNode ret;
        ret.nodeType = "DATA";
        ret.args = QStringList({"POINTER","__ret__"});
        funArgAtt.subNodes.append(ret);


        VarDefineInfo varInfo;
        varInfo.isConst = 0;
        varInfo.isExtern = 0;
        varInfo.isStatic = 0;
        varInfo.varName = "|__funcRetVar__|";
        varInfo.psdlName ="__ret__";
        varInfo.arraySize.clear();
        varInfo.pointerLevel = attTmp.pointerLevel + 1;
        varInfo.baseTypeIndex = attTmp.typeIndex;
        varInfo.isFuncVar = 1;
        IndexVN varIndex = objInfo.getVarDefineInfoNode()->defineVar(varInfo);
        if(varIndex.isEmpty()){
            objInfo.endAnalysisFunction();
            return 0;
        }
        funcIsHaveReturnValue = varIndex;

    }


    for(int i = 0;i<funAttInfo.length();i++){
        DataTypeInfo *typeInfo = objInfo.getRootDataTypeInfoNode()->getDataType(funAttInfo[i].info.typeIndex);
        DataBaseType baseType = typeInfo->baseType;

        VarDefineInfo varInfo;
        varInfo.isConst = funAttInfo[i].info.isConst;
        varInfo.isExtern = 0;
        varInfo.isStatic = 0;
        varInfo.varName = funAttInfo[i].name;
        varInfo.arraySize.clear();
        varInfo.pointerLevel = funAttInfo[i].info.pointerLevel;
        varInfo.baseTypeIndex = funAttInfo[i].info.typeIndex;
        varInfo.isFuncVar = 1;
        IndexVN varIndex = objInfo.defineVar(varInfo);
        if(varIndex.isEmpty()){
            objInfo.endAnalysisFunction();
            return 0;
        }
        QString psdlName = objInfo.getRootVarDefineInfoNode()->getVar(varIndex)->psdlName;

        PsdlIR_Compiler::MiddleNode arg;


        if(funAttInfo[i].info.pointerLevel>=1){
            arg.nodeType = "DATA";
            arg.args = QStringList({"POINTER",psdlName});
        }else if(baseType==DataBaseType_UNION || baseType==DataBaseType_STRUCT){
            arg.nodeType = "ARRAY";
            arg.args = QStringList({psdlName,QString::number(typeInfo->dataBaseTypeBytes)+"D"});
        }else{
            QString argType;
            switch (baseType) {
                case DataBaseType_INT:argType= "INT";break;
                case DataBaseType_SHORT:argType= "SHORT";break;
                case DataBaseType_CHAR:argType= "BYTE";break;
                case DataBaseType_LONG:argType= "LONG";break;
                case DataBaseType_UINT:argType= "UINT";break;
                case DataBaseType_USHORT:argType= "USHORT";break;
                case DataBaseType_UCHAR:argType= "UBYTE";break;
                case DataBaseType_ULONG:argType= "ULONG";break;
                case DataBaseType_FLOAT:argType= "FLOAT";break;
                case DataBaseType_DOUBLE:argType= "DOUBLE";break;
                case DataBaseType_ENUM:argType= "INT";break;
                default:{
                    objInfo.endAnalysisFunction();
                    return 0;
                };
            }
            arg.nodeType = "DATA";
            arg.args = QStringList({argType,psdlName});
        }
        funArgAtt.subNodes.append(arg);

    }

    if(isStdcall){
        PsdlIR_Compiler::MiddleNode ret;
        ret.nodeType = "ARRAY";
        ret.args = QStringList({"__valist__","0D"});
        funArgAtt.subNodes.append(ret);
        objInfo.defineValist();
    }
    funDefNode.atts.append(funArgAtt);
    QList<PsdlIR_Compiler::MiddleNode> bodyAtt,varsAtt;
    if(analysisFunOrder(functionIndex,funcIsHaveReturnValue,codeDef,bodyAtt,varsAtt,objInfo)==0){
        objInfo.endAnalysisFunction();
        return 0;
    }
    bool isHaveGotoError = 0;
    for(int i = 0;i<objInfo.jumpMarks.length();i++){
        QString name = objInfo.jumpMarks[i].markName;
        if(!objInfo.defMarks.contains(name)){
            QString srcPath = objInfo.jumpMarks[i].srcPath;
            int line = objInfo.jumpMarks[i].line,col = objInfo.jumpMarks[i].col;
            objInfo.appendPrompt("GOTO语句跳转的标记点未定义",srcPath,line,col);
            isHaveGotoError = 1;
        }
    }
    if(isHaveGotoError){
        objInfo.endAnalysisFunction();
        return 0;
    }

    PsdlIR_Compiler::MiddleNode_Att funVarsAtt,funBodyAtt;
    funVarsAtt.attName = "VARS";
    funVarsAtt.subNodes = varsAtt;
    funBodyAtt.attName = "BODY";
    funBodyAtt.subNodes = bodyAtt;

    funDefNode.atts.append(funVarsAtt);
    funDefNode.atts.append(funBodyAtt);
    funDefNode.atts.append(extendAtts);
    objInfo.endAnalysisFunction();
    objInfo.rootMiddleNode.getAttFromName("BODY")->subNodes.append(funDefNode);

    return 1;
}

PsdlIR_Compiler::MiddleNode analysisGrammatical_C(SrcObjectInfo &objInfo,
                         PhraseList & phraseList,bool &status){

    //合并所有相临的字符串token
    PhraseList replaceTokens;
    bool lastIsStr = 0;
    QString strText;
    for(int i = 0;i<phraseList.length();i++){
        if(phraseList[i].PhraseType == Phrase::ConstValue_STRING){
            lastIsStr = 1;
            QString tmp = phraseList[i].text;
            tmp.removeLast();
            tmp.removeFirst();
            strText += tmp;
        }else if(lastIsStr){
            Phrase tmp;
            tmp.PhraseType = Phrase::ConstValue_STRING;
            tmp.text = "\""+strText+"\"";

            replaceTokens.append(tmp);
            lastIsStr = 0;
            strText.clear();
        }

        if(phraseList[i].PhraseType != Phrase::ConstValue_STRING){
            replaceTokens.append(phraseList[i]);
        }
    }
    phraseList = replaceTokens;


    phraseList = phraseList.toTreeTokens();//将token转为树状形式


    bool isHaveError = 0;
    status = 0;
    //检查标识符是否符合要求(预编译过程中标识符只要不是符号就行，而c语言代码中标识符有更多限制)
    for(int i = 0;i<phraseList.length();i++){
        if(phraseList[i].isIdentifier()){
            if(!judgeIsIdentifying(phraseList[i].text)){//检查是否符合c代码的标识符要求
                isHaveError = 1;
                objInfo.appendPrompt("定义的标识符不符合当前C语言的标识符语法要求",phraseList[i].srcPath,
                                        phraseList[i].line,phraseList[i].col);

            }
        }
    }


    if(isHaveError){
        return PsdlIR_Compiler::MiddleNode();
    }

   //全局定义/声明语句的解析
   enum{
       Indeter,//不确定
       FunctionDefine,//是函数定义语句
       TypeDefine,//新数据类型定义语句
       VarFunStatement//全局变量/函数声明语句,结构体/枚举/共用体声明语句
   }thisSentenseType;

   while(phraseList.length()!=0){
       PhraseList sentence;
       if(phraseList.getDelimiterNum(";","{}")!=0){
           //可能是类型定义语句或声明语句
           thisSentenseType = Indeter;
       }else{
           int tmp = phraseList.getDelimiterNum("()","{}");
           if(tmp==0){
               //可能是类型定义语句或声明语句
               thisSentenseType = Indeter;
           }else{
               //是函数定义语句
               thisSentenseType = FunctionDefine;
           }
       }

       if(thisSentenseType==Indeter){
           //判断是类型定义语句还是声明语句
           int tmp = phraseList.getKeyWordNum("typedef",";");
           if(tmp==0){
               //声明语句
               thisSentenseType = VarFunStatement;
           }else if(tmp==1){
               //类型定义语句
               thisSentenseType = TypeDefine;
           }else{
               isHaveError = 1;
               objInfo.appendPrompt("语法有误",phraseList[0].srcPath,phraseList[0].line,phraseList[0].col);
               continue;
           }
       }
       if(thisSentenseType == FunctionDefine){
           sentence = phraseList.getPhraseLeftSec("{}");
       }else if(thisSentenseType==VarFunStatement || thisSentenseType==TypeDefine){
           sentence = phraseList.getPhraseLeftSec(";");
       }else{
           isHaveError = 1;
           objInfo.appendPrompt("语法有误",phraseList[0].srcPath,phraseList[0].line,phraseList[0].col);
           continue;
       }
       if(thisSentenseType==TypeDefine){
           //自定义一个新的数据类型
           if(analysisTypeDefineSentence(sentence,objInfo)==0){
               isHaveError = 1;
               continue;
           }
       }else if(thisSentenseType==VarFunStatement){
           //声明一个全局变量或函数
           if(sentence.getKeyWordNum("extern")){
               if(analysisExternGlobalVarOrFun(sentence,objInfo)==0){
                   isHaveError = 1;
                   continue;
               }
           }else{
               if(analysisStatementGlobalVarOrFun(sentence,objInfo)==0){
                   isHaveError = 1;
                   continue;
               }
           }
       }else if(thisSentenseType==FunctionDefine){
           if(analysisDefineFunction(sentence,objInfo)==0){
               isHaveError = 1;
               continue;
           }
       }else{
           isHaveError = 1;
           objInfo.appendPrompt("语法有误",phraseList[0].srcPath,phraseList[0].line,phraseList[0].col);
           continue;
       }
   }
   status = !isHaveError;
   return objInfo.rootMiddleNode;
}
}}
QT_END_NAMESPACE

