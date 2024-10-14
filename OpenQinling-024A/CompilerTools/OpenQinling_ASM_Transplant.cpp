 #include "OpenQinling_ASM_Transplant.h"
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_DataStruct.h"
#include <QDebug>
#include "math.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
#if CPU_023A
#define FALSE 0xffffffff

//CPU物理寄存器编号对应关系
static QList<QStringList> cpuPhysicsRegs = {
    {"R0","D0"},
    {"R1","D1"},
    {"R2","D2"},
    {"R3","D3"},
    {"R4","D4"},
    {"R5","D5"},
    {"R6","D6"},
    {"R7","D7"},
    {"R8","D8"},
    {"R9","D9"},
    {"R10","D10"},
    {"R11","D11"},
    {"R12","D12"},
    {"R13","D13"},
    {"R14","D14"},
    {"R15","D15"},
    {"R16","D16"},
    {"R17","D17"},
    {"R18","D18"},
    {"R19","D19"},
    {"R20","D20","PV0","X0"},
    {"R21","D21","PV1","X1"},
    {"R22","D22","PV2","X2"},
    {"R23","D23","PV3","X3"},
    {"R24","D24","RV","Y"},
    {"R25","D25","BJS"},
    {"R26","SP"},
    {"R27","TPC"},
    {"R28","IPC"},
    {"R29","PC"},
    {"R30","SYS"}
};
//CPU所有读内存类型名称与编号对应关系
static QList<QStringList> allReadMemTypes = {
    {"UBYTE","U8"},
    {"BYTE","I8"},
    {"USHORT","U16"},
    {"SHORT","I16"},
    {"UINT","U32"},
    {"INT","I32"},
    {"FLOAT","F32"},
    {"ULONG","LONG","DOUBLE","U64","I64","F64","REG"}
};

//CPU所有写内存类型名称与编号对应关系
static QList<QStringList> allWriteMemTypes = {
    {"UBYTE","BYTE","U8","I8"},
    {"USHORT","SHORT","U16","I16"},
    {"UINT","INT","U32","I32"},
    {"FLOAT","F32"},
    {"ULONG","LONG","DOUBLE","U64","I64","F64","REG"}
};


//CPU所有基础运算的子类型编号和名称对应关系 (大类编号3)
static QStringList allBaseCodeType = {
    "ADD",
    "SUB",
    "MUL",
    "UDIV",
    "IDIV",
    "UREM",
    "IREM",
    "BAND",
    "BOR",
    "BNOT",
    "AND",
    "OR",
    "NOT",
    "XOR",
    "SHL",
    "USHR",
    "ISHR",
    "EC",
    "NEC",
    "ULC",
    "ULEC",
    "UMEC",
    "UMC",
    "ILC",
    "ILEC",
    "IMEC",
    "IMC",
    "LDI8",
    "LDU16",
    "LDI16",
    "LDU32",
    "LDI32"
};
//CPU所有扩展运算的子类型编号和名称的对应关系 (大类编号4)
static QStringList allExpCodeType = {
    "FADD",
    "FSUB",
    "FMUL",
    "FDIV",
    "FLC",
    "FLEC",
    "FMEC",
    "FMC",
    "FTI",
    "ITF",
    "UTF"
};
//CPU初始化转移数据相关指令的编号与名称对应关系 (大类编号5)
static QStringList allMovInitCodeType = {
    "MOV"
};

//CPU软中断指令
static QStringList allSoftIntCodeType = {
    "SINT"
};

//CPU重启指令
static QStringList allRestartCodeType = {
    "RST"
};


//CPU系统模式控制指令 (大类编号0-2)
static QStringList allSysModCtrlCodeType = {
    "EI",
    "DI",
    "EP"
};

//程序短跳转指令 (大类编号6 - 0)
static QStringList allShortJempCodeType = {
    "JMP",
    "JNE"
};

//程序长跳转指令 (大类编号6 - 1)
static QStringList allLongJempCodeType = {
    "RT",
    "IT",
    "EIT"
};

//CPU内存读写 (大类编号1-0)
static QStringList allMemCodeType = {
    "RD",
    "WE"
};

//CPU出入栈 (大类编号1-1)
static QStringList allIOStackCodeType = {
    "POP",
    "PUSH"
};

//CPU堆栈内存读写 (大类编号2)
static QStringList allSMemCodeType = {
    "SRD",
    "SWE"
};

//无用、NOP指令 (大类编号7)
static QStringList allNopCodeType = {
    "NOP"
};

//伪指令
static QStringList allPseudoCodeType = {
    "IT64", //初始化64位立即数 (解析为ASS22 + ASS22 + ASS22)
    "IT40",//初始化40位立即数 (解析为MOV + ASS22)
};



///////////////023A汇编编译器移植接口实现函数///////////////////////////
//解析该参数的寄存器
uint compileDataChannel(QString arg_txt){
    arg_txt = arg_txt.toUpper();
    for(int i = 0;i<cpuPhysicsRegs.length();i++){
        if(cpuPhysicsRegs[i].contains(arg_txt))return i;
    }
    return FALSE;
}

//解析汇编指令中内存、栈读写的数据大小二进制码
uint compileRwDataType(QString arg_txt,bool isWrite){
    arg_txt = arg_txt.toUpper();
    if(isWrite){
        for(int i = 0;i<allWriteMemTypes.length();i++){
            if(allWriteMemTypes[i].contains(arg_txt))return i+8;
        }
    }else{
        for(int i = 0;i<allReadMemTypes.length();i++){
            if(allReadMemTypes[i].contains(arg_txt))return i;
        }
    }

    return FALSE;
}

//解析立即数:
//arg_txt: 立即数文本
uint compileImmeNum(QString arg_txt,bool & success,bool &isNegative,uint32_t &useBits){
    sucess = 0;
    if(arg_txt.length()==0){
        sucess = 0;
        return FALSE;
    }else if(arg_txt=="NULL"){
        sucess = 1;
        return 0;
    }
    QChar basic = arg_txt[arg_txt.length()-1];
    QString num_txt;
    if(QString("HDOB").contains(basic)){
        num_txt = arg_txt.left(arg_txt.length()-1);
    }else{
        basic = 'D';
        num_txt = arg_txt;
    }
    bool isSu;
    uint getNum;

    isNegative = 0;
    //判断是否是负数
    if(num_txt[0] == '-'){
        isNegative = 1;
        num_txt.removeFirst();
    }
    //根据立即数的最后一位判断是什么进制
    if(basic=='H')getNum = num_txt.toULongLong(&isSu,16);
    else if(basic=='D')getNum = num_txt.toULongLong(&isSu,10);
    else if(basic=='O')getNum = num_txt.toULongLong(&isSu,8);
    else if(basic=='B')getNum = num_txt.toULongLong(&isSu,2);
    else{
        getNum = arg_txt.toULongLong(&isSu,10);
    }
    if(isNegative){
        getNum = -getNum;
    }

    sucess = isSu;
    return getNum;
}


//解析该参数为立即数或参数通道
uint compileImmeNumChannnel(QString arg_txt,uint*num){
    if(num==NULL)return FALSE;
    uint dc = compileDataChannel(arg_txt);

    if(dc == FALSE){//如果参数不是一个寄存器，那么判断是否是一个立即数
        bool su;
        if(num != NULL){
            *num = compileImmeNum(arg_txt,su);
            return su ? 31 : FALSE;
        }
        return FALSE;
    }

    return dc;
}

//解析该参数为寻址立即数的通道 RD REG,D0,%D1+78D; //访存位置 = D1寄存器数据 + 78
uint compileAddressNumChannnel(QString arg_txt){
    if(arg_txt.isEmpty())return FALSE;

    QString tmp =  arg_txt;
    uint reg = FALSE;
    uint num = 0;
    if(arg_txt[0] == '%'){
        arg_txt.removeFirst();
        QString regText;
        QString numText;

        while(arg_txt.length()!=0){
            if(arg_txt[0] == '+' || arg_txt[0] == '-'){
                break;
            }else{
                regText.append(arg_txt[0]);
                arg_txt.removeFirst();
            }
        }
        numText = arg_txt;
        reg = compileDataChannel(regText);
        bool su;
        num = compileImmeNum(numText,su);
        if(!su)return FALSE;
    }else{
        reg = compileImmeNumChannnel(arg_txt,&num);
    }
    if(reg == FALSE){
        return FALSE;
    }
    num &= (uint)0x3fff;
    reg <<= 14;
    return reg | num;
}

//IT64 ITADD IT32的初始化值解析
uint64_t compileInitValue(QString arg_txt,bool &su){
    su = 0;
    arg_txt = arg_txt.toUpper();
    if(arg_txt.length()==0){
        return 0;
    }
    QChar basic = arg_txt[arg_txt.length()-1];
    QString num_txt;
    if(QString("HDOBF").contains(basic)){
        num_txt = arg_txt.left(arg_txt.length()-1);
    }else{
        basic = 'D';
        num_txt = arg_txt;
    }
    if(num_txt.isEmpty())return 0;
    bool isSu;
    QWORD getNum;

    bool isNegative = 0;
    //判断是否是负数
    if(num_txt[0] == '-' && basic!='F'){
        isNegative = 1;
        num_txt.removeFirst();
    }
    //根据立即数的最后一位判断是什么进制
    if(basic=='H')getNum = num_txt.toULongLong(&isSu,16);
    else if(basic=='D')getNum = num_txt.toULongLong(&isSu,10);
    else if(basic=='O')getNum = num_txt.toULongLong(&isSu,8);
    else if(basic=='B')getNum = num_txt.toULongLong(&isSu,2);
    else if(basic=='F'){// double型 (024ACPU内部没有FLOAT型,只支持DOUBLE型,FLOAT型在CPU内部都转为DOUBLE型处理与存储)
        getNum = num_txt.toDouble(&isSu);
    }else{
        getNum = arg_txt.toULongLong(&isSu,10);
    }
    if(isNegative){
        getNum = 0-getNum.toULong();
    }
    su = isSu;
    return getNum.toULong();
}

struct CodeGroupImmeBitrange{
    uint high;
    uint low;
};

//将IT64 ITADD IT32的初始化值切割为多份作为组合指令中多条分离指令的立即数
QList<uint32_t> compileInitCodeImme(uint64_t immeNum,QList<CodeGroupImmeBitrange> bitrange){
    QList<uint32_t> immelist;
    for(int i = 0;i<bitrange.length();i++){
        uint bits = bitrange[i].high - bitrange[i].low + 1;
        uint64_t mask = ((1ULL << bits) - 1)<<bitrange[i].low;
        uint64_t tmp = immeNum & mask;
        tmp = tmp >> bitrange[i].low;
        immelist.append(tmp);
    }
    return immelist;
}





/////////////////单指令编译器////////////////////////
DWORD compileOperationCode(uint baseTypeCode,uint operTypeCode,QStringList args,bool isDoubleArgCode,int &status){
    if(isDoubleArgCode){
        if(args.length() != 2 && args.length() != 1){
            status = 2;
            return 0;
        }
    }else if(args.length() != 2 && args.length() != 3){
        status = 2;
        return 0;
    }

    uint32_t byteCode = 0;
    byteCode |= baseTypeCode<<29;
    byteCode |= operTypeCode<<24;

    uint res = compileDataChannel(args[0]);
    if(res == FALSE){
        status = 3;
        return 0;
    }
    byteCode |= res<<19;

    if(isDoubleArgCode){
        byteCode |= 31<<14;
    }else{
        int index = args.length() == 3 ? 1 : 0;
        uint numA = compileDataChannel(args[index]);
        if(numA == FALSE){
            status = 3;
            return 0;
        }
        byteCode |= numA<<14;
    }

    uint imme = 0;
    uint numB = compileImmeNumChannnel(args[args.length()-1],&imme);
    if(numB == FALSE){
        status = 3;
        return 0;
    }else if(numB == 31 && isDoubleArgCode && args.length()==1){
        status = 3;
        return 0;
    }
    imme &= 0x1ff;
    byteCode |= numB<<9;
    byteCode |= imme;
    return byteCode;
}


////////////////移植接口重写//////////////////////////
//当前指令的参数位是否支持使用标记
//argIndex:当前是第几个指令参数
//argTotal:当前汇编指令中总共几个参数
bool thisCodeOrderIsSupportLabel(QString order,int argIndex,int argTotal){
    order = order.toUpper();
    bool isIT64 = order == "IT64" && argIndex==1 && argTotal == 2;
    bool isIT40 = order == "IT40" && argIndex==1 && argTotal == 2;
    bool isMOV = order == "MOV" && argIndex==1 && argTotal == 2;
    bool isJMPJNE = (order == "JMP" || order == "JNE")  && argIndex==0 && argTotal == 1;
    return isIT64 || isJMPJNE || isIT40 || isMOV;
}
//该指令支持的标记最大字节数
uint thisCodeOrderSupportMaxBits(QString order){
    order = order.toUpper();
    if(order == "IT40"){
        return 40;
    }else if(order == "IT64"){
        return 64;
    }else if(order == "JMP" || order == "JNE"){
        return 27;
    }else if(order == "MOV"){
        return 17;
    }
    return 0;
}

//标记引用语法中未声明higth,low。取默认的higth,low
void getMarkDefaultHL(QString order,uint &h,uint &l){
    order = order.toUpper();
    if(order == "ITADD"){
        h = 39;
    }else if(order == "IT64"){
        h = 63;
    }else if(order == "JMP" || order == "JNE"){
        h = 26;
    }else if(order == "MOV"){
        h =16;
    }
    l = 0;
}

//确定标记的标记值在当前指令字节码中的order_offset_Byte、order_offset_Bit
QList<MarkQuote> markQuoteFromCodeOrder(QString order,
                                        int low,int higth,int offset,QString blockName,QString markName//analysisCodeBlockMarkQuote编译生成的
                                        ){
    order = order.toUpper();
    //需要生成quote.order_offset_Byte和quote.order_offset_Bit
    QList<MarkQuote> quotes;
    MarkQuote quote;
    //[标记内存块.内存块位置+偏移量](高位-低位)
    quote.offset = offset;//标记值的偏移量
    quote.affiBlockName = blockName;//标记指向的内存块
    quote.name = markName;//标记指向的内存块的位置
    if(order == "IT40"){
        if(higth>=22){
            quote.higth = higth;
            quote.low = 22;
            uint bits = 32-(quote.higth-quote.low+1);
            quote.order_offset_Byte = bits/8;
            quote.order_offset_Bit = bits%8;
            quotes.append(quote);
        }
        if(low<=21){
            quote.higth = 21;
            quote.low = low;
            uint bits = 32-(quote.higth-quote.low+1);
            quote.order_offset_Byte = bits/8 + 4;
            quote.order_offset_Bit = bits%8;
            quotes.append(quote);
        }
        return quotes;
    }else if(order == "IT64"){//22
        QList<int> markBitOffset;
        int higth,low,bitOff;
        if(quote.higth>=44){
            higth = quote.higth;
            low = quote.low<44 ? 44 : quote.low;
            bitOff = higth-low+1;
            bitOff = 22-bitOff;
            markBitOffset.append(bitOff);
        }
        if(quote.higth>=22 && quote.low<44){
            higth = quote.higth>43 ? 43 : quote.higth;
            low = quote.low<22 ? 22 : quote.low;
            bitOff = higth-low+1;
            bitOff = 42-bitOff;
            markBitOffset.append(bitOff);
        }
        if(quote.low<22){
            higth = quote.higth>21 ? 21 : quote.higth;
            low = quote.low;
            bitOff = higth-low+1;
            bitOff = 64-bitOff;
            markBitOffset.append(bitOff);
        }
        for(int i = 0;i<markBitOffset.length();i++){
            quote.order_offset_Byte = markBitOffset[i]/8;
            quote.order_offset_Bit = markBitOffset[i]%8;
            quotes.append(quote);
        }
        return quotes;
    }else if(order == "MOV" || order == "JMP" || order == "JNE"){
        quote.low = low;//标记值的地位
        quote.higth = higth;//标记值的高位
        uint bits = higth - low + 1;
        uint offsetBits = 32 - bits;
        quote.order_offset_Byte = offsetBits / 8;
        quote.order_offset_Bit = offsetBits % 8;
        quotes.append(quote);
    }
    return quotes;
}






//功能:编译一条汇编指令
//参数: asmOrder一条汇编指令文本 status返回编译情况[0成功 >=1失败]
//返回值:返回编译出的二进制码,失败则QByteArray长度为0
/*
 * status失败类型
 * 1: 使用了指令集中未定义的指令
 * 2: 指令的参数数量不正确
 * 3: 使用了指令集中未定义的寄存器
 * 4: 使用了该参数位上禁止使用的寄存器
 * 5: 指令中立即数的大小超过了该指令允许的范围
 * 6: 在禁止使用立即数的参数位上使用了立即数
 * 7: 内存读写的字节数不符合要求
 * 8: 位操作，操作的位数不符合要求
 */
QByteArray compileCodeAsm(QString name,QStringList args,int *status){
    name = name.toUpper();
    if(status==0)return QByteArray();
    *status = 0;
    if(allBaseCodeType.contains(name)){
        static QStringList doubleTypes = {
            "NOT",
            "BNOT",
            "LDI8",
            "LDU16",
            "LDI16",
            "LDU32",
            "LDI32"
        };
        bool isDoubleArgCode = doubleTypes.contains(name);
        return compileOperationCode(3,allBaseCodeType.indexOf(name),args,isDoubleArgCode,*status).toByteArray();
    }else if(allExpCodeType.contains(name)){
        bool isDoubleArgCode = name=="FTI" || name=="ITF" || name=="UTF";
        return compileOperationCode(4,allExpCodeType.indexOf(name),args,isDoubleArgCode,*status).toByteArray();
    }else if(allShortJempCodeType.contains(name)){
        if(args.length()!=1){
            *status = 2;
            return QByteArray();
        }
        uint32_t imme = 0;
        uint32_t off = compileImmeNumChannnel(args[0],&imme);
        if(off!=31){
            *status = 3;
            return QByteArray();
        }
        imme &= 0x7ffffff;
        uint32_t code = (6<<29 | 0<<28 | allShortJempCodeType.indexOf(name)<<27 | imme);
        return DWORD(code).toByteArray();
    }else if(allLongJempCodeType.contains(name)){
        uint32_t code = (6<<29 | 1<<28 | allLongJempCodeType.indexOf(name));
        return DWORD(code).toByteArray();
    }else if(allMemCodeType.contains(name)){
        if(args.length()!=3){
            *status = 2;
            return QByteArray();
        }
        uint32_t ioType = compileRwDataType(args[0],allMemCodeType.indexOf(name));
        uint32_t ioReg = compileDataChannel(args[1]);

        uint32_t ioAdd = compileAddressNumChannnel(args[2]);
        if(ioReg==FALSE || ioAdd==FALSE || ioType==FALSE){
            *status = ioType==FALSE ? 4 :3;
            return QByteArray();
        }

        uint32_t code = (1<<29 | 0<<28 | ioType<<24 | ioReg<<19 | ioAdd);
        return DWORD(code).toByteArray();
    }else if(allIOStackCodeType.contains(name)){
        if(args.length()!=2){
            *status = 2;
            return QByteArray();
        }
        uint32_t ioType = compileRwDataType(args[0],allIOStackCodeType.indexOf(name));
        uint32_t ioImme = 0;
        uint32_t ioReg = compileImmeNumChannnel(args[1],&ioImme);
        if(ioReg==FALSE || ioReg==FALSE || ioType==FALSE){
            *status = ioType==FALSE ? 4 :3;
            return QByteArray();
        }
        ioImme &= (uint32_t)0x7ffff;

        uint32_t code = (1<<29 | 1<<28 | ioType<<24 | ioReg<<19 | ioImme);
        return DWORD(code).toByteArray();
    }else if(allSMemCodeType.contains(name)){
        if(args.length()!=3){
            *status = 2;
            return QByteArray();
        }
        uint32_t ioType = compileRwDataType(args[0],allSMemCodeType.indexOf(name));
        uint32_t ioReg = compileDataChannel(args[1]);
        uint32_t addImme = 0;
        uint32_t addReg = compileImmeNumChannnel(args[2],&addImme);
        if(ioReg==FALSE || addReg!=31 || ioType==FALSE){
            *status = ioType==FALSE ? 4 :3;
            return QByteArray();
        }
        addImme &= (uint32_t)0xfffff;
        uint32_t code = (2<<29 |ioType<<25 | ioReg<<20 | addImme);
        return DWORD(code).toByteArray();
    }else if(allSysModCtrlCodeType.contains(name)){
        uint32_t code = (0<<29 | 2<<25 | allSysModCtrlCodeType.indexOf(name));
        return DWORD(code).toByteArray();
    }else if(allMovInitCodeType.contains(name)){
        if(args.length()!=2){
            *status = 2;
            return QByteArray();
        }
        uint32_t res = compileDataChannel(args[0]);

        uint32_t imme = 0;
        uint32_t reg = compileImmeNumChannnel(args[1],&imme);
        if(reg==FALSE || res==FALSE){
            *status = 3;
            return QByteArray();
        }
        imme &= (uint32_t)0x0003ffff;

        uint32_t code = (5<<29 | 0<<28 | res<<23 |reg<<18 | imme);
        return DWORD(code).toByteArray();
    }else if(allSoftIntCodeType.contains(name)){
        if(args.length()!=1){
            *status = 2;
            return QByteArray();
        }
        uint32_t imme = 0;
        uint32_t off = compileImmeNumChannnel(args[0],&imme);
        if(off!=31){
            *status = 3;
            return QByteArray();
        }else if(imme<=15 || imme >= 256){
            *status = 5;
            return QByteArray();
        }
        uint32_t code = (0<<29 | 1<<25 | allSoftIntCodeType.indexOf(name) | imme);
        return DWORD(code).toByteArray();
    }else if(allRestartCodeType.contains(name)){
        return DWORD(0).toByteArray();
    }else if(allNopCodeType.contains(name)){
        return DWORD(0xffffffff).toByteArray();
    }else if(name == "IT64"){
        if(args.length()!=2){
            *status = 2;
            return QByteArray();
        }
        bool su;
        uint64_t num = compileInitValue(args[1],su);
        QList<uint32_t> immes = compileInitCodeImme(num,{{63,46},{45,23},{22,0}});
        uint32_t res = compileDataChannel(args[0]);
        if(res==FALSE){
            *status = 3;
            return QByteArray();
        }

        uint32_t code = 5<<29 | 1<<28 | res<<23;

        uint32_t code1 = (code | immes[0]);
        uint32_t code2 = (code | immes[1]);
        uint32_t code3 = (code | immes[2]);
        return DWORD(code1).toByteArray() + DWORD(code2).toByteArray() + DWORD(code3).toByteArray();
    }else if(name == "IT40"){
        if(args.length()!=2){
            *status = 2;
            return QByteArray();
        }
        bool su;
        uint64_t num = compileInitValue(args[1],su);
        QList<uint32_t> immes = compileInitCodeImme(num,{{40,23},{22,0}});
        uint32_t res = compileDataChannel(args[0]);
        if(res==FALSE){
            *status = 3;
            return QByteArray();
        }

        uint32_t code1 = (5<<29 | 0<<28 | res<<23 | 31<<18 |immes[0]);
        uint32_t code2 = (5<<29 | 1<<28 | res<<23 | immes[1]);
        return DWORD(code1).toByteArray() + DWORD(code2).toByteArray();
    }
    *status = 1;
    return QByteArray();
}

//告知编译器,有哪些限定符
QStringList asmCustomInstructions(){
    QStringList allInstruc;
    for(int i = 0;i<cpuPhysicsRegs.length();i++){
        allInstruc += cpuPhysicsRegs[i];
    }
    for(int i = 0;i<allReadMemTypes.length();i++){
        allInstruc += allReadMemTypes[i];
    }
    for(int i = 0;i<allWriteMemTypes.length();i++){
        allInstruc += allWriteMemTypes[i];
    }
    allInstruc += allBaseCodeType;

    allInstruc += allExpCodeType;
    allInstruc += allShortJempCodeType;
    allInstruc += allLongJempCodeType;
    allInstruc += allMemCodeType;
    allInstruc += allIOStackCodeType;
    allInstruc += allSMemCodeType;
    allInstruc += allSysModCtrlCodeType;
    allInstruc += allMovInitCodeType;
    allInstruc += allSoftIntCodeType;
    allInstruc += allRestartCodeType;
    allInstruc += allPseudoCodeType;
    return allInstruc;
}
#endif

}}
QT_END_NAMESPACE
