#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#pragma execution_character_set("utf-8")

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
    else if(arg_txt=="R7"||arg_txt=="r7"){
        return 7;
    }
    else if(arg_txt=="R8"||arg_txt=="r8"){
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
        return -1;
    }
    return -1;
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
        return -1;
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
    else if(arg_txt=="R7"||arg_txt=="r7"){
        return 7;
    }
    else if(arg_txt=="R8"||arg_txt=="r8"){
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
            return -1;
        }

        if(isSu){
            *num = getNum;
            return 0;
        }
        else{
            return -1;
        }
    }
    return -1;
}

//编译出错的提示文本生成
QByteArray compileError(int index,QString asm_txt,QString prompt){
    char tmp[256];
    sprintf(tmp,"%d.<%s>=%s",index,asm_txt.toUtf8().data(),prompt.toUtf8().data());
    return tmp;
}
//编译四则运算/浮整转换/移位运算/二进制运算 格式特点[31:27]主模式 [26:25]子模式 [23:20]参数1 [19:26]参数2 [15:0]立即数
uint compileALU_MOV(int mode,int sonMode,QStringList argsList){
    uint orderBin = mode<<27 | sonMode<<24;
    if(argsList.length()!=2)return -1;

    uint operand1 = compileDataChannel(argsList.at(0));
    if(operand1==-1)return -1;

    orderBin = orderBin | operand1<<20;

    uint num = 0;
    uint operand2 = compileImmeNumChannnel(argsList.at(1),&num);
    if(operand2==-1)return -1;

    if(operand2==0){
        if(num>65535)return -1;
        orderBin = orderBin | num;
    }else{
        orderBin = orderBin | operand2<<16;
    }
    return orderBin;
}
//编译跳转/软中断指令 格式特点[31:27]主模式 [26:0]立即数
uint compileJMP_JT_INT(int mode,QStringList argsList){
    uint orderBin = mode<<27;
    if(argsList.length()!=1)return -1;

    uint num = 0;
    uint operand1 = compileImmeNumChannnel(argsList.at(0),&num);
    if(operand1!=0)return -1;
    if(mode==13 && (num<16 | num>255))return -1;
    else if(num>65535) return -1;

    return orderBin | num;
}

//汇编文本编译为二进制指令
QByteArray compileCode(QString asm_txt,uint* isSuccess){
    QByteArray bin;
    asm_txt = asm_txt.remove("\r");
    asm_txt = asm_txt.remove(" ");
    asm_txt = asm_txt.remove("\n");
    asm_txt = asm_txt.remove("\t");
    QStringList asm_order = asm_txt.split(";");
    asm_order.removeLast();
    uint index=0;

    foreach(QString order_txt,asm_order){
        uint this_order_bin;
        index++;
        if(order_txt=="NULL"||order_txt=="null"){//空指令。让cpu空闲一个周期
            this_order_bin = 0;
        }else if(order_txt=="RT"||order_txt=="rt"){//功能块返回
            this_order_bin = 14<<27;
        }else if(order_txt=="IT"||order_txt=="it"){//中断返回
            this_order_bin = 14<<27 | 1;
        }else if(order_txt=="EI"||order_txt=="ei"){//开启中断
            this_order_bin = 15<<27;
        }else if(order_txt=="DI"||order_txt=="di"){//关闭中断
            this_order_bin = 15<<27 | 1;
        }else if(order_txt=="EP"||order_txt=="ep"){//开启保护模式
            this_order_bin = 15<<27 | 2;
        }else if(order_txt=="EV"||order_txt=="ev"){//开启虚拟内存模式
            this_order_bin = 15<<27 | 3;
        }else if(order_txt=="DV"||order_txt=="dv"){//关闭虚拟内存模式
            this_order_bin = 15<<27 | 4;
        }else if(order_txt=="RST"||order_txt=="rst"){//重启cpu
            this_order_bin = 16<<27;
        }
        else{//有参指令的处理
            //解析出指令操作码、参数
            QStringList orderArgs = order_txt.split(":");
            if(orderArgs.length()!=2){*isSuccess=0;return compileError(index,order_txt,"format error");}
            QString order = orderArgs.at(0);
            orderArgs = orderArgs.at(1).split(",");
            uint order_bin;

            if(order=="UADD" || order=="uadd" ){
                order_bin = compileALU_MOV(1,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="USUB" || order=="usub" ){
                order_bin = compileALU_MOV(1,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="UMUL" || order=="umul" ){
                order_bin = compileALU_MOV(1,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="UDIV" || order=="udiv" ){
                order_bin = compileALU_MOV(1,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="ADD" || order=="add" ){
                order_bin = compileALU_MOV(2,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="SUB" || order=="sub" ){
                order_bin = compileALU_MOV(2,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="MUL" || order=="mul" ){
                order_bin = compileALU_MOV(2,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="DIV" || order=="div" ){
                order_bin = compileALU_MOV(2,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FADD" || order=="fadd" ){
                order_bin = compileALU_MOV(3,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FSUB" || order=="fsub" ){
                order_bin = compileALU_MOV(3,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FMUL" || order=="fmul" ){
                order_bin = compileALU_MOV(3,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FDIV" || order=="fdiv" ){
                order_bin = compileALU_MOV(3,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="SAL" || order=="sal" ){
                order_bin = compileALU_MOV(5,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="SAR" || order=="sar" ){
                order_bin = compileALU_MOV(5,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="SOL" || order=="sol" ){
                order_bin = compileALU_MOV(5,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="SOR" || order=="sor" ){
                order_bin = compileALU_MOV(5,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="UITF" || order=="uitf" ){
                order_bin = compileALU_MOV(4,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="ITF" || order=="itf" ){
                order_bin = compileALU_MOV(4,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FTI" || order=="fti" ){
                order_bin = compileALU_MOV(4,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="TPN" || order=="tpn" ){
                order_bin = compileALU_MOV(4,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="AND" || order=="and" ){
                order_bin = compileALU_MOV(6,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="OR" || order=="or" ){
                order_bin = compileALU_MOV(6,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="NOT" || order=="not" ){
                order_bin = compileALU_MOV(6,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="XOR" || order=="xor" ){
                order_bin = compileALU_MOV(6,3,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="RD" || order=="rd"){
                if(orderArgs.length()!=3){*isSuccess=0;return compileError(index,order_txt,"order format error");}
                order_bin = 7<<27;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<20;

                uint num;
                uint address = compileImmeNumChannnel(orderArgs.at(2),&num);
                if(address==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                else if(address == 0){
                    if(num>65535){*isSuccess=0;return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | address<<16;
                }

            }else if(order=="WE" || order=="we" ){
                if(orderArgs.length()!=3){*isSuccess=0;return compileError(index,order_txt,"order format error");}
                order_bin = 7<<27 | 1<<26;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<20;
                uint num;
                uint address = compileImmeNumChannnel(orderArgs.at(2),&num);
                if(address==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                else if(address == 0){
                    if(num>65535){*isSuccess=0;return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | address<<16;
                }
            }else if(order=="POP" || order=="pop"){
                if(orderArgs.length()!=2){*isSuccess=0;return compileError(index,order_txt,"order format error");}
                order_bin = 8<<27;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint data = compileDataChannel(orderArgs.at(1));
                if(data==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                order_bin = order_bin | data<<16;


            }else if(order=="PUSH" || order=="push" ){
                if(orderArgs.length()!=2){*isSuccess=0;return compileError(index,order_txt,"order format error");}
                order_bin = 8<<27 | 1<<26;
                uint size_bin = compileRwDataSize(orderArgs.at(0));
                if(size_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"rw size error");}
                order_bin = order_bin | size_bin<<24;

                uint num;
                uint data = compileImmeNumChannnel(orderArgs.at(1),&num);
                if(data==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
                else if(data == 0){
                    if(num>65535){*isSuccess=0;return compileError(index,order_txt,"num too big");}
                    order_bin = order_bin | num;
                }else{
                    order_bin = order_bin | data<<16;
                }


            }else if(order=="MOV" || order=="mov" ){
                order_bin = compileALU_MOV(9,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="JC" || order=="jc" ){
                order_bin = compileALU_MOV(10,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="JMP" || order=="jmp" ){
                order_bin = compileJMP_JT_INT(11,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="JT" || order=="jt" ){
                order_bin = compileJMP_JT_INT(12,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="INT" || order=="int" ){
                order_bin = compileJMP_JT_INT(13,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="UCMP" || order=="ucmp" ){
                order_bin = compileALU_MOV(17,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="CMP" || order=="cmp" ){
                order_bin = compileALU_MOV(17,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="FCMP" || order=="fcmp" ){
                order_bin = compileALU_MOV(17,2,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="BTD" || order=="btd" ){
                order_bin = compileALU_MOV(18,0,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else if(order=="WTD" || order=="wtd" ){
                order_bin = compileALU_MOV(18,1,orderArgs);
                if(order_bin==0xffffffff){*isSuccess=0;return compileError(index,order_txt,"channel select error");}
            }else{
                *isSuccess=0;
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

    qDebug()<<"compile code finish";
    return bin;
}

//将二进制数据编译为tb测试文本
QString compileTBtxt(QByteArray bin,uint address){
    QString tb_txt;
    for(int i = 0;i<bin.length();i++){
        QString num_16sys = QString::number((uchar)bin.data()[i],16);
        while(num_16sys.length()<2){
            num_16sys = num_16sys.insert(0,"0");
        }
        tb_txt+="\tdata["+QString::number(i+address)+"] <= 8'h"+num_16sys+";\r\n";
    }
    return tb_txt;
}

//编译有符号整形
unsigned long long compileUInt(QString order,uint*isSuccess){
    if(order.length()==0){
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }


    unsigned long long bin = 0;
    bool com_isSuccess = 0;

    if(order.at(order.length()-1)>=48 && order.at(order.length()-1)<=57){
        bin = order.toULongLong(&com_isSuccess,10);
    }else if(order.length()>=2){
        QString numSys = order.right(1);
        QString num = order.left(order.length()-1);
        if(numSys=="B"||numSys=="b"){
            bin = num.toULongLong(&com_isSuccess,2);
        }else if(numSys=="O"||numSys=="o"){
            bin = num.toULongLong(&com_isSuccess,8);
        }else if(numSys=="D"||numSys=="d"){
            bin = num.toULongLong(&com_isSuccess,10);
        }else if(numSys=="H"||numSys=="h"){
            bin = num.toULongLong(&com_isSuccess,16);
        }else{
            if(isSuccess!=NULL){
                *isSuccess = 0;
            }
            return 0;
        }
    }else{
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }
    if(com_isSuccess==0 && isSuccess!=NULL){
        *isSuccess = 0;
    }
    else if(isSuccess!=NULL){
        *isSuccess = 1;
    }
    return bin;
}

//编译无符号整形
long long compileInt(QString order,uint*isSuccess){
    if(order.length()==0){
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }


    unsigned long long bin = 0;
    bool com_isSuccess = 0;

    if(order.at(order.length()-1)>=48 && order.at(order.length()-1)<=57){
        bin = order.toLongLong(&com_isSuccess,10);
    }else if(order.length()>=2){
        QString numSys = order.right(1);
        QString num = order.left(order.length()-1);
        if(numSys=="B"||numSys=="b"){
            bin = num.toLongLong(&com_isSuccess,2);
        }else if(numSys=="O"||numSys=="o"){
            bin = num.toLongLong(&com_isSuccess,8);
        }else if(numSys=="D"||numSys=="d"){
            bin = num.toLongLong(&com_isSuccess,10);
        }else if(numSys=="H"||numSys=="h"){
            bin = num.toLongLong(&com_isSuccess,16);
        }else{
            if(isSuccess!=NULL){
                *isSuccess = 0;
            }
            return 0;
        }
    }else{
        if(isSuccess!=NULL){
            *isSuccess = 0;
        }
        return 0;
    }
    if(com_isSuccess==0 && isSuccess!=NULL){
        *isSuccess = 0;
    }
    else if(isSuccess!=NULL){
        *isSuccess = 1;
    }
    return bin;
}

//获取字符串中某种字符所有非被引号包裹的字符地址
QList<uint> getNoQuoCharIndex(QString order,QChar ch){
    QList<uint> spaceIndex;//文本中所有空格的索引号
    QList<uint> quoIndex;//文本中双引号的索引地址(\"排除掉)
    for(int i = 0;i<order.length();i++){
        if(order.at(i)==ch){
            spaceIndex.append(i);
        }
        else if(order.at(i)=='\"'){
            if(i!=0){
                if(order.at(i-1)!='\\'){
                    quoIndex.append(i);
                }
            }else{
                quoIndex.append(i);
            }
        }
    }

    //判断出被引号所包裹的字符索引号
    /*
     * 判断标准:
     *   一个字符前面的引号数量为单数，并在其之后至少存在一个引号
     */
    QList<uint> quoSpaceIndex;//被引号包裹的空格的索引号
    foreach(uint space,spaceIndex){
        int last_number = 0;//在该空格前的引号数量
        int next_number = 0;//在该空格后的引号数量
        foreach(uint quo,quoIndex){
            if(quo<space){
                last_number++;
            }else{
                next_number++;
            }
        }
        if(last_number%2==1 && next_number>=1){
            quoSpaceIndex.append(space);
        }
    }

    //筛选出非被引号包裹的
    QList<uint> noQuoSpaceIndex;
    foreach(uint space,spaceIndex){
        int isRepeat = 0;
        foreach(uint quoSpace,quoSpaceIndex){
            isRepeat = isRepeat || (quoSpace==space);
        }
        if(!isRepeat){
            noQuoSpaceIndex.append(space);
        }
    }
    return noQuoSpaceIndex;
}

//去除字符串引号包裹外的字符
QString removeChar(QString order,QChar ch){
    QList<uint> noQuoSpaceIndex = getNoQuoCharIndex(order,ch);
    //字符串每减去一个字符，会让后面的空格索引位置都减1
    for(int i = 0;i<noQuoSpaceIndex.length();i++){
        order.remove(noQuoSpaceIndex.at(i)-i,1);
    }
    return order;
}

//替换字符串引号包裹外的字符
QString replaceChar(QString order,QChar ch,QChar necChar){
    QList<uint> noQuoSpaceIndex = getNoQuoCharIndex(order,ch);
    //字符串每减去一个字符，会让后面的空格索引位置都减1
    for(int i = 0;i<noQuoSpaceIndex.length();i++){
        order.replace(noQuoSpaceIndex.at(i),1,necChar);
    }
    return order;
}
//字符串转义字符替换/字符串内数据取出[如果字符串外有字符，返回出错]
QByteArray replaceString(QByteArray txt){
    int isInString = 0;//当前遍历的字符是否在字符串中
    QByteArray strData;
    for(int i = 0;i<txt.length();i++){
        QChar c = txt.at(i);
        int inString = 0;

        if(c=='\"'){
            if(isInString){
                if(txt.at(i-1)!='\\'){
                    isInString = 0;
                }
            }else{
                if(isInString){
                    isInString = 0;
                }else{
                    inString = 1;
                }
            }
        }
        if(isInString){
            strData.append(txt.at(i));
        }

        if(inString){
            isInString = 1;
        }
    }

    QByteArray seplaceStr;//替换转义字符后的字符串
    for(int i=0;i<strData.length();i++){
        if(strData.at(i)=='\\'){
            if(i!=strData.length()-1){
                if(strData.at(i+1)=='a'){
                    seplaceStr.append('\a');
                    i++;
                }else if(strData.at(i+1)=='b'){
                    seplaceStr.append('\b');
                    i++;
                }else if(strData.at(i+1)=='f'){
                    seplaceStr.append('\f');
                    i++;
                }else if(strData.at(i+1)=='n'){
                    seplaceStr.append('\n');
                    i++;
                }else if(strData.at(i+1)=='r'){
                    seplaceStr.append('\r');
                    i++;
                }else if(strData.at(i+1)=='t'){
                    seplaceStr.append('\t');
                    i++;
                }else if(strData.at(i+1)=='v'){
                    seplaceStr.append('\v');
                    i++;
                }else if(strData.at(i+1)=='\\'){
                    seplaceStr.append('\\');
                    i++;
                }else if(strData.at(i+1)=='?'){
                    seplaceStr.append('\?');
                    i++;
                }else if(strData.at(i+1)=='\''){
                    seplaceStr.append('\'');
                    i++;
                }else if(strData.at(i+1)=='\"'){
                    seplaceStr.append('\"');
                    i++;
                }else if(strData.at(i+1)=='0'){
                    seplaceStr.append('\0');
                    i++;
                }
            }
        }else{
            seplaceStr.append(strData.at(i));
        }
    }
    return seplaceStr;
}

//数据编译器
QByteArray compileData(QString asm_txt,uint* isSuccess){
    QByteArray bin;
    asm_txt = asm_txt.remove("\r");
    asm_txt = asm_txt.remove("\n");
    asm_txt = removeChar(asm_txt,'\t');
    asm_txt = removeChar(asm_txt,' ');
    asm_txt = replaceChar(asm_txt,';','\n');
    QStringList asm_data = asm_txt.split("\n");
    asm_data.removeLast();
    uint index=0;

    foreach(QString order_txt,asm_data){
        QStringList orderArgs = order_txt.split(":");
        if(orderArgs.length()!=2){
            if(isSuccess!=NULL){
                *isSuccess=0;
            }
            return compileError(index,order_txt,"format error");
        }
        QString order = orderArgs.at(0);

        if(order=="BYTE"||order=="byte"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-127 || tmp>127){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                char data = (char)tmp;
                bin.append(data);
            }
        }else if(order=="UBYTE"||order=="ubyte"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>255){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                uchar data = (char)tmp;
                bin.append(data);
            }
        }else if(order=="SHORT"||order=="short"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-32767 || tmp>32767){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                short data = (short)tmp;
                char*p = (char*)&data;
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="USHORT"||order=="ushort"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>65535){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                ushort data = (ushort)tmp;
                char*p = (char*)&data;
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="INT"||order=="int"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp<-2147483647 || tmp>2147483647){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big or small");
                }
                int data = (int)tmp;
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="UINT"||order=="uint"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                else if(tmp>4294967296){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number too big");
                }
                uint data = (uint)tmp;
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="LONG"||order=="long"){

            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                long long tmp = compileInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                long long data = (long long)tmp;
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="ULONG"||order=="ulong"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                uint cmpSuccess = 0;
                unsigned long long tmp = compileUInt(thisData,&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                unsigned long long data = tmp;
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="FLOAT"||order=="float"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                bool cmpSuccess = 0;
                float data = thisData.toFloat(&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                char*p = (char*)&data;
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="DOUBLE"||order=="double"){
            foreach(QString thisData,orderArgs.at(1).split(',')){
                bool cmpSuccess = 0;
                double data = thisData.toDouble(&cmpSuccess);
                if(cmpSuccess==0){
                    if(isSuccess!=NULL){
                        *isSuccess=0;
                    }
                    return compileError(index,order_txt,"number format error");
                }
                char*p = (char*)&data;
                bin.append(p[7]);
                bin.append(p[6]);
                bin.append(p[5]);
                bin.append(p[4]);
                bin.append(p[3]);
                bin.append(p[2]);
                bin.append(p[1]);
                bin.append(p[0]);
            }
        }else if(order=="STR(UTF8)"||order=="str(utf8)"||order=="STR"||order=="str"){
            //已utf8格式编译字符串[str默认为utf8格式]
            bin.append(replaceString(orderArgs.at(1).toUtf8()));
        }else{
            if(isSuccess!=NULL){
                *isSuccess=0;
            }
            return compileError(index,order_txt,"thers is no such order");
        }
    }
    qDebug()<<"compile data finish";
    return bin;
}

//预处理:去除注释[支持#]
QString removeExpNote(QString txt){
    QStringList list = txt.split('\n');//每行的文本
    QStringList out;
    foreach(QString line,list){
        QStringList d = line.split("#");
        if(d.length()>1){
            out.append(d.at(0)+"\r\n");
        }else{
            out.append(d.at(0));
        }
    }
    return out.join("");
}


int main(int argc, char *argv[])
{
    QStringList args;

    for(int i = 1;i<argc;i++){
        args.append(QString::fromLocal8Bit(argv[i]));
    }

    if(args.length()>=2 && args.length()<=3){

        if(args.at(0)=="-c"){//编译指令汇编
            //打开汇编文本文件
            QFile asm_file(args.at(1));
            if(!asm_file.open(QIODevice::ReadWrite)){

                qDebug()<<"\033[40;31mError:not fint asm file!\033[0m";
                return -1;
            }
            QString asm_txt = asm_file.readAll();//读取汇编文本

            asm_txt = removeExpNote(asm_txt);//去除注释
            //编译为2进制指令
            uint isSuccess;
            QByteArray bin = compileCode(asm_txt,&isSuccess);
            if(isSuccess==0){

                qDebug()<<"\033[40;31mCompile Error:"<<bin.data()<<"!\033[0m";
                return -1;
            }
            //将2进制数据写回文件
            QFile bin_file;
            if(args.length()==2){//如果没指定二进制机器码的输出文件，就在汇编文件目录下创建一个同名.bin文件，存储输出的二进制码
                QFileInfo asm_fileinfo(asm_file);
                bin_file.setFileName(asm_fileinfo.absolutePath()+asm_fileinfo.baseName()+".bin");
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }else{
                bin_file.setFileName(args.at(2));
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }
            qDebug()<<"\033[40;32mwrite back binary machine instruction finish!\033[0m";
        }
        else if(args.at(0)=="-d"){//编译数据
            //打开汇编文本文件
            QFile asm_file(args.at(1));
            if(!asm_file.open(QIODevice::ReadWrite)){

                qDebug()<<"\033[40;31mError:not fint asm file!\033[0m";
                return -1;
            }
            QString asm_txt = asm_file.readAll();//读取汇编文本

            asm_txt = removeExpNote(asm_txt);//去除注释
            uint isSuccess;
            QByteArray bin = compileData(asm_txt,&isSuccess);
            if(isSuccess==0){
                qDebug()<<"\033[40;31mCompile Error:"<<bin.data()<<"!\033[0m";
                return -1;
            }
            //将2进制数据写回文件
            QFile bin_file;
            if(args.length()==2){//如果没指定二进制机器码的输出文件，就在汇编文件目录下创建一个同名.bin文件，存储输出的二进制码
                QFileInfo asm_fileinfo(asm_file);
                bin_file.setFileName(asm_fileinfo.absolutePath()+asm_fileinfo.baseName()+".bin");
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }else{
                bin_file.setFileName(args.at(2));
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }
            qDebug()<<"\033[40;32mwrite back binary machine instruction finish!\033[0m";
        }
        else if(args.at(0)=="-t"){//将二进制数据转换为TB测试文本
            if(args.length()!=3){
                qDebug()<<"\033[40;31mError:parameter format error\033[0m";
                return -1;
            }
            QFile asm_file(args.at(2));
            if(!asm_file.open(QIODevice::ReadWrite)){
                qDebug()<<"\033[40;31mError:not fint asm file!\033[0m";
                return -1;
            }
            QByteArray bin = asm_file.readAll();//读取二进制数据
            QString address_txt = args.at(1);
            uint address = 0;
            bool isSuccess = 0;
            if(address_txt.right(1)=="H"||address_txt.right(1)=="h"){
                address = address_txt.left(address_txt.length()-1).toUInt(&isSuccess,16);
            }else if(address_txt.right(1)=="D"||address_txt.right(1)=="d"){
                address = address_txt.left(address_txt.length()-1).toUInt(&isSuccess,10);
            }else if(address_txt.right(1)=="O"||address_txt.right(1)=="o"){
                address = address_txt.left(address_txt.length()-1).toUInt(&isSuccess,8);
            }else if(address_txt.right(1)=="B"||address_txt.right(1)=="b"){
                address = address_txt.left(address_txt.length()-1).toUInt(&isSuccess,2);
            }
            if(isSuccess==0){
                qDebug()<<"\033[40;31mAddress number format error!\033[0m";
                return -1;
            }
            QString tb_txt = compileTBtxt(bin,address);
            qDebug()<<tb_txt.toUtf8().data();
            qDebug()<<"\033[40;32mwrite back binary data finish!\033[0m";
        }
        else{
            qDebug()<<"\033[40;31mError:parameter format error\033[0m";
            return -1;
        }
    }
    else{
        qDebug()<<"\033[40;31mError:parameter format error\033[0;0m";
        return -1;
    }
    return 0;
}
