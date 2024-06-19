#include "OpenQinling_PSDL_AnalysisFunction.h"
#include "OpenQinling_PSDL_ConstValueAnalysis.h"
#include "OpenQinling_PSDL_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

struct CodeSegGotoLabel{
    QString gotoLabelName;//跳转到的地址
    int orderIndex;//如果是跳转jne/jmp指令,指令的索引号
};
struct CodeSegGotoLabelList{
    QList<CodeSegGotoLabel> labels;

    int length(){
        return labels.length();
    }

    void append(QString label){
        labels.append({label,-1});
    }
    void append(QString gotoLabel,int index){
        labels.append({gotoLabel,index});
    }

    QString toString(){
        QStringList list;
        for(int i = 0;i<labels.length();i++){
            if(labels[i].orderIndex==-1){
                list.append("["+labels[i].gotoLabelName+"]");
            }else{
                list.append("["+labels[i].gotoLabelName+","+QString::number(labels[i].orderIndex)+"]");
            }

        }
        return "("+list.join(",")+")";
    }

    bool contains(QString name){
        for(int i = 0;i<labels.length();i++){
            if(labels[i].gotoLabelName==name){
                return 1;
            }
        }
        return 0;
    }
};

//代码段
//typedef QList<MiddleNode> CodeSegment;
struct CodeSegment:public QList<MiddleNode>{
    bool isHaveBeginLabel = 0;
    QString blockBeginLabelName;//其它段跳转到该段起始位置的标记
    bool isHaveGoto = 0;
    CodeSegGotoLabelList blockAllGotoLabels;//该段跳转到其它段的所有标记的名称
    QString blockEndGotoLabel;//段尾结束跳的标记的名称
    bool isVisible = 0;
};
//将当前段所可能跳转到的段isVisible置1
void searchMaybeGotoSegment(CodeSegment&thisSeg,QList<CodeSegment> &allSegment){
    if(!thisSeg.isHaveGoto)return;
    CodeSegGotoLabelList gotoLabel = thisSeg.blockAllGotoLabels;
    for(int i = 0;i<allSegment.length();i++){
        if(allSegment[i].isVisible || !allSegment[i].isHaveBeginLabel)continue;

        if(gotoLabel.contains(allSegment[i].blockBeginLabelName)){
            allSegment[i].isVisible = 1;
            searchMaybeGotoSegment(allSegment[i],allSegment);
        }
    }
}

//将函数中的指令分段
QList<CodeSegment> splitCodeSegment(QList<MiddleNode> &codeList){
    QList<CodeSegment> allSegment;
    CodeSegment tmp;
    QString thisBlockBeginLabel;
    tmp.clear();
    int thisSegBeginIndex = 0;
    for(int i = 0;i<codeList.length();i++){
        if(codeList[i].nodeType == "JNE" || codeList[i].nodeType == "JMP"){
            tmp.isHaveGoto = 1;
            QString gotoLabel = codeList[i].args[0].mid(6,codeList[i].args[0].length()-7);
            if(!tmp.blockAllGotoLabels.contains(gotoLabel)){
                tmp.blockAllGotoLabels.append(gotoLabel,i-thisSegBeginIndex);
            }
            if(codeList[i].nodeType == "JMP"){
                tmp.blockEndGotoLabel = gotoLabel;
            }
        }

        if(codeList[i].nodeType == "LABEL"){
            if(i>=1 && codeList[i-1].nodeType!="JMP"){
                tmp.isHaveGoto = 1;
                if(!tmp.blockAllGotoLabels.contains(codeList[i].args[0])){
                    tmp.blockAllGotoLabels.append(codeList[i].args[0]);
                }
                tmp.blockEndGotoLabel = codeList[i].args[0];
            }


            if(tmp.length()!=0){
                tmp.isHaveBeginLabel = allSegment.length()!=0;
                tmp.blockBeginLabelName = thisBlockBeginLabel;
                allSegment.append(tmp);
            }
            tmp = CodeSegment();
            tmp.append(codeList[i]);
            thisSegBeginIndex = i;
            thisBlockBeginLabel = codeList[i].args[0];
        }else{
            tmp.append(codeList[i]);
        }
    }
    if(tmp.length()!=0){
        tmp.isHaveBeginLabel = thisBlockBeginLabel!="";
        tmp.blockBeginLabelName = thisBlockBeginLabel;
        allSegment.append(tmp);
    }

    return allSegment;
}

//去除不可达代码
//allGotoLabel:函数中定义的所有标记
void removeUnreachableCode(QList<MiddleNode> &codeList,bool &isHaveReturnHandle){
    //所有的代码段
    QList<CodeSegment> allSegment;
    CodeSegment tmp;
    for(int i = 0;i<codeList.length();i++){
        if(codeList[i].nodeType == "LABEL"){
            allSegment.append(tmp);
            tmp.clear();
            tmp.append(codeList[i]);
        }else{
            tmp.append(codeList[i]);
        }
    }
    if(tmp.length()!=0){
        allSegment.append(tmp);
        tmp.clear();
    }

    //去除jmp后面的代码
    for(int i = 0;i<allSegment.length();i++){
        tmp.clear();
        for(int j = 0;j<allSegment[i].length();j++){
            tmp.append(allSegment[i][j]);
            if(allSegment[i][j].nodeType == "JMP"){
                break;
            }
        }
        allSegment[i] = tmp;
    }
    codeList.clear();
    for(int i = 0;i<allSegment.length();i++){
        codeList.append(allSegment[i]);
    }
    allSegment.clear();

    allSegment = splitCodeSegment(codeList);

    if(allSegment.length()==0)return;
    allSegment[0].isVisible = 1;
    searchMaybeGotoSegment(allSegment[0],allSegment);

    for(int i = 0;i<allSegment.length();i++){
        if(!allSegment[i].isVisible){
            allSegment.removeAt(i);
            i--;
        }
    }

    codeList.clear();
    isHaveReturnHandle = 0;
    for(int i = 0;i<allSegment.length();i++){
        if(allSegment[i].blockBeginLabelName=="|RETURN|"){
            isHaveReturnHandle = 1;
        }
        codeList.append(allSegment[i]);
    }

}

//变量的使用情况
class VarUse{
public:
    QString type;       //变量数据类型
    int arrSize;    //数组大小
    QString varName;    //变量名
    bool isUse = 0;     //是否在代码中被使用过
    bool isArg;         //该变量的局部变量函数参数(函数参数为1)
    bool isRetrievedAddress = 0;//是否被取过地址
    bool isUseStack = 0;//变量是否在程序运行过程中使用堆栈空间去暂存(数组类型该参数无效)
    uint offset = 0;

    QString toString(){
        QString tmp = type + "(" + varName;
        if(type=="ARRAY"){
            tmp += ","+QString::number(arrSize);
        }
        tmp+= ")";
        if(isArg){
            tmp += ".ARG";
        }else{
            tmp += ".VAR";
        }

        if(isRetrievedAddress){
            tmp += "<RetrievedAddress>";
        }

        if(isUseStack){
            tmp += "<UseStack>";
        }
        return tmp;
    }
};

//获取一个指令中的操作数
class CodeOperand{
public:
    QString operText;//操作数的文本
    bool isValue;    //该操作数是否是一个变量
    bool rw;         //该操作数是源操作数还是目标操作数.源操作数为0，目标操作数为1

    QString toString(){
        return (rw ? "W:" : "R:") +operText + (isValue ? "<Value>" : "");
    }
};
bool thisOperIsValue(QString operText,QList<VarUse> &allFunValue){
    for(int i = 0;i<allFunValue.length();i++){
        if(allFunValue[i].varName==operText && allFunValue[i].type!="ARRAY"){
            return true;
        }
    }
    return false;
}
QList<CodeOperand> getCodeOperand(MiddleNode &code,QList<VarUse> &allFunValue){
    QString codeType = code.nodeType;
    QList<CodeOperand> oper;

    CodeOperand tmp;
    //有参数的指令,获取其中的源操作数变量/目标写入变量
    if(codeType=="ADD"||codeType=="SUB"||codeType=="MUL"||codeType=="DIV"||codeType=="REM"||
       codeType=="UADD"||codeType=="USUB"||codeType=="UMUL"||codeType=="UDIV"||
       codeType=="FADD"||codeType=="FSUB"||codeType=="FMUL"||codeType=="FDIV"||
       codeType=="NOT"||codeType=="OR"||codeType=="AND"||codeType=="XOR"||
       codeType=="SAL"||codeType=="SAR"||codeType=="MOV"||codeType=="INIT"||
       codeType=="ITF"||codeType=="UITF"||codeType=="FTI"||codeType=="BTD"||codeType=="WTD"){
        //[0]目标操作数,[1]/[2]源操作数
        for(int i = 0;i<code.args.length();i++){
            tmp.rw = i==0;
            tmp.operText = code.args[i];
            tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
            oper.append(tmp);
        }
    }else if(codeType=="INIT64"){
        //[0]/[1]目标操作数
        tmp.rw = 1;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="EC"||codeType=="NEC"||
             codeType=="LC"||codeType=="MC"||
             codeType=="LEC"||codeType=="MEC"||
             codeType=="UEC"||codeType=="UNEC"||
             codeType=="ULC"||codeType=="UMC"||
             codeType=="ULEC"||codeType=="UMEC"||
             codeType=="FEC"||codeType=="FNEC"||
             codeType=="FLC"||codeType=="FMC"||
             codeType=="FLEC"||codeType=="FMEC"){
        //[0]/[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="PUSH"){
        //[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);
    }else if(codeType=="RD" || codeType=="SRD"){
        //[1]目标操作数,[2]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="WE" || codeType=="SWE"){
        //[1]/[2]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);
    }else if(codeType=="ADDRESS"){

        //[0]目标操作数
        tmp.rw = 1;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);


        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = 0;
        oper.append(tmp);


        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="ARRSTORE"){
        //[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[3];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="ARRLOAD"){
        //[1]目标操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[3];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="COPYARR" || codeType=="STDCALL" || codeType=="CALL"){
        return oper;
    }else if(codeType=="PUSHARRAY" || codeType=="JNE" || codeType=="JMP" || codeType=="LABEL"){
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="CLEAR"){
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = 0;
        oper.append(tmp);
    }else{
        return oper;
    }
    return oper;
}


//物理寄存器使用情况
class PhyRegUsage{
public:
    QString regName;//物理寄存器名
    bool isUsing = 0;//是否正被变量占用中
    bool isUpdate = 0;//存在寄存器中的变量数值是否被改变过
    VarUse * useVar=NULL;//使用该物理寄存器的局部变量信息

    bool isUsed = 0;//该寄存器是否在整个函数运行中有曾被占用过
};
//局部变量池[如果是fpu集成在cpu中的,那么函数指令编译时就让浮点变量与整型变量共用一个VariablePool,否则分别放在2个VariablePool]
class VariablePool{
public:
    struct OrderOper{
         QString type;//指令类型
         QList<CodeOperand> opers;//指令的参数信息


         QString toString(){
             QString tmp = type + " ";
             QStringList list;
             for(int i = 0;i<opers.length();i++){
                 list.append(opers[i].operText);
             }
             tmp.append(list.join(","))+";";
             return tmp;
         }

         //指令中是否使用了指定的变量
         bool isUseVar(QString varName){
             for(int i = 0;i<opers.length();i++){
                 if(opers[i].isValue && opers[i].operText == varName){
                     return 1;
                 }
             }
             return 0;
         }
         //如果变量使用存在,获取该变量是用于源操作数还是目标操作数(从指令操作数右至左搜索)
         //0:作源操作数 1:作目标操作数
         bool getFistUseVarType(QString varName){
             for(int i = opers.length()-1;i>=0;i--){
                 if(opers[i].isValue && opers[i].operText == varName){
                     return opers[i].rw;
                 }
             }
             return 0;
         }
         bool getLastUseVarType(QString varName){
             for(int i = 0;i<=opers.length();i++){
                 if(opers[i].isValue && opers[i].operText == varName){

                     return opers[i].rw;
                 }
             }
             return 0;
         }
    };

    struct OrderOpersSegs:public QList<OrderOper>{
        bool isHaveBeginLabel = 0;
        QString blockBeginLabelName;//其它段跳转到该段起始位置的标记
        bool isHaveGoto = 0;
        CodeSegGotoLabelList blockAllGotoLabels;//该段跳转到其它段的所有标记的名称
        QString blockEndGotoLabel;//段尾结束跳的标记的名称
        bool isVisible = 0;

        QString toString(){
            QString tmp;
            if(isHaveBeginLabel){
                tmp+="Begin["+blockBeginLabelName+"]\r\n";
            }
            if(isHaveGoto){
                tmp+="Goto["+blockAllGotoLabels.toString()+"]\r\n";
                tmp+="End["+blockEndGotoLabel+"]\r\n";
            }
            for(int i = 0;i<this->length();i++){
                tmp += this->operator[](i).toString()+"\r\n";
            }
            return tmp;

        }

        //判断当前段中是否使用了指定的变量,并返回首次使用该变量时是作为目标操作数还是源操作数
        //-1:未使用 0:作源操作数 1:作目标操作数
        int getFistUseVarType(QString varName){
            for(int i = 0;i<this->length();i++){
                if(this->operator[](i).isUseVar(varName)){
                    return this->operator[](i).getFistUseVarType(varName);
                }
            }
            return -1;
        }

        int getLastUseVarType(QString varName){
            for(int i = this->length()-1;i>=0;i--){
                if(this->operator[](i).isUseVar(varName)){
                    return this->operator[](i).getFistUseVarType(varName);
                }
            }
            return -1;
        }
    };


    //初始化局部变量池,传入cpu支持的所有通用运算寄存器的名称以及定义的所有局部变量
    //(funNode传入函数指令列表的节点属性
    VariablePool(QStringList regNames,
                 QList<VarUse>&vars,
                 QList<MiddleNode>&bodyCodeList,
                 QList<int> stackTopBuoy = QList<int>()){
        for(int i = 0;i<regNames.length();i++){
            PhyRegUsage reg;
            reg.regName = regNames[i];
            regs.append(reg);
        }
        this->vars = vars;
        QList<CodeSegment> tmp = splitCodeSegment(bodyCodeList);
        for(int i = 0;i<tmp.length();i++){
            OrderOpersSegs seg;
            for(int j = 0;j<tmp[i].length();j++){
                OrderOper oper;
                oper.type = tmp[i][j].nodeType;
                oper.opers = getCodeOperand(tmp[i][j],this->vars);
                seg.append(oper);
            }
            seg.isVisible = tmp[i].isVisible;
            seg.isHaveGoto = tmp[i].isHaveGoto;
            seg.isHaveBeginLabel = tmp[i].isHaveBeginLabel;
            seg.blockAllGotoLabels = tmp[i].blockAllGotoLabels;
            seg.blockEndGotoLabel = tmp[i].blockEndGotoLabel;
            seg.blockBeginLabelName = tmp[i].blockBeginLabelName;
            this->bodyCodeSegs.append(seg);
        }


        this->stackTopBuoy = stackTopBuoy;

        for(int i = 0;i<this->vars.length();i++){
            if(this->vars[i].isRetrievedAddress || this->vars[i].type=="ARRAY" || this->vars[i].isArg){
                this->vars[i].isUseStack = 1;
            }
        }
    }

    //根据名称,获取一个变量信息的对象。找不到返回NULL
    VarUse* getVarDefine(QString varName){
        for(int i = 0;i<vars.length();i++){
            if(vars[i].varName==varName){
                return &vars[i];
            }
        }
        return NULL;
    }


    //尝试为一个指令中所用到的一个变量分配所使用的物理寄存器,返回分配到的寄存器名
    //valueName变量名 proRegs禁止分配的寄存器
    //rwOper:申请使用该寄存器是完成 0.用于暂存指令的源操作数 1.用于暂存指令的目标操作数
    //codeIndex:当前指令是整个函数指令中的第几个(用于变量开销/临近值的计算)
    //proRegs:避开使用的寄存器
    //isBeenAlloc:返回是否执行该函数前已经被分配了。如果已经被分配了，那就无需再进行分配
    //isSeize:返回是否是抢占的其它变量使用的寄存器
    //seizeVar:返回被抢占变量的指针
    //seizeVarIsUpdate:返回抢占的变量是否在寄存器中改变过值
    //返回值:正常返回分配到使用权的物理寄存器指针。如果返回NULL说明分配出错
    PhyRegUsage* typeAllocReg(QString varName,
                         bool rwOper,
                         int segIndex,int offset,
                         QStringList proRegs,
                         bool &isBeenAlloc,
                         bool &isSeize,
                         VarUse**seizeVar,
                         bool &seizeVarIsUpdate){
        isSeize = 0;
        seizeVarIsUpdate =0;
        isBeenAlloc = 1;
        //判断该变量是否已经被分配了,如果是直接返回当前变量被分配到的寄存器名
        for(int i = 0;i<regs.length();i++){
            if(regs[i].isUsing){
                if(regs[i].useVar->varName==varName){
                    regs[i].isUpdate = regs[i].isUpdate||rwOper;
                    return &regs[i];
                }
            }
        }

        isBeenAlloc = 0;

        //如果该变量不存在,直接退出
        if(getVarDefine(varName)==NULL){
            return NULL;
        }

        //尝试寻找接下来开销值为0,且无需入栈保存的临时变量寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            if(regs[i].isUsing){
                VarUse* useVar = regs[i].useVar;
                if(getOverhead(useVar->varName,segIndex,offset)!=0)continue;//开销值不为0
                if(checkVarIsStoreToStack(useVar->varName,segIndex,offset))continue;//需要入栈保存
                //开销为0且不需要入栈保存
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }



        //尝试寻找未被分配的寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            if(!regs[i].isUsing){

                //找到了未被分配的寄存器,将该寄存器分配给当前变量
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }

        isSeize = 1;


        //当前找到的最优选项
        /*最优算法的参数:
         * 1.后续顺序代码段中该变量使用的次数最少
         * 2.距离下一次使用该变量间隔几条指令
         *
         * 要求使用次数最少，其次间隔的指令尽可能多
         *
         *
         */
        QString optimalReg;
        uint optimalProximity=0;
        uint optimalOverhead=0xffffffff;
        //寻找最优的暂存操作数的寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            //该寄存器当前使用的var
            VarUse* useVar = regs[i].useVar;
            uint proximity = getProximity(useVar->varName,segIndex,offset);//临近值
            uint overhead = getOverhead(useVar->varName,segIndex,offset);//开销值
            if(proximity==0xffffffff || overhead==0xffffffff)return NULL;
            if(proximity==0)continue;

            if((overhead<optimalOverhead)||
               (overhead==optimalOverhead && proximity>optimalProximity)){
                optimalReg = regs[i].regName;
                optimalProximity = proximity;
                optimalOverhead = overhead;
            }
        }

        if(optimalReg==""){
            //分配失败
            return NULL;
        }

        for(int i = 0;i<regs.length();i++){
            if(regs[i].regName == optimalReg){
                //找到了最优选择的寄存器,将该寄存器分配给当前变量
                if(seizeVar!=NULL){
                    *seizeVar = regs[i].useVar;
                }
                seizeVarIsUpdate = regs[i].isUpdate;
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }
        return NULL;
    }

    //获取一个变量是否是数组
    bool varIsArray(QString varName){
        VarUse* var = getVarDefine(varName);
        if(var==NULL)return 0;
        return var->varName=="ARRAY";
    }

    //获取一个变量是否是要使用堆栈空间的(数组始终为是,被取过地址的变量也始终为是)
    bool getVarIsUseStackPlace(QString varName){
        VarUse* var = getVarDefine(varName);
        if(var==NULL)return 0;
        return var->isUseStack;
    }

    //在一个变量可能需要写入堆栈内存时判断该变量是否必须强制存在栈中
    //例如: sub a,b,c; 执行完毕后,继续执行其它指令时发现寄存器不够用了。
    //此时寄存器分配算法选择将a缓存到内存中,来让出寄存器使用权。此时判断a是否有必要一定得存入栈中,如果是非必要的,那么可以直接将a的数据给舍弃掉
    bool checkVarIsStoreToStack(QString varName,int segIndex,int offset){
        VarUse* useVar = getVarDefine(varName);
        if(useVar==NULL)return false;
        if(useVar->isRetrievedAddress)return 1;


        //搜索当前指令接下来的指令,是否有首次使用该变量作为指令的源操作数。如果有,说明该变量必须存入栈中
        for(int i = offset+1;i<bodyCodeSegs[segIndex].length();i++){
            if(!bodyCodeSegs[segIndex][i].isUseVar(varName))continue;
            bool useType = bodyCodeSegs[segIndex][i].getFistUseVarType(varName);
            if(useType){
                return 0;
            }else{
                useVar->isUseStack = 1;
                return 1;
            }
        }

        //搜索当前指令段可能跳转到的所有段,如果在段中首次使用该变量作为指令的源操作数,说明该变量必须存入栈中
        if(getAllSegIsStoreVar(varName,segIndex,offset)){
            useVar->isUseStack = 1;
            return 1;
        }

        return 0;
    }

    //在一个变量可能需要从堆栈中内存中加载时,判断该变量是否需要强制在内存中进行加载
    //例如: mov b,a;在执行该指令时,a的数据不在寄存器中。此时就要判断a是否一定得从栈中加载出来使用
    //如果a不是一个参数,并且a变量在此前并没有被赋过值,就说明该变量可以不去从栈中加载,而是直接使用寄存器中的无效值进行运算(写程序时坚决不能如此操作,否则将导致程序运行的不稳定)
    //反之,则a就必须从栈中加载保存的缓存值
    bool checkVarIsLoadFromStack(QString varName,int segIndex,int offset){
        VarUse* useVar = getVarDefine(varName);
        if(useVar==NULL)return false;
        if(useVar->isRetrievedAddress || useVar->isArg)return 1;


        //搜索当前指令接之前的指令,是否有首次使用该变量作为指令的目标操作数。如果有,说明该变量必须从栈中加载
        for(int i = offset-1;i>=0;i--){
            if(!bodyCodeSegs[segIndex][i].isUseVar(varName))continue;
            bool useType = bodyCodeSegs[segIndex][i].getLastUseVarType(varName);
            if(useType){
                useVar->isUseStack = 1;
                return 1;
            }
        }

        if(getAllSegIsLoadVar(varName,segIndex,offset)){
            useVar->isUseStack = 1;
            return 1;
        }
        return  0;
    }


    //生成其中一段指令的低级代码
    QList<MiddleNode> generateSegLowCode(OrderOpersSegs&segOrders,int segIndex,bool &status){
        QList<MiddleNode>replceBodyCodes;

        bool endIsJmpGoto = 0;//段末尾是否是jmp跳转
        QString jmpLabel;//如果是jmp跳转,jmp跳的地址

        for(int i = 0;i<segOrders.length();i++){
            status = 0;
            QString codeType = segOrders[i].type;//指令类型


            QList<MiddleNode> prefixOper;
            if(codeType=="JNE"){
                //jne是一大段中的子小段,所以也需要执行段尾数据保存的操作
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing&&regs[j].isUpdate){
                        if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                            prefixOper.append(generateLoadStoreValueCode(
                                                       1,
                                                       regs[j].useVar->varName,
                                                       regs[j].useVar->type,
                                                       regs[j].regName
                                                       ));
                        }

                        regs[j].isUpdate = 0;
                    }
                    regs[j].isUsing = 0;
                }
                replceBodyCodes.append(prefixOper);
                MiddleNode operNode;
                operNode.nodeType = codeType;
                operNode.args = QStringList({segOrders[i].opers[0].operText});
                replceBodyCodes.append(operNode);
                continue;
            }else if(codeType=="JMP"){
                endIsJmpGoto = 1;
                jmpLabel = segOrders[i].opers[0].operText;
                break;
            }else if(codeType=="CALL" || codeType=="STDCALL"){
                //将当前寄存器中所有变更过并且被取过地址的，则存入栈中的本体
                //将所有被取过地址的寄存器解除与变量的映射
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing){
                        if(!regs[j].useVar->isRetrievedAddress)continue;
                        if(regs[j].isUpdate){
                            if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                                prefixOper.append(generateLoadStoreValueCode(
                                                           1,
                                                           regs[j].useVar->varName,
                                                           regs[j].useVar->type,
                                                           regs[j].regName
                                                           ));
                                regs[j].isUpdate = 0;
                            }
                        }
                        regs[j].isUsing = 0;
                    }
                }
                replceBodyCodes.append(prefixOper);
                MiddleNode operNode;
                operNode.nodeType = codeType;
                replceBodyCodes.append(operNode);
                continue;
            }else if(segOrders[i].opers.length()==0){
                //没有参数的指令原样不变
                MiddleNode operNode;
                operNode.nodeType = codeType;
                replceBodyCodes.append(operNode);
                continue;
            }
            if(codeType=="RD" || codeType=="WE"){
                //将当前寄存器中所有变更过并且被取过地址的，存入栈中的本体。并设为没有变更过的
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing&&regs[j].isUpdate){
                        if(!regs[j].useVar->isRetrievedAddress)continue;

                        if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                            prefixOper.append(generateLoadStoreValueCode(
                                                       1,
                                                       regs[j].useVar->varName,
                                                       regs[j].useVar->type,
                                                       regs[j].regName
                                                       ));
                        }

                        regs[j].isUpdate = 0;
                    }
                }
            }


            QList<CodeOperand> opers = segOrders[i].opers;
            QList<MiddleNode> loadStorVarCode;
            QStringList allOperReg;
            //将指令中使用的各个变量替换为分配到的寄存器
            for(int j = opers.length()-1;j>=0;j--){
                if(!opers[j].isValue)continue;
                bool isBeenAlloc;//当前变量是否已经在寄存器了,无需进行映射
                bool isSeize;    //未当前变量分配的寄存器是否是此前映射到了其它的变量
                VarUse* seizeVar;//如果当前变量是抢占了其它变量使用的寄存器,被抢占的变量名
                bool seizeVarHasUpdate;//被抢占的变量在寄存器中数据是否有过更改
                PhyRegUsage* allocReg = typeAllocReg(
                                opers[j].operText,
                                opers[j].rw,
                                segIndex,i,
                                allOperReg,
                                isBeenAlloc,
                                isSeize,
                                &seizeVar,
                                seizeVarHasUpdate);
                if(allocReg==NULL){
                    return replceBodyCodes;
                }
                allOperReg.append(allocReg->regName);

                //在寄存器中则直接继续使用这个寄存器即可
                opers[j].operText = allocReg->regName;

                if(!isBeenAlloc && !isSeize){
                    //如果不在寄存器中,但是分配到了空闲的寄存器
                    //且当前变量是源操作数则将其从内存加载到寄存器。目标操作数则无需加载
                    if(!opers[j].rw){
                        if(checkVarIsLoadFromStack(allocReg->useVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       0,
                                                       allocReg->useVar->varName,
                                                       allocReg->useVar->type,
                                                       allocReg->regName
                                                       ));
                        }

                    }
                }else if(!isBeenAlloc && isSeize){


                    //如果不在寄存器中，且是抢占的另一个变量的寄存器使用权
                    //如果被抢占的变量在寄存器中有过变更,将变更的数据写入寄存器
                    if(seizeVarHasUpdate){
                        if(checkVarIsStoreToStack(seizeVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       1,
                                                       seizeVar->varName,
                                                       seizeVar->type,
                                                       allocReg->regName
                                                       ));
                        }
                    }
                    //当前变量是源操作数则将其从内存加载到寄存器。目标操作数则无需加载
                    if(!opers[j].rw){
                        if(checkVarIsLoadFromStack(seizeVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       0,
                                                       allocReg->useVar->varName,
                                                       allocReg->useVar->type,
                                                       opers[j].operText
                                                       ));
                        }
                    }
                }

            }

            //生成指令的代码
            MiddleNode operNode;
            operNode.nodeType = codeType;
            for(int j = 0;j<opers.length();j++){
                operNode.args.append(opers[j].operText);
            }



            if(codeType=="WE"){
                //将当前寄存器中所有被取过地址的解除变量与其的映射
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing){
                        if(!regs[j].useVar->isRetrievedAddress)continue;
                        regs[j].isUsing = 0;
                    }
                }
            }
            replceBodyCodes.append(prefixOper+loadStorVarCode);
            replceBodyCodes.append(operNode);
        }

        //段尾处理
        //将所有在寄存器中变更过数据的变量全部存入堆栈
        //同时将寄存器与变量的映射全部解除
        QList<MiddleNode> prefixOper;
        for(int j = 0;j<regs.length();j++){
            if(regs[j].isUsing&&regs[j].isUpdate){
                if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,segOrders.length()-1)){
                    prefixOper.append(generateLoadStoreValueCode(
                                               1,
                                               regs[j].useVar->varName,
                                               regs[j].useVar->type,
                                               regs[j].regName
                                               ));
                }

                regs[j].isUpdate = 0;
            }
            regs[j].isUsing = 0;
        }
        replceBodyCodes.append(prefixOper);
        if(endIsJmpGoto){
            MiddleNode operNode;
            operNode.nodeType = "JMP";
            operNode.args = QStringList({jmpLabel});
            replceBodyCodes.append(operNode);
        }
        /////////////////////////////////////////////////
        status = 1;
        return replceBodyCodes;
    }


    //生成低级代码:将代码中使用的所有变量映射到寄存器,并生成读写堆栈变量本体的代码
    QList<MiddleNode> generateLowCode(bool &status){
        QList<MiddleNode>replceBodyCodes;
        for(int i = 0;i<bodyCodeSegs.length();i++){
            replceBodyCodes.append(generateSegLowCode(bodyCodeSegs[i],i,status));
            if(status==0)return replceBodyCodes;
        }
        return replceBodyCodes;
    }

    //获取所有在程序中使用到的寄存器名称
    QStringList getAllUsedRegNames(){
        QStringList names;
        for(int i = 0;i<regs.length();i++){
            if(regs[i].isUsed){
                names.append(regs[i].regName);
            }
        }
        return names;
    }



    QList<VarUse> vars;//所有局部变量
    QList<PhyRegUsage> regs;  //所有物理寄存器

    QList<OrderOpersSegs>bodyCodeSegs; //函数指令段
    QList<int> stackTopBuoy;  //栈顶浮动字节序列(与函数指令一一对应,记录了函数中每条指令时栈顶的浮动变化)

private:
    //已一个段为起始点,搜索出所有该段可能是从哪些段跳转来的,判断是否在这些段中是否有使用指定的变量为目标操作数
    bool seachAllSegIsLoadVar(QString varName,OrderOpersSegs&thisSeg,QList<OrderOpersSegs> &allSegment,int rootSegIndex,int rootLine){
        if(!thisSeg.isHaveBeginLabel)return 0;
        QString fromLabel = thisSeg.blockBeginLabelName;
        for(int i = 0;i<allSegment.length();i++){
            if(allSegment[i].isVisible || !allSegment[i].isHaveGoto)continue;
            if(allSegment[i].blockAllGotoLabels.contains(fromLabel)){
                int startSeachIndex = 0;
                if(allSegment[i].blockEndGotoLabel == fromLabel){
                    //从段末尾开始向上排查
                    startSeachIndex = allSegment[i].length()-1;

                }else{
                    //从段末尾,寻找到首个跳转到fromLabel的jne指令开始排查
                    for(int j = 0;j<allSegment[i].blockAllGotoLabels.length();j++){
                        if(allSegment[i].blockAllGotoLabels.labels[j].gotoLabelName != fromLabel)continue;

                        if(allSegment[i].blockAllGotoLabels.labels[j].orderIndex > startSeachIndex){
                            startSeachIndex = allSegment[i].blockAllGotoLabels.labels[j].orderIndex;
                        }
                    }
                }
                //搜索段中,是否有将该变量做目标操作数
                for(int j = startSeachIndex;j>=0;j--){
                    if(rootSegIndex == i && j==rootLine-1){
                        allSegment[i].isVisible = 1;
                        goto next;
                    }
                    if(!allSegment[i][j].isUseVar(varName)){
                        continue;
                    }
                    if(allSegment[i][j].getLastUseVarType(varName)){
                        return 1;
                    }
                }

                //向下递归搜索下一个段
                allSegment[i].isVisible = 1;
                if(seachAllSegIsLoadVar(varName,allSegment[i],allSegment,rootSegIndex,rootLine)){
                    return 1;
                }
                next:
                continue;
            }
        }

        return 0;
    }
    //判断当前所有的段中,是否有修改当前变量的值
    bool getAllSegIsLoadVar(QString varName,int rootSegIndex,int rootLine){
        for(int i = 0;i<bodyCodeSegs.length();i++){
            bodyCodeSegs[i].isVisible = 0;
        }
        return seachAllSegIsLoadVar(varName,bodyCodeSegs[rootSegIndex],bodyCodeSegs,rootSegIndex,rootLine);
    }
    //已一个段为起始点,搜索出所有该段可能跳转到的段,判断是否在段中首次使用该变量作为指令的源操作数
    bool seachAllSegIsStoreVar(QString varName,OrderOpersSegs&thisSeg,bool isSeachRoot,int thisSegIndex,QList<OrderOpersSegs> &allSegment,int rootSegIndex,int rootLine){

        if(!thisSeg.isHaveGoto)return 0;
        CodeSegGotoLabelList gotoLabel = thisSeg.blockAllGotoLabels;
        if(thisSegIndex == rootSegIndex){

            for(int i = 0;i<gotoLabel.length();i++){
                int index = gotoLabel.labels[i].orderIndex;
                if(index==-1){
                    index = thisSeg.length();
                }
                if(isSeachRoot? index<rootLine : index>rootLine){
                    gotoLabel.labels.removeAt(i);
                    i--;
                }
            }
        }
        for(int i = 0;i<allSegment.length();i++){
            if(allSegment[i].isVisible || !allSegment[i].isHaveBeginLabel)continue;
            if(gotoLabel.contains(allSegment[i].blockBeginLabelName)){
                int useType;
                if(rootSegIndex==i){
                    useType = -1;
                    for(int j = 0;j<=rootLine;j++){
                        if(allSegment[i][j].isUseVar(varName)){
                            useType = allSegment[i][j].getFistUseVarType(varName);
                            break;
                        }
                    }
                }else{
                    useType = allSegment[i].getFistUseVarType(varName);
                }
                if(useType==0){
                    //首次是用作源操作数,那无需向下搜索了,说明在该段中并首次使用该变量的数据,就必须要存入堆栈
                    return 1;
                }else if(useType==-1){
                    //如果全程都没有使用该操作数,向下搜索该段可能跳转到的段
                    allSegment[i].isVisible = 1;
                    if(seachAllSegIsStoreVar(varName,allSegment[i],0,i,allSegment,rootSegIndex,rootLine)){
                        return 1;
                    }
                }//如果首次是用作目标操作数,那无需向下搜索了,说明在该段中并不需要再次使用该变量的数据,可以不需要存入堆栈
            }
        }
        return 0;
    }
    //判断当前所有的段中,是否依赖当前段中对该变量所修改的数据
    bool getAllSegIsStoreVar(QString varName,int rootSegIndex,int rootLine){
        for(int i = 0;i<bodyCodeSegs.length();i++){
            bodyCodeSegs[i].isVisible = 0;
        }

        bool ret = seachAllSegIsStoreVar(varName,bodyCodeSegs[rootSegIndex],1,rootSegIndex,bodyCodeSegs,rootSegIndex,rootLine);
        return ret;
    }
    //获取指定的变量的开销值
    uint getOverhead(QString valueName,int segIndex,int offset){
        uint num = 0;
        if(segIndex==-1)return -1;
        for(int i = offset;i<bodyCodeSegs[segIndex].length();i++){
            if(bodyCodeSegs[segIndex][i].type=="JNE"){
                return num;
            }
            if(bodyCodeSegs[segIndex][i].isUseVar(valueName)){
                num++;
            }
        }
        return num;
    }

    //获取指定的变量的临近值(如果当前连续程序块内没有任何使用该变量的话，返回0xffffffff)
    uint getProximity(QString valueName,int segIndex,int offset){
        if(segIndex==-1)return -1;
        for(int i = offset;i<bodyCodeSegs[segIndex].length();i++){
            if(bodyCodeSegs[segIndex][i].type=="JNE"){
                return 0xffffffff;
            }
            if(bodyCodeSegs[segIndex][i].isUseVar(valueName)){
                return i;
            }
        }
        return 0xffffffff;
    }
    //获取一条指令属于哪个段的第几条
    int getThisCodeFromSegIndex(int lineIndex,int &segOffset){
        int allLine = 0;
        for(int i = 0;i<bodyCodeSegs.length();i++){
            segOffset = lineIndex-allLine;
            allLine += bodyCodeSegs[i].length();
            if(lineIndex<allLine){
                return i;
            }
        }
        return -1;
    }

    //生成从堆栈中变量与寄存器交换数据的指令
    //isStore:是将寄存器数据存入变量,还是从变量取到寄存器(0取出,1存入)
    //stackOffset:变量相对于栈顶的偏移
    //type:变量的数据类型
    //reg: 要取到的寄存器名
    MiddleNode generateLoadStoreValueCode(bool isStore,QString varName,QString type,QString reg){
        MiddleNode code;
        code.nodeType = isStore ? "SWE" : "SRD";
        QString rwBytes;
        if(type=="UBYTE"||type=="BYTE"){
            rwBytes = "BYTE";
        }else if(type=="USHORT"||type=="SHORT"){
            rwBytes = "WORD";
        }else if(type=="UINT"||type=="INT"||type=="FLOAT"){
            rwBytes = "DWORD";
        }
        QString rwAddress = "<offset:"+varName+">";
        code.args = QStringList({rwBytes,reg,rwAddress});
        return code;
    }
};


//生成的结果= |{name}num| 通过allGotoLabel生成一个num数字序号,防止出现重复标记名
QString generateJumpLabel(QString name,QStringList &allGotoLabel){
    int i = 0;
    while(allGotoLabel.contains("|"+name+QString::number(i)+"|")){
        i++;
    }
    return "|"+name+QString::number(i)+"|";
}

//扁平化流程块
void flatProcessBlock(MiddleNode_Att &codes,QStringList &allGotoLabel,bool&status){
    /*
     * PROCESS(比较运算指令,a,b).IF{
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
     *      JNE([this.|IF_ELSE{num}|])
     *      //比较值!=0 是执行的代码
     *      JMP([this.|IF_END{num}|]);
     *      LABEL([IF_END{num}|);
     *      //比较值==0 是执行的代码
     *      LABEL(|IF_ELSE{num}|);
     * }
     *
     *只存在else块
     * {
     *      //判断是执行if还是else块的代码
     *      比较运算指令(a,b);
     *      JNE([this.|IF_ELSE{num}|]);
     *      JMP([this.|IF_END{num}|]);
     *      LABEL(|IF_ELSE{num}|);
     *      //比较值==0 是执行的代码
     *      LABEL(|IF_END{num}|);
     * }
     *
     * 只存在if块
     * {
     *      //判断是执行if还是else块的代码
     *      比较运算指令(a,b);
     *      JNE([this.|IF_END{num}|])
     *      //比较值!=0 执行的代码
     *      LABEL(|IF_END{num}|);
     * }
     *
     *
     */

    for(int i = 0;i<codes.subNodes.length();i++){

        MiddleNode &thisNode = codes.subNodes[i];
        if(thisNode.nodeType=="PROCESS"){
            if(thisNode.args.length()!=3 || thisNode.areThereOnlyTheseAtt({"IF","ELSE"})){
                status=false;
                return;
            }
            QList<MiddleNode> flatCodes;
            MiddleNode_Att* ifCodes = thisNode.getAttFromName("IF");
            MiddleNode_Att* elseCodes = thisNode.getAttFromName("ELSE");
            if(ifCodes==NULL && elseCodes==NULL){
                status = false;
                return;
            }

            MiddleNode cmpCode;
            cmpCode.nodeType=thisNode.args[0].toUpper();
            cmpCode.args.append({thisNode.args[1],thisNode.args[2]});
            flatCodes.append(cmpCode);

            MiddleNode ifgotoCode;
            ifgotoCode.nodeType="JNE";
            QString gotoLabel = generateJumpLabel((ifCodes==0?"IF_END":"IF_ELSE"),allGotoLabel);
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
                gotoLabel = generateJumpLabel("IF_END",allGotoLabel);
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
//tempVarIndex:使用第几个临时变量(会自动由编译器在堆栈中创建名为 __temp{tempVarIndex}__ 的变量)
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
                                     bool isHaveCallFun,
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

    if(isHaveCallFun){
        tmp.nodeType = "POP";
        tmp.args = QStringList({"DWORD","TPC"});
        retCodes.append(tmp);
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

struct Call64bitOperFun_ArgInfo{
    QString operText;
    bool isConstValue;
    uint constValueSize;
    uint argIs64Bit;
};

//生成调用64位操作函数的指令表
QList<MiddleNode> generateCall64bitOperFunCode(QString funName,
                                               QList<Call64bitOperFun_ArgInfo> args,
                                               QString returnPtrName,
                                               bool retIs64,
                                               QList<VarUse> &varUseInfo){
    if(args.length()>2 || args.length()==0)return QList<MiddleNode>();

    QList<MiddleNode> codeList;
    MiddleNode tmp;

    for(int i = args.length()-1;i>=0;i--){
        if(args[i].isConstValue){
            if(args[i].argIs64Bit){
                tmp.nodeType = "INIT64";
                QString tmp0 = getTempVar(varUseInfo);
                QString tmp1 = getTempVar(varUseInfo);
                tmp.args = QStringList({tmp0,tmp1,args[i].operText});
                codeList.append(tmp);
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",tmp0});
                codeList.append(tmp);
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",tmp1});
                codeList.append(tmp);
            }else{
                if(args[i].constValueSize>16){
                    tmp.nodeType = "INIT";
                    QString tmp0 = getTempVar(varUseInfo);
                    tmp.args = QStringList({tmp0,args[i].operText});
                    codeList.append(tmp);
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",tmp0});
                    codeList.append(tmp);
                }else{
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",args[i].operText});
                    codeList.append(tmp);
                }
            }
        }else if(args[i].argIs64Bit){
            QStringList name = bit64VarTo32VarName(args[i].operText);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",name[1]});
            codeList.append(tmp);
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",name[0]});
            codeList.append(tmp);
        }else{
            tmp.nodeType = "PUSH";
            tmp.args = QStringList({"DWORD",args[i].operText});
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
    QWORD_MOV,//64位可直接赋值
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
        return QWORD_MOV;
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
        return QWORD_MOV;
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
        return QWORD_MOV;
    }else{
        if(sucess!=NULL){
            *sucess = false;
        }
        return 0;
    }
}

//将定义的标记点转为统一格式 MARK+编号,以此方便后续解析
void transMarkName_getAllLabelName(QList<MiddleNode> &codes,QStringList &allLabelNames){
    for(int i = 0;i<codes.length();i++){
        MiddleNode &thisCode = codes[i];

        if(thisCode.nodeType == "PROCESS"){
            MiddleNode_Att * ifAtt = thisCode.getAttFromName("IF");
            MiddleNode_Att * elseAtt = thisCode.getAttFromName("ELSE");
            if(ifAtt!=NULL){
                transMarkName_getAllLabelName(ifAtt->subNodes,allLabelNames);
            }
            if(elseAtt!=NULL){
                transMarkName_getAllLabelName(elseAtt->subNodes,allLabelNames);
            }
        }else if(thisCode.nodeType == "LABEL" && thisCode.args.length()==1){
            allLabelNames.append(thisCode.args[0]);
            thisCode.args[0] = "MARK"+QString::number(allLabelNames.length()-1);

        }
    }
}

//将定义的标记点转为统一格式 MARK+编号,以此方便后续解析
void transMarkName_resetGotoLabel(QList<MiddleNode> &codes,QStringList &allLabelNames){
    for(int i = 0;i<codes.length();i++){
        MiddleNode &thisCode = codes[i];
        if(thisCode.nodeType == "PROCESS"){
            MiddleNode_Att * ifAtt = thisCode.getAttFromName("IF");
            MiddleNode_Att * elseAtt = thisCode.getAttFromName("ELSE");
            if(ifAtt!=NULL){
                transMarkName_resetGotoLabel(ifAtt->subNodes,allLabelNames);
            }
            if(elseAtt!=NULL){
                transMarkName_resetGotoLabel(elseAtt->subNodes,allLabelNames);
            }
        }else if(thisCode.nodeType == "GOTO" && thisCode.args.length()==1){
            for(int j = 0;j<allLabelNames.length();j++){
                if(allLabelNames[j] == thisCode.args[0]){
                    thisCode.args[0] = "MARK"+QString::number(j);
                    break;
                }
            }
        }
    }
}

void transMarkName(QList<MiddleNode> &codes){
    QStringList allLabelNames;
    transMarkName_getAllLabelName(codes,allLabelNames);
    transMarkName_resetGotoLabel(codes,allLabelNames);
}

//判断数据类型是否相同
bool judgeArgsTypeIsIdentical(QList<OrderArg> args){
    if(args.length()==0)return 1;
    QString lastType = args[0].getType();
    if(lastType.length() == 0)return 0;
    if(lastType[0] == "U"){
        lastType.remove(0,1);
    }
    for(int i = 1;i<args.length();i++){
        QString type = args[i].getType();


        if(type.length() == 0)return 0;

        if(type[0] == "U"){
            type.remove(0,1);
        }



        if(type != lastType){
            return 0;
        }
    }
    return 1;
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

    //将定义的标记点转为统一格式 MARK+编号,以此方便后续解析
    transMarkName(codes->subNodes);

    //扁平化流程块
    flatProcessBlock(*codes,allGotoLabel,status);
    if(status==0)return;
    status = 0;

    //将定义的变量名转为统一格式 VAR+编号 ,以此方便后续解析
    {
        for(int i = 0;i<varUseInfo.length();i++){
            QString yName = varUseInfo[i].varName;
            QString tName = "VAR"+QString::number(i);
            varUseInfo[i].varName = tName;
            QList<MiddleNode> &allCodes = codes->subNodes;
            for(int j = 0;j<allCodes.length();j++){
                MiddleNode &thisCode = allCodes[j];
                if(thisCode.nodeType == "LABEL" ||
                   thisCode.nodeType == "GOTO" ||
                   thisCode.nodeType == "JNE"){
                    continue;
                }
                for(int k = 0;k<thisCode.args.length();k++){
                    QString &thisArg = thisCode.args[k];
                    if(thisArg.length()>=3 &&
                       thisArg[0] == "|"){
                        QStringList useArrInfo = thisArg.mid(1,thisArg.length()-2).split(":");
                        if(useArrInfo.length()!=2){
                            return;
                        }
                        if(useArrInfo[0] == yName){
                            useArrInfo[0] = tName;
                        }
                        thisArg = "|"+useArrInfo.join(":")+"|";
                    }else if(thisArg == yName){
                        thisArg = tName;
                    }
                }
            }
        }


    }

    //编译替换为低级指令
    QList<MiddleNode> codeList = codes->subNodes;

    QList<MiddleNode> replseList;

    bool isHaveCallFun = 0;//当前函数中是否有调用其它函数
    bool isHaveReturnHandle = 0;//当前函数是否有函数退出处理




    static const QString returnLabel =  "|RETURN|";
    allGotoLabel.append(returnLabel);
    //将函数调用/函数退出/跳转等无变量操作指令进行替换
    for(int i = 0;i<codeList.length();i++){
        status = 0;
        QString nodeType = codeList[i].nodeType;
        MiddleNode tmp;
        if(nodeType=="RETURN"){//函数返回
            tmp.nodeType = "JMP";
            tmp.args.append("[this."+returnLabel+"]");
            replseList.append(tmp);
            allGotoCodeJmpLabels.append(returnLabel);
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
            if(codeList[i].args[0]  ==  returnLabel){
                return;
            }
            if(!allGotoLabel.contains(codeList[i].args[0])){//重复定义跳转点
                allGotoLabel.append(codeList[i].args[0]);
            }


            replseList.append(codeList[i]);
        }else if(nodeType=="ARRCOPY"){//数组拷贝
            if(codeList[i].args.length() != 3)return;

            //传入函数参数(从右到左依次入栈)
            for(int j = codeList[i].args.length()-1;j>=0;j--){
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({codeList[i].args[j]});
                replseList.append(tmp);
            }

            //设置函数跳转地址为|MEMORY_DATA_COPY|系统内置函数
            tmp.nodeType = "CALL";
            tmp.args = QStringList({"[|MEMORY_DATA_COPY|]"});
            replseList.append(tmp);
            if(!operFun.contains("|MEMORY_DATA_COPY|")){
                operFun.append("|MEMORY_DATA_COPY|");
                allAddressLabel.prepend("|MEMORY_DATA_COPY|");
            }
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
    retLabelCodeNode.args = QStringList({returnLabel});
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

           if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL)return;
           QString Atype =A.getType();
           QString Btype =B.getType();
           QString Ctype =C.getType();
           if(Atype=="ARRAY"|| !judgeArgsTypeIsIdentical({A,B,C}))return;
           bool argB_isConstValue = B.useVar == NULL;
           bool argC_isConstValue = C.useVar == NULL;

           if(Atype=="DOUBLE" || Atype=="ULONG" || Atype=="LONG"){//64位运算
               QString funName = "|"+Atype+"_"+nodeType+"|";
               Call64bitOperFun_ArgInfo vb;
               vb.isConstValue = argB_isConstValue;
               vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
               vb.argIs64Bit = 1;
               vb.constValueSize = B.size;

               Call64bitOperFun_ArgInfo vc;
               vc.isConstValue = argC_isConstValue;
               vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
               vc.argIs64Bit = 1;
               vc.constValueSize = C.size;

               if(!operFun.contains(funName))operFun += funName;
               codeList += generateCall64bitOperFunCode(funName,
                                            {vb,vc},
                                            A.useVar->varName,
                                            1,
                                            varUseInfo
                                            );
           }else{//正常使用cpu硬件的运算
               QString orderName;
               if(Atype=="FLOAT"){
                   orderName = "F"+nodeType;
               }else if(Atype[0] == 'U'){
                   orderName = "U"+nodeType;
               }else{
                   orderName = nodeType;
               }


               QString vb_text,vc_text;
               if(argB_isConstValue){
                   vb_text = getTempVar(varUseInfo);
                   MiddleNode initCode;
                   initCode.nodeType = B.size >= 16 ? "INIT" : "MOV";
                   initCode.args = QStringList({vb_text,B.constValue});
                   codeList += initCode;
               }else{
                   vb_text = B.useVar->varName;
               }

               if(argC_isConstValue){
                   if(C.size <=12){
                       vc_text = C.constValue;
                   }else{
                       vc_text = getTempVar(varUseInfo);
                       MiddleNode initCode;
                       initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                       initCode.args = QStringList({vc_text,C.constValue});
                       codeList += initCode;
                   }
               }else{
                   vc_text = C.useVar->varName;
               }

                MiddleNode code;
                code.nodeType = orderName;
                code.args = QStringList({A.useVar->varName,vb_text,vc_text});
                codeList += code;
           }
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
            else if(!judgeArgsTypeIsIdentical({A,B,C}))return;

            bool argB_isConstValue = B.useVar == NULL;
            bool argC_isConstValue = C.useVar == NULL;

            if(Atype=="DOUBLE" || Atype=="ULONG" || Atype=="LONG"){//64位运算
                QString funName = "|"+Atype+"_"+nodeType+"|";
                Call64bitOperFun_ArgInfo vb;
                vb.isConstValue = argB_isConstValue;
                vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
                vb.argIs64Bit = 1;
                vb.constValueSize = B.size;

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vb,vc},
                                             A.useVar->varName,
                                             1,
                                             varUseInfo
                                             );
            }else{//正常使用cpu硬件的运算
                QString orderName;
                if(Atype[0] == 'U'){
                    orderName = "UDIV";
                }else{
                    orderName = "DIV";
                }
                QString vb_text,vc_text;
                if(argB_isConstValue){
                    vb_text = getTempVar(varUseInfo);
                    MiddleNode initCode;
                    initCode.nodeType = B.size >= 16 ? "INIT" : "MOV";
                    initCode.args = QStringList({vb_text,B.constValue});
                    codeList += initCode;
                }else{
                    vb_text = B.useVar->varName;
                }

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }


                 QString ret_text = getTempVar(varUseInfo);

                 MiddleNode code;
                 code.nodeType = orderName;
                 code.args = QStringList({ret_text,vb_text,vc_text});
                 codeList += code;

                 MiddleNode retVarCode;
                 retVarCode.nodeType = "MOV";
                 retVarCode.args = QStringList({A.useVar->varName,"FLAG"});
                 codeList += retVarCode;
            }
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
                QString funName ="|"+ B.getType() + "TO" + A.getType()+"|";
                bool retIs64 = mov_type==DWORD_T_QWORD || mov_type==QWORD_T_QWORD;

                bool argB_isConstValue = B.useVar == NULL;

                Call64bitOperFun_ArgInfo vb;
                vb.isConstValue = argB_isConstValue;
                vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
                vb.argIs64Bit = 1;
                vb.constValueSize = B.size;

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vb},
                                             A.useVar->varName,
                                             retIs64,
                                             varUseInfo
                                             );
            }else if(mov_type==QWORD_MOV){
                QStringList v1Name = bit64VarTo32VarName(A.useVar->varName);
                if(B.useVar!=NULL){//非立即数运算
                    QStringList v2Name = bit64VarTo32VarName(B.useVar->varName);
                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({v1Name[1],v2Name[1]});
                    codeList.append(tmp);
                    tmp.nodeType = "MOV";
                    tmp.args = QStringList({v1Name[0],v2Name[0]});
                    codeList.append(tmp);
                }else{
                    if(B.size>32){
                        //初始化
                        tmp.nodeType = "INIT64";
                        tmp.args = QStringList({v1Name[1],v1Name[0],B.constValue});
                        codeList.append(tmp);
                    }else if(B.size>16){
                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({v1Name[1],"0D"});
                        codeList.append(tmp);

                        tmp.nodeType = "INIT";
                        tmp.args = QStringList({v1Name[0],B.constValue});
                        codeList.append(tmp);
                    }else{
                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({v1Name[1],"0D"});
                        codeList.append(tmp);

                        tmp.nodeType = "MOV";
                        tmp.args = QStringList({v1Name[0],B.constValue});
                        codeList.append(tmp);
                    }
                }
            }else return;

        }else if(nodeType=="SAL"||
                 nodeType=="SAR"||
                 nodeType=="OR"||
                 nodeType=="AND"||
                 nodeType=="XOR"){
            if(thisCode.args.length()!=3){
                return;
            };
            //获取运算参数
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);

            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(Atype=="ARRAY"||Atype=="FLOAT"||Atype=="DOUBLE"||!judgeArgsTypeIsIdentical({A,B,C}))return;
            bool argB_isConstValue = B.useVar == NULL;
            bool argC_isConstValue = C.useVar == NULL;

            if(Atype=="ULONG" || Atype=="LONG"){//64位运算
                QString funName = "|"+Atype+"_"+nodeType+"|";
                Call64bitOperFun_ArgInfo vb;
                vb.isConstValue = argB_isConstValue;
                vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
                vb.argIs64Bit = 1;
                vb.constValueSize = B.size;

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vb,vc},
                                             A.useVar->varName,
                                             1,
                                             varUseInfo
                                             );
            }else{//正常使用cpu硬件的运算
                QString orderName = nodeType;

                QString vb_text,vc_text;
                if(argB_isConstValue){
                    vb_text = getTempVar(varUseInfo);
                    MiddleNode initCode;
                    initCode.nodeType = B.size >= 16 ? "INIT" : "MOV";
                    initCode.args = QStringList({vb_text,B.constValue});
                    codeList += initCode;
                }else{
                    vb_text = B.useVar->varName;
                }

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }

                 MiddleNode code;
                 code.nodeType = orderName;
                 code.args = QStringList({A.useVar->varName,vb_text,vc_text});
                 codeList += code;
            }
        }else if(nodeType=="NOT"){
            if(thisCode.args.length()!=2){
                return;
            };
            //获取运算参数
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);

            if(A.isValid==0 || C.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Ctype =C.getType();
            if(Atype=="ARRAY"||Atype=="FLOAT"||Atype=="DOUBLE"||!judgeArgsTypeIsIdentical({A,C}))return;
            bool argC_isConstValue = C.useVar == NULL;

            if(Atype=="ULONG" || Atype=="LONG"){//64位运算
                QString funName = "|"+Atype+"_"+nodeType+"|";

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vc},
                                             A.useVar->varName,
                                             1,
                                             varUseInfo
                                             );
            }else{//正常使用cpu硬件的运算
                QString orderName = nodeType;

                QString vc_text;

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }

                 MiddleNode code;
                 code.nodeType = orderName;
                 code.args = QStringList({A.useVar->varName,vc_text});
                 codeList += code;
            }
        }else if(nodeType=="BNOT"){
            if(thisCode.args.length()!=2){
                return;
            };
            //获取运算参数
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);

            if(A.isValid==0 || C.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Ctype =C.getType();
            if(Atype!="UBYTE")return;
            bool argC_isConstValue = C.useVar == NULL;

            if(Atype=="ULONG" || Atype=="LONG"){//64位运算
                QString funName = "|"+Atype+"_"+nodeType+"|";

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vc},
                                             A.useVar->varName,
                                             1,
                                             varUseInfo
                                             );
            }else{//正常使用cpu硬件的运算
                QString orderName = "NOT";

                QString vc_text;

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }

                QString tmpVar = getTempVar(varUseInfo);



                MiddleNode code;
                code.nodeType = orderName;
                code.args = QStringList({tmpVar,vc_text});
                codeList += code;

                code.nodeType = "MOV";
                code.args = QStringList({A.useVar->varName,"FLAG"});
                codeList += code;


            }
        }else if(nodeType=="BOR"||
                 nodeType=="BAND"||
                 nodeType=="CMEC"||
                nodeType=="CMNEC"||
                nodeType=="CMLC"||
                nodeType=="CMMC"||
                nodeType=="CMLEC"||
                nodeType=="CMMEC"){
            if(thisCode.args.length()!=3){
                return;
            };
            //获取运算参数
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg B = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[2],allAddressLabel,varUseInfo);

            if(A.isValid==0 || B.isValid==0 || C.isValid==0 || A.useVar==NULL)return;
            QString Atype =A.getType();
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if((Atype != "UBYTE" && Atype != "BYTE") || !judgeArgsTypeIsIdentical({B,C}))return;
            bool argB_isConstValue = B.useVar == NULL;
            bool argC_isConstValue = C.useVar == NULL;

            if(Btype=="DOUBLE" || Btype=="ULONG" || Btype=="LONG"){//64位运算
                QString funName = "|"+Btype+"_"+nodeType+"|";
                Call64bitOperFun_ArgInfo vb;
                vb.isConstValue = argB_isConstValue;
                vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
                vb.argIs64Bit = 1;
                vb.constValueSize = B.size;

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                if(!operFun.contains(funName))operFun += funName;

                QList<MiddleNode> test = generateCall64bitOperFunCode(funName,
                                                                        {vb,vc},
                                                                        A.useVar->varName,
                                                                        0,
                                                                        varUseInfo
                                                                        );
                codeList += test;
            }else{//正常使用cpu硬件的运算
                QString orderName;
                bool isCMcode = 0;
                if(nodeType.left(2) == "CM"){
                    isCMcode = 1;
                    orderName = nodeType.mid(2,nodeType.length()-2);
                }else{
                    orderName = nodeType.mid(1,nodeType.length()-1);
                }


                QString vb_text,vc_text;
                if(argB_isConstValue){
                    vb_text = getTempVar(varUseInfo);
                    MiddleNode initCode;
                    initCode.nodeType = B.size >= 16 ? "INIT" : "MOV";
                    initCode.args = QStringList({vb_text,B.constValue});
                    codeList += initCode;
                }else{
                    vb_text = B.useVar->varName;
                }

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }

                if(isCMcode){
                    MiddleNode code;
                    code.nodeType = orderName;
                    code.args = QStringList({vb_text,vc_text});
                    codeList += code;
                    code.nodeType = "MOV";
                    code.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList += code;
                }else{
                    QString tmpVar = getTempVar(varUseInfo);
                    MiddleNode code;
                    code.nodeType = orderName;
                    code.args = QStringList({tmpVar,vb_text,vc_text});
                    codeList += code;
                    code.nodeType = "MOV";
                    code.args = QStringList({A.useVar->varName,"FLAG"});
                    codeList += code;
                }
            }
        }else if(nodeType=="EC"||
                 nodeType=="NEC"||
                 nodeType=="LC"||
                 nodeType=="MC"||
                 nodeType=="LEC"||
                 nodeType=="MEC"){
            if(thisCode.args.length()!=2){
                return;
            };
            //获取运算参数
            OrderArg B = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            OrderArg C = getOrderArg(thisCode.args[1],allAddressLabel,varUseInfo);

            if(B.isValid==0 || C.isValid==0)return;
            QString Btype =B.getType();
            QString Ctype =C.getType();
            if(!judgeArgsTypeIsIdentical({B,C}))return;
            bool argB_isConstValue = B.useVar == NULL;
            bool argC_isConstValue = C.useVar == NULL;

            if(Btype=="DOUBLE" || Btype=="ULONG" || Btype=="LONG"){//64位运算
                QString funName = "|"+Btype+"_CM"+nodeType+"|";
                Call64bitOperFun_ArgInfo vb;
                vb.isConstValue = argB_isConstValue;
                vb.operText = argB_isConstValue ? B.constValue : B.useVar->varName;
                vb.argIs64Bit = 1;
                vb.constValueSize = B.size;

                Call64bitOperFun_ArgInfo vc;
                vc.isConstValue = argC_isConstValue;
                vc.operText = argC_isConstValue ? C.constValue : C.useVar->varName;
                vc.argIs64Bit = 1;
                vc.constValueSize = C.size;

                QString tmpVar = getTempVar(varUseInfo);

                if(!operFun.contains(funName))operFun += funName;
                codeList += generateCall64bitOperFunCode(funName,
                                             {vb,vc},
                                             tmpVar,
                                             0,
                                             varUseInfo
                                             );

                MiddleNode retCode;
                retCode.nodeType = "MOV";
                retCode.args = QStringList({"FLAG",tmpVar});
                codeList+=retCode;
            }else{//正常使用cpu硬件的运算
                QString orderName = nodeType;

                QString vb_text,vc_text;
                if(argB_isConstValue){
                    vb_text = getTempVar(varUseInfo);
                    MiddleNode initCode;
                    initCode.nodeType = B.size >= 16 ? "INIT" : "MOV";
                    initCode.args = QStringList({vb_text,B.constValue});
                    codeList += initCode;
                }else{
                    vb_text = B.useVar->varName;
                }

                if(argC_isConstValue){
                    if(C.size <=12){
                        vc_text = C.constValue;
                    }else{
                        vc_text = getTempVar(varUseInfo);
                        MiddleNode initCode;
                        initCode.nodeType = C.size >= 16 ? "INIT" : "MOV";
                        initCode.args = QStringList({vc_text,C.constValue});
                        codeList += initCode;
                    }
                }else{
                    vc_text = C.useVar->varName;
                }
                 MiddleNode code;
                 code.nodeType = orderName;
                 code.args = QStringList({vb_text,vc_text});
                 codeList += code;
            }
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
            if(Btype!="UINT" && Btype!="INT"){
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

        }else if(nodeType=="PUSH"){
            //函数参数入栈运算
            if(thisCode.args.length()!=1)return;


            if(thisCode.args[0].length()>=3 && thisCode.args[0][0] == "|"){
                //将一个数组入栈
                QStringList pushArrayText = thisCode.args[0].mid(1,thisCode.args[0].length()-2).split(":");
                if(pushArrayText.length()!=2)return;


                QString arrayPtrName = pushArrayText[0];
                QString arraySizeText = pushArrayText[1];

                OrderArg arrPtr = getOrderArg(arrayPtrName,allAddressLabel,varUseInfo);
                if(arrPtr.isValid==0 || arrPtr.getType()=="ARRAY" ||
                   (arrPtr.getType() != "UINT" && arrPtr.getType()!="INT"))return;


                arrayPtrName = arrPtr.useVar->varName;

                bool su;
                uint arraySizeNum = analysisStdConst(arraySizeText,&su);

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



                //将数组的字节数入栈
                bool isUseTemp1 = getNumDigit(arraySizeNum)>16;//如果要入栈的大于16bit,申请临时变量
                QString temp1;
                if(isUseTemp1){
                    temp1 = getTempVar(varUseInfo);
                    //调用函数数组型变量的复制函数
                    tmp.nodeType = "INIT";
                    tmp.args = QStringList({temp1,QString::number(arraySizeNum)+"D"});
                    codeList.append(tmp);
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",temp1});
                    codeList.append(tmp);
                }else{
                    //调用函数数组型变量的复制函数
                    tmp.nodeType = "PUSH";
                    tmp.args = QStringList({"DWORD",QString::number(arraySizeNum)+"D"});
                    codeList.append(tmp);
                }

                //将数组的指针入栈
                tmp.nodeType = "PUSH";
                tmp.args = QStringList({"DWORD",arrayPtrName});
                codeList.append(tmp);

                //调用数组传参拷贝函数
                tmp.nodeType = "INIT";
                tmp.args = QStringList({"TPC","[|FUN_ARG_ARRAY_COPY|]"});
                codeList.append(tmp);
                if(!operFun.contains("|FUN_ARG_ARRAY_COPY|")){
                    operFun.append("|FUN_ARG_ARRAY_COPY|");
                }
                tmp.nodeType = "COPYARR";
                tmp.args = QStringList();
                codeList.append(tmp);

                continue;
            }
            OrderArg A = getOrderArg(thisCode.args[0],allAddressLabel,varUseInfo);
            if(A.isValid==0 || A.getType()=="ARRAY")return;
            QString Atype =A.getType();
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

            if(A.useVar==NULL || B.useVar==NULL || (A.getType()!="UINT" && A.getType()!="INT") || C.useVar!=NULL)return;
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
                                            isHaveCallFun,
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

     //拼接 函数入口指令 函数体自定义指令 函数退出指令
     bodyCodeList = funEntryCodes + replseList + funReturnCodes;
     status = 1;
}



//判断一个函数中的指令是否符合要求
bool thisCodeDefIsMeetReq(MiddleNode &code){
    QString codeType = code.nodeType;
    //拥有3个参数的指令
    static const QStringList have3ArgOrder = {
        "ADD","SUB","MUL","DIV","REM","SAL","SAR","AND","OR",
        "XOR","BAND","BOR","CMEC","CMNEC","CMLC","CMMC","CMLEC","CMMEC",
        "ADDRESS","ARRCOPY"
    };
    //拥有两个参数的指令
    static const QStringList have2ArgOrder = {
        "MOV","STORE","LOAD","NOT","BNOT"
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
    if(node.areThereOnlyTheseAtt({"BODY","ARGS","VARS","EXPORT","STDCALL","APPCALL","DRIVERCALL","INTERRUPT"})){
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
    if(!thisFunDefIsMeetReq(node)){
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
 * //四则运算(a/b/c的数据类型必须为相同)bc可为常量
 * ADD(a,b,c);  a=b+c
 * SUB(a,b,c);  a=b-c
 * MUL(a,b,c);  a=b*c
 * DIV(a,b,c);  a=b/c
 *
 * //取模运算(a/b/c必须为整型,同时数据类型完全相同)bc可为常量
 * REM(a,b,c);
 *
 * //移位运算(a和b的类型必须相同,abc的数据类型不可是浮点型)bc可为常量
 * SAL(a,b,c);  a=b<<c
 * SAR(a,b,c);  a=b>>c
 *
 * //位运算(a/b/c位数必须相同,且为整型)bc可为常量
 * NOT(a,b);    a=~b
 * AND(a,b,c);  a=b&c
 * OR(a,b,c);   a=b|c
 * XOR(a,b,c);  a=b^c
 *
 * //布尔运算(a必须是ubyte,bc类型必须相同)bc可为常量
 * BNOT(a,b);   a=!b
 * BAND(a,b,c); a=b&&c
 * BOR(a,b,c);  a=b||c
 *
 * //比较运算(a必须是ubyte,bc类型必须相同)bc可为常量
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
 * //数组数据拷贝(a原数组指针,b拷贝到的数组指针,c必须是无符号整型常量(可为常量))
 * ARRCOPY(a,b,c);
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
 * PROCESS(比较运算指令,a,b).IF{
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
 *  //函数参数:
 *      常量值             (传入一个常量)
 *      变量名             (存入一个变量)
 *      |指针变量:字节数|   (存入一个数组)
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
}


}
QT_END_NAMESPACE
