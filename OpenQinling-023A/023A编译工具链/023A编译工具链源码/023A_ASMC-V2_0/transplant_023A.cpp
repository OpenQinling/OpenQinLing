#include "transplant.h"
#include "asmc_typedef.h"
#include <QDebug>
#include <math.h>
#if CPU_023A

#define FALSE 0xffffffff
///////////////023A汇编编译器移植接口实现函数///////////////////////////


//解析该参数的寄存器
uint compileDataChannel(QString arg_txt){
    if(arg_txt=="R1"||arg_txt=="r1")return 1;
    else if(arg_txt=="R2"||arg_txt=="r2")return 2;
    else if(arg_txt=="R3"||arg_txt=="r3")return 3;
    else if(arg_txt=="R4"||arg_txt=="r4")return 4;
    else if(arg_txt=="R5"||arg_txt=="r5")return 5;
    else if(arg_txt=="R6"||arg_txt=="r6")return 6;
    else if(arg_txt=="CS"||arg_txt=="cs"||arg_txt=="R7"||arg_txt=="r7")return 7;
    else if(arg_txt=="DS"||arg_txt=="ds"||arg_txt=="R8"||arg_txt=="r8")return 8;
    else if(arg_txt=="FLAG"||arg_txt=="flag")return 9;
    else if(arg_txt=="PC"||arg_txt=="pc")return 10;
    else if(arg_txt=="TPC"||arg_txt=="tpc")return 11;
    else if(arg_txt=="IPC"||arg_txt=="ipc")return 12;
    else if(arg_txt=="SP"||arg_txt=="sp")return 13;
    else if(arg_txt=="TLB"||arg_txt=="tlb")return 14;
    else if(arg_txt=="SYS"||arg_txt=="sys")return 15;
    return FALSE;
}
//解析汇编指令中内存、栈读写的数据大小二进制码
uint compileRwDataSize(QString arg_txt){
    if(arg_txt=="BYTE"||arg_txt=="byte")return 1;
    else if(arg_txt=="WORD"||arg_txt=="word")return 2;
    else if(arg_txt=="DWORD"||arg_txt=="dword")return 3;
    return FALSE;
}
//解析该参数为立即数或参数通道
uint compileImmeNumChannnel(QString arg_txt,uint*num){
    if(num==NULL)return FALSE;
    uint dc = compileDataChannel(arg_txt);

    if(dc == FALSE){//如果参数不是一个寄存器，那么判断是否是一个立即数
        if(arg_txt=="NULL"){
            *num = 0;
            return 0;
        }
        QString basic = arg_txt.right(1);
        QString num_txt = arg_txt.left(arg_txt.length()-1);
        bool isSu;
        uint getNum;

        //根据立即数的最后一位判断是什么进制
        if(basic=="h"||basic=="H")getNum = num_txt.toUInt(&isSu,16);
        else if(basic=="d"||basic=="D")getNum = num_txt.toUInt(&isSu,10);
        else if(basic=="o"||basic=="O")getNum = num_txt.toUInt(&isSu,8);
        else if(basic=="b"||basic=="B")getNum = num_txt.toUInt(&isSu,2);
        else{
            getNum = arg_txt.toUInt(&isSu,10);
        }
        if(isSu){
            *num = getNum;
            return 0;
        }
        else{
            return FALSE;
        }
    }

    return dc;
}

/////////////////单指令编译器////////////////////////
//格式转换运算、转移运算、比较运算 格式特点[31:27]主模式 [26:24]子模式 [23:20]参数1 [19:16]参数2 [15:0]立即数(0-65535)
DWORD compileTRANS_MOV(int mode,int sonMode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;

    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2){
        *status = 2;
        return FALSE;
    }

    uint operand1 = compileDataChannel(argsList.at(0));
    if(operand1==FALSE){
        *status = 3;
        return FALSE;
    }

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1),&num);
    if(operand2==FALSE){
        *status = 3;
        return FALSE;
    }

    if(operand2==0){
        if(num>65535){
            *status = 5;
            return FALSE;
        }
        orderBin = orderBin | num;
    }else{
        orderBin = orderBin | operand2<<16;
    }
    *status = 0;
    return orderBin;
}

//常量赋值指令 格式特点[31:27]主模式 [26:24]子模式 [23:20]参数1 [15:0]立即数(0-65535)
DWORD compileASS(int mode,int sonMode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2){
        *status = 2;
        return FALSE;
    }

    uint operand1 = compileDataChannel(argsList.at(0));
    if(operand1==FALSE){
        *status = 3;
        return FALSE;
    }

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1),&num);
    if(operand2==FALSE){
        *status = 3;
        return FALSE;
    }

    if(operand2==0){
        if(num>65535){
            *status = 5;
            return FALSE;
        }
        orderBin = orderBin | num;
    }else{
        return FALSE;
    }
    *status = 0;
    return orderBin;
}

//编译四则运算/移位运算/二进制运算 格式特点[31:27]主模式 [26:24]子模式 [23:20]参数1 [19:16]参数2 [15:12]写回参数 [11:0]立即数(0-4095)
DWORD compileOper(int mode,int sonMode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2 && argsList.length()!=3){
        *status = 2;
        return FALSE;
    }

    int arg_mode = argsList.length()==3;//汇编格式是否为3参数
    uint operand1 = compileDataChannel(argsList.at(0+arg_mode));
    if(operand1==FALSE){
        *status = 3;
        return FALSE;
    }

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1+arg_mode),&num);
    if(operand2==FALSE){
        *status = 3;
        return FALSE;
    }

    if(operand2==0){
        if(num>4095){
            *status = 5;
            return FALSE;
        }
        orderBin = orderBin | num;
    }else{
        orderBin = orderBin | operand2<<16;
    }


    if(arg_mode){
        uint back = compileDataChannel(argsList.at(0));
        if(back==FALSE){
            *status = 3;
            return FALSE;
        }
        orderBin = orderBin | back<<12;
    }else{
        orderBin = orderBin | operand1<<12;
    }
    *status = 0;
    return orderBin;
}

//编译跳转/软中断指令 格式特点[31:27]主模式 [26:0]立即数
DWORD compileJMP_JT_INT(int mode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    uint orderBin = mode<<27;
    if(argsList.length()!=1){
        *status = 2;
        return FALSE;
    }

    uint num = 0;
    uint operand1 = compileImmeNumChannnel(argsList.at(0),&num);
    if(operand1!=0){
        *status = 4;
        return FALSE;
    }
    if(mode==13 && (num<16 | num>255)){
        *status = 5;
        return FALSE;
    }
    else if(num>134217727){
        *status = 5;
        return FALSE;
    }
    *status = 0;
    return orderBin | num;
}

//内存读写指令 格式特点[31:27]主模式 [26]读还是写(1为写) [25:24]读写字节数 [23:20]读写数据 [19:16]地址 [15:0]地址立即数
DWORD compileRD_WE(int mode,bool rwMode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    if(argsList.length()!=3){
        *status = 2;
        return FALSE;
    }
    uint order_bin = 0;
    uint size_bin = compileRwDataSize(argsList.at(0));
    if(size_bin==FALSE){
        *status = 7;
        return FALSE;
    }
    order_bin = mode<<27 | size_bin<<24 | rwMode<<26;

    uint data = compileDataChannel(argsList.at(1));
    if(data==FALSE){
        *status = 3;
        return FALSE;
    }
    order_bin = order_bin | data<<20;

    uint num;
    uint address = compileImmeNumChannnel(argsList.at(2),&num);
    if(address==FALSE){
        *status = 3;
        return FALSE;
    }
    else if(address == 0){
        if(num>65535){
            *status = 5;
            return FALSE;
        }
        order_bin = order_bin | num;
    }else{
        order_bin = order_bin | address<<16;
    }
    *status = 0;
    return order_bin;
}

//入栈指令
DWORD compilePUSH(int mode,QStringList argsList,int*status){
    uint order_bin = 0;
    if(argsList.length()!=2){
        *status = 2;
        return FALSE;
    }
    order_bin = mode<<27 | 1<<26;
    uint size_bin = compileRwDataSize(argsList.at(0));
    if(size_bin==FALSE){
        *status = 7;
        return FALSE;
    }
    order_bin = order_bin | size_bin<<24;

    uint num;
    uint data = compileImmeNumChannnel(argsList.at(1),&num);
    if(data==FALSE){
        *status = 3;
        return FALSE;
    }
    else if(data == 0){
        if(num>65535){
            *status = 5;
            return FALSE;
        }
        order_bin = order_bin | num;
    }else{
        order_bin = order_bin | data<<16;
    }
    *status = 0;
    return order_bin;
}

//出栈指令
DWORD compilePOP(int mode,QStringList argsList,int*status){
    uint order_bin = 0;
    if(argsList.length()!=2){
        *status = 2;
        return FALSE;
    }
    order_bin = mode<<27;
    uint size_bin = compileRwDataSize(argsList.at(0));
    if(size_bin==FALSE){
        *status = 7;
        return FALSE;
    }
    order_bin = order_bin | size_bin<<24;

    uint data = compileDataChannel(argsList.at(1));
    if(data==FALSE){
        *status = 3;
        return FALSE;
    }
    order_bin = order_bin | data<<16;
    *status = 0;
    return order_bin;
}

//栈读写指令 [31:27]主模式 [26:25]读写字节数 [24:21]读写的寄存器 [20:0]地址立即数(0-2MB) 读写的实际地址=SP寄存器+地址立即数
DWORD compileSRW(int mode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    uint orderBin = mode<<27;
    if(argsList.length()!=3){
        *status = 2;
        return FALSE;
    }
    uint size = compileRwDataSize(argsList.at(0));
    if(size==FALSE){
        *status = 7;
        return FALSE;
    }

    orderBin |= size<<25;

    uint rwReg = compileDataChannel(argsList.at(1));
    if(rwReg==FALSE){
        *status = 3;
        return FALSE;
    }
    orderBin |= rwReg<<21;

    uint off = 0;
    uint address = compileImmeNumChannnel(argsList.at(2),&off);
    if(address!=0){
        *status = 4;
        return FALSE;
    }

    if(off>=2097152){
        *status = 5;
        return FALSE;
    }
    orderBin |= off;
    *status = 0;
    return orderBin;
}

//位操作指令 [31:27]主模式 [26:24]子模式 [23:20]写入寄存器 [19:16]源寄存器 [15:10]立即数(0-63) [9:5]高位限制 [4:0]低位限制
DWORD compileBitOper(int mode,int subMode,QStringList argsList,int*status){
    if(status==NULL)return FALSE;
    uint orderBin = mode<<27 | subMode<<24;

    if(argsList.length()!=3){
        *status = 2;
        return FALSE;
    }

    //编译高低位限制
    QStringList ml_txt = argsList.at(0).split("-");
    if(ml_txt.length()!=2){
        *status = 2;
        return FALSE;
    }

    bool m_src = 0,l_src=0;
    uint m = ml_txt.at(0).toUInt(&m_src);
    uint l = ml_txt.at(1).toUInt(&l_src);
    if(m_src==0 || l_src==0){
        *status = 8;
        return FALSE;
    }
    if(m>31 || l>31 || l>m){
        *status = 8;
        return FALSE;
    }

    orderBin |= m<<5 | l;

    //编译目标寄存器通道
    uint p_reg = compileDataChannel(argsList.at(1));
    if(p_reg==FALSE){
        *status = 3;
        return FALSE;
    }
    orderBin |= p_reg<<20;

    //编译源寄存器通道
    uint inum = 0;
    uint s_reg = compileImmeNumChannnel(argsList.at(2),&inum);
    if(s_reg==FALSE){
        *status = 3;
        return FALSE;
    }
    if(s_reg==0){
        if(inum>63){
            *status = 5;
            return FALSE;
        }
        orderBin |= inum <<10;
    }else{
        orderBin |= s_reg<<16;
    }
    *status = 0;
    return orderBin;
}

////////////////组合指令编译器////////////////////////
//2条指令组合而成:[mov:寄存器,高16位值; ass:寄存器,低16位值] 大小:2dword
QDwordArray compileINIT(QStringList argsList,int*status){
    if(status==NULL)return QDwordArray();
    if(argsList.length()!=2){
        *status = 2;
        return QDwordArray();
    }

    QString immeStr = argsList.at(1);
    QString basic = immeStr.right(1);
    QString num_txt = immeStr.left(immeStr.length()-1);
    bool isSu;
    DWORD getNum;
    //解析立即数数值
    if(immeStr=="NULL"||immeStr=="null"){
        isSu = 1;
        getNum = 0;
    }
    else if(basic=="h"||basic=="H")getNum = num_txt.toUInt(&isSu,16);
    else if(basic=="d"||basic=="D")getNum = num_txt.toUInt(&isSu,10);
    else if(basic=="o"||basic=="O")getNum = num_txt.toUInt(&isSu,8);
    else if(basic=="b"||basic=="B")getNum = num_txt.toUInt(&isSu,2);
    else if(basic=="f"||basic=="F")getNum = num_txt.toFloat(&isSu);
    else{
        getNum = immeStr.toInt(&isSu,10);
    }
    if(isSu==0){
        *status = 4;
        return QDwordArray();
    }

    QWordArray wordArr = getNum.toWordArray();

    //分别解析出立即数的高16位数据和低16位数据
    uint heigh = wordArr[0].toUShort();
    uint low = wordArr[1].toUShort();

    QStringList com_args;
    com_args.append(argsList.at(0));
    com_args.append(QString::number(heigh,16)+"H");

    //判断所用寄存器是否符合要求[INIT组合指令仅支持R1-R8,IPC,TPC,FLAG,SP,TLB寄存器]
    if( com_args[0]!="R1"&&com_args[0]!="r1"&&
        com_args[0]!="R2"&&com_args[0]!="r2"&&
        com_args[0]!="R3"&&com_args[0]!="r3"&&
        com_args[0]!="R4"&&com_args[0]!="r4"&&
        com_args[0]!="R5"&&com_args[0]!="r5"&&
        com_args[0]!="R6"&&com_args[0]!="r6"&&
        com_args[0]!="R7"&&com_args[0]!="r7"&&
        com_args[0]!="R8"&&com_args[0]!="r8"&&
        com_args[0]!="FLAG"&&com_args[0]!="flag"&&
        com_args[0]!="SP"&&com_args[0]!="sp"&&
        com_args[0]!="TPC"&&com_args[0]!="tpc"&&
        com_args[0]!="TLB"&&com_args[0]!="tlb"&&
        com_args[0]!="IPC"&&com_args[0]!="ipc"
    ){

        *status = 4;
        return QDwordArray();
    }
    int com_status;
    //编译mov指令
    DWORD mov = compileTRANS_MOV(9,0,com_args,&com_status);
    com_args[1] = QString::number(low,16)+"H";
    //编译ass指令
    DWORD ass = compileASS(9,1,com_args,&com_status);

    QDwordArray bin;
    bin.append(mov);
    bin.append(ass);
    *status = 0;
    return bin;
}

//4条指令组合而成:[mov:寄存器,31-24位值; ass:寄存器,23-16位值; mov:寄存器,15-8位值; ass:寄存器,7-0位值] 大小:4dword
//指令参数： INIT64 a,b,c;  a=存储高32位数据的寄存器, b=存储低32位数据的变量, c=要初始化的64位常量值
QDwordArray compileINIT64(QStringList argsList,int*status){
    if(status==NULL)return QDwordArray();
    if(argsList.length()!=3){
        *status = 2;
        return QDwordArray();
    }

    qDebug()<<"-------------------------------"<<argsList;
    QString immeStr = argsList.at(2);
    QString basic = immeStr.right(1);
    QString num_txt = immeStr.left(immeStr.length()-1);
    bool isSu;
    QWORD getNum;
    //解析立即数数值
    if(immeStr=="NULL"||immeStr=="null"){
        isSu = 1;
        getNum = (long long)0;
    }

    else if(basic=="h"||basic=="H")getNum = num_txt.toULongLong(&isSu,16);
    else if(basic=="d"||basic=="D")getNum = num_txt.toULongLong(&isSu,10);
    else if(basic=="o"||basic=="O")getNum = num_txt.toULongLong(&isSu,8);
    else if(basic=="b"||basic=="B")getNum = num_txt.toULongLong(&isSu,2);
    else if(basic=="f"||basic=="F")getNum = num_txt.toDouble(&isSu);
    else{
        getNum = immeStr.toLongLong(&isSu,10);
    }

    if(isSu==0){
        *status = 4;
        return QDwordArray();
    }

    QDwordArray wordArr = getNum.toDWordArray();

    QDwordArray retCodeBin;

    qDebug()<<QStringList({argsList[0],QString::number(wordArr[0].toUInt(),16)+"H"});
    retCodeBin+=compileINIT({argsList[0],QString::number(wordArr[0].toUInt(),16)+"H"},status);
    if(*status != 0)return retCodeBin;

    retCodeBin+=compileINIT({argsList[1],QString::number(wordArr[1].toUInt(),16)+"H"},status);
    if(*status != 0)return retCodeBin;

    *status = 0;
    return retCodeBin;
}

QStringList opers = {"UADD","uadd",
                     "USUB","usub",
                     "UMUL","umul",
                     "UDIV","udiv",
                     "ADD","add",
                     "SUB","sub",
                     "MUL","mul",
                     "DIV","div",
                     "FADD","fadd",
                     "FSUB","fsub",
                     "FMUL","fmul",
                     "FDIV","fdiv",
                     "SAL","sal",
                     "SAR","sar",
                     "SOL","sol",
                     "SOR","sor",
                     "AND","and",
                     "OR","or",
                     "XOR","xor"};
QStringList opers_not = {"NOT","not"};
QStringList trans_mov = {"UITF","uitf",
                         "ITF","itf",
                         "FTI","fti",
                         "TPN","tpn",
                         "MOV","mov",
                         "UEC","uec",
                         "UNEC","unec",
                         "UMC","umc",
                         "ULC","ulc",
                         "UMEC","umec",
                         "ULEC","ulec",
                         "FEC","fec",
                         "FNEC","fnec",
                         "FMC","fmc",
                         "FLC","flc",
                         "FMEC","fmec",
                         "FLEC","flec",
                         "EC","ec",
                         "NEC","nec",
                         "MC","mc",
                         "LC","lc",
                         "MEC","mec",
                         "LEC","lec",
                         "BTD","btd",
                         "WTD","wtd"};
QStringList ass = {"ASS","ass"};
QStringList jmp_int = {"JNE","jne",
                          "JMP","jmp",
                          "JT","jt",
                          "INT","int"};
QStringList rd_we = {"WE","we",
                     "RD","rd"};
QStringList push = {"PUSH","push"};
QStringList srw = {"SRD","srd",
                   "SWE","swe"};
QStringList bitOper = {"SET","set",
                       "GET","get"};
QStringList init = {"INIT","init"};
QStringList init64 = {"INIT64","init64"};
#define CodeTypeOper 1
#define CodeTypeOperNot 2
#define CodeTypeTransMov 3
#define CodeTypeAss 4
#define CodeTypeJMPINT 5
#define CodeTypeRDWE 6
#define CodeTypePush 7
#define CodeTypeSrw 8
#define CodeTypeBitoper 9
#define CodeTypeInit 10
#define CodeTypeInit64 11
int getCodeType(QString order){
    if(opers.contains(order)){
        return CodeTypeOper;
    }else if(opers_not.contains(order)){
        return CodeTypeOperNot;
    }else if(trans_mov.contains(order)){
        return CodeTypeTransMov;
    }else if(ass.contains(order)){
        return CodeTypeAss;
    }else if(jmp_int.contains(order)){
        return CodeTypeJMPINT;
    }else if(rd_we.contains(order)){
        return CodeTypeRDWE;
    }else if(push.contains(order)){
        return CodeTypePush;
    }else if(srw.contains(order)){
        return CodeTypeSrw;
    }else if(bitOper.contains(order)){
        return CodeTypeBitoper;
    }else if(init.contains(order)){
        return CodeTypeInit;
    }else if(init64.contains(order)){
        return CodeTypeInit64;
    }
    return 0;
}
////////////////移植接口重写//////////////////////////
//当前指令是否支持使用标记
bool thisCodeOrderIsSupportLabel(QString order,int argIndex,int argTotal){
    int type = getCodeType(order);

    if(type==CodeTypeOper){
        return (argTotal==2 && argIndex==1) ||
                (argTotal==3 && argIndex==2);
    }else if(type==CodeTypeOperNot){
        return (argTotal==2 && argIndex==1);
    }else if(type==CodeTypeTransMov ||
             type==CodeTypeAss ||
             type==CodeTypePush||
             type==CodeTypeInit){
        return (argTotal==2 && argIndex==1);
    }else if(type==CodeTypeJMPINT){
        return (argTotal==1 && argIndex==0);
    }else if(type==CodeTypeRDWE||
             type==CodeTypeSrw||
             type==CodeTypeBitoper||
             type==CodeTypeInit64){
        return (argTotal==3 && argIndex==2);
    }
    return false;
}
//该指令支持的标记最大字节数
uint thisCodeOrderSupportMaxBits(QString order){
    int type = getCodeType(order);

    if(type==CodeTypeTransMov||
       type==CodeTypeAss||
       type==CodeTypeRDWE||
       type==CodeTypePush){
        return 16;
    }else if(type==CodeTypeOper){
        return 12;
    }else if(type==CodeTypeJMPINT){
        return 27;
    }else if(type==CodeTypeSrw){
        return 21;
    }else if(type==CodeTypeBitoper){
        return 6;
    }else if(type==CodeTypeInit){
        return 32;
    }else if(type==CodeTypeInit64){
        return 64;
    }
    return 0;
}

//标记引用语法中未声明higth,low。取默认的higth,low
void getMarkDefaultHL(QString order,uint &h,uint &l){
    int orderType = getCodeType(order);
    if(orderType==CodeTypeOper){
        h = 11;
    }else if(orderType==CodeTypeOperNot){
        h = 11;
    }else if(orderType==CodeTypeTransMov){
        h = 15;
    }else if(orderType==CodeTypeAss){
        h = 15;
    }else if(orderType==CodeTypeJMPINT){
        h = 26;
    }else if(orderType==CodeTypeRDWE){
        h = 15;
    }else if(orderType==CodeTypeSrw){
        h = 20;
    }else if(orderType==CodeTypeBitoper){
        h = 5;
    }else if(orderType==CodeTypePush){
        h = 15;
    }else{
        h = 31;
    }
    l = 0;
}

//确定标记的order_offset_Byte、order_offset_Bit
QList<MarkQuote> markQuoteFromCodeOrder(QString order,
                                        int low,int higth,int offset,QString blockName,QString markName//analysisCodeBlockMarkQuote编译生成的
                                        ){
    //需要生成quote.order_offset_Byte和quote.order_offset_Bit
    QList<MarkQuote> quotes;
    MarkQuote quote;
    //[标记内存块.内存块位置+偏移量](高位-低位)
    quote.low = low;//标记值的地位
    quote.higth = higth;//标记值的高位
    quote.offset = offset;//标记值的偏移量
    quote.affiBlockName = blockName;//标记指向的内存块
    quote.name = markName;//标记指向的内存块的位置


    int bits = higth-low+1;
    int type = getCodeType(order);
    if(type==CodeTypeTransMov||
       type==CodeTypeAss||
       type==CodeTypeRDWE||
       type==CodeTypePush||
       type==CodeTypeOper ||
       type==CodeTypeOperNot||
       type==CodeTypeJMPINT||
       type==CodeTypeSrw){
        bits = 32-bits;
        quote.order_offset_Byte = bits/8;
        quote.order_offset_Bit = bits%8;
        quotes.append(quote);
        return quotes;
    }else if(type==CodeTypeBitoper){
        bits = 32-bits-10;
        quote.order_offset_Byte = 4-bits/8;
        quote.order_offset_Bit = 8-bits%8;
        quotes.append(quote);
        return quotes;
    }else if(type==CodeTypeInit){
        if(higth>=16){
            quote.higth = higth;
            quote.low = 16;
            bits = 32-(quote.higth-quote.low+1);
            quote.order_offset_Byte = bits/8;
            quote.order_offset_Bit = bits%8;
            quotes.append(quote);
        }
        if(low<=15){
            quote.higth = 15;
            quote.low = low;
            bits = 32-(quote.higth-quote.low+1);
            quote.order_offset_Byte = bits/8 + 4;
            quote.order_offset_Bit = bits%8;
            quotes.append(quote);
        }
        return quotes;
    }else if(type==CodeTypeInit64){
        QList<int> markBitOffset;
        int higth,low,bitOff;
        if(quote.higth>=48){
            higth = quote.higth;
            low = quote.low<48 ? 48 : quote.low;
            bitOff = higth-low+1;
            bitOff = 16-bitOff;
            markBitOffset.append(bitOff);
        }
        if(quote.higth>=32 && quote.low<48){
            higth = quote.higth>47 ? 47 : quote.higth;
            low = quote.low<32 ? 32 : quote.low;
            bitOff = higth-low+1;
            bitOff = 32-bitOff;
            markBitOffset.append(bitOff);
        }
        if(quote.higth>=16 && quote.low<32){
            higth = quote.higth>31 ? 31 : quote.higth;
            low = quote.low<16 ? 16 : quote.low;
            bitOff = higth-low+1;
            bitOff = 48-bitOff;
            markBitOffset.append(bitOff);
        }
        if(quote.low<16){
            higth = quote.higth>15 ? 15 : quote.higth;
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
    }
    return QList<MarkQuote>();
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
    if(status==NULL)return 0;
    *status = 1;
    if(name=="NOP"||name=="nop"){//空指令。让cpu空闲一个周期 0
        *status = 0;
        return DWORD(0).toByteArray();
    }else if(name=="RT"||name=="rt"){//功能块返回 14.0
        *status = 0;
        return DWORD(14<<27).toByteArray();
    }else if(name=="IT"||name=="it"){//中断返回 14.1
        *status = 0;
        return DWORD(14<<27 | 1).toByteArray();
    }else if(name=="EIT"||name=="eit"){//中断返回并开启中断 14.0
        *status = 0;
        return DWORD(14<<27 | 2).toByteArray();
    }else if(name=="VIT"||name=="vit"){//中断返回并开启虚拟内存\中断 14.1
        *status = 0;
        return DWORD(14<<27 | 3).toByteArray();
    }else if(name=="PVT"||name=="pvt"){//中断返回并开启虚拟内存\中断\保护模式 14.1
        *status = 0;
        return DWORD(14<<27 | 4).toByteArray();
    }
    else if(name=="EI"||name=="ei"){//开启中断 15
        *status = 0;
        return DWORD(15<<27).toByteArray();
    }else if(name=="DI"||name=="di"){//关闭中断 15.1
        *status = 0;
        return DWORD(15<<27 | 1).toByteArray();
    }else if(name=="EP"||name=="ep"){//开启保护模式 15.2
        *status = 0;
        return DWORD(15<<27 | 2).toByteArray();
    }else if(name=="EV"||name=="ev"){//开启虚拟内存模式 15.3
        *status = 0;
        return DWORD(15<<27 | 3).toByteArray();
    }else if(name=="DV"||name=="dv"){//关闭虚拟内存模式 15.4
        *status = 0;
        return DWORD(15<<27 | 4).toByteArray();
    }else if(name=="RST"||name=="rst"){//重启cpu 31
        *status = 0;
        return DWORD(31<<27).toByteArray();
    }else if(name=="UADD" || name=="uadd" ){//1.0
        return compileOper(1,0,args,status).toByteArray();
    }else if(name=="USUB" || name=="usub" ){//1.1
        return compileOper(1,1,args,status).toByteArray();
    }else if(name=="UMUL" || name=="umul" ){//1.2
        return compileOper(1,2,args,status).toByteArray();
    }else if(name=="UDIV" || name=="udiv" ){//1.3
        return compileOper(1,3,args,status).toByteArray();
    }else if(name=="ADD" || name=="add" ){//2.0
        return compileOper(2,0,args,status).toByteArray();
    }else if(name=="SUB" || name=="sub" ){//2.1
        return compileOper(2,1,args,status).toByteArray();
    }else if(name=="MUL" || name=="mul" ){//2.2
        return compileOper(2,2,args,status).toByteArray();
    }else if(name=="DIV" || name=="div" ){//2.3
        return compileOper(2,3,args,status).toByteArray();
    }else if(name=="FADD" || name=="fadd" ){//3.0
        return compileOper(3,0,args,status).toByteArray();
    }else if(name=="FSUB" || name=="fsub" ){//3.1
        return compileOper(3,1,args,status).toByteArray();
    }else if(name=="FMUL" || name=="fmul" ){//3.2
        return compileOper(3,2,args,status).toByteArray();
    }else if(name=="FDIV" || name=="fdiv" ){//3.3
        return compileOper(3,3,args,status).toByteArray();
    }else if(name=="SAL" || name=="sal" ){//5.0
        return compileOper(5,0,args,status).toByteArray();
    }else if(name=="SAR" || name=="sar" ){//5.1
        return compileOper(5,1,args,status).toByteArray();
    }else if(name=="SOL" || name=="sol" ){//5.2
        return compileOper(5,2,args,status).toByteArray();
    }else if(name=="SOR" || name=="sor" ){//5.3
        return compileOper(5,3,args,status).toByteArray();
    }else if(name=="UITF" || name=="uitf" ){//4.0
        return compileTRANS_MOV(4,0,args,status).toByteArray();
    }else if(name=="ITF" || name=="itf" ){//4.1
        return compileTRANS_MOV(4,1,args,status).toByteArray();
    }else if(name=="FTI" || name=="fti" ){//4.2
        return compileTRANS_MOV(4,2,args,status).toByteArray();
    }else if(name=="TPN" || name=="tpn" ){//4.3
        return compileTRANS_MOV(4,3,args,status).toByteArray();
    }else if(name=="AND" || name=="and" ){//6.0
        return compileOper(6,0,args,status).toByteArray();
    }else if(name=="OR" || name=="or" ){//6.1
        return compileOper(6,1,args,status).toByteArray();
    }else if(name=="NOT" || name=="not" ){//6.2
        return compileOper(6,2,args,status).toByteArray();
    }else if(name=="XOR" || name=="xor" ){//6.3
        return compileOper(6,3,args,status).toByteArray();
    }else if(name=="RD" || name=="rd"){//7.0
        return compileRD_WE(7,0,args,status).toByteArray();
    }else if(name=="WE" || name=="we" ){//7.1
        return compileRD_WE(7,1,args,status).toByteArray();
    }else if(name=="SRD" || name=="srd"){//16
        return compileSRW(16,args,status).toByteArray();
    }else if(name=="SWE" || name=="swe"){//17
        return compileSRW(17,args,status).toByteArray();
    }else if(name=="POP" || name=="pop"){//8.0
        return compilePOP(8,args,status).toByteArray();
    }else if(name=="PUSH" || name=="push" ){//8.1
        return compilePUSH(8,args,status).toByteArray();
    }else if(name=="MOV" || name=="mov" ){//9.0
        return compileTRANS_MOV(9,0,args,status).toByteArray();
    }else if(name=="ASS" || name=="ass" ){//9.1
        return compileASS(9,1,args,status).toByteArray();
    }else if(name=="JNE" || name=="jne" ){//10
        return compileJMP_JT_INT(10,args,status).toByteArray();
    }else if(name=="JMP" || name=="jmp" ){//11
        return compileJMP_JT_INT(11,args,status).toByteArray();
    }else if(name=="JT" || name=="jt" ){//12
        return compileJMP_JT_INT(12,args,status).toByteArray();
    }else if(name=="INT" || name=="int" ){//13
        return compileJMP_JT_INT(13,args,status).toByteArray();
    }else if(name=="UEC" || name=="uec"){//20.0
        return compileTRANS_MOV(20,0,args,status).toByteArray();
    }else if(name=="UNEC" || name=="unec"){//20.1
        return compileTRANS_MOV(20,1,args,status).toByteArray();
    }else if(name=="UMC" || name=="umc"){//20.2
        return compileTRANS_MOV(20,2,args,status).toByteArray();
    }else if(name=="ULC" || name=="ulc"){//20.3
        return compileTRANS_MOV(20,3,args,status).toByteArray();
    }else if(name=="UMEC" || name=="umec"){//20.4
        return compileTRANS_MOV(20,4,args,status).toByteArray();
    }else if(name=="ULEC" || name=="ulec"){//20.5
        return compileTRANS_MOV(20,5,args,status).toByteArray();
    }else if(name=="EC" || name=="ec"){//21.0
        return compileTRANS_MOV(21,0,args,status).toByteArray();
    }else if(name=="NEC" || name=="nec"){//21.1
        return compileTRANS_MOV(21,1,args,status).toByteArray();
    }else if(name=="MC" || name=="mc"){//21.2
        return compileTRANS_MOV(21,2,args,status).toByteArray();
    }else if(name=="LC" || name=="lc"){//21.3
        return compileTRANS_MOV(21,3,args,status).toByteArray();
    }else if(name=="MEC" || name=="mec"){//21.4
        return compileTRANS_MOV(21,4,args,status).toByteArray();
    }else if(name=="LEC" || name=="lec"){//21.5
        return compileTRANS_MOV(21,5,args,status).toByteArray();
    }else if(name=="FEC" || name=="fec"){//22.0
        return compileTRANS_MOV(22,0,args,status).toByteArray();
    }else if(name=="FNEC" || name=="fnec"){//22.1
        return compileTRANS_MOV(22,1,args,status).toByteArray();
    }else if(name=="FMC" || name=="fmc"){//22.2
        return compileTRANS_MOV(22,2,args,status).toByteArray();
    }else if(name=="FLC" || name=="flc"){//22.3
        return compileTRANS_MOV(22,3,args,status).toByteArray();
    }else if(name=="FMEC" || name=="fmec"){//22.4
        return compileTRANS_MOV(22,4,args,status).toByteArray();
    }else if(name=="FLEC" || name=="flec"){//22.5
        return compileTRANS_MOV(22,5,args,status).toByteArray();
    }else if(name=="BTD" || name=="btd" ){//18.0
        return compileTRANS_MOV(18,0,args,status).toByteArray();
    }else if(name=="WTD" || name=="wtd" ){//18.1
        return compileTRANS_MOV(18,1,args,status).toByteArray();
    }else if(name=="SET" || name=="set" ){//19.0
        return compileBitOper(19,0,args,status).toByteArray();
    }else if(name=="GET" || name=="get" ){//19.1
        return compileBitOper(19,1,args,status).toByteArray();
    }else if(name=="INIT"||name=="init"){//9.0 - 9.1
        return DwordToByteArray(compileINIT(args,status));
    }else if(name=="INIT64"||name=="init64"){//9.0 - 9.1
        return DwordToByteArray(compileINIT64(args,status));
    }
    QByteArray bin;
    return bin;
}

QStringList asmCustomInstructions(){
    return QStringList({"NOP","nop",
                       "RT","rt",
                       "IT","it",
                       "EIT","eit",
                       "VIT","vit",
                       "PVT","pvt",
                       "EI","ei",
                       "DI","di",
                       "EP","ep",
                       "EV","ev",
                       "DV","dv",
                       "RST","rst",
                       "UADD","uadd",
                       "USUB","usub",
                       "UMUL","umul",
                       "UDIV","udiv",
                       "ADD","add",
                       "SUB","sub",
                       "MUL","mul",
                       "DIV","div",
                       "FADD","fadd",
                       "FSUB","fsub",
                       "FMUL","fmul",
                       "FDIV","fdiv",
                       "SAL","sal",
                       "SAR","sar",
                       "SOL","sol",
                       "SOR","sor",
                       "UITF","uitf",
                       "ITF","itf",
                       "FTI","fti",
                       "TPN","tpn",
                       "AND","and",
                       "OR","or",
                       "NOT","not",
                       "XOR","xor",
                       "WE","we",
                       "RD","rd",
                       "SRD","srd",
                       "SWE","swe",
                       "PUSH","push",
                       "POP","pop",
                       "MOV","mov",
                       "ASS","ass",
                       "JNE","jne",
                       "JMP","jmp",
                       "JT","jt",
                       "INT","int",
                       "UEC","uec",
                       "UNEC","unec",
                       "UMC","umc",
                       "ULC","ulc",
                       "UMEC","umec",
                       "ULEC","ulec",
                       "FEC","fec",
                       "FNEC","fnec",
                       "FMC","fmc",
                       "FLC","flc",
                       "FMEC","fmec",
                       "FLEC","flec",
                       "EC","ec",
                       "NEC","nec",
                       "MC","mc",
                       "LC","lc",
                       "MEC","mec",
                       "LEC","lec",
                       "BTD","btd",
                       "WTD","wtd",
                       "SET","set",
                       "GET","get",
                       "INIT","init",
                       "INIT64","init64",
                       "BYTE","byte",
                       "WORD","word",
                       "DWORD","dword",
                       "R1","r1",
                       "R2","r2",
                       "R3","r3",
                       "R4","r4",
                       "R5","r5",
                       "R6","r6",
                       "R7","r7","CS","cs",
                       "R8","r8","DS","ds",
                       "FLAG","flag",
                       "TPC","tpc",
                       "IPC","ipc",
                       "PC","pc",
                       "SP","sp",
                       "SYS","sys",
                       "TLB","tlb"});
}
#endif
