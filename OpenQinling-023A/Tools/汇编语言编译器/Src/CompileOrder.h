#ifndef COMPILEORDER_H
#define COMPILEORDER_H
#pragma execution_character_set("utf-8")
#include <QString>
#include <QStringList>
#include "CompileErrorHandle.h"
#include "QDebug"

#define FALSE 0xffffffff
//解析汇编指令中运算参数通道的二进制码
uint compileDataChannel(QString arg_txt){
    if(arg_txt=="R1"||arg_txt=="r1"){
        return 1;
    }
    else if(arg_txt=="R2"||arg_txt=="r2"){
        return 2;
    }
    else if(arg_txt=="R3"||arg_txt=="r3"){
        return 3;
    }
    else if(arg_txt=="R4"||arg_txt=="r4"){
        return 4;
    }
    else if(arg_txt=="R5"||arg_txt=="r5"){
        return 5;
    }
    else if(arg_txt=="R6"||arg_txt=="r6"){
        return 6;
    }
    else if(arg_txt=="CS"||arg_txt=="cs"){
        return 7;
    }
    else if(arg_txt=="DS"||arg_txt=="ds"){
        return 8;
    }
    else if(arg_txt=="FLAG"||arg_txt=="flag"){
        return 9;
    }
    else if(arg_txt=="PC"||arg_txt=="pc"){
        return 10;
    }
    else if(arg_txt=="TPC"||arg_txt=="tpc"){
        return 11;
    }
    else if(arg_txt=="IPC"||arg_txt=="ipc"){
        return 12;
    }
    else if(arg_txt=="SP"||arg_txt=="sp"){
        return 13;
    }
    else if(arg_txt=="TLB"||arg_txt=="tlb"){
        return 14;
    }
    else if(arg_txt=="SYS"||arg_txt=="sys"){
        return 15;
    }
    else{
        return FALSE;
    }
    return FALSE;
}
//解析汇编指令中内存、栈读写的数据大小二进制码
uint compileRwDataSize(QString arg_txt){
    if(arg_txt=="BYTE"||arg_txt=="byte"){
        return 1;
    }
    else if(arg_txt=="WORD"||arg_txt=="word"){
        return 2;
    }
    else if(arg_txt=="DWORD"||arg_txt=="dword"){
        return 3;
    }
    else{
        return FALSE;
    }
}
//解析汇编指令中可为立即数的参数通道
uint compileImmeNumChannnel(QString arg_txt,uint*num){
    if(arg_txt=="R1"||arg_txt=="r1"){
        return 1;
    }
    else if(arg_txt=="R2"||arg_txt=="r2"){
        return 2;
    }
    else if(arg_txt=="R3"||arg_txt=="r3"){
        return 3;
    }
    else if(arg_txt=="R4"||arg_txt=="r4"){
        return 4;
    }
    else if(arg_txt=="R5"||arg_txt=="r5"){
        return 5;
    }
    else if(arg_txt=="R6"||arg_txt=="r6"){
        return 6;
    }
    else if(arg_txt=="CS"||arg_txt=="cs"){
        return 7;
    }
    else if(arg_txt=="DS"||arg_txt=="ds"){
        return 8;
    }
    else if(arg_txt=="FLAG"||arg_txt=="flag"){
        return 9;
    }
    else if(arg_txt=="PC"||arg_txt=="pc"){
        return 10;
    }
    else if(arg_txt=="TPC"||arg_txt=="tpc"){
        return 11;
    }
    else if(arg_txt=="IPC"||arg_txt=="ipc"){
        return 12;
    }
    else if(arg_txt=="SP"||arg_txt=="sp"){
        return 13;
    }
    else if(arg_txt=="TLB"||arg_txt=="tlb"){
        return 14;
    }
    else if(arg_txt=="SYS"||arg_txt=="sys"){
        return 15;
    }
    else{
        QString basic = arg_txt.right(1);
        QString num_txt = arg_txt.left(arg_txt.length()-1);
        bool isSu;
       uint getNum;
        if(basic=="h"||basic=="H"){
            getNum = num_txt.toUInt(&isSu,16);
        }
        else if(basic=="d"||basic=="D"){

            getNum = num_txt.toUInt(&isSu,10);

        }
        else if(basic=="o"||basic=="O"){
            getNum = num_txt.toUInt(&isSu,8);
        }
        else if(basic=="b"||basic=="B"){
            getNum = num_txt.toUInt(&isSu,2);
        }
        else{
            return FALSE;
        }

        if(isSu){
            *num = getNum;
            return 0;
        }
        else{
            return FALSE;
        }
    }
    return FALSE;
}

//格式转换运算、转移运算、比较运算 格式特点[31:27]主模式 [26:24]子模式 [23:20]参数1 [19:16]参数2 [15:0]立即数(0-65535)
uint compileTRANS_MOV(int mode,int sonMode,QStringList argsList){
    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2)return FALSE;

    uint operand1 = compileDataChannel(argsList.at(0));
    if(operand1==FALSE)return FALSE;

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1),&num);
    if(operand2==FALSE)return FALSE;

    if(operand2==0){
        if(num>65535)return FALSE;
        orderBin = orderBin | num;
    }else{
        orderBin = orderBin | operand2<<16;
    }
    return orderBin;
}

//编译四则运算/移位运算/二进制运算 格式特点[31:27]主模式 [26:24]子模式 [23:20]参数1 [19:16]参数2 [15:12]写回参数 [11:0]立即数(0-4095)
uint compileALU(int mode,int sonMode,QStringList argsList){
    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2 && argsList.length()!=3)return FALSE;

    int arg_mode = argsList.length()==3;//汇编格式是否为3参数
    uint operand1 = compileDataChannel(argsList.at(0+arg_mode));
    if(operand1==FALSE)return FALSE;

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1+arg_mode),&num);
    if(operand2==FALSE)return FALSE;

    if(operand2==0){
        if(num>4095)return FALSE;
        orderBin = orderBin | num;
    }else{
        orderBin = orderBin | operand2<<16;
    }


    if(arg_mode){
        uint back = compileDataChannel(argsList.at(0));
        if(back==FALSE)return FALSE;
        orderBin = orderBin | back<<12;
    }else{
        orderBin = orderBin | operand1<<12;
    }
    return orderBin;
}

//编译跳转/软中断指令 格式特点[31:27]主模式 [26:0]立即数(0FALSE34217728)
uint compileJMP_JT_INT(int mode,QStringList argsList){
    uint orderBin = mode<<27;
    if(argsList.length()!=1)return FALSE;

    uint num = 0;
    uint operand1 = compileImmeNumChannnel(argsList.at(0),&num);
    if(operand1!=0)return FALSE;
    if(mode==13 && (num<16 | num>255))return FALSE;
    else if(num>134217727) return FALSE;
    return orderBin | num;
}

//栈读写指令 [31:27]主模式 [26:25]读写字节数 [24:21]读写的寄存器 [20:0]地址立即数(0-2MB) 读写的实际地址=SP寄存器+地址立即数
uint compileSRW(int mode,QStringList argsList){
    uint orderBin = mode<<27;
    if(argsList.length()!=3)return FALSE;

    uint size = compileRwDataSize(argsList.at(0));
    if(size==FALSE)return FALSE;

    orderBin |= size<<25;

    uint rwReg = compileDataChannel(argsList.at(1));

    orderBin |= rwReg<<21;

    uint off = 0;
    uint address = compileImmeNumChannnel(argsList.at(2),&off);
    if(address!=0)return FALSE;

    orderBin |= off;

    return orderBin;
}

//位操作指令 [31:27]主模式 [26:24]子模式 [23:20]写入寄存器 [19:16]源寄存器 [15:10]立即数(0-63) [9:5]高位限制 [4:0]低位限制
uint compileBitOper(int subMode,QStringList argsList){
    uint orderBin = 19<<27 | subMode<<24;

    if(argsList.length()!=3)return FALSE;

    //编译高低位限制
    QStringList ml_txt = argsList.at(0).split("-");
    if(ml_txt.length()!=2)return FALSE;

    bool m_src = 0,l_src=0;
    uint m = ml_txt.at(0).toUInt(&m_src);
    uint l = ml_txt.at(1).toUInt(&l_src);
    if(m_src==0 || l_src==0)return FALSE;
    if(m>31 || l>31 || l>m)return FALSE;

    orderBin |= m<<5 | l;

    //编译目标寄存器通道
    uint p_reg = compileDataChannel(argsList.at(1));
    if(p_reg==FALSE)return FALSE;
    orderBin |= p_reg<<20;

    //编译源寄存器通道
    uint inum = 0;
    uint s_reg = compileImmeNumChannnel(argsList.at(2),&inum);
    if(s_reg==FALSE)return FALSE;
    if(s_reg==0){
        if(inum>63)return FALSE;
        orderBin |= inum <<10;
    }else{
        orderBin |= s_reg<<16;
    }

    return orderBin;
}

//汇编文本编译为二进制指令[需先规范化代码格式，否则编译会出错]
QByteArray compileCode(QStringList asm_order,uint* isSuccess){
    QByteArray bin;
    uint index=0;

    foreach(QString order_txt,asm_order){
        uint this_order_bin;
        index++;
        if(order_txt=="NULL"||order_txt=="null"){//空指令。让cpu空闲一个周期 0
            this_order_bin = 0;
        }else if(order_txt=="RT"||order_txt=="rt"){//功能块返回 14.0
            this_order_bin = 14<<27;
        }else if(order_txt=="IT"||order_txt=="it"){//中断返回 14.1
            this_order_bin = 14<<27 | 1;
        }else if(order_txt=="EIT"||order_txt=="eit"){//中断返回并开启中断 14.0
            this_order_bin = 14<<27 | 2;
        }else if(order_txt=="VIT"||order_txt=="vit"){//中断返回并开启虚拟内存\中断 14.1
            this_order_bin = 14<<27 | 3;
        }else if(order_txt=="PVT"||order_txt=="pvt"){//中断返回并开启虚拟内存\中断\保护模式 14.1
            this_order_bin = 14<<27 | 4;
        }
        else if(order_txt=="EI"||order_txt=="ei"){//开启中断 15
            this_order_bin = 15<<27;
        }else if(order_txt=="DI"||order_txt=="di"){//关闭中断 15.1
            this_order_bin = 15<<27 | 1;
        }else if(order_txt=="EP"||order_txt=="ep"){//开启保护模式 15.2
            this_order_bin = 15<<27 | 2;
        }else if(order_txt=="EV"||order_txt=="ev"){//开启虚拟内存模式 15.3
            this_order_bin = 15<<27 | 3;
        }else if(order_txt=="DV"||order_txt=="dv"){//关闭虚拟内存模式 15.4
            this_order_bin = 15<<27 | 4;
        }else if(order_txt=="RST"||order_txt=="rst"){//重启cpu 31
            this_order_bin = 31<<27;
        }
        else{//有参指令的处理
            //解析出指令操作码、参数
            QStringList orderArgs = order_txt.split(":");
            if(orderArgs.length()!=2){
                if(isSuccess!=NULL){*isSuccess=0;}

                return compileError(index,order_txt,"format error");
            }
            QString order = orderArgs.at(0);
            orderArgs = orderArgs.at(1).split(",");
            uint order_bin;

            if(order=="UADD" || order=="uadd" ){//1.0
                order_bin = compileALU(1,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="USUB" || order=="usub" ){//1.1
                order_bin = compileALU(1,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UMUL" || order=="umul" ){//1.2
                order_bin = compileALU(1,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UDIV" || order=="udiv" ){//1.3
                order_bin = compileALU(1,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="ADD" || order=="add" ){//2.0
                order_bin = compileALU(2,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SUB" || order=="sub" ){//2.1
                order_bin = compileALU(2,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="MUL" || order=="mul" ){//2.2
                order_bin = compileALU(2,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="DIV" || order=="div" ){//2.3
                order_bin = compileALU(2,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FADD" || order=="fadd" ){//3.0
                order_bin = compileALU(3,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FSUB" || order=="fsub" ){//3.1
                order_bin = compileALU(3,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FMUL" || order=="fmul" ){//3.2
                order_bin = compileALU(3,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FDIV" || order=="fdiv" ){//3.3
                order_bin = compileALU(3,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SAL" || order=="sal" ){//5.0
                order_bin = compileALU(5,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SAR" || order=="sar" ){//5.1
                order_bin = compileALU(5,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SOL" || order=="sol" ){//5.2
                order_bin = compileALU(5,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SOR" || order=="sor" ){//5.3
                order_bin = compileALU(5,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UITF" || order=="uitf" ){//4.0
                order_bin = compileTRANS_MOV(4,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="ITF" || order=="itf" ){//4.1
                order_bin = compileTRANS_MOV(4,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FTI" || order=="fti" ){//4.2
                order_bin = compileTRANS_MOV(4,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="TPN" || order=="tpn" ){//4.3
                order_bin = compileTRANS_MOV(4,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="AND" || order=="and" ){//6.0
                order_bin = compileALU(6,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="OR" || order=="or" ){//6.1
                order_bin = compileALU(6,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="NOT" || order=="not" ){//6.2
                if(orderArgs.length()!=2){
                    if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"format error");
                }
                order_bin = compileALU(6,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="XOR" || order=="xor" ){//6.3
                order_bin = compileALU(6,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="RD" || order=="rd"){//7.0
                if(orderArgs.length()!=3){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"order format error");}
                order_bin = 7<<27;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<20;

                uint num;
                uint address = compileImmeNumChannnel(orderArgs.at(2),&num);
                if(address==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                else if(address == 0){
                    if(num>65535){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | address<<16;
                }
            }else if(order=="WE" || order=="we" ){//7.1
                if(orderArgs.length()!=3){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"order format error");}
                order_bin = 7<<27 | 1<<26;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<20;
                uint num;

                uint address = compileImmeNumChannnel(orderArgs.at(2),&num);
                if(address==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                else if(address == 0){
                    if(num>65535){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | address<<16;
                }
            }
            else if(order=="SRD" || order=="srd"){//16
                order_bin = compileSRW(16,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="SWE" || order=="swe"){//17
                order_bin = compileSRW(17,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="POP" || order=="pop"){//8.0
                if(orderArgs.length()!=2){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"order format error");}
                order_bin = 8<<27;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<16;


            }else if(order=="PUSH" || order=="push" ){//8.1
                if(orderArgs.length()!=2){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"order format error");}
                order_bin = 8<<27 | 1<<26;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint num;
                uint data = compileImmeNumChannnel(orderArgs.at(1),&num);
                if(data==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
                else if(data == 0){
                    if(num>65535){*isSuccess=0;return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | data<<16;
                }


            }else if(order=="MOV" || order=="mov" ){//9.0
                order_bin = compileTRANS_MOV(9,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="JNE" || order=="jne" ){//10
                order_bin = compileJMP_JT_INT(10,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }
            else if(order=="JMP" || order=="jmp" ){//11
                order_bin = compileJMP_JT_INT(11,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="JT" || order=="jt" ){//12
                order_bin = compileJMP_JT_INT(12,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="INT" || order=="int" ){//13
                order_bin = compileJMP_JT_INT(13,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }
            else if(order=="UEC" || order=="uec"){
                order_bin = compileTRANS_MOV(20,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UNEC" || order=="unec"){
                order_bin = compileTRANS_MOV(20,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UMC" || order=="umc"){
                order_bin = compileTRANS_MOV(20,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="ULC" || order=="ulc"){
                order_bin = compileTRANS_MOV(20,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="UMEC" || order=="umec"){
                order_bin = compileTRANS_MOV(20,4,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="ULEC" || order=="ulec"){
                order_bin = compileTRANS_MOV(20,5,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="EC" || order=="ec"){
                order_bin = compileTRANS_MOV(21,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="NEC" || order=="nec"){
                order_bin = compileTRANS_MOV(21,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="MC" || order=="mc"){
                order_bin = compileTRANS_MOV(21,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="LC" || order=="lc"){
                order_bin = compileTRANS_MOV(21,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="MEC" || order=="mec"){
                order_bin = compileTRANS_MOV(21,4,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="LEC" || order=="lec"){
                order_bin = compileTRANS_MOV(21,5,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FEC" || order=="fec"){
                order_bin = compileTRANS_MOV(22,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FNEC" || order=="fnec"){
                order_bin = compileTRANS_MOV(22,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FMC" || order=="fmc"){
                order_bin = compileTRANS_MOV(22,2,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FLC" || order=="flc"){
                order_bin = compileTRANS_MOV(22,3,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FMEC" || order=="fmec"){
                order_bin = compileTRANS_MOV(22,4,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="FLEC" || order=="flec"){
                order_bin = compileTRANS_MOV(22,5,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }
            else if(order=="BTD" || order=="btd" ){//18.0
                order_bin = compileTRANS_MOV(18,0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="WTD" || order=="wtd" ){//18.1
                order_bin = compileTRANS_MOV(18,1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }
            else if(order=="SET" || order=="set" ){//19.0
                order_bin = compileBitOper(0,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }else if(order=="GET" || order=="get" ){//19.1
                order_bin = compileBitOper(1,orderArgs);
                if(order_bin==FALSE){if(isSuccess!=NULL){*isSuccess=0;}return compileError(index,order_txt,"channel select error");}
            }
            else{
                if(isSuccess!=NULL){*isSuccess=0;}
                return compileError(index,order_txt,"thers is no such order");
            }
            this_order_bin = order_bin;
        }

        char*p = (char*)&this_order_bin;
        uint tmp;
        char*tp = (char*)&tmp;
        tp[0] = p[3];
        tp[1] = p[2];
        tp[2] = p[1];
        tp[3] = p[0];
        this_order_bin = tmp;
        bin.append((char*)&this_order_bin,4);
    }

    if(isSuccess!=NULL){
        *isSuccess = 1;
    }
    return bin;
}

#endif // COMPILEORDER_H
