#ifndef ANALYSISFUNCTION_H
#define ANALYSISFUNCTION_H
#include <Middle_typedef.h>
#include <constValueAnalysis.h>

//生成的结果= {name}num 通过allGotoLabel生成一个num数字序号,防止出现重复标记名
QString generateJumpLabel(QString name,QStringList &allGotoLabel){
    int i = 0;
    while(allGotoLabel.contains(name+QString::number(i))){
        i++;
    }
    return name+QString::number(i);
}

//扁平化流程块
void flatProcessBlock(MiddleNode_Att &codes,QStringList &allGotoLabel,bool&status){
    /*
     * PROCESS(比较运算指令,a,b).JUDGE{//可不写
     *    //判断是执行if还是else块的代码,通过设置决定变量的值来实现
     * }.IF{
     *    //比较值!=0 是执行的代码
     * }.ELSE{
     *    //比较值==0 是执行的代码
     * }
     *
     * 转换为:
     *
     *if/else块同时存在
     * {
     *      //判断是执行if还是else块的代码
     *      比较运算指令(a,b);
     *      JNE([this.*if_else*num])
     *      //比较值!=0 是执行的代码
     *      JMP([this.*if_end*num]);
     *      LABEL(*if_else*num);
     *      //比较值==0 是执行的代码
     *      LABEL(*if_end*num);
     * }
     *
     *只存在else块
     * {
     *      //判断是执行if还是else块的代码
     *      比较运算指令(a,b);
     *      JNE([this.*if_else*num]);
     *      JMP([this.*if_end*num]);
     *      LABEL(*if_else*num);
     *      //比较值==0 是执行的代码
     *      LABEL(*if_end*num);
     * }
     *
     * 只存在if块
     * {
     *      //判断是执行if还是else块的代码
     *      比较运算指令(a,b);
     *      JNE([this.*if_end*num])
     *      //比较值!=0 执行的代码
     *      LABEL(*if_end*num);
     * }
     *
     *
     */

    for(int i = 0;i<codes.subNodes.length();i++){

        MiddleNode &thisNode = codes.subNodes[i];
        if(thisNode.nodeType=="PROCESS"){
            if(thisNode.args.length()!=3 || thisNode.areThereOnlyTheseAtt({"JUDGE","IF","ELSE"})){
                status=false;
                return;
            }
            QList<MiddleNode> flatCodes;

            MiddleNode_Att* judegCodes = thisNode.getAttFromName("JUDGE");
            MiddleNode_Att* ifCodes = thisNode.getAttFromName("IF");
            MiddleNode_Att* elseCodes = thisNode.getAttFromName("ELSE");
            if(ifCodes==NULL && elseCodes==NULL){
                status = false;
                return;
            }

            if(judegCodes!=0){
                flatProcessBlock(*judegCodes,allGotoLabel,status);
                if(!status)return;

                flatCodes.append(judegCodes->subNodes);
            }

            MiddleNode cmpCode;
            cmpCode.nodeType=thisNode.args[0];
            cmpCode.args.append({thisNode.args[1],thisNode.args[2]});
            flatCodes.append(cmpCode);

            MiddleNode ifgotoCode;
            ifgotoCode.nodeType="JNE";
            QString gotoLabel = generateJumpLabel((ifCodes==0?"*if_end*":"*if_else*"),allGotoLabel);
            allGotoLabel.append(gotoLabel);
            ifgotoCode.args.append("[this."+gotoLabel+"]");
            flatCodes.append(ifgotoCode);

            MiddleNode ifgotoLabel;
            ifgotoLabel.nodeType="LABEL";
            ifgotoLabel.args.append(gotoLabel);

            if(ifCodes!=0){
                flatProcessBlock(*ifCodes,allGotoLabel,status);
                if(!status)return;
                flatCodes.append(ifCodes->subNodes);
            }

            if(elseCodes!=0){
                MiddleNode gotoCode;
                gotoCode.nodeType="JMP";
                gotoLabel = generateJumpLabel("*if_end*",allGotoLabel);
                allGotoLabel.append(gotoLabel);
                gotoCode.args.append("[this."+gotoLabel+"]");
                flatCodes.append(gotoCode);
                flatCodes.append(ifgotoLabel);
                flatProcessBlock(*elseCodes,allGotoLabel,status);
                if(!status)return;
                flatCodes.append(elseCodes->subNodes);
                ifgotoLabel.args[0] = gotoLabel;
                flatCodes.append(ifgotoLabel);
            }else{
                flatCodes.append(ifgotoLabel);
            }


            codes.subNodes.removeAt(i);
            for(int j = 0;j<flatCodes.length();j++){
                codes.subNodes.insert(i+j,flatCodes[j]);
            }

            i+=flatCodes.length()-1;
        }
    }
    status = 1;
}

//判断是否存在一个局部变量
bool isHaveVar(QString varName,QList<VarUse> &vars){
    for(int i = 0;i<vars.length();i++){
        if(vars[i].varName == varName){
            return true;
        }
    }
    return false;
}

//获取一个参数的指令参数
//指令参数
class OrderArg{
public:
    bool isValid = 0;//是否有效.如果无效说明参数解析失败了
    VarUse*useVar=NULL;//该参数使用的变量
    //如果useVar==NULL&&isValid说明该参数是一个常量
    QString constValue;//常量值
    QString type;//常量值数据类型
    uint size;//常量值的有效Bit数

    //获取参数数据类型
    QString getType(){
        if(useVar!=NULL){
            return useVar->type;
        }
        return type;
    }
};
OrderArg getOrderArg(QString argText,QStringList &allAddressLabel,QList<VarUse> &varUseInfo){
    OrderArg arg;
    bool isConstValue;
    arg.constValue = analysisConstValue(argText,allAddressLabel,isConstValue,arg.type,arg.size);
    if(isConstValue==1){
        //是一个常量立即数
        arg.useVar = NULL;
        arg.isValid = 1;
        return arg;
    }
    argText = "l_"+argText+"_l";


    for(int i = 0;i<varUseInfo.length();i++){
        if(varUseInfo[i].varName == argText){
            //是一个变量
            arg.useVar = &varUseInfo[i];
            arg.isValid = 1;
            arg.useVar->isUse = 1;
            return arg;
        }
    }
    arg.isValid = 0;
    return arg;
}


//尝试申请使用一个临时变量
QString getTempVar(QList<VarUse> &varUseInfo){
    int tempVarIndex = 0;
    QString tempVarName;
    while(1){
        tempVarName = "__temp"+QString::number(tempVarIndex)+"__";
        for(int i = 0;i<varUseInfo.length();i++){
            if(varUseInfo[i].varName == tempVarName){
                tempVarIndex++;
                goto pk;
            }
        }
        break;
        pk:
        continue;
    }
    VarUse tempVar;
    tempVar.isArg = 0;
    tempVar.isUse = 1;
    tempVar.varName = tempVarName;
    tempVar.type = "UINT";
    varUseInfo.append(tempVar);
    return tempVarName;
}

//判断一个指令的立即数是否超过了该指令允许的立即数位数,如果超过了就选用临时变量的方式去赋值
//返回是否需要
//tempVarIndex:使用第几个临时变量(会自动由编译器在堆栈中创建名为 *temp*tempVarIndex 的变量)
//size:当前指令支持的变量立即数位数
bool isUseTempVar(uint size,OrderArg&arg,QList<VarUse> &varUseInfo,QString&tempVarName){
      if(arg.size<=size){
          return false;
      }

      tempVarName = getTempVar(varUseInfo);
      return true;
}



//生成函数入口汇编指令
QList<MiddleNode> generateEntryCode(MiddleNode &node,//函数描述节点
                                    bool isHaveCallFun,//函数中是否有调用另一个函数
                                    QList<VarUse> &allFunctionVaribleInfo,//函数中所有定义的变量/数组信息
                                    QStringList pushRegNames,//函数中使用到的所有寄存器
                                    uint &varUseStackSize,//返回函数临时变量使用的堆栈字节数
                                    uint &argUseStackSize,//返回函数参数使用的堆栈字节数
                                    bool&status){
    status = 0;
    uint stackVarSize = 0;//函数局部变量占用的栈帧大小
    QList<MiddleNode> mallocStackVar;//函数开头分配堆栈临时变量的指令集合
    QList<MiddleNode> pushRegNodes;//入栈保存物理寄存器数据的汇编指令



    //解析出所有的局部变量要用到多少字节数,以及变量的栈顶偏移
    for(int i = allFunctionVaribleInfo.length()-1;i>=0;i--){
        if(allFunctionVaribleInfo[i].isArg || !allFunctionVaribleInfo[i].isUseStack)continue;
        allFunctionVaribleInfo[i].offset = stackVarSize;
        if(allFunctionVaribleInfo[i].type == "ARRAY"){
            stackVarSize += allFunctionVaribleInfo[i].arrSize;
        }else{
            stackVarSize += getVarTypeSize(allFunctionVaribleInfo[i].type);
        }
    }

    //创建局部变量存储区域
    MiddleNode mallocVar;
    if(stackVarSize!=0){
        if(getNumDigit(stackVarSize)<=16){
            mallocVar.nodeType="USUB";
            mallocVar.args = QStringList({"SP",QString::number(stackVarSize)+"D"});
            mallocStackVar.prepend(mallocVar);
        }else{
            mallocVar.nodeType="USUB";
            mallocVar.args = QStringList({"SP","R1"});
            mallocStackVar.prepend(mallocVar);
            mallocVar.nodeType="INIT";
            mallocVar.args = QStringList({"R1",QString::number(stackVarSize)+"D"});
            mallocStackVar.prepend(mallocVar);
            if(!pushRegNames.contains("R1")){
                pushRegNames.append("R1");
            }
        }
    }
    varUseStackSize = stackVarSize;

    //统计出函数中有多少个变量/参数被使用到了
    //通用运算寄存器入栈保存
    MiddleNode pushReg;
    pushReg.nodeType="PUSH";
    pushReg.args= QStringList({"DWORD",""});
    for(int i = 0;i<pushRegNames.length();i++){
        pushReg.args[1] = pushRegNames[i];
        pushRegNodes.append(pushReg);
        stackVarSize+=4;
    }

    bool isKernelInter = node.getAttFromName("INTERRUPT")!=0;//当前函数是内核中断处理函数
    bool isDriverCall = node.getAttFromName("DRIVERCALL")!=0;//当前函数是驱动程序调用内核功能的处理函数
    bool isAppCall = node.getAttFromName("APPCALL")!=0;//当前函数是应用程序调用内核功能的处理函数

    //如果同时设置了多种模式，报错退出
    bool isInterrupt = isKernelInter;
    if(isInterrupt && isDriverCall){
        return mallocStackVar;
    }else{
        isInterrupt = isInterrupt||isDriverCall;
    }
    if(isInterrupt && isAppCall){
        return mallocStackVar;
    }else{
        isInterrupt = isInterrupt||isAppCall;
    }

    //如果是中断处理函数,将ipc入栈保存
    if(isInterrupt){
        pushReg.args[1] = "IPC";
        pushRegNodes.append(pushReg);
        stackVarSize+=4;
        pushRegNames.append(pushReg.args[1]);
    }
    //如果函数中调用了其它函数,将tpc入栈保存
    if(isHaveCallFun){
        pushReg.args[1] = "TPC";
        pushRegNodes.append(pushReg);
        stackVarSize+=4;
        pushRegNames.append(pushReg.args[1]);
    }


    //解析出函数的参数定义
    argUseStackSize = 0;
    for(int i = 0;i<allFunctionVaribleInfo.length();i++){
        if(!allFunctionVaribleInfo[i].isArg || !allFunctionVaribleInfo[i].isUseStack)continue;
        allFunctionVaribleInfo[i].offset = stackVarSize;
        if(allFunctionVaribleInfo[i].type == "ARRAY"){
            stackVarSize += allFunctionVaribleInfo[i].arrSize;
            argUseStackSize += allFunctionVaribleInfo[i].arrSize;
        }else{
            int bytes = getVarTypeSize(allFunctionVaribleInfo[i].type);
            stackVarSize += bytes;
            argUseStackSize += bytes;
        }
    }
    status = 1;
    return pushRegNodes+mallocStackVar;
}

//生成函数退出指令
QList<MiddleNode> generateReturnCode(MiddleNode &node,
                                     uint varUseStackSize,
                                     uint argUseStackSize,
                                     QStringList &pushRegNames,
                                     bool&status){
    status = 0;
     QList<MiddleNode> retCodes;
    bool isKernelInter = node.getAttFromName("INTERRUPT")!=0;//当前函数是内核中断处理函数
    bool isDriverCall = node.getAttFromName("DRIVERCALL")!=0;//当前函数是驱动程序调用内核功能的处理函数
    bool isAppCall = node.getAttFromName("APPCALL")!=0;//当前函数是应用程序调用内核功能的处理函数
    bool isINTERRUPT = isKernelInter||isDriverCall||isAppCall;

    bool isStdCall = node.getAttFromName("STDCALL")!=0;//当前函数是否是stdcall调用方式

    MiddleNode tmp;

    //如果是中断处理函数,退出前先关闭中断响应,进入临界模式
    if(isINTERRUPT){
        tmp.nodeType = "DI";
        tmp.args = QStringList();
        retCodes.append(tmp);
    }

    //删除所有局部变量 sp+=局部变量占用的总字节数
    if(varUseStackSize!=0){
        uint digit = getNumDigit(varUseStackSize);
        if(digit>12){
            tmp.nodeType = "INIT";
            tmp.args = QStringList({"R1",QString::number(varUseStackSize)+"D"});
            retCodes.append(tmp);
            tmp.nodeType = "UADD";
            tmp.args = QStringList({"SP","R1"});
            retCodes.append(tmp);
        }else{
            tmp.nodeType = "UADD";
            tmp.args = QStringList({"SP",QString::number(varUseStackSize)+"D"});
            retCodes.append(tmp);
        }
    }

    //将暂存在栈中的寄存器数据出栈
    for(int i = pushRegNames.length()-1;i>=0;i--){
        tmp.nodeType = "POP";
        tmp.args = QStringList({"DWORD",pushRegNames[i]});
        retCodes.append(tmp);
    }

    //如果不是stdcall调用方式,清除函数参数的占用
    if(argUseStackSize!=0 && !isStdCall){
        uint digit = getNumDigit(argUseStackSize);
        if(digit>12){
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD","R1"});
            retCodes.append(tmp);

            tmp.nodeType = "INIT";
            tmp.args = QStringList({"R1",QString::number(argUseStackSize+4)+"D"});
            retCodes.append(tmp);

            tmp.nodeType = "UADD";
            tmp.args = QStringList({"SP","R1"});
            retCodes.append(tmp);

            tmp.nodeType = "USUB";
            tmp.args = QStringList({"R1","SP","R1"});
            retCodes.append(tmp);

            tmp.nodeType = "RD";
            tmp.args = QStringList({"DWORD","R1","R1"});
            retCodes.append(tmp);
        }else{
            tmp.nodeType = "UADD";
            tmp.args = QStringList({"SP",QString::number(argUseStackSize)+"D"});
            retCodes.append(tmp);
        }
    }
    //退出函数指令
    if(isKernelInter){
        tmp.nodeType = "EIT";
    }else if(isDriverCall){
        tmp.nodeType = "VIT";
    }else if(isAppCall){
        tmp.nodeType = "PVT";
    }else{
        tmp.nodeType = "RT";
    }
    tmp.args = QStringList();
    retCodes.append(tmp);
    status = 1;
    return retCodes;
}


//将1个64位变量的名称替换为64位变量拆分成的2个32位变量的名称
//返回String[2],[0]为low32位,[1]为high32位
QStringList bit64VarTo32VarName(QString varName){
    return QStringList({"__"+varName+"_low__","__"+varName+"_high__"});;
}


//生成调用64位操作函数的指令表
QList<MiddleNode> generateCall64bitOperFunCode(QString funName,
                                               QStringList argVarNames,//最多2个参数
                                               QString returnPtrName,
                                               QString constValue,
                                               uint constValueSize,
                                               bool isHaveConstValue,
                                               bool arg1Is64,//第一个参数是否是64位
                                               bool arg2Is64,//第二个参数是否是64位
                                               bool retIs64,
                                               QList<VarUse> &varUseInfo){
    QList<MiddleNode> codeList;
    MiddleNode tmp;
    if(isHaveConstValue){
        if(argVarNames.length()==0 ? arg1Is64 : arg2Is64){
            tmp.nodeType = "INIT64";

            QString tmp0 = getTempVar(varUseInfo);
            QString tmp1 = getTempVar(varUseInfo);
            tmp.args = QStringList({tmp0,tmp1,constValue});
            codeList.append(tmp);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",tmp0});
            codeList.append(tmp);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",tmp1});
            codeList.append(tmp);
        }else{
            if(constValueSize>16){
                tmp.nodeType = "INIT";
                QString tmp0 = getTempVar(varUseInfo);
                tmp.args = QStringList({tmp0,constValue});
                codeList.append(tmp);
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",tmp0});
                codeList.append(tmp);
            }else{
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",constValue});
                codeList.append(tmp);
            }

        }
    }


    for(int i = argVarNames.length()-1;i>=0;i--){
        if(i==1 ? arg2Is64 : arg1Is64){

            QStringList name = bit64VarTo32VarName(argVarNames[i]);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",name[1]});
            codeList.append(tmp);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",name[0]});
            codeList.append(tmp);
        }else{
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",argVarNames[i]});
            codeList.append(tmp);
        }
    }

    if(retIs64){
        QStringList name = bit64VarTo32VarName(returnPtrName);
        tmp.nodeType = "ADDRESS";
        QString tmp0 = getTempVar(varUseInfo);
        tmp.args = QStringList({tmp0,name[0],"0d"});
        codeList.append(tmp);
        tmp.nodeType = "PUSH";
        tmp.args = QStringList({"DWORD",tmp0});
        codeList.append(tmp);

        tmp.nodeType = "ADDRESS";
        tmp.args = QStringList({tmp0,name[1],"0d"});
        codeList.append(tmp);
        tmp.nodeType = "PUSH";
        tmp.args = QStringList({"DWORD",tmp0});
        codeList.append(tmp);
    }else{
        tmp.nodeType = "ADDRESS";
        QString tmp0 = getTempVar(varUseInfo);
        tmp.args = QStringList({tmp0,returnPtrName,"0d"});
        codeList.append(tmp);
        tmp.nodeType = "PUSH";
        tmp.args = QStringList({"DWORD",tmp0});
        codeList.append(tmp);
    }



    //设置函数跳转地址
    tmp.nodeType = "INIT";
    tmp.args = QStringList({"TPC","["+funName+"]"});
    codeList.append(tmp);

    //跳转入函数
    tmp.nodeType = "CALL";
    tmp.args = QStringList();
    codeList.append(tmp);
    return codeList;
}


enum{
    MOV,//可直接赋值
    ITF,//有符号整型转浮点
    BTD_ITF,//先将byte转Int后再转浮点
    WTD_ITF,//先将short转Int后再转浮点
    UITF,//无符号整型转浮点
    BTD,//byte转int
    WTD,//short转int
    FTI,//浮点转整型
    DWORD_T_QWORD,//1-4字节的数据类型转为8字节数据类型
    QWORD_T_DWORD,//8字节数据类型转为1-4字节的数据类型
    QWORD_T_QWORD//8字节数据类型间的转换
};//获取数据转换模式
uint getTypeTransMode(QString Atype,QString Btype,bool *sucess =NULL){
    if(sucess!=NULL){
        *sucess = true;
    }
    if((Atype=="UBYTE"||Atype=="USHORT"||Atype=="UINT")&&
       (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT"||
        Btype=="BYTE"||Btype=="SHORT"||Btype=="INT")){
        return MOV;

    }else if((Atype=="UBYTE"||Atype=="USHORT"||Atype=="UINT")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_DWORD;
    }else if((Atype=="UBYTE"||Atype=="USHORT"||Atype=="UINT")&&
             (Btype=="FLOAT")){
        return FTI;
    }else if((Atype=="UBYTE"||Atype=="USHORT"||Atype=="UINT")&&
             (Btype=="DOUBLE")){
        return QWORD_T_DWORD;
    }

    else if((Atype=="BYTE")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT"||
             Btype=="BYTE"||Btype=="SHORT"||Btype=="INT")){
        return MOV;
    }else if((Atype=="BYTE")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_DWORD;
    }else if((Atype=="BYTE")&&
             (Btype=="FLOAT")){
        return FTI;
    }else if((Atype=="BYTE")&&
             (Btype=="DOUBLE")){
        return QWORD_T_DWORD;
    }

    else if((Atype=="SHORT")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT"||
             Btype=="SHORT"||Btype=="INT")){
        return MOV;
    }else if(Atype=="SHORT"&&Btype=="BYTE"){
        return BTD;

    }else if((Atype=="SHORT")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_DWORD;
    }else if((Atype=="SHORT")&&
             (Btype=="FLOAT")){
        return FTI;
    }else if((Atype=="SHORT")&&
             (Btype=="DOUBLE")){
        return QWORD_T_DWORD;
    }

    else if((Atype=="INT")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT"||
             Btype=="INT")){
        return MOV;
    }else if(Atype=="INT"&&Btype=="BYTE"){
        return BTD;
    }else if(Atype=="INT"&&Btype=="SHORT"){
        return WTD;

    }else if((Atype=="INT")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_DWORD;
    }else if((Atype=="INT")&&
             (Btype=="FLOAT")){
        return FTI;
    }else if((Atype=="INT")&&
             (Btype=="DOUBLE")){
        return QWORD_T_DWORD;
    }

    else if((Atype=="LONG")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT")){
        return DWORD_T_QWORD;
    }else if(Atype=="LONG"&&Btype=="BYTE"){
        return DWORD_T_QWORD;
    }else if(Atype=="LONG"&&Btype=="SHORT"){
        return DWORD_T_QWORD;
    }else if(Atype=="LONG"&&Btype=="INT"){
        return DWORD_T_QWORD;
    }else if((Atype=="LONG")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_QWORD;
    }else if((Atype=="LONG")&&
             (Btype=="FLOAT")){
        return DWORD_T_QWORD;
    }else if((Atype=="LONG")&&
             (Btype=="DOUBLE")){
        return QWORD_T_QWORD;
    }

    else if((Atype=="ULONG")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT"||
             Btype=="BYTE"||Btype=="SHORT"||Btype=="INT")){
        return DWORD_T_QWORD;
    }else if((Atype=="ULONG")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_QWORD;
    }else if((Atype=="ULONG")&&
             (Btype=="FLOAT")){
        return DWORD_T_QWORD;
    }else if((Atype=="ULONG")&&
             (Btype=="DOUBLE")){
        return QWORD_T_QWORD;
    }

    else if((Atype=="FLOAT")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT")){
        return UITF;
    }else if(Atype=="FLOAT"&&Btype=="BYTE"){
        return BTD_ITF;
    }else if(Atype=="FLOAT"&&Btype=="SHORT"){
        return WTD_ITF;
    }else if(Atype=="FLOAT"&&Btype=="INT"){
        return ITF;
    }else if((Atype=="FLOAT")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_DWORD;
    }else if((Atype=="FLOAT")&&
             (Btype=="FLOAT")){
        return MOV;
    }else if((Atype=="FLOAT")&&
             (Btype=="DOUBLE")){
        return QWORD_T_DWORD;
    }

    else if((Atype=="DOUBLE")&&
            (Btype=="UBYTE"||Btype=="USHORT"||Btype=="UINT")){
        return DWORD_T_QWORD;
    }else if(Atype=="DOUBLE"&&Btype=="BYTE"){
        return DWORD_T_QWORD;
    }else if(Atype=="DOUBLE"&&Btype=="SHORT"){
        return DWORD_T_QWORD;
    }else if(Atype=="DOUBLE"&&Btype=="INT"){
        return DWORD_T_QWORD;
    }else if((Atype=="DOUBLE")&&
             (Btype=="ULONG"||Btype=="LONG")){
        return QWORD_T_QWORD;
    }else if((Atype=="DOUBLE")&&
             (Btype=="FLOAT")){
        return DWORD_T_QWORD;
    }else if(Atype=="DOUBLE"&&Btype=="DOUBLE"){
        return QWORD_T_QWORD;
    }else{
        if(sucess!=NULL){
            *sucess = false;
        }
        return 0;
    }
}


//编译为低级语言指令
void compileToLower(MiddleNode &funNode,//函数定义节点
                    QStringList &allAddressLabel,//当前全局范围内的地址标记
                    QStringList &operFun,//返回使用到的操作函数
                    QList<MiddleNode> &bodyCodeList,//返回编译生成的函数主体逻辑功能指令
                    bool&status){
    status=0;
    QStringList allGotoLabel;//所有函数内跳转标记名
    QStringList allGotoCodeJmpLabels;//所有函数内jmp指令要跳转到的标记
    MiddleNode_Att * codes = funNode.getAttFromName("BODY");
    MiddleNode_Att * vars = funNode.getAttFromName("VARS");
    MiddleNode_Att * args = funNode.getAttFromName("ARGS");
    if(codes==NULL)return;
    if(vars==NULL){
        MiddleNode_Att tmp;
        tmp.attName = "VARS";
        funNode.atts.append(tmp);
        vars = funNode.getAttFromName("VARS");
    }
    if(args==NULL){
        MiddleNode_Att tmp;
        tmp.attName = "ARGS";
        funNode.atts.append(tmp);
        args = funNode.getAttFromName("ARGS");
    }

    QStringList allVarNameTmp;//所有局部变量的暂存
    QList<VarUse> varUseInfo;//函数变量在代码中的使用情况
    for(int i = 0;i<vars->subNodes.length();i++){
        MiddleNode& argNode = vars->subNodes[i];
        if(argNode.atts.length()!=0 || argNode.args.length()!=2){
            return;
        }

        VarUse defineInfo;
        if(argNode.nodeType=="DATA"){
            QString &type = argNode.args[0];
            type = type.toUpper();
            type = pointerTypeConvert(type);
            QString &name = argNode.args[1];
            if(!nameIsConform(name) || !varTypeIsConform(type) || allVarNameTmp.contains(name)){
                return;
            }
            defineInfo.type = type;
            defineInfo.varName = name;
        }else if(argNode.nodeType=="ARRAY"){
            QString type = "ARRAY";
            type = pointerTypeConvert(type);
            QString &name = argNode.args[0];
            if(!nameIsConform(name) || allVarNameTmp.contains(name)){
                return;
            }
            defineInfo.type = type;
            defineInfo.varName = name;
            bool suceess;
            defineInfo.arrSize = analysisStdConst(argNode.args[1],&suceess);
            if(!suceess)return;
        }else{
            return;
        }
        defineInfo.isArg = 0;
        varUseInfo.append(defineInfo);
    }
    for(int i = 0;i<args->subNodes.length();i++){
        MiddleNode& argNode = args->subNodes[i];
        if(argNode.atts.length()!=0){
            return;
        }

        VarUse defineInfo;
        if(argNode.nodeType=="DATA"){
            if(argNode.args.length()!=2)return;
            QString &type = argNode.args[0];
            type = type.toUpper();
            type = pointerTypeConvert(type);
            QString &name = argNode.args[1];
            if(!nameIsConform(name) || !varTypeIsConform(type) || allVarNameTmp.contains(name)){
                return;
            }

            defineInfo.type = type;
            defineInfo.varName = name;
        }else if(argNode.nodeType=="ARRAY"){
            if(argNode.args.length()!=2)return;
            QString type = "ARRAY";
            type = pointerTypeConvert(type);
            QString &name = argNode.args[0];
            if(!nameIsConform(name) || allVarNameTmp.contains(name)){
                return;
            }
            defineInfo.type = type;
            defineInfo.varName = name;
            argNode.args[1];
            bool suceess;
            defineInfo.arrSize = analysisStdConst(argNode.args[1],&suceess);
            if(!suceess)return;
        }else{
            return;
        }
        defineInfo.isArg = 1;
        varUseInfo.append(defineInfo);
    }

    for(int i = 0;i<varUseInfo.length();i++){
        varUseInfo[i].varName = "l_"+varUseInfo[i].varName+"_l";
    }


    //扁平化流程块
    flatProcessBlock(*codes,allGotoLabel,status);
    if(status==0)return;

    //编译替换为低级指令
    QList<MiddleNode> codeList = codes->subNodes;

    QList<MiddleNode> replseList;

    bool isHaveCallFun = 0;//当前函数中是否有调用其它函数
    bool isHaveReturnHandle = 0;//当前函数是否有函数退出处理


    allGotoLabel.append("*return*");
    //将函数调用/函数退出/跳转等无变量操作指令进行替换
    for(int i = 0;i<codeList.length();i++){
        status = 0;
        QString nodeType = codeList[i].nodeType;
        MiddleNode tmp;
        if(nodeType=="RETURN"){//函数返回
            tmp.nodeType = "JMP";
            tmp.args.append("[this.*return*]");
            replseList.append(tmp);
            allGotoCodeJmpLabels.append("*return*");
        }else if(nodeType=="GOTO"){//无条件跳转
            if(codeList[i].args.length()!=1)return;
            tmp.nodeType = "JMP";
            tmp.args.append("[this."+codeList[i].args[0]+"]");
            allGotoCodeJmpLabels.append(codeList[i].args[0]);
            replseList.append(tmp);
        }else if(nodeType=="CALL"){//函数调用
            isHaveCallFun = 1;
            if(codeList[i].args.length()<1 && !codeList[i].areThereOnlyTheseAtt({"STDCALL"}))return;

            //传入函数参数(从右到左依次入栈)
            for(int j = codeList[i].args.length()-1;j>0;j--){

                tmp.nodeType = "PUSH";
                tmp.args = QStringList({codeList[i].args[j]});
                replseList.append(tmp);
            }

            bool stdCall = codeList[i].getAttFromName("STDCALL")!=NULL;
            //设置函数跳转地址
            tmp.nodeType = stdCall ? "STDCALL" : "CALL";
            tmp.args = QStringList({codeList[i].args[0]});
            replseList.append(tmp);

            //函数返回后清理入栈参数
            //如果不写STDCALL属性,默认为cdecl.无需调用者清栈
            if(stdCall && codeList[i].args.length()>1){
                //如果是stdcall调用约定,要调用者进行清栈
                tmp.nodeType = "CLEAR";
                tmp.args = codeList[i].args.mid(1,codeList[i].args.length()-1);
                replseList.append(tmp);
            }
        }else if(nodeType=="LABEL"){//跳转点定义
            if(codeList[i].args[0]  ==  "*return*"){
                return;
            }
            if(!allGotoLabel.contains(codeList[i].args[0])){//重复定义跳转点
                allGotoLabel.append(codeList[i].args[0]);
            }


            replseList.append(codeList[i]);
        }else{
            replseList.append(codeList[i]);
        }
    }



    //检查跳转标记是否有重复的,有就退出
    QStringList checkRepLabelTmp;
    for(int i = 0;i<allGotoLabel.length();i++){
        if(checkRepLabelTmp.contains(allGotoLabel[i])){
            return;
        }
        checkRepLabelTmp.append(allGotoLabel[i]);
    }
    //检查goto跳转的标记是否都存在,不存在则退出
    for(int i = 0;i<allGotoCodeJmpLabels.length();i++){
        if(!allGotoLabel.contains(allGotoCodeJmpLabels[i])){
            return;
        }
    }

    MiddleNode retLabelCodeNode;
    retLabelCodeNode.nodeType = "LABEL";
    retLabelCodeNode.args = QStringList({"*return*"});
    replseList.append(retLabelCodeNode);
    codeList.clear();

    //优化: 去除不可达代码
    removeUnreachableCode(replseList,isHaveReturnHandle);

    //将有变量操作的指令进行替换
    for(int i = 0;i<replseList.length();i++){
        MiddleNode &thisCode = replseList[i];
        status = 0;
        QString nodeType = thisCode.nodeType;
        MiddleNode tmp;
        if(nodeType=="ADD"||
           nodeType=="SUB"||
           nodeType=="MUL"||
           nodeType=="DIV"){
           //四则运算
           if(thisCode.args.length()!=3){
               return;
           };
           //获取运算参数
           OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
           OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
           OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
           if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
           QString Atype =A.getType();
           QString Btype =B.getType();
           QString Ctype =C.getType();
           if(Atype=="ARRAY"||Btype=="ARRAY"||Ctype=="ARRAY")return;

           uint mov_type = getTypeTransMode(Btype,Ctype,&status);
           if(!status)return;
           status = 0;
           uint ret_mov_type = getTypeTransMode(Atype,Btype,&status);
           if(!status)return;
           status = 0;
           if(mov_type==MOV){
               bool retIsTransType = ret_mov_type!=MOV;

               QString retTempVar;
               if(retIsTransType){
                   retTempVar = getTempVar(varUseInfo);
               }else{
                   retTempVar = A.useVar->varName;
               }


               //整型的四则运算
               if(C.useVar!=NULL){//非立即数运算
                   tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                   tmp.args = QStringList({retTempVar,B.useVar->varName,C.useVar->varName});
                   codeList.append(tmp);
               }else{//立即数运算
                   //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                   QString tempVarName;
                   bool isUseTemp = isUseTempVar(12,C,varUseInfo,tempVarName);
                   if(isUseTemp){
                       //初始化
                       tmp.nodeType = C.size>16 ? "INIT" : "MOV";
                       tmp.args = QStringList({tempVarName,C.constValue});
                       codeList.append(tmp);
                       //运算
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,tempVarName});
                       codeList.append(tmp);
                   }else{
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,C.constValue});
                       codeList.append(tmp);
                   }
               }
               if(!retIsTransType)continue;
               //运算结果转数据类型
               if(  ret_mov_type==ITF ||
                     ret_mov_type==UITF||
                     ret_mov_type==BTD ||
                     ret_mov_type==WTD ||
                     ret_mov_type==FTI){
                   QString movTypeName;
                   if(ret_mov_type==ITF){
                        movTypeName = "ITF";
                   }else if(ret_mov_type==UITF){
                        movTypeName = "UITF";
                   }else if(ret_mov_type==BTD){
                        movTypeName = "BTD";
                   }else if(ret_mov_type==WTD){
                        movTypeName = "WTD";
                   }else if(ret_mov_type==FTI){
                        movTypeName = "FTI";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
               }else if(ret_mov_type==BTD_ITF||ret_mov_type==WTD_ITF){
                   QString movTypeName;
                   if(ret_mov_type==BTD_ITF){
                       movTypeName = "BTD";
                   }else if(ret_mov_type==WTD_ITF){
                        movTypeName = "WTD";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
                   tmp.nodeType = "ITF";
                   tmp.args = QStringList({A.useVar->varName,A.useVar->varName});
                   codeList.append(tmp);
               }else if(ret_mov_type==DWORD_T_QWORD){
                   QString funName ="*"+ B.getType() + "TO" + A.getType()+"*";
                   codeList.append(generateCall64bitOperFunCode(funName,
                                                {retTempVar},
                                                A.useVar->varName,
                                                "",
                                                0,0,0,0,1,varUseInfo));
               }else return;
           }else if(mov_type==ITF ||
              mov_type==UITF||
              mov_type==BTD ||
              mov_type==WTD ||
              mov_type==FTI){
               QString movTypeName;
               if(mov_type==ITF){
                    movTypeName = "ITF";
               }else if(mov_type==UITF){
                    movTypeName = "UITF";
               }else if(mov_type==BTD){
                    movTypeName = "BTD";
               }else if(mov_type==WTD){
                    movTypeName = "WTD";
               }else if(mov_type==FTI){
                    movTypeName = "FTI";
               }

               QString transTemp = getTempVar(varUseInfo);

               bool retIsTransType = ret_mov_type!=MOV;
               QString retTempVar;
               if(retIsTransType){
                   retTempVar = transTemp;
               }else{
                   retTempVar = A.useVar->varName;
               }


               //整型的四则运算
               if(C.useVar!=NULL){//非立即数运算
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({transTemp,C.useVar->varName});
                   codeList.append(tmp);

                   tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                   tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                   codeList.append(tmp);
               }else{//立即数运算
                   //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                   bool isUseTemp = C.size>16;
                   if(isUseTemp){
                       //初始化
                       tmp.nodeType = "INIT";
                       tmp.args = QStringList({transTemp,C.constValue});
                       codeList.append(tmp);
                       //转换类型
                       tmp.nodeType = movTypeName;
                       tmp.args = QStringList({transTemp,transTemp});
                       codeList.append(tmp);
                       //运算
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                       codeList.append(tmp);
                   }else{
                       //转换类型
                       tmp.nodeType = movTypeName;
                       tmp.args = QStringList({transTemp,C.constValue});
                       codeList.append(tmp);
                       //运算
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                       codeList.append(tmp);
                   }
               }

               if(!retIsTransType)continue;
               //运算结果转数据类型
               if(  ret_mov_type==ITF ||
                     ret_mov_type==UITF||
                     ret_mov_type==BTD ||
                     ret_mov_type==WTD ||
                     ret_mov_type==FTI){
                   QString movTypeName;
                   if(ret_mov_type==ITF){
                        movTypeName = "ITF";
                   }else if(ret_mov_type==UITF){
                        movTypeName = "UITF";
                   }else if(ret_mov_type==BTD){
                        movTypeName = "BTD";
                   }else if(ret_mov_type==WTD){
                        movTypeName = "WTD";
                   }else if(ret_mov_type==FTI){
                        movTypeName = "FTI";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
               }else if(ret_mov_type==BTD_ITF||ret_mov_type==WTD_ITF){
                   QString movTypeName;
                   if(ret_mov_type==BTD_ITF){
                       movTypeName = "BTD";
                   }else if(ret_mov_type==WTD_ITF){
                        movTypeName = "WTD";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
                   tmp.nodeType = "ITF";
                   tmp.args = QStringList({A.useVar->varName,A.useVar->varName});
                   codeList.append(tmp);
               }else if(ret_mov_type==DWORD_T_QWORD){
                   QString funName ="*"+ B.getType() + "TO" + A.getType()+"*";
                   codeList.append(generateCall64bitOperFunCode(funName,
                                                {retTempVar},
                                                A.useVar->varName,
                                                "",
                                                0,0,0,0,1,varUseInfo));
               }else return;
           }else if(mov_type==BTD_ITF||mov_type==WTD_ITF){
               QString movTypeName;
               if(mov_type==BTD_ITF){
                   movTypeName = "BTD";
               }else if(mov_type==WTD_ITF){
                    movTypeName = "WTD";
               }

               QString transTemp = getTempVar(varUseInfo);

               bool retIsTransType = ret_mov_type!=MOV;
               QString retTempVar;
               if(retIsTransType){
                   retTempVar = transTemp;
               }else{
                   retTempVar = A.useVar->varName;
               }


               if(C.useVar!=NULL){//非立即数运算
                   //转为4字节有符号整型
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({transTemp,C.useVar->varName});
                   codeList.append(tmp);
                   //转为浮点型
                   tmp.nodeType = "ITF";
                   tmp.args = QStringList({transTemp,transTemp});
                   codeList.append(tmp);
                   //运算
                   tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                   tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                   codeList.append(tmp);
               }else{
                   if(B.size>16){
                       //初始化
                       tmp.nodeType = "INIT";
                       tmp.args = QStringList({transTemp,C.constValue});
                       codeList.append(tmp);
                       //转为4字节有符号整型
                       tmp.nodeType = movTypeName;
                       tmp.args = QStringList({transTemp,transTemp});
                       codeList.append(tmp);
                       //转为浮点型
                       tmp.nodeType = "ITF";
                       tmp.args = QStringList({transTemp,transTemp});
                       codeList.append(tmp);
                       //运算
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                       codeList.append(tmp);
                   }else{
                       //转为4字节有符号整型
                       tmp.nodeType = movTypeName;
                       tmp.args = QStringList({transTemp,C.constValue});
                       codeList.append(tmp);
                       //转为浮点型
                       tmp.nodeType = "ITF";
                       tmp.args = QStringList({transTemp,transTemp});
                       codeList.append(tmp);
                       //运算
                       tmp.nodeType = (Btype[0]=='U' ? "U": (Btype[0]=='F' ? "F": "")) + nodeType;
                       tmp.args = QStringList({retTempVar,B.useVar->varName,transTemp});
                       codeList.append(tmp);
                   }
               }

               if(!retIsTransType)continue;
               //运算结果转数据类型
               if(  ret_mov_type==ITF ||
                     ret_mov_type==UITF||
                     ret_mov_type==BTD ||
                     ret_mov_type==WTD ||
                     ret_mov_type==FTI){
                   QString movTypeName;
                   if(ret_mov_type==ITF){
                        movTypeName = "ITF";
                   }else if(ret_mov_type==UITF){
                        movTypeName = "UITF";
                   }else if(ret_mov_type==BTD){
                        movTypeName = "BTD";
                   }else if(ret_mov_type==WTD){
                        movTypeName = "WTD";
                   }else if(ret_mov_type==FTI){
                        movTypeName = "FTI";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
               }else if(ret_mov_type==BTD_ITF||ret_mov_type==WTD_ITF){
                   QString movTypeName;
                   if(ret_mov_type==BTD_ITF){
                       movTypeName = "BTD";
                   }else if(ret_mov_type==WTD_ITF){
                        movTypeName = "WTD";
                   }
                   tmp.nodeType = movTypeName;
                   tmp.args = QStringList({A.useVar->varName,retTempVar});
                   codeList.append(tmp);
                   tmp.nodeType = "ITF";
                   tmp.args = QStringList({A.useVar->varName,A.useVar->varName});
                   codeList.append(tmp);
               }else if(ret_mov_type==DWORD_T_QWORD){
                   QString funName ="*"+ B.getType() + "TO" + A.getType()+"*";
                   codeList.append(generateCall64bitOperFunCode(funName,
                                                {retTempVar},
                                                A.useVar->varName,
                                                "",
                                                0,0,0,0,1,varUseInfo));
               }else return;
           }else if(mov_type==DWORD_T_QWORD ||
                    mov_type==QWORD_T_DWORD ||
                    mov_type==QWORD_T_QWORD){

               QString funName ="*"+B.getType()+"_"+nodeType+"_"+C.getType()+"_TO_"+A.getType()+"*";
               bool arg1Is64 = getVarTypeSize(Btype)>4;
               bool arg2Is64 = getVarTypeSize(Ctype)>4;
               bool retIs64 = getVarTypeSize(Atype)>4;

               if(!operFun.contains(funName))operFun.append(funName);
               if(C.useVar!=NULL){//非立即数运算
                   //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)
                   codeList.append(generateCall64bitOperFunCode(funName,
                                                {B.useVar->varName,C.useVar->varName},
                                                A.useVar->varName,
                                                C.constValue,
                                                C.size,0,arg1Is64,arg2Is64,retIs64,varUseInfo));
               }else{//立即数运算
                   codeList.append(generateCall64bitOperFunCode(funName,
                                                {B.useVar->varName},
                                                A.useVar->varName,
                                                C.constValue,
                                                C.size,1,arg1Is64,arg2Is64,retIs64,varUseInfo));
               }
           }else return;
        }else if(nodeType=="REM"){
            //取模运算
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(Atype=="DOUBLE" || Atype=="FLOAT" || Atype=="ARRAY")return;
            if(Btype=="DOUBLE" || Btype=="FLOAT" ||Btype=="ARRAY")return;
            if(Ctype=="DOUBLE" || Ctype=="FLOAT" ||Ctype=="ARRAY")return;

            uint mov_type = getTypeTransMode(Btype,Ctype,&status);
            if(!status)return;
            status = 0;

            if(mov_type==MOV){
                bool retIsTransType = Atype=="LONG"||Atype=="ULONG";
                QString retTempVar = getTempVar(varUseInfo);
                //整型的四则运算
                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                    tmp.args = QStringList({retTempVar,B.useVar->varName,C.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算]
                    if(C.size>12){
                        //初始化
                        tmp.nodeType = C.size>16 ? "INIT" : "MOV";
                        tmp.args = QStringList({retTempVar,C.constValue});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                        tmp.args = QStringList({retTempVar,B.useVar->varName,retTempVar});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                        tmp.args = QStringList({retTempVar,B.useVar->varName,C.constValue});
                        codeList.append(tmp);
                    }
                }
                if(retIsTransType){
                    QString yType = (Btype[0]=='U' ? "UINT":"INT");
                    QString funName ="*"+ yType + "TO" + A.getType()+"*";

                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({retTempVar,"FLAG"});
                    codeList.append(tmp);

                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {retTempVar},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,0,0,0,1,varUseInfo));
                }else{
                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList.append(tmp);
                }
            }else if(mov_type==BTD ||
                     mov_type==WTD){
                QString movTypeName;
                if(mov_type==BTD){
                     movTypeName = "BTD";
                }else if(mov_type==WTD){
                     movTypeName = "WTD";
                }
                bool retIsTransType = Atype=="LONG"||Atype=="ULONG";
                QString transTemp = getTempVar(varUseInfo);

                //整型的四则运算
                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({transTemp,C.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                    tmp.args = QStringList({transTemp,B.useVar->varName,transTemp});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    bool isUseTemp = C.size>16;
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);
                        //转换类型
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                        tmp.args = QStringList({transTemp,B.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }else{
                        //转换类型
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = (Btype[0]=='U' ? "UDIV":"DIV");
                        tmp.args = QStringList({transTemp,B.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }
                }

                if(retIsTransType){
                    QString yType = (Btype[0]=='U' ? "UINT":"INT");
                    QString funName ="*"+ yType + "TO" + A.getType()+"*";

                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({transTemp,"FLAG"});
                    codeList.append(tmp);

                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {transTemp},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,0,0,0,1,varUseInfo));
                }else{
                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList.append(tmp);
                }
            }else if(mov_type==DWORD_T_QWORD ||
                     mov_type==QWORD_T_DWORD ||
                     mov_type==QWORD_T_QWORD){
                QString funName ="*"+B.getType()+"_"+nodeType+"_"+C.getType()+"_TO_"+A.getType()+"*";
                bool arg1Is64 = getVarTypeSize(Btype)>4;
                bool arg2Is64 = getVarTypeSize(Ctype)>4;
                bool retIs64 = getVarTypeSize(Atype)>4;

                if(!operFun.contains(funName))operFun.append(funName);
                if(B.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName,C.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,0,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,1,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }
            }else return;
        }else if(nodeType=="MOV"){
            //转移运算兼数据类型转换运算
            if(thisCode.args.length()!=2)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY")return;


            QString operFunName;

            //解析出mov_type
            uint mov_type = getTypeTransMode(Atype,Btype,&status);
            if(!status)return;
            status = 0;

            if(mov_type==MOV ||
               mov_type==ITF ||
               mov_type==UITF ||
               mov_type==BTD ||
               mov_type==WTD ||
               mov_type==FTI){
                QString movTypeName;
                if(mov_type==MOV){
                    movTypeName = "MOV";
                }else if(mov_type==ITF){
                     movTypeName = "ITF";
                }else if(mov_type==UITF){
                     movTypeName = "UITF";
                }else if(mov_type==BTD){
                     movTypeName = "BTD";
                }else if(mov_type==WTD){
                     movTypeName = "WTD";
                }else if(mov_type==FTI){
                     movTypeName = "FTI";
                }


                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName});
                    codeList.append(tmp);
                }else{
                    if(B.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                    }
                }

            }else if(mov_type==BTD_ITF||mov_type==WTD_ITF){
                QString movTypeName;
                if(mov_type==BTD_ITF){
                    movTypeName = "BTD";
                }else if(mov_type==WTD_ITF){
                     movTypeName = "WTD";
                }

                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName});
                    codeList.append(tmp);
                    tmp.nodeType = "ITF";
                    tmp.args = QStringList({A.useVar->varName,A.useVar->varName});
                    codeList.append(tmp);
                }else{
                    if(B.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({A.useVar->varName,A.useVar->varName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==DWORD_T_QWORD ||
                     mov_type==QWORD_T_DWORD ||
                     mov_type==QWORD_T_QWORD){
                QString funName ="*"+ B.getType() + "TO" + A.getType()+"*";
                bool argIs64 = mov_type==QWORD_T_DWORD || mov_type==QWORD_T_QWORD;
                bool retIs64 = mov_type==DWORD_T_QWORD || mov_type==QWORD_T_QWORD;

                if(!operFun.contains(funName))operFun.append(funName);
                if(B.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,0,argIs64,1,retIs64,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,1,argIs64,1,retIs64,varUseInfo));
                }
            }
        }else if(nodeType=="SAL"||
                 nodeType=="SAR"){
            //移位运算
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if((getVarTypeSize(Atype) != getVarTypeSize(Btype))||
                    Ctype=="DOUBLE" || C.getType()=="FLOAT"
                    ){
                return;
            }
            if(Atype=="ARRAY"||Btype=="ARRAY"||Ctype=="ARRAY")return;
            QStringList name;
            if(getVarTypeSize(Ctype)==8){
                name = bit64VarTo32VarName(C.useVar->varName);
            }

            if(getVarTypeSize(Atype)<=4){
                //整型的四则运算
                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName,getVarTypeSize(Ctype)==8?name[0]:C.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    QString tempVarName;

                    bool isUseTemp = isUseTempVar(12,C,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,C.constValue});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,B.useVar->varName,tempVarName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,B.useVar->varName,C.constValue});
                        codeList.append(tmp);
                    }
                }
            }else{
                QString funName = "*"+nodeType+"*";
                if(!operFun.contains(funName))operFun.append(funName);
                if(C.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)

                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName,
                                                  getVarTypeSize(Ctype)==8?name[0]:C.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,0,1,0,1,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,1,1,0,1,varUseInfo));
                }
            }
        }else if(nodeType=="NOT"){
            //位取反运算
            if(thisCode.args.length()!=2)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY")return;
            if(getVarTypeSize(Atype) != getVarTypeSize(Btype)){
                return;
            }
            if(getVarTypeSize(Atype)<=4){
                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    QString tempVarName;

                    bool isUseTemp = isUseTempVar(12,B,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,B.constValue});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,tempVarName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                    }
                }
            }else{
                QString funName = "*"+nodeType+"64Byte*";
                if(!operFun.contains(funName))operFun.append(funName);
                if(B.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)

                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,0,1,1,1,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {},
                                                 A.useVar->varName,
                                                 B.constValue,
                                                 B.size,1,1,1,1,varUseInfo));
                }
            }

        }else if(nodeType=="OR"||
                 nodeType=="AND"||
                 nodeType=="XOR"){
            //位与或异或运算
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY"||Ctype=="ARRAY")return;
            if((getVarTypeSize(Atype) != getVarTypeSize(Btype))||
                (getVarTypeSize(Atype) != getVarTypeSize(Ctype))){
                return;
            }

            if(getVarTypeSize(Atype)<=4){
                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName,C.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    QString tempVarName;

                    bool isUseTemp = isUseTempVar(12,C,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,C.constValue});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,B.useVar->varName,tempVarName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({A.useVar->varName,B.useVar->varName,C.constValue});
                        codeList.append(tmp);
                    }
                }
            }else{
                QString funName = "*"+nodeType+"64Byte*";
                if(!operFun.contains(funName))operFun.append(funName);
                if(C.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)

                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName,
                                                  C.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,0,1,1,1,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,1,1,1,1,varUseInfo));
                }
            }
        }else if(nodeType=="BNOT"){
            //布尔非
            if(thisCode.args.length()!=2)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY")return;
            if((Atype!="BYTE" && Atype!="UBYTE")||
               (Btype!="BYTE" && Btype!="UBYTE")){
                return;
            }

            if(B.useVar!=NULL){//非立即数运算
                tmp.nodeType = nodeType.mid(1,nodeType.length()-1);
                tmp.args = QStringList({getTempVar(varUseInfo),B.useVar->varName});
                codeList.append(tmp);

                tmp.nodeType = "MOV";
                tmp.args = QStringList({A.useVar->varName,"FLAG"});
                codeList.append(tmp);
            }else{//立即数运算
                //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                tmp.nodeType = nodeType.mid(1,nodeType.length()-1);
                tmp.args = QStringList({getTempVar(varUseInfo),B.constValue});
                codeList.append(tmp);
                tmp.nodeType = "MOV";
                tmp.args = QStringList({A.useVar->varName,"FLAG"});
                codeList.append(tmp);
            }


        }else if(nodeType=="BOR"||
                 nodeType=="BAND"){
            //布尔与或
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY"||Ctype=="ARRAY")return;
            if((Atype!="BYTE" && Atype!="UBYTE")||
               (Btype!="BYTE" && Btype!="UBYTE")||
               (Ctype!="BYTE" && Ctype!="UBYTE")){
                return;
            }

            if(C.useVar!=NULL){//非立即数运算
                tmp.nodeType = nodeType.mid(1,nodeType.length()-1);
                tmp.args = QStringList({getTempVar(varUseInfo),B.useVar->varName,C.useVar->varName});
                codeList.append(tmp);

                tmp.nodeType = "MOV";
                tmp.args = QStringList({A.useVar->varName,"FLAG"});
                codeList.append(tmp);
            }else{//立即数运算
                //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                tmp.nodeType = nodeType.mid(1,nodeType.length()-1);
                tmp.args = QStringList({getTempVar(varUseInfo),B.useVar->varName,C.constValue});
                codeList.append(tmp);
                tmp.nodeType = "MOV";
                tmp.args = QStringList({A.useVar->varName,"FLAG"});
                codeList.append(tmp);
            }
        }else if(nodeType=="CMEC"||
                 nodeType=="CMNEC"||
                 nodeType=="CMLC"||
                 nodeType=="CMMC"||
                 nodeType=="CMLEC"||
                 nodeType=="CMMEC"){
            //比较运算
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY"||Ctype=="ARRAY")return;
            if(Atype != "BYTE" && Atype != "UBYTE")return;

            QString cmpType = (Btype[0]=='U'?"U":(Btype[0]=='F'?"F":"")) + nodeType.mid(2,nodeType.length()-2);

            uint mov_type = getTypeTransMode(Btype,Ctype,&status);
            if(!status)return;
            status = 0;
            if(mov_type==MOV){
                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({B.useVar->varName,C.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    QString tempVarName;

                    bool isUseTemp = isUseTempVar(16,B,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,C.constValue});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,tempVarName});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,C.constValue});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==ITF ||
                     mov_type==UITF||
                     mov_type==BTD ||
                     mov_type==WTD ||
                     mov_type==FTI){
                QString movTypeName;
                if(mov_type==ITF){
                     movTypeName = "ITF";
                }else if(mov_type==UITF){
                     movTypeName = "UITF";
                }else if(mov_type==BTD){
                     movTypeName = "BTD";
                }else if(mov_type==WTD){
                     movTypeName = "WTD";
                }else if(mov_type==FTI){
                     movTypeName = "FTI";
                }

                QString transTemp = getTempVar(varUseInfo);

                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({transTemp,C.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({B.useVar->varName,transTemp});
                    codeList.append(tmp);

                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    if(C.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);
                        //类型转换
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);

                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==BTD_ITF||
                     mov_type==WTD_ITF){
                QString movTypeName;
                if(mov_type==BTD_ITF){
                    movTypeName = "BTD";
                }else if(mov_type==WTD_ITF){
                     movTypeName = "WTD";
                }
                QString transTemp = getTempVar(varUseInfo);

                if(C.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({transTemp,C.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = "ITF";
                    tmp.args = QStringList({transTemp,transTemp});
                    codeList.append(tmp);

                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({B.useVar->varName,transTemp});
                    codeList.append(tmp);

                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    if(C.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);
                        //类型转换
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,C.constValue});
                        codeList.append(tmp);

                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({B.useVar->varName,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({A.useVar->varName,"FLAG"});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==DWORD_T_QWORD ||
                     mov_type==QWORD_T_DWORD ||
                     mov_type==QWORD_T_QWORD){
                QString funName ="*"+B.getType()+"_"+nodeType.mid(2,nodeType.length()-2)+"_"+C.getType()+"*";
                bool arg1Is64 = getVarTypeSize(Btype)>4;
                bool arg2Is64 = getVarTypeSize(Ctype)>4;
                bool retIs64 = 0;
                if(!operFun.contains(funName))operFun.append(funName);
                if(C.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName,C.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,0,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {B.useVar->varName},
                                                 A.useVar->varName,
                                                 C.constValue,
                                                 C.size,1,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }
            }else return;

        }else if(nodeType=="EC"||
                 nodeType=="NEC"||
                 nodeType=="LC"||
                 nodeType=="MC"||
                 nodeType=="LEC"||
                 nodeType=="MEC"){
            //比较运算
            if(thisCode.args.length()!=2){
                return;
            };
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);

            if(A.isValid==0 || B.isValid==0 || A.useVar==NULL){
                return;
            };
            QString Atype =A.getType();
            QString Btype =B.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY")return;


            QString cmpType = (Atype[0]=='U'?"U":(Atype[0]=='F'?"F":"")) + nodeType;

            uint mov_type = getTypeTransMode(Atype,Btype,&status);
            if(!status)return;
            status = 0;
            if(mov_type==MOV){
                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({A.useVar->varName,B.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    QString tempVarName;

                    bool isUseTemp = isUseTempVar(16,B,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,B.constValue});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,tempVarName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,B.constValue});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==ITF ||
                     mov_type==UITF||
                     mov_type==BTD ||
                     mov_type==WTD ||
                     mov_type==FTI){
                QString movTypeName;
                if(mov_type==ITF){
                     movTypeName = "ITF";
                }else if(mov_type==UITF){
                     movTypeName = "UITF";
                }else if(mov_type==BTD){
                     movTypeName = "BTD";
                }else if(mov_type==WTD){
                     movTypeName = "WTD";
                }else if(mov_type==FTI){
                     movTypeName = "FTI";
                }

                QString transTemp = getTempVar(varUseInfo);

                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({transTemp,B.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({A.useVar->varName,transTemp});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    if(B.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({transTemp,B.constValue});
                        codeList.append(tmp);
                        //类型转换
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);
                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,B.constValue});
                        codeList.append(tmp);

                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==BTD_ITF||
                     mov_type==WTD_ITF){
                QString movTypeName;
                if(mov_type==BTD_ITF){
                    movTypeName = "BTD";
                }else if(mov_type==WTD_ITF){
                     movTypeName = "WTD";
                }
                QString transTemp = getTempVar(varUseInfo);

                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = movTypeName;
                    tmp.args = QStringList({transTemp,B.useVar->varName});
                    codeList.append(tmp);

                    tmp.nodeType = "ITF";
                    tmp.args = QStringList({transTemp,transTemp});
                    codeList.append(tmp);

                    tmp.nodeType = cmpType;
                    tmp.args = QStringList({A.useVar->varName,transTemp});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    if(B.size>16){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({transTemp,B.constValue});
                        codeList.append(tmp);
                        //类型转换
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = movTypeName;
                        tmp.args = QStringList({transTemp,B.constValue});
                        codeList.append(tmp);

                        tmp.nodeType = "ITF";
                        tmp.args = QStringList({transTemp,transTemp});
                        codeList.append(tmp);

                        tmp.nodeType = cmpType;
                        tmp.args = QStringList({A.useVar->varName,transTemp});
                        codeList.append(tmp);
                    }
                }
            }else if(mov_type==DWORD_T_QWORD ||
                     mov_type==QWORD_T_DWORD ||
                     mov_type==QWORD_T_QWORD){
                QString funName ="*"+A.getType()+"_"+nodeType+"_"+B.getType()+"*";
                bool arg1Is64 = getVarTypeSize(Atype)>4;
                bool arg2Is64 = getVarTypeSize(Btype)>4;
                bool retIs64 = 0;

                QString tempValue = getTempVar(varUseInfo);
                if(!operFun.contains(funName))operFun.append(funName);
                if(B.useVar!=NULL){//非立即数运算
                    //64位运算的函数参数表 (uint*Ahptr,uint*Alptr,uint Bh,uint Bl,uint Ch,uint Cl)
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {A.useVar->varName,B.useVar->varName},
                                                 tempValue,
                                                 B.constValue,
                                                 B.size,0,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }else{//立即数运算
                    codeList.append(generateCall64bitOperFunCode(funName,
                                                 {A.useVar->varName},
                                                 tempValue,
                                                 B.constValue,
                                                 B.size,1,arg1Is64,arg2Is64,retIs64,varUseInfo));
                }
                tmp.nodeType = "MOV";
                tmp.args = QStringList({"FLAG",tempValue});
                codeList.append(tmp);
            }else return;

        }else if(nodeType=="STORE"||
                 nodeType=="LOAD"){
            //内存读写运算
            if(thisCode.args.length()!=2)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            if(A.isValid==0 || B.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            if(Atype=="ARRAY"||Btype=="ARRAY")return;
            if(Btype!="UINT"){
                return;
            }
            QString rwSize;
            if(getVarTypeSize(Atype)==1){
                rwSize = "BYTE";
            }else if(getVarTypeSize(Atype)==2){
                rwSize = "WORD";
            }else if(getVarTypeSize(Atype)>=4){
                rwSize = "DWORD";
            }
            if(getVarTypeSize(Atype)<=4){
                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,A.useVar->varName,B.useVar->varName});
                    codeList.append(tmp);
                }else{//立即数运算
                    //如果C是一个常量,且常量无法作为立即数放在指令中,替换为通过临时变量方式进行运算
                    //初始化
                    QString tempVar = getTempVar(varUseInfo);
                    tmp.nodeType = "INIT";
                    tmp.args = QStringList({tempVar,B.constValue});
                    codeList.append(tmp);
                    //运算
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,A.useVar->varName,tempVar});
                    codeList.append(tmp);
                }
            }else{
                QString tempVar = getTempVar(varUseInfo);

                QStringList name = bit64VarTo32VarName(A.useVar->varName);
                if(B.useVar!=NULL){//非立即数运算
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,name[0],B.useVar->varName});
                    codeList.append(tmp);
                    tmp.nodeType = "UADD";
                    tmp.args = QStringList({tempVar,B.useVar->varName,"4d"});
                    codeList.append(tmp);
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,name[1],tempVar});
                    codeList.append(tmp);
                }else{
                    tmp.nodeType = "INIT";
                    tmp.args = QStringList({tempVar,B.constValue});
                    codeList.append(tmp);
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,name[0],tempVar});
                    codeList.append(tmp);
                    tmp.nodeType = "UADD";
                    tmp.args = QStringList({tempVar,tempVar,"4d"});
                    codeList.append(tmp);
                    tmp.nodeType = nodeType=="STORE" ? "WE" : "RD";
                    tmp.args = QStringList({rwSize,name[1],tempVar});
                    codeList.append(tmp);
                }
            }

        }else if(nodeType=="ARRSTORE"||
                 nodeType=="ARRLOAD"){
            //局部数组读写
            if(thisCode.args.length()!=3)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);

            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL || B.useVar==NULL || C.useVar!=NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();

            if(Btype!="ARRAY" || Atype=="ARRAY" || Ctype=="LONG" || Ctype=="ULONG" || Ctype=="DOUBLE" || Ctype=="FLOAT")return;
            QString rwSize;
            if(getVarTypeSize(Atype)==1){
                rwSize = "BYTE";
            }else if(getVarTypeSize(Atype)==2){
                rwSize = "WORD";
            }else if(getVarTypeSize(Atype)>=4){
                rwSize = "DWORD";
            }

            if(getVarTypeSize(Atype)<=4){
                tmp.nodeType = nodeType;
                tmp.args = QStringList({rwSize,A.useVar->varName,B.useVar->varName,C.constValue});
                codeList.append(tmp);
            }else{
                QStringList name = bit64VarTo32VarName(A.useVar->varName);
                tmp.nodeType = nodeType;
                tmp.args = QStringList({rwSize,name[0],B.useVar->varName,C.constValue});
                codeList.append(tmp);

                uint constOffset = analysisStdConst(C.constValue);
                if(constOffset==0xffffffff){
                    return;
                }
                constOffset+=4;
                tmp.nodeType = nodeType;
                tmp.args = QStringList({rwSize,name[1],B.useVar->varName,QString::number(constOffset)+"D"});
                codeList.append(tmp);
            }
        }else if(nodeType=="PUSH"){
            //函数参数入栈运算
            if(thisCode.args.length()!=1)return;
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            if(A.isValid==0)return;

            QString Atype =A.getType();
            if(Atype=="ARRAY"){//数组的入栈
                if(A.useVar==NULL)return;

                //开辟入栈后的存储区域
                uint arraySizeNum = A.useVar->arrSize;

                if(getNumDigit(arraySizeNum)>16){
                    tmp.nodeType = "INIT";

                    QString tmpVar = getTempVar(varUseInfo);
                    tmp.args = QStringList({tmpVar,QString::number(arraySizeNum)+"D"});
                    codeList.append(tmp);
                    tmp.nodeType = "SUB";
                    tmp.args = QStringList({"SP",tmpVar});
                    codeList.append(tmp);
                }else{
                    tmp.nodeType = "SUB";
                    tmp.args = QStringList({"SP",QString::number(arraySizeNum)+"D"});
                    codeList.append(tmp);
                }



                tmp.nodeType = "PUSHARRAY";
                tmp.args = QStringList({QString::number(arraySizeNum)});
                codeList.append(tmp);



                //如果要入栈的大于16bit,申请临时变量
                bool isUseTemp1 = getNumDigit(A.useVar->arrSize)>16;
                QString temp1;
                if(isUseTemp1){
                    temp1 = getTempVar(varUseInfo);
                    //调用函数数组型变量的复制函数
                    tmp.nodeType = "INIT";
                    tmp.args = QStringList({temp1,QString::number(A.useVar->arrSize)+"D"});
                    codeList.append(tmp);
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",temp1});
                    codeList.append(tmp);
                }else{
                    //调用函数数组型变量的复制函数
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",QString::number(A.useVar->arrSize)+"D"});
                    codeList.append(tmp);
                }

                //传入必要的数据
                QString arrBase = getTempVar(varUseInfo);
                tmp.nodeType = "ADDRESS";
                tmp.args = QStringList({arrBase,A.useVar->varName,"0D"});
                codeList.append(tmp);

                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",arrBase});
                codeList.append(tmp);
                //调用函数
                tmp.nodeType = "INIT";
                tmp.args = QStringList({"TPC","[funArgArrayCopy]"});
                codeList.append(tmp);
                if(!operFun.contains("funArgArrayCopy")){
                    operFun.append("funArgArrayCopy");
                }
                tmp.nodeType = "COPYARR";
                tmp.args = QStringList();
                codeList.append(tmp);

                continue;
            }
            //普通变量的入栈

            QString rwSize;
            if(getVarTypeSize(Atype)==1){
                rwSize = "BYTE";
            }else if(getVarTypeSize(Atype)==2){
                rwSize = "WORD";
            }else if(getVarTypeSize(Atype)>=4){
                rwSize = "DWORD";
            }


            if(A.useVar!=NULL){
                if(getVarTypeSize(Atype)<=4){
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({rwSize,A.useVar->varName});
                    codeList.append(tmp);
                }else{
                    tmp.nodeType = nodeType;
                    QStringList name = bit64VarTo32VarName(A.useVar->varName);
                    tmp.args = QStringList({rwSize,name[1]});
                    codeList.append(tmp);
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({rwSize,name[0]});
                    codeList.append(tmp);
                }
            }else{

                if(getVarTypeSize(Atype)<=2){
                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({rwSize,A.constValue});
                    codeList.append(tmp);
                }else if(getVarTypeSize(Atype)<=4){
                    QString tempVarName;
                    bool isUseTemp = isUseTempVar(16,A,varUseInfo,tempVarName);
                    if(isUseTemp){
                        //初始化
                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({tempVarName,A.constValue});
                        codeList.append(tmp);

                        //运算
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({rwSize,tempVarName});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = nodeType;
                        tmp.args = QStringList({rwSize,A.constValue});
                        codeList.append(tmp);
                    }
                }else{
                    QString tempVar0 = getTempVar(varUseInfo);
                    QString tempVar1 = getTempVar(varUseInfo);
                    tmp.nodeType = "INIT64";
                    tmp.args = QStringList({tempVar0,tempVar1,A.constValue});
                    codeList.append(tmp);

                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({rwSize,tempVar0});
                    codeList.append(tmp);

                    tmp.nodeType = nodeType;
                    tmp.args = QStringList({rwSize,tempVar1});
                    codeList.append(tmp);
                }
            }
        }else if(nodeType=="CLEAR"){
            //stdcall调用约定时,清空函数函数入栈参数
            if(thisCode.args.length()<1)return;

            uint size = 0;
            for(int i = 0;i<thisCode.args.length();i++){
                OrderArg tmp = getOrderArg(thisCode.args[i],allAddressLabel,varUseInfo);
                if(tmp.isValid==0)return;

                QString type =tmp.getType();
                if(type=="ARRAY"){
                    size+=tmp.useVar->arrSize;
                }else{
                    size+=getVarTypeSize(type);
                }

            }
            if(getNumDigit(size)<=12){
                tmp.nodeType = "CLEAR";
                tmp.args = QStringList({"SP",QString::number(size)+"D"});
                codeList.append(tmp);
            }else{
                tmp.nodeType = "MOV";
                QString tempVar = getTempVar(varUseInfo);
                tmp.args = QStringList({tempVar,QString::number(size)+"D"});
                codeList.append(tmp);
                tmp.nodeType = "CLEAR";
                tmp.args = QStringList({"SP",tempVar});
                codeList.append(tmp);
            }

        }else if(nodeType=="CALL"||nodeType=="STDCALL"){
            OrderArg funAdd = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            if(funAdd.isValid==0 || funAdd.getType()!="UINT")return;
            if(funAdd.getType()=="ARRAY")return;
            QString funAddText;
            if(funAdd.useVar==NULL){
                tmp.nodeType = "INIT";
                funAddText = funAdd.constValue;
            }else{
                tmp.nodeType = "MOV";
                funAddText = funAdd.useVar->varName;
            }

            tmp.args = QStringList({"TPC",funAddText});
            codeList.append(tmp);
            tmp.nodeType = nodeType;
            tmp.args = QStringList();
            codeList.append(tmp);
        }else if(nodeType=="JMP"||
                 nodeType=="JNE"||
                 nodeType=="INIT"||
                 nodeType=="LABEL"){
            codeList.append(replseList[i]);
            continue;
        }else if(nodeType=="ADDRESS"){
            //获取地址的指令,不进行解析。仅仅判断其中使用了哪些变量,将这些变量设为已被使用
            if(thisCode.args.length()!=3)return;
            //获取运算参数
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);

            if(A.useVar==NULL || B.useVar==NULL || A.getType()!="UINT" || C.useVar!=NULL)return;
            B.useVar->isRetrievedAddress = 1;

            QString getAddVarName = B.useVar->varName;
            if(B.getType()=="ULONG"||B.getType()=="LONG"||B.getType()=="DOUBLE"){
                QStringList name = bit64VarTo32VarName(getAddVarName);
                getAddVarName = name[0];
            }
            tmp.nodeType = "ADDRESS";
            tmp.args = QStringList({A.useVar->varName,getAddVarName,C.constValue});


            codeList.append(tmp);
        }else{
            //不支持的运算
            return;
        }
    }



    //优化:去除未被使用的局部变量
    for(int i = 0;i<varUseInfo.length();i++){
        if(varUseInfo[i].isUse==0 && !varUseInfo[i].isArg){
            varUseInfo.removeAt(i);
            i--;
        }
    }

    //将64位的变量定义替换为2个uint32类型的变量
    for(int i = 0;i<varUseInfo.length();i++){
        QString type = varUseInfo[i].type;
        if(type=="ULONG" || type=="LONG" || type=="DOUBLE"){
            VarUse lowVar = varUseInfo[i],highVar = varUseInfo[i];
            lowVar.type = "UINT";
            highVar.type = "UINT";

            QStringList name32 = bit64VarTo32VarName(lowVar.varName);
            lowVar.varName = name32[0];
            highVar.varName = name32[1];

            varUseInfo.removeAt(i);

            varUseInfo.insert(i,highVar);
            varUseInfo.insert(i+1,lowVar);
            i++;
        }
    }

    //将变量的使用映射到对寄存器的使用上,并生成堆栈变量数据与寄存器变量数据的同步指令
    VariablePool pfool({"R1","R2","R3","R4","R5","R6","R7","R8"},varUseInfo,codeList);

    funNode.getAttFromName("BODY")->subNodes = codeList;
    qDebug()<<funNode.toString().toUtf8().data();

    codeList = pfool.generateLowCode(status);

    if(status==0){
        return;
    }
    status = 0;
    varUseInfo = pfool.vars;

    QStringList allUsedRegNames = pfool.getAllUsedRegNames();
    //生成函数入口指令
    uint varUseStackBytes = 0,argUseStackBytes = 0;
    QList<MiddleNode> funReturnCodes;
    QList<MiddleNode> funEntryCodes = generateEntryCode(funNode,isHaveCallFun,varUseInfo,allUsedRegNames,varUseStackBytes,argUseStackBytes,status);
    if(status==0){
        return;
    }
    status = 0;


    //生成函数退出指令
    if(isHaveReturnHandle){
        funReturnCodes = generateReturnCode(funNode,
                                             varUseStackBytes,
                                             argUseStackBytes,
                                             allUsedRegNames,
                                             status);

        if(status==0){
            return;
        }
        status = 0;
    }
    uint stackTopBuoy = 0;
    QList<int> stackTopBuoyList;//返回函数主体逻辑功能指令执行时的栈顶浮动
    replseList.clear();


    //解析获取取局部变量地址/读写函数中数组的指令,并解析出函数代码执行过程中栈顶浮动变化的信息
     for(int i = 0;i<codeList.length();i++){
        MiddleNode &thisCode = codeList[i];
        status = 0;
        QString nodeType = thisCode.nodeType;
        MiddleNode tmp;
        if(nodeType=="ADDRESS"){
            QString movVar = thisCode.args[0];
            QString getAddVar = thisCode.args[1];
            QString offsetText = thisCode.args[2];
            //offset只能够是数值型立即数
            int offset;
            QString numText = offsetText.left(offsetText.length()-1);
            QString sys = offsetText.right(1).toUpper();
            bool suceess = 0;
            if(sys=='H'){
                offset = numText.toLong(&suceess,16);
            }else if(sys=='B'){
                offset = numText.toLong(&suceess,2);
            }else if(sys=='O'){
                offset = numText.toLong(&suceess,8);
            }else if(sys=='D'){
                offset = numText.toLong(&suceess,10);
            }
            if(!suceess){
                return;
            }
            uint varStackOff = 0xffffffff;
            //获取被取地址的栈顶偏移，并将其设为被取地址
            for(int i = 0;i<varUseInfo.length();i++){
                if(varUseInfo[i].varName == getAddVar){
                    varStackOff = varUseInfo[i].offset;
                    break;
                }
            }
            if(varStackOff==0xffffffff){
                return;
            }
            varStackOff+= offset+stackTopBuoy;

            offsetText = QString::number(varStackOff)+"D";
            if(getNumDigit(varStackOff)<=12){
                stackTopBuoyList.append(stackTopBuoy);
                tmp.nodeType = "ADD";
                tmp.args = QStringList({movVar,"SP",offsetText});
                replseList.append(tmp);
            }else if(getNumDigit(varStackOff)<=16){
                stackTopBuoyList.append(stackTopBuoy);
                stackTopBuoyList.append(stackTopBuoy);
                tmp.nodeType = "MOV";
                tmp.args = QStringList({movVar,"SP"});
                replseList.append(tmp);
                tmp.nodeType = "ADD";
                tmp.args = QStringList({movVar,offsetText});
                replseList.append(tmp);
            }else{
                stackTopBuoyList.append(stackTopBuoy);
                stackTopBuoyList.append(stackTopBuoy);
                tmp.nodeType = "INIT";
                tmp.args = QStringList({movVar,offsetText});
                replseList.append(tmp);
                tmp.nodeType = "ADD";
                tmp.args = QStringList({movVar,"SP"});
                replseList.append(tmp);
            }
        }else if(nodeType=="ARRSTORE"||
                 nodeType=="ARRLOAD"){
            stackTopBuoyList.append(stackTopBuoy);
            QString movVar = thisCode.args[1];
            QString array = thisCode.args[2];
            QString offsetText = thisCode.args[3];

            uint offsetNum = analysisStdConst(offsetText,&status);
            if(status==0)return;
            status=0;
            uint varStackOff = 0xffffffff;
            //获取被取地址的栈顶偏移，并将其设为被取地址
            for(int i = 0;i<varUseInfo.length();i++){
                if(varUseInfo[i].varName == array){
                    varStackOff = varUseInfo[i].offset;
                    break;
                }
            }
            if(varStackOff==0xffffffff){
                return;
            }
            varStackOff+= offsetNum+stackTopBuoy;

            if(getNumDigit(varStackOff)>21){
                return;
            }
            tmp.nodeType = nodeType=="ARRSTORE" ? "SWE" : "SRD";
            tmp.args = QStringList({thisCode.args[0],movVar,QString::number(varStackOff)+"D"});
            replseList.append(tmp);
        }else if(nodeType=="PUSH"){
            stackTopBuoyList.append(stackTopBuoy);
            //如果函数参数入栈,栈顶上浮
            if(thisCode.args[0]=="DWORD"){
                stackTopBuoy+=4;
            }else if(thisCode.args[0]=="WORD"){
                stackTopBuoy+=2;
            }else if(thisCode.args[0]=="BYTE"){
                stackTopBuoy+=1;
            }
            replseList.append(codeList[i]);
        }else if(nodeType=="PUSHARRAY"){
            stackTopBuoy += thisCode.args[0].toUInt();
        }else if(nodeType=="CALL"){
            stackTopBuoyList.append(stackTopBuoy);
            //普通函数被调用返回后,栈顶恢复
            stackTopBuoy = 0;
            tmp.nodeType = "RT";
            replseList.append(tmp);
        }else if(nodeType=="STDCALL"){
            //stdcall函数被调用后,栈顶不自动恢复
            stackTopBuoyList.append(stackTopBuoy);
            tmp.nodeType = "RT";
            replseList.append(tmp);
        }else if(nodeType=="CLEAR"){
            //stdcall函数执行完清栈代码后栈顶恢复
            stackTopBuoyList.append(stackTopBuoy);
            stackTopBuoy = 0;
            codeList[i].nodeType = "UADD";
            replseList.append(codeList[i]);
        }else if(nodeType=="COPYARR"){
            stackTopBuoyList.append(stackTopBuoy);
            //函数参数的复制完成后,栈顶+8
            stackTopBuoy -= 8;
            tmp.nodeType = "RT";
            replseList.append(tmp);
        }else{
            stackTopBuoyList.append(stackTopBuoy);
            replseList.append(codeList[i]);
        }
    }
     codeList.clear();


     //解析出<offset:变量名>对应的堆栈偏移值
     for(int i = 0;i<replseList.length();i++){
         //qDebug()<<replseList[i].toString().toUtf8().data();
         MiddleNode &thisCode = replseList[i];
         for(int j = 0;j<thisCode.args.length();j++){
             QString argText = thisCode.args[j];
             if(argText.length()>=10 && argText.mid(0,8)=="<offset:" && argText.right(1)==">"){
                 argText.remove(0,8);
                 argText.remove(argText.length()-1,1);

                 uint varStackOff = 0xffffffff;
                 //获取被取地址的栈顶偏移，并将其设为被取地址
                 for(int i = 0;i<varUseInfo.length();i++){
                     if(varUseInfo[i].varName == argText){
                         varStackOff = varUseInfo[i].offset;
                         break;
                     }
                 }
                 if(varStackOff==0xffffffff){
                     return;
                 }

                 varStackOff += stackTopBuoyList[i];
                thisCode.args[j] = QString::number(varStackOff)+"D";
             }
         }
     }
     qDebug()<<"--------------------------------";
     for(int i = 0;i<varUseInfo.length();i++){
         qDebug()<<varUseInfo[i].toString().toUtf8().data()<<varUseInfo[i].offset;
     }


     //拼接 函数入口指令 函数体自定义指令 函数退出指令
     bodyCodeList = funEntryCodes + replseList + funReturnCodes;
     status = 1;
}



//判断一个函数中的指令是否符合要求
bool thisCodeDefIsMeetReq(MiddleNode &code){
    QString codeType = code.nodeType;
    //拥有3个参数的指令
    static const QStringList have3ArgOrder = {
        "MOV","ADD","SUB","MUL","DIV","REM","SAL","SAR","AND","OR",
        "XOR","BAND","BOR","CMEC","CMNEC","CMLC","CMMC","CMLEC","CMMEC",
        "ADDRESS","ARRSTORE","ARRLOAD"
    };
    //拥有两个参数的指令
    static const QStringList have2ArgOrder = {
        "STORE","LOED","NOT","BNOT"
    };
    //拥有一个参数的指令
    static const QStringList have1ArgOrder = {
        "GOTO","LABEL"
    };
    //没有参数的指令
    static const QStringList notHaveArgOrder = {
        "RETURN"
    };
    //可变参数数量的指令(>=1)
    static const QStringList variableArgOrder = {
        "CALL"
    };

    if(have3ArgOrder.contains(codeType)){
        return code.args.length()==3;
    }else if(have2ArgOrder.contains(codeType)){
        return code.args.length()==2;
    }else if(have1ArgOrder.contains(codeType)){
        return code.args.length()==1;
    }else if(notHaveArgOrder.contains(codeType)){
        return code.args.length()==0;
    }else if(variableArgOrder.contains(codeType)){
        return code.args.length()>=1;
    }else if(codeType=="PROCESS"){
        //程序流程块指令
        if(code.args.length()!=3){
            return 0;
        }

        //递归搜索流程块中定义的指令是否符合要求
        MiddleNode_Att*judgeBlock = code.getAttFromName("JUDGE");
        MiddleNode_Att*ifBlock = code.getAttFromName("IF");
        MiddleNode_Att*elseBlock = code.getAttFromName("ELSE");

        if(judgeBlock!=NULL){
            for(int i = 0;i<judgeBlock->subNodes.length();i++){
                if(!thisCodeDefIsMeetReq(judgeBlock->subNodes[i])){
                    return 0;
                }
            }
        }

        if(ifBlock!=NULL){
            for(int i = 0;i<ifBlock->subNodes.length();i++){
                if(!thisCodeDefIsMeetReq(ifBlock->subNodes[i])){
                    return 0;
                }
            }
        }

        if(elseBlock!=NULL){
            for(int i = 0;i<elseBlock->subNodes.length();i++){
                if(!thisCodeDefIsMeetReq(elseBlock->subNodes[i])){
                    return 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

//判断函数的定义是否符合要求
bool thisFunDefIsMeetReq(MiddleNode &node){
    if(node.args.length()!=1)return 0;

    //检查函数是否定义了不支持的属性
    if(!node.areThereOnlyTheseAtt({"BODY","ARGS","VARS","EXPORT","STDCALL","APPCALL","DRIVERCALL","INTERRUPT"})){
        return 0;
    }

    //检查函数是否未定义必要的属性
    MiddleNode_Att*body = node.getAttFromName("BODY");
    if(body==NULL)return 0;

    //检查函数中定义的指令,是否存在定义了不支持的指令或指令参数不正确
    for(int i = 0;i<body->subNodes.length();i++){
        if(!thisCodeDefIsMeetReq(body->subNodes[i])){
            return 0;
        }
    }

    return 1;
}

//解析函数
QString analysisFunction(MiddleNode &node,QStringList &allAddressLabel,QStringList &operFun,bool&status){
    QString asmText;
    status = 0;
    //判断函数的定义是否符合要求
    if(thisFunDefIsMeetReq(node)){
        return asmText;
    }
    //解析出函数的名字
    QString funName = node.args[0];
    allAddressLabel.append(funName);

    //解析出的函数定义了哪些参数和局部变量
    QList<VarUse> defineVar;//定义的局部变量
    QList<VarUse> defineArg;//定义的参数

    //完成中间语言指令到低级语言指令的编译
    QList<MiddleNode> bodyCodeList;//返回编译生成的函数主体逻辑功能指令
    compileToLower(node,
                   allAddressLabel,
                   operFun,
                   bodyCodeList,
                   status);
    if(!status)return asmText;
    //生成汇编文本
    MiddleNode_Att *bodyNode = node.getAttFromName("BODY");
    bool isExport = node.getAttFromName("EXPORT");
    bodyNode->subNodes = bodyCodeList;
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
    asmText += " "+node.args[0]+"{\r\n";
    asmText += codeAsmText;
    asmText += "};\r\n";
    status = 1;
    return asmText;
}


/*函数局部变量/参数定义指令
 * //定义变量
 * DATA(数据类型,变量名);
 * //定义数组
 * ARRAY(数组名,数组字节数);
 *
 * 如果 数据类型 为long/ulong/double时生成2个uint
 * 分别名称前缀 *high* *low*
 * 例如 long test 生成2个 uint *high*test; uint *low*test
 *
 */

/*函数执行逻辑指令
 *
 * //数据转移(a/b数据类型不同的情况下降自动进行数据类型转换)b可为常量
 * MOV(a,b);
 *
 * //四则运算(a/b/c的数据类型必须完全相同)c可为常量
 * ADD(a,b,c);  a=b+c
 * SUB(a,b,c);  a=b-c
 * MUL(a,b,c);  a=b*c
 * DIV(a,b,c);  a=b/c
 *
 * //取模运算(a/b/c必须为整型,同时数据类型完全相同)c可为常量
 * REM(a,b,c);
 *
 * //移位运算(a和b的位数必须相同,c的数据类型不可是浮点型)c可为常量
 * SAL(a,b,c);  a=b<<c
 * SAR(a,b,c);  a=b>>c
 *
 * //位运算(a/b/c位数必须相同)not中b可为常量,其它c可为常量
 * NOT(a,b);    a=~b
 * AND(a,b,c);  a=b&c
 * OR(a,b,c);   a=b|c
 * XOR(a,b,c);  a=b^c
 *
 * //布尔运算(a/b/c都必须是byte或ubyte类型)not中b可为常量,其它c可为常量
 * BNOT(a,b);   a=!b
 * BAND(a,b,c); a=b&&c
 * BOR(a,b,c);  a=b||c
 *
 * //比较运算(b/c必须相同数据类型,a必须整型)c可为常量
 * CMEC(a,b,c);   a=(b==c)
 * CMNEC(a,b,c);  a=(b!=c)
 * CMMC(a,b,c);   a=b>c
 * CMLC(a,b,c);   a=b<c
 * CMMEC(a,b,c);  a=(b>=c)
 * CMLEC(a,b,c);  a=(b<=c)
 *
 * //内存操作指令(a无数据类型限制,b必须是pointer)b可为常量
 * STORE(a,b);  *b=a
 * LODE(a,b);   a=*b
 *
 * //局部数组读写(a无数据类型限制,b必须是一个局部数组,c必须是无符号整型常量)
 * ARRSTORE(a,b,c);
 * ARRLOAD(a,b,c);
 *
 * //读取一个局部变量的内存地址到另一个局部指针变量中,c只可为数值型常量
 * ADDRESS(a,b,c); a=(&b)+c
 *          ADD a,sp,offset<b>+c;
 *
 * //条件执行程序块(.IF和.ELSE至少要先一个)/
 * //a/b必须相同数据类型,b可为常量
 * EC  b==c
 * NEC b!=c
 * MC  b>c
 * LC  b<c
 * MEC b>=c)
 * LEC b<=c)
 * PROCESS(比较运算指令,a,b).JUDGE{//可不写
 *    //jmp语句判断是执行if还是else块的代码,通过设置决定变量的值来实现
 * }.IF{
 *    //决定变量的值!=0 是执行的代码
 * }.ELSE{
 *    //决定变量的值==0 是执行的代码
 * }
 *
 * //无条件跳转/
 * LABEL(标记名); //标记一个跳转位置
 * GOTO(标记名);  //跳转到标记点指定的位置
 *
 * //函数调用
 * CALL(指向函数地址的变量/或函数地址常量,函数参数....);
 *
 * //函数退出/
 * RETURN;
 */

/*函数类型
 * cpu模式
 *  应用模式:  开启中断响应,开启保护模式,启用虚拟内存(应用程序的运行模式,多进程状态下运行,同时限制CPU控制权限)
 *  驱动模式:  开启中断响应,不启用保护模式,启用虚拟内存(驱动程序的运行模式,支持多进程,同时拥有CPU的完整控制权限)
 *  内核模式:  开启中断响应,不启用保护模式,不启用虚拟内存(正常裸机状态下的运行模式,操作系统内核态下的运行模式)
 *  临界模式:  关闭中断响应,不启用保护模式,不启用虚拟内存(CPU重启后以及中断处理时的默认运行模式)
 *
 *
 * 无任何后缀:普通函数
 *
 * 设置后缀,声明为中断处理函数(中断处理函数默认运行在临界  模式下,但也可以调用汇编指令手动调整到其它运行模式)
 * //设置为由软中断触发
 * APPCALL:  应用程序通过软中断调用系统内核功能的处理函数(在处理完毕后会自动将cpu切换回应用模式下)
 * DRIVERCALL: 驱动程序通过软中断调用系统内核功能的处理函数(在处理完毕后自动将cpu切换回驱动模式下)
 * //设置为由外部中断触发
 * INTERRUPT: 系统内核的中断响应处理函数(在处理完毕后自动将cpu切换回内核模式)
 *
 *
 *
 *
 */


#endif // ANALYSISFUNCTION_H
