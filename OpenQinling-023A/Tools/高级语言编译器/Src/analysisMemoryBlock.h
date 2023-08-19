#ifndef ANALYSISMEMORYBLOCK_H
#define ANALYSISMEMORYBLOCK_H
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#pragma execution_character_set("utf-8")
#include "CompileErrorHandle.h"
#include "CompileData.h"
#include "CompileTBtxt.h"
#include "CompileOrder.h"
#include "STD_CodeTXT.h"
#include <iostream>
#include <QDataStream>
#include <QBuffer>
using namespace std;

//字符串提取器，并将原字符串的位置替换为标志号,标志号对应一个被提取的字符串，用于还原
//标志号的结构: $类型码 索引码 [类型为2位16进制数，索引号为5位16进制数]
//示例:   $01100a1  01是字符串的类型码，100a1对应字符串缓存库中第0x100a1位置的字符串
QString extractString(QString txt,QStringList* bank){
    //txt是要提取字符串的文本，bank是被提取字符串缓存库
    QList<uint> quoIndex;// "号的索引地址(去除\")
    for(int i = 0;i<txt.length();i++){
        if(i!=0&&txt.at(i)=='\"' && txt.at(i-1)!='\\'){
            quoIndex.append(i);
        }else if(i==0&&txt.at(i)=='\"'){
            quoIndex.append(i);
        }
    }
    if(quoIndex.length()==0){
        return txt;
    }
    bool isInStr = 0;
    uint quoIndex_index = 0;

    QString strTmp;//缓存提取出的文本
    QString retStr;//保存已经经过过滤将字符串替换为标志号的文本
    //提取出文本
    if(bank!=NULL){
        for(int i=0;i<txt.length();i++){
           bool isRetStr = 0;
            //如果索引号碰上了
            if(i==quoIndex.at(quoIndex_index)){
                if(isInStr){
                    isRetStr = 1;
                }else{
                    isInStr = 1;
                }
                quoIndex_index++;
            }


            if(isInStr){
                strTmp.append(txt.at(i));
            }else{
                retStr.append(txt.at(i));
            }
            if(isRetStr){
                isInStr = 0;
                QString num = QString::number(bank->length(),16);
                while(num.length()!=5){
                    num = "0"+num;
                }
                retStr.append("$01"+num);
                bank->append(strTmp);
                strTmp.clear();
            }
        }
        return retStr;
    }
    return txt;
}

//解析代码块伪指令、解决数据冒险
class Order{
public:
    bool isMark = 0;//当前指令是否被标记了
    QString markName;//如果被标记了，标记号
    QString order;//指令操作码
    QStringList args;//指令参数
    QString toString(){
        QString tmp;
        foreach(QString t,args){
            tmp+=" "+t;
        }
        QString mark = "";
        if(isMark){
            mark = markName+">";
        }
        return mark + order+":"+tmp;
    }
};
//标记类型
class MemoryMark{
public:
    QString markName;//标记名 内存块名.标记名
    uint address;//标记对应的内存地址

    QString toString(){
        return markName+"("+QString::number(address,16)+")";
    }
};
//内存块类型
class MemoryBlock{
public:
    bool isAuto;//内存基址是否为待自动分配中
    uint base;//内存块基地址
    bool lenEffective;//是否已经解析出长度信息了
    uint length;//内存块长度
    QString type;//内存块类型
    QString name;//内存块名字
    QString data;//内存块文本数据


    QList<Order> orderList;//DATA/CODE内存块的解析出的指令表

    QByteArray bin;//编译出的二进制数据

    QString toString(){
        QString tmp = "(type:" + type + ";"
                +"base:"+(isAuto? "auto": "0x"+QString::number(base,16)) + ";"
                +"end:"+(lenEffective? QString::number(length,10):"unknown")+";"
                +"name:" +name + ")";

        tmp+="{"+data+"}";
        return tmp;
    }
};
//内存块语法分析器
/*内存块语法:
 * (内存块类型 内存块基地址 内存块名字){内存块内文本}
 * 内存块类型:DATA数据块  CODE代码块 BIN外部二进制块
 * 示例:
 * (CODE 0000H Main){
 *    #程序代码
 * };#代码块
 *
 * (BIN a000H Data){
 *    "F:\image\test1.png";
 *    "F:\image\test2.png";
 * };#外部二进制块，括号内填文件路径。
 */
QList<MemoryBlock> extractMemoryBlock(QString txt,int*isSuc){
    QList<MemoryBlock> blockDatas;
    //获取内存块完整文本内容
    //已'('符和';'符作为功能块的识别符
    QStringList blockString;
    QString blockStringTmp;
    bool isInBlock = 0;
    for(int i = 0;i<txt.length();i++){
        if(txt.at(i)=="("){
            isInBlock = 1;
        }

        if(isInBlock){
            blockStringTmp.append(txt.at(i));
        }else if(txt.at(i)!=" "&&
                 txt.at(i)!="\r"&&
                 txt.at(i)!="\n"&&
                 txt.at(i)!="\0"&&
                 txt.at(i)!=";"){
            if(isSuc!=NULL){
                *isSuc=0;
            }
            return blockDatas;
        }
        if(txt.at(i)=="}"){
            isInBlock = 0;
            blockString.append(blockStringTmp);
            blockStringTmp.clear();
        }
    }

    QString* parenthesesText = new QString[blockString.length()];
    QString* braceText = new QString[blockString.length()];
    bool inParentheses=0;
    bool inBrace = 0;
    //获取每个块的()内文本与{}内文本
    for(int i = 0;i<blockString.length();i++){
        QString blockTXT = blockString.at(i);
        for(int j = 0;j<blockTXT.length();j++){
            if(blockTXT.at(j)==")"){
                inParentheses = 0;
            }else if(blockTXT.at(j)=="}"){
                inBrace = 0;
            }

            if(inParentheses){
                parenthesesText[i].append(blockTXT.at(j));
            }else if(inBrace){
                braceText[i].append(blockTXT.at(j));
            }

            if(blockTXT.at(j)=="("){
                inParentheses = 1;
            }else if(blockTXT.at(j)=="{"){
                inBrace = 1;
            }
        }
    }



    QString*blockName = new QString[blockString.length()];//解析出的内存块名称
    QString*blockType = new QString[blockString.length()];//解析出的内存块类型文本
    QString*blockBase = new QString[blockString.length()];//解析出的内存块基地址数值文本
    //从()中解析出 内存块类型、内存块名、内存块基地址
    for(int i = 0;i<blockString.length();i++){
        int k = 0;//1表示已开始解析内存块类型，2开始解析基地址，3开始解析名称
        bool isCplt = 1;//解析是否完成
        for(int j = 0;j<parenthesesText[i].length();j++){
            QChar tmp = parenthesesText[i].at(j);
            if(tmp!=' '&&isCplt){
                k++;
                isCplt = 0;
            }

            if(tmp==' '&&isCplt==0){
                isCplt = 1;
                if(k==3){
                    continue;
                }
            }

            if(k==1&&isCplt==0){
                blockType[i].append(tmp);
            }else if(k==2&&isCplt==0){
                blockBase[i].append(tmp);
            }else if(k==3&&isCplt==0){
                blockName[i].append(tmp);
            }
        }
    }
    delete []parenthesesText;

    QList<MemoryBlock>p;

    for(int i = 0;i<blockString.length();i++){
        MemoryBlock blockTmp;
        blockTmp.type = blockType[i];
        blockTmp.name = blockName[i];
        blockTmp.data = braceText[i];
        blockTmp.length = 0;
        blockTmp.lenEffective = 0;

        if(blockBase[i]=="AUTO"||blockBase[i]=="auto"){
            blockTmp.base = 0;
            blockTmp.isAuto = 1;
        }else{
            blockTmp.isAuto = 0;
            QString numberSystem = blockBase[i].right(1);
            QString number = blockBase[i].left(blockBase[i].length()-1);
            bool isSuccess =0;
            if(numberSystem=="H"||numberSystem=="h"){
                blockTmp.base = number.toUInt(&isSuccess,16);
            }else if(numberSystem=="D"||numberSystem=="d"){
                blockTmp.base = number.toUInt(&isSuccess,10);
            }else if(numberSystem=="O"||numberSystem=="o"){
                blockTmp.base = number.toUInt(&isSuccess,8);
            }else if(numberSystem=="B"||numberSystem=="b"){
                blockTmp.base = number.toUInt(&isSuccess,2);
            }
            if(!isSuccess){
                if(isSuc!=NULL){
                    *isSuc=0;
                }
                return p;
            }
        }
        p.append(blockTmp);
    }
    delete []blockType;
    delete []blockName;
    delete []blockBase;
    delete []braceText;

    if(isSuc!=NULL){
        *isSuc=1;
    }
    return p;
}


//获取一个4周期指令所影响的寄存器和依赖的寄存器
class Depend{
public://-1表示无影响/依赖的寄存器
    uint effect[2] = {0xffffffff,0xffffffff};//指令所影响的寄存器号
    uint dep[2] = {0xffffffff,0xffffffff};//指令运行所依赖的寄存器号
    //判断下一个指令和当前指令是否有依赖关系
    bool determine(Depend&next){
        bool t1 = next.dep[0]!=-1 ? effect[0]==next.dep[0] : 0;
        bool t2 = next.dep[1]!=-1 ? effect[0]==next.dep[1] : 0;
        bool t3 = next.dep[0]!=-1 ? effect[1]==next.dep[0] : 0;
        bool t4 = next.dep[1]!=-1 ? effect[1]==next.dep[1] : 0;
        return t1||t2||t3||t4;
    }
    //打印输出信息
    QString toString(){
        QString ef1 = QString::number((int)effect[0]);
        QString ef2 = QString::number((int)effect[1]);
        QString dep1 = QString::number((int)dep[0]);
        QString dep2 = QString::number((int)dep[1]);
        return "effect:"+ef1+","+ef2+"|dep:"+dep1+","+dep2;
    }
};

//去除掉\t和多个空格以及首尾的空格
QString dislodgeSpace(QString txt){
    QString tmp;
    //去除\t
    foreach(QChar c,txt){
        if(c=="\t"){
            tmp.append(" ");
        }else{
            tmp.append(c);
        }
    }
    //将多个空格合并为1个空格
    QString ret;
    bool isSpace = 0;
    for(int i = 0;i<tmp.length();i++){
        QChar c = tmp.at(i);
        if(!isSpace && c==" "){
            ret.append(" ");

            isSpace = 1;
        }else if(c!=" "){
            ret.append(c);
            isSpace = 0;
        }
    }
    //去除首尾的空格
    while(ret.at(0)==" "){
        ret.remove(0,1);
    }
    while(ret.right(1)==" "){
        ret.remove(ret.length()-1,1);
    }
    return ret;
}

//解析BIN内存块获取文件数据[从BIN内存块中解析出文件路径，并获取文件数据]
QByteArray dislodgeBinBlock(QString blockName,QString txt,QStringList strText,int*isSrc,QList<MemoryMark>*markList){
    txt.remove(" ");
    txt.remove("\r");
    txt.remove("\n");
    QStringList pathOrderList = txt.split(";");
    pathOrderList.removeLast();
    QByteArray byteArray;
    foreach(QString pathText,pathOrderList){
        if(markList!=NULL){
            QList<QString>slist = pathText.split(">");
            if(slist.length()==2){
                pathText = slist[1];
                MemoryMark mark;
                mark.markName = blockName+"."+slist[0];
                mark.address = byteArray.length();
                markList->append(mark);
            }
        }
        if(pathText.length()==8 && pathText.left(3)=="$01"){
            bool isSrccess = 0;
            int number = pathText.right(5).toInt(&isSrccess,16);
            if(!isSrc){
                QString path = strText.at(number);
                path = path.mid(1,path.length()-2);

                QFile file(path);
                if(file.open(QFile::ReadOnly)){
                    QByteArray bin = file.readAll();
                    file.close();
                    byteArray.append(bin);

                }else{
                    //解析出错
                    if(isSrc!=NULL){
                        *isSrc = 0;
                    }
                    return byteArray;
                }
            }else{
                //解析出错
                if(isSrc!=NULL){
                    *isSrc = 0;
                }
                return byteArray;
            }
        }else{
            //解析出错
            if(isSrc!=NULL){
                *isSrc = 0;
            }
            return byteArray;
        }
    }
    if(isSrc!=NULL){
        *isSrc = 1;
    }
    return byteArray;
}

//从内存块描述文本中解析出指令表
QList<Order> analysisOrderGroup(QString txt){
    QList<Order> ordes;//解析出的指令表
    txt.remove(" ");
    txt.remove("\r");
    txt.remove("\n");
    QStringList orderList = txt.split(";");
    //DATA/CODE内存块的指令都遵循如下格式:
    //指令操作符: 指令参数1,指令参数2,指令参数3,....;
    foreach(QString orderTxt,orderList){
        if(orderTxt!=""){
            Order thisOrder;
            QStringList tmp = orderTxt.split(":");

            //解析出参数
            QStringList args;
            if(tmp.length()>1){
                args = tmp[1].split(",");
            }

            //解析出标志号
            tmp = tmp[0].split(">");
            if(tmp.length()==2){
                thisOrder.order = tmp[1];
                thisOrder.isMark = 1;
                thisOrder.markName = tmp[0];
            }else{
                thisOrder.order = tmp[0];
                thisOrder.isMark = 0;
            }

            thisOrder.args = args;
            ordes.append(thisOrder);
        }
    }
    return ordes;
}

//判断一条指令是几周期的[为0表示该指令是错误的]
uint getOrderCycle(Order order){
    if(order.order=="UADD"||order.order=="uadd"||//
       order.order=="USUB"||order.order=="usub"||//
       order.order=="UMUL"||order.order=="umul"||//
       order.order=="UDIV"||order.order=="udiv"||//
       order.order=="ADD"||order.order=="add"||//
       order.order=="SUB"||order.order=="sub"||//
       order.order=="MUL"||order.order=="mul"||//
       order.order=="DIV"||order.order=="div"||//
       order.order=="FADD"||order.order=="fadd"||//
       order.order=="FSUB"||order.order=="fsub"||//
       order.order=="FMUL"||order.order=="fmul"||//
       order.order=="FDIV"||order.order=="fdiv"||//
       order.order=="SAL"||order.order=="sal"||//
       order.order=="SAR"||order.order=="sar"||//
       order.order=="SOL"||order.order=="sol"||//
       order.order=="SOR"||order.order=="sor"||//
       order.order=="UITF"||order.order=="uitf"||//
       order.order=="ITF"||order.order=="itf"||//
       order.order=="FTI"||order.order=="fti"||//
       order.order=="TPN"||order.order=="tpn"||//
       order.order=="AND"||order.order=="and"||//
       order.order=="OR"||order.order=="or"||//
       order.order=="NOT"||order.order=="not"||//
       order.order=="XOR"||order.order=="xor"||//
       order.order=="RD"||order.order=="rd"||//
       order.order=="WE"||order.order=="we"||//
       order.order=="PUSH"||order.order=="push"||//
       order.order=="POP"||order.order=="pop"||//
       order.order=="MOV"||order.order=="mov"||//
       order.order=="JC"||order.order=="jc"||//
       order.order=="INT"||order.order=="int"||
       order.order=="UCMP"||order.order=="ucmp"||//
       order.order=="CMP"||order.order=="cmp"||//
       order.order=="FCMP"||order.order=="cmp"||//
       order.order=="BTD"||order.order=="btd"||//
       order.order=="WTD"||order.order=="wtd"){//
       return 4;
    }
    else if(order.order=="RT"||order.order=="rt"||
            order.order=="IT"||order.order=="IT"||
            order.order=="EI"||order.order=="ei"||
            order.order=="DI"||order.order=="di"||
            order.order=="EP"||order.order=="ep"||
            order.order=="EV"||order.order=="ev"||
            order.order=="DV"||order.order=="dv"||
            order.order=="RST"||order.order=="rst"||
            order.order=="JMP"||order.order=="jmp"||
            order.order=="JT"||order.order=="jt"){
        return 1;
    }else{
        return 1;
    }
}

//获取一个指令影响的寄存器和依赖的寄存器
Depend getOrderDepend(Order order){
    Depend dep;
    if(order.order=="UADD"||order.order=="uadd"||//
       order.order=="USUB"||order.order=="usub"||//
       order.order=="UMUL"||order.order=="umul"||//
       order.order=="UDIV"||order.order=="udiv"||//
       order.order=="ADD"||order.order=="add"||//
       order.order=="SUB"||order.order=="sub"||//
       order.order=="MUL"||order.order=="mul"||//
       order.order=="DIV"||order.order=="div"||//
       order.order=="FADD"||order.order=="fadd"||//
       order.order=="FSUB"||order.order=="fsub"||//
       order.order=="FMUL"||order.order=="fmul"||//
       order.order=="FDIV"||order.order=="fdiv"||//
       order.order=="SAL"||order.order=="sal"||//
       order.order=="SAR"||order.order=="sar"||//
       order.order=="SOL"||order.order=="sol"||//
       order.order=="SOR"||order.order=="sor"||//
       order.order=="AND"||order.order=="and"||//
       order.order=="OR"||order.order=="or"||//
       order.order=="NOT"||order.order=="not"||//
       order.order=="XOR"||order.order=="xor"||
       order.order=="UCMP"||order.order=="ucmp"||
       order.order=="CMP"||order.order=="cmp"||
       order.order=="FCMP"||order.order=="cmp"){
        //影响选定的x1、flag寄存器，依赖选定的x1、x2[x2如果是立即数则不依赖]
        if(order.args.length()!=2){
            return dep;
        }
        uint x1 = compileDataChannel(order.args.at(0));
        uint x2 = compileDataChannel(order.args.at(1));
        uint flag = 9;
        dep.effect[0] = x1;
        dep.effect[1] = flag;
        dep.dep[0] = x1;
        dep.dep[1] = x2;
    }else if(order.order=="UITF"||order.order=="uitf"||
             order.order=="ITF"||order.order=="itf"||
             order.order=="FTI"||order.order=="fti"||
             order.order=="TPN"||order.order=="tpn"||
             order.order=="BTD"||order.order=="btd"||
             order.order=="WTD"||order.order=="wtd"||
             order.order=="MOV"||order.order=="mov"){
        //影响选定的x1，依赖选定的x2
        if(order.args.length()!=2){
            return dep;
        }
        uint x1 = compileDataChannel(order.args.at(0));
        uint x2 = compileDataChannel(order.args.at(1));
        dep.effect[0] = x1;
        dep.dep[0] = x2;
    }else if(order.order=="JC"||order.order=="jc"){
        //影响tpc,依赖于x1、x2
        uint x1 = compileDataChannel(order.args.at(0));
        uint x2 = compileDataChannel(order.args.at(1));
        uint tpc = 11;
        uint pc = 10;
        dep.effect[0] = tpc;
        dep.dep[0] = x1;

        if(x2==0xffffffff){
            dep.dep[1] = pc;
        }else{
            dep.dep[1] = x2;
        }

    }else if(order.order=="WE"||order.order=="we"){
        //无影响的，依赖于x2,x3
        if(order.args.length()!=3){
            return dep;
        }
        uint x2 = compileDataChannel(order.args.at(1));
        uint x3 = compileDataChannel(order.args.at(2));
        uint r8 = 8;
        dep.dep[0] = x2;
        if(x3==0xffffffff){
            dep.dep[1] = r8;
        }else{
            dep.dep[1] = x3;
        }

    }else if(order.order=="RD"||order.order=="rd"){
        //影响x2，依赖于x3
        if(order.args.length()!=3){
            return dep;
        }
        uint x2 = compileDataChannel(order.args.at(1));
        uint x3 = compileDataChannel(order.args.at(2));
        uint r8 = 8;
        dep.effect[0] = x2;
        if(x3==0xffffffff){
            dep.dep[1] = r8;
        }else{
            dep.dep[1] = x3;
        }
    }else if(order.order=="PUSH"||order.order=="push"){
        //影响sp，依赖于x2,依赖于sp
        if(order.args.length()!=2){
            return dep;
        }
        uint x2 = compileDataChannel(order.args.at(1));
        uint sp = 13;
        dep.effect[0] = sp;
        dep.dep[0] = x2;
        dep.dep[1] = sp;

    }else if(order.order=="POP"||order.order=="pop"){
        //影响x2，影响sp、依赖sp
        if(order.args.length()!=2){
            return dep;
        }
        uint x2 = compileDataChannel(order.args.at(1));
        uint sp = 13;
        dep.effect[0] = x2;
        dep.effect[1] = sp;
        dep.dep[0] = sp;
    }else if(order.order=="RT"||order.order=="rt"){
        uint tpc = 11;
        dep.dep[0] = tpc;
    }else if(order.order=="IT"||order.order=="it"){
        uint ipc = 12;
        dep.dep[0] = ipc;
    }else if(order.order=="JT"||order.order=="jt"){
        //jt情况特殊，虽然其指令执行过程中不需要tpc寄存器的值作为参数，但会修改tpc寄存器。
        //而如果前一条四周期指令也要修改tpc寄存器的话，很可能出现四周期修改tpc的时间滞后于jt指令修改的时间
        //从而造成cpu程序执行顺序的出错。所以jt可以看做也依赖于tpc寄存器的值
        uint tpc = 11;
        dep.dep[0] = tpc;
    }
    return dep;
}


//插入空指令[nullNumber返回插入的空指令数量。指令空占率是程序质量的重要参考标志，越低质量越高
//023A的空占率极限在0%-300%之间，平均44%(对于023B会引入数据旁路架构，能将平均空占率降至10%左右);023A的程序空占率应当控制在50%以下才算合格。
QList<Order> insertNULLOrder(QList<Order> orders,int*nullNumber=NULL){
    QList<Order> cplted;
    int null_num = 0;

    //解决等待读取数据过程中防止被修改数据[一个指令读取操作数前，被后面的指令修改掉了原值]
    //该问题只会在当前为4周期指令，下一个为单周期指令，且两指令会操作ipc或tpc寄存器情况下
    //并解决系统控制指令[如果系统控制指令前面一个指令是4周期，那么该指令必须在3个null后执行]
    QList<Order> cplted_tmp;
    for(int i = 0;i<orders.length()-1;i++){
        cplted_tmp.append(orders[i]);
        if(getOrderCycle(orders[i])==4 && getOrderCycle(orders[i+1])==1){
            Order nullOrder;
            nullOrder.order = "NULL";
            nullOrder.isMark = 0;

            if(orders[i+1].order=="EI"||orders[i+1].order=="ei"||
               orders[i+1].order=="DI"||orders[i+1].order=="di"||
               orders[i+1].order=="EP"||orders[i+1].order=="ep"||
               orders[i+1].order=="EV"||orders[i+1].order=="ev"||
               orders[i+1].order=="DV"||orders[i+1].order=="dv"||
               orders[i+1].order=="RST"||orders[i+1].order=="rst"){
                cplted_tmp.append(nullOrder);
                cplted_tmp.append(nullOrder);
                cplted_tmp.append(nullOrder);
                null_num+=3;
                continue;
            }


            Depend thisDep = getOrderDepend(orders[i]);
            uint tpc = 11;
            uint ipc =12;
            bool isDepTPC = thisDep.dep[0]==tpc || thisDep.dep[1]==tpc;//当前指令是否依赖tpc
            bool isDepIPC = thisDep.dep[0]==ipc || thisDep.dep[1]==ipc;//是否依赖ipc

            //下一条指令是否会修改tpc寄存器
            bool nextEffTPC = orders[i+1].order=="RT" || orders[i+1].order=="rt" ||
                            orders[i+1].order=="JT" || orders[i+1].order=="JT";
            //下一条指令是否会修改ipc寄存器
            bool nextEffIPC = orders[i+1].order=="IT" || orders[i+1].order=="it";



            if(isDepTPC&&nextEffTPC){
                cplted_tmp.append(nullOrder);
                null_num++;
            }else if(isDepIPC&&nextEffIPC){
                cplted_tmp.append(nullOrder);
                null_num++;
            }
        }
    }
    if(orders.length()>=1){
        cplted_tmp.append(orders.last());
    }

    //解决读数据对写数据的依赖[读数据操作要等前面的写数据操作完成后才能进行]
    for(int i = 0;i<cplted_tmp.length();i++){

        Depend thisDep = getOrderDepend(cplted_tmp[i]);
        Depend lastDep1,lastDep2,lastDep3;
        if(cplted.length()>=1){
            lastDep1 = getOrderDepend(cplted.at(cplted.length()-1));
        }
        if(cplted.length()>=2){
            lastDep2 = getOrderDepend(cplted.at(cplted.length()-2));
        }
        if(cplted.length()>=3){
            lastDep3 = getOrderDepend(cplted.at(cplted.length()-3));
        }


        Order nullOrder;
        nullOrder.order = "NULL";
        nullOrder.isMark = 0;
        //4周期指令只受前2条指令影响，1周期指令受前3条指令影响
        if(getOrderCycle(cplted_tmp[i])==4){
            if(lastDep1.determine(thisDep)){
                //前1条指令与该指令有依赖，插入3个空指令
                cplted.append(nullOrder);
                cplted.append(nullOrder);
                null_num+=2;
            }else if(lastDep2.determine(thisDep)){
                //前2条指令与该指令有依赖，插入2个空指令
                cplted.append(nullOrder);
                null_num++;
            }

        }else if(getOrderCycle(cplted_tmp[i])==1){
            if(lastDep1.determine(thisDep)){
                //前1条指令与该指令有依赖，插入3个空指令
                cplted.append(nullOrder);
                cplted.append(nullOrder);
                cplted.append(nullOrder);
                null_num+=3;
            }else if(lastDep2.determine(thisDep)){
                //前2条指令与该指令有依赖，插入2个空指令
                cplted.append(nullOrder);
                cplted.append(nullOrder);
                null_num+=2;
            }else if(lastDep3.determine(thisDep)){
                //前3条指令与该指令有依赖，插入1个空指令
                cplted.append(nullOrder);
                null_num++;
            }

            bool last1_isNULL = 1,last2_isNULL = 1;
            if(cplted.length()>=1){
                if(cplted.at(cplted.length()-1).order!="NULL"){
                    last1_isNULL = 0;
                }
            }
            if(cplted.length()>=2){
                if(cplted.at(cplted.length()-2).order!="NULL"){
                    last2_isNULL = 0;
                }
            }

            if(last2_isNULL==0 && last1_isNULL==0){
                cplted.append(nullOrder);
                cplted.append(nullOrder);
                null_num++;
                null_num++;
            }
            if(last2_isNULL==1 && last1_isNULL==0){
                cplted.append(nullOrder);
                null_num++;
            }

        }

        cplted.append(cplted_tmp[i]);

        //如果为空指令其后至少插入2个空指令，避免跳转到的地址与空指令执行前的程序有依赖

    }

    if(nullNumber!=NULL){
        *nullNumber = null_num;
    }
    return cplted;
}

//从标志号中提取出字符串
QString getStringFromFlag(QString flagTXT,QStringList &str){
    if(flagTXT.left(3)=="$01"){
        QString number = flagTXT.right(5);
        bool isSrc = 0;
        int p = number.toInt(&isSrc,16);
        return str.at(p);
    }
    return "";
}
//还原被转换为标志号的字符串
QString reductionString(QString flagTXT,QStringList &str){
    QString tmp;
    for(int i = 0;i<flagTXT.length();i++){
        if(i<(flagTXT.length()-7) && flagTXT.mid(i,3)=="$01"){
            tmp+=getStringFromFlag(flagTXT.mid(i,8),str);
            i+=7;
        }else{
            tmp+=flagTXT.at(i);
        }
    }
    return tmp;
}


//从CODE内存块中解析出标记
QList<MemoryMark> analysisCodeMemoryMark(MemoryBlock &block){
    QList<Order> list = block.orderList;
    QList<MemoryMark> markList;
    for(int i = 0;i<list.length();i++){
        if(list[i].isMark){
            MemoryMark mark;
            mark.address = i*4+block.base;
            mark.markName = block.name+"."+list[i].markName;
            markList.append(mark);
        }
    }
    return markList;
}
//从DATA内存块中解析出标记
QList<MemoryMark>analysisDataMemoryMark(MemoryBlock &block,QStringList&strTXT){
    QList<Order> list = block.orderList;
    QList<MemoryMark> markList;
    int offset = 0;
    for(int i = 0;i<list.length();i++){
        if(list[i].isMark){
            MemoryMark mark;
            mark.markName = block.name+"."+list[i].markName;
            mark.address = block.base+offset;

            int argLength = list[i].args.length();
            if(list[i].order=="INT"||list[i].order=="int"||
               list[i].order=="UINT"||list[i].order=="uint"||
               list[i].order=="FLOAT"||list[i].order=="float"){
               offset+=4*argLength;
            }else if(list[i].order=="BYTE"||list[i].order=="byte"||
                     list[i].order=="UBYTE"||list[i].order=="ubyte"){
                offset+=argLength;
            }else if(list[i].order=="USHORT"||list[i].order=="byte"||
                     list[i].order=="UBYTE"||list[i].order=="ubyte"){
                offset+=2*argLength;
            }else if(list[i].order=="ULONG"||list[i].order=="ulong"||
                     list[i].order=="LONG"||list[i].order=="long"||
                     list[i].order=="DOUBLE"||list[i].order=="double"){
                offset+=8*argLength;
            }else if(list[i].order=="STR"||list[i].order=="str"||
                     list[i].order=="STR(UTF8)"||list[i].order=="str(utf8)"){
                int strLength = 0;
                foreach(QString str,list[i].args){
                    str = getStringFromFlag(str,strTXT);
                    QByteArray byte = replaceString(str.toUtf8());
                    strLength+=byte.length();
                }
                offset+=strLength;
            }
            markList.append(mark);
        }
    }
    return markList;
}
//标记引用解析
QList<Order> analysisMarkQuote(MemoryBlock &memoryBlock,QList<MemoryMark>& memoryMark){
    QList<Order> orderList = memoryBlock.orderList;
    QList<Order> retOrder;
    for(int i = 0;i<orderList.length();i++){
        QStringList args = orderList[i].args;
        QStringList retArgs;

        foreach(QString arg,args){
            if(arg.at(0)=="["&&( arg.at(arg.length()-1)=="]" || arg.at(arg.length()-1)==">" )){
                QString markName;//标记名
                QString offsetTXT;//偏移文本
                QString sectTXT_high;//位选文本高位
                QString sectTXT_low;//位选文本低位

                QString number="0d";//解析出的立即数文本
                int p = 0;
                for(int j = 1;j<arg.length();j++){
                    if(arg.at(j)!="+" && arg.at(j)!="]"&&p==0){
                        markName.append(arg.at(j));
                    }else if((arg.at(j)=="+")&&p==0){
                        p++;
                    }else if(arg.at(j)=="]"&&p==0){
                        p = 2;
                    }else if(arg.at(j)!="]"&&p==1){
                        offsetTXT.append(arg.at(j));
                    }else if(arg.at(j)=="]"&&p==1){
                        p++;
                    }else if(arg.at(j)=="<"&&p==2){
                        p++;
                    }else if(arg.at(j)!=">"&&arg.at(j)!="-" && p==3){
                        sectTXT_high.append(arg.at(j));
                    }else if(arg.at(j)=="-" && p==3){
                        p++;
                    }else if(arg.at(j)!=">"&&p==4){
                        sectTXT_low.append(arg.at(j));
                    }else if(arg.at(j)==">"&&p==3){
                        p++;
                    }
                }

                bool high_isSrc=0;
                bool low_isSrc = 0;
                bool offset_isSrc = 0;
                bool base_isSrc = 0;
                int high = 31;
                int low = 0;
                uint offset = 0;
                uint base = 0;

                //解析位选择
                if(sectTXT_high==""&&sectTXT_low==""){
                    high_isSrc = 1;
                    low_isSrc = 1;
                }else{
                    high = sectTXT_high.toUInt(&high_isSrc);
                    low = sectTXT_low.toUInt(&low_isSrc);
                }
                //解析基地址
                if(markName.split(".")[0]=="this"){
                    markName = memoryBlock.name+"."+markName.split(".")[1];
                }
                for(int i = 0;i<memoryMark.length();i++){
                    if(memoryMark.at(i).markName==markName){
                        base_isSrc = 1;
                        base = memoryMark.at(i).address;
                    }
                }

                //解析偏移地址
                if(offsetTXT==""){
                    offset = 0;
                    offset_isSrc = 1;
                }else if(offsetTXT.right(1)=="H"||offsetTXT.right(1)=="h"){
                    offset = offsetTXT.left(offsetTXT.length()-1).toUInt(&offset_isSrc,16);
                }else if(offsetTXT.right(1)=="D"||offsetTXT.right(1)=="d"){
                    offset = offsetTXT.left(offsetTXT.length()-1).toUInt(&offset_isSrc,10);
                }else if(offsetTXT.right(1)=="O"||offsetTXT.right(1)=="o"){
                    offset = offsetTXT.left(offsetTXT.length()-1).toUInt(&offset_isSrc,8);
                }else if(offsetTXT.right(1)=="B"||offsetTXT.right(1)=="b"){
                    offset = offsetTXT.left(offsetTXT.length()-1).toUInt(&offset_isSrc,2);
                }



                if(high_isSrc&&low_isSrc&&offset_isSrc&&base_isSrc&&high<=31&&low>=0){
                    uint address = base+offset;
                    address = address<<(31-high);
                    address = address>>(31-high);
                    address = address>>low;
                    retArgs.append(QString::number(address,16)+"H");
                }else{
                    retArgs.append(arg);
                }

            }else{
                retArgs.append(arg);
            }
        }
        Order order = orderList[i];
        order.args = retArgs;


        retOrder.append(order);
    }


    return retOrder;
}

//从内存块中的指令组中还原出文本
QStringList reductionASMtxt(QList<Order> &orderList){
    QStringList asm_txt;
    foreach(Order o,orderList){
        if(o.args.length()==0){
            asm_txt.append(o.order);
        }else{
            QStringList list = o.args;
            QString args_string;
            for(int i = 0;i<list.length();i++){
                args_string.append(list.at(i));
                if(i<=list.length()-2){
                    args_string.append(",");
                }
            }

            asm_txt.append(o.order+":"+args_string);
        }
    }
    return asm_txt;
}

//编译CODE内存块
QByteArray compileCodeBlock(QList<Order> &orderList,int*isSrc=NULL){
    QStringList asm_txt = reductionASMtxt(orderList);
    QByteArray bin;
    uint isSuccess = 0;
    qDebug()<<asm_txt;
    bin = compileCode(asm_txt,&isSuccess);
    if(isSrc!=NULL){
        *isSrc = isSuccess;
    }
    return bin;
}
//编译DATA内存块
QByteArray compileDataBlock(QList<Order> &orderList,QStringList list,int*isSrc=NULL){
    QStringList tmp = reductionASMtxt(orderList);
    QStringList asm_txt;
    for(int i = 0;i<tmp.length();i++){
        asm_txt.append(reductionString(tmp[i],list));
    }
    uint isSuccess = 0;

    QByteArray bin = compileData(asm_txt,&isSuccess);
    if(isSrc!=NULL){
        *isSrc = isSuccess;
    }
    return bin;
}

//判断一个文本中是否有@Import宏,如果有提取出来
QStringList getImportPath(QString txt,QStringList &strTXT,QString * filterTxt = NULL){
    //去除注释
    txt = removeExpNote(txt);
    //导入宏定义解析。导入宏是最特殊的宏，需要单独解析
    QStringList list = txt.split("\n");
    QStringList importPath;//当前文本中所有导入路径


    for(int i = 0;i<list.length();i++){
        QString str = list[i];

        str = dislodgeSpace(str);
        str = str.remove("\r");


        if(str.at(0)=="@"){//如果行首为@符表示就是宏定义的行
            QString order;//宏定义指令
            QStringList args;//宏定义参数
            //获取宏定义的指令与参数
            args = str.split(" ");

            order = args[0].mid(1,args[0].length()-1);
            args.removeFirst();
            if(order=="Import"){
                //读取导入宏指向的路径
                foreach(QString s,args){
                    QString stringText = getStringFromFlag(s,strTXT);
                    QFileInfo file(stringText.mid(1,stringText.length()-2));
                    importPath.append(file.absoluteFilePath());
                }
            }else if(filterTxt!=NULL){
                filterTxt->append(str+"\r\n");
            }
        }else if(filterTxt!=NULL){
            filterTxt->append(str+"\r\n");
        }

    }


    return importPath;
}

//递归查询当前工程所包含的所有文件路径
void queryAllImport(QStringList importPath,QStringList *strTXT,QStringList *queried,QStringList*queryError,QString *filterTxt=NULL){
    //importPath为当前文件所import的文件路径，所谓递归查询的根节点。queried为已经查询到的节点列表
    //对于已经排查过的节点，就不再深入进去排查，并且不重复记录。
    //queryError返回查询失败的路径
    if(queried==NULL){
        return;
    }
    foreach(QString path,importPath){
        QFile file(path);
        if(file.open(QFile::ReadOnly)){
            if(!queried->contains(path)){
                //如果能打开该路径指向的文件,且该路径还并未被查询过，就进入该路径节点下进行查询
                queried->append(path);//将该节点存入已查询集
                //从当前节点的文本中读取@Import的路径
                QString file_txt = file.readAll();
                file_txt = extractString(file_txt,strTXT);
                QStringList thisNode = getImportPath(file_txt,*strTXT,filterTxt);
                file.close();
                queryAllImport(thisNode,strTXT,queried,queryError,filterTxt);
            }
        }else if(queryError!=NULL){
            queryError->append(path);
        }
    }
    return;
}

//宏定义类型
class Macro{
public:
    QString order;//宏类型
    QStringList args;//宏参数

    QString toString(){
        QString argStr;
        for(int i = 0;i<args.length();i++){
            if(i<args.length()-1){
                argStr+=args.at(i)+" ";
            }else{
                argStr+=args.at(i);
            }
        }
        return "@"+order+" "+argStr;
    }
};

//文本中提取出所有的宏定义文本
QList<Macro> extractMacro(QString txt,QString*filterTxt=NULL){
    txt.remove("\r");
    txt = dislodgeSpace(txt);

    QStringList list = txt.split("\n");
    QStringList filter;
    QList<Macro> macroList;
    foreach(QString order,list){
        if(order.at(0)==" "){
            order = order.remove(0,1);
        }
        if(order.right(1)==" "){
            order = order.remove(order.length()-1,1);
        }
        if(order.at(0)=="@"){
            Macro macro;
            QStringList args = order.split(" ");
            macro.order = args.at(0).mid(1,args.at(0).length()-1);
            macro.args = args.mid(1,args.length()-1);
            macroList.append(macro);
        }else{
            filter.append(order);
        }
    }

    if(filterTxt!=NULL){
        *filterTxt = filter.join("\r\n");
    }

    return macroList;
}

//获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
QList<bool> getBlockOverlap(QList<uint>&base,QList<uint>&length){
    QList<bool> isOverlap;
    for(int i = 0;i<base.length()-1;i++){
        if(base[i]+length[i]>base[i+1]){
            isOverlap.append(1);
        }else{
            isOverlap.append(0);
        }
    }
    return isOverlap;
}
//合并重叠的块
void mergeOverlapBlock(QList<uint>base,QList<uint>length,
                       QList<uint>*retBasePtr,QList<uint>*retLengthPtr){
    //先对块进行排序
    for(int i=0;i<base.length();i++){
        for(int j=0;j<base.length()-1-i;j++){
            if(base[j]>base[j+1]){
                uint tmp = base[j];
                base[j] = base[j+1];
                base[j+1] = tmp;
                ////////////////////////////////
                tmp = length[j];
                length[j] = length[j+1];
                length[j+1] = tmp;
            }
        }
    }
    QList<bool> overlap = getBlockOverlap(base,length);
    //将length暂存块的结束地址
    for(int i=0;i<base.length();i++){
        length[i] += base[i];
    }

    QList<uint> BlockBase;
    QList<uint> BlockEnd;
    bool isInBlock = 0;

    for(int i=0;i<overlap.length();i++){
        if(!isInBlock){
            BlockBase.append(base[i]);
            isInBlock = true;
        }
        if(!overlap[i]){
            BlockEnd.append(length[i]);
            isInBlock = false;
        }
    }
    if(!isInBlock){
        BlockBase.append(base[base.length()-1]);
    }
    BlockEnd.append(length[base.length()-1]);
    for(int i = 0;i<BlockBase.length();i++){
        BlockEnd[i]-=BlockBase[i];
    }

    if(retBasePtr!=NULL){
        *retBasePtr = BlockBase;
    }
    if(retLengthPtr!=NULL){
        *retLengthPtr = BlockEnd;
    }

    return;
}

//宏定义解析
//mode是解析出的代码编译模式，有cpu内存架构模式、指令架构模式、程序入口地址/CODE块、内存分配方式等。
//errorCode如果解析头文件出错的话，返回错误码。如果没任何出错，返回0。同时，如果有错误，函数返回错误提示信息

//解析宏定义(宏定义语法: @宏类型 参数)
/*
 * 导入外部汇编文件宏: @Import ".asm/.info文件名" ....
 *      //会将这些.asm文件的文本全部合并在一起。注(如果有循环带入同一个.asm文件会自动过滤)
 * 定义编译方式的宏(一般单独写在一个.info文件中)
 *      @MemoryArch FN或HD (FN为冯诺依曼架构，HD为哈佛架构。)(强制编写)
 *                  根据cpu构建的soc类型进行选择。如果HD架构的话,DATA/BIN类型的内存块会与DATA内存块分别编译出不同的.bin文件
 *                  并且DATA/BIN和DATA之间如果有地址重叠，不会报错
 *      @InstArch 023A 指令架构类型选择(当前只有023A一种选择)(强制编写)
 *      @Inlet 程序的入口地址 入口指令内存块(非强制编写，如果当所有CODE块地址全都是auto时该值才起效，否则无用)
 *      @CodeMemInfo 指令内存1基址 指令内存1长度 指令内存2基址 指令内存2长度...(可定义多个不连续的，但至少指定1个)
 *                  (如果程序中有CODE内存块是auto，会根据该宏信息自动分配基址。
 *                   如果CODE块的基址都是程序员确定的，该值用于当CODE块使用的地址超出了范围后给程序员报错提示)
 *      @DataMemInfo 数据内存1基址 数据内存1长度...(可定义多个不连续的，但至少指定1个)
 *                  (和上面的CodeMemInfo宏功能一样
 * 文本替换 @Define:原文本,替换出的文本
 */
QString analysisMacro(QString txt,QString thisFilePath,QStringList*strTXT,
                      QString *MemoryArchModePtr,
                      QString *InstArchModePtr,
                      uint*InletAddressPtr,
                      QString*InletCodeBlockPtr,
                      QList<uint> *CodeMemBasePtr,
                      QList<uint> *CodeMemLengthPtr,
                      QList<uint> *DataMemBasePtr,
                      QList<uint> *DataMemLengthPtr,
                      uint*stackBasePtr,
                      uint*stackLengthPtr,
                      int*errorCode){
    /*errorCode:
     * 1.导入外部源文件出错
     * 2.解析内存架构出错
     * 3.解析指令架构出错
     * 4.解析入口地址出错
     * 5.解析指令内存范围出错
     * 6.解析数据内存范围出错
     * 7.文本替换出错
     * 8.Data/Code块范围重叠[只有冯诺依曼cpu才有可能出该问题]
     * 9.栈大小/栈底地址定义出错
     */

    QString filterTxt;//合并项目中所有源文件文本
    QStringList importList = getImportPath(txt,*strTXT,&filterTxt);
    QStringList queried = {thisFilePath};
    QStringList queryError;
    queryAllImport(importList,strTXT,&queried,&queryError,&filterTxt);
    queried.removeFirst();

    //如果有没能打开
    if(queryError.length()>0){
        QString importErrorPath;
        foreach(QString path,queryError){
            importErrorPath+="\t"+path+";\n";
        }
        if(errorCode!=NULL){
            *errorCode = 1;
        }
        return "Can't open these src files\n"+importErrorPath;
    }

    //提取出所有的宏定义语句
    QList<Macro> macroList=extractMacro(filterTxt,&filterTxt);

    QString MemoryArchMode="";//内存架构
    QString InstArchMode="";//指令架构
    uint InletAddress=-1;//入口地址
    QString InletCodeBlock="";//入口指令内存块名字
    QList<uint> CodeMemBase;//指令内存的范围列表
    QList<uint> CodeMemLength;//指令内存的范围列表
    QList<uint> DataMemBase;//数据内存的范围列表
    QList<uint> DataMemLength;//指令内存的范围列表
    QList<QString> Replaced;//被替换文本
    QList<QString> replace;//要替换出的文本
    uint stackBase=0,stackLength=0;//栈底地址和栈内存大小
    //获取到宏定义存储的数据
    foreach(Macro macro,macroList){
        if(macro.order=="MemoryArch"){
            if(macro.args.length()==1){
                if(MemoryArchMode==""){
                    if(macro.args.at(0)!="FN"&&macro.args.at(0)!="HD"){
                        if(errorCode!=NULL){
                            *errorCode = 2;
                        }
                        return "Unknown MemoryArch";
                    }
                    MemoryArchMode = macro.args.at(0);
                }else{
                    if(errorCode!=NULL){
                        *errorCode = 2;
                    }
                    return "MemoryArch Macro duplicate definition";
                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 2;
                }
                return "MemoryArch Macro args length error";
            }
        }
        else if(macro.order=="InstArch"){
            if(macro.args.length()==1){
                if(InstArchMode==""){
                    InstArchMode = macro.args.at(0);
                }else{
                    if(errorCode!=NULL){
                        *errorCode = 3;
                    }
                    return "InstArch Macro duplicate definition";
                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 3;
                }
                return "InstArch Macro args length error";
            }
        }
        else if(macro.order=="Inlet"){
            if(macro.args.length()==2){
                if(InletAddress==-1||InletCodeBlock==""){
                    InletCodeBlock = macro.args.at(1);

                    QString numText = macro.args.at(0);
                    uint num = 0;
                    bool isSrc = 0;
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 4;
                        }
                        return "Inlet address number format error";
                    }

                    if(isSrc!=0){
                        InletAddress = num;
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 4;
                        }
                        return "Inlet address number format error";
                    }
                }else{
                    if(errorCode!=NULL){
                        *errorCode = 4;
                    }
                    return "Inlet Macro duplicate definition";
                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 4;
                }
                return "Inlet Macro args length error";
            }
        }
        else if(macro.order=="CodeMemInfo"){
            if(macro.args.length()%2==0){
                for(int i = 0;i<macro.args.length();i+=2){
                    QString numText = macro.args.at(i);
                    uint num = 0;
                    uint num2 = 0;
                    bool isSrc = 0;
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 5;
                        }
                        return "CodeMemInfo base address number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 5;
                        }
                        return "CodeMemInfo base address number format error";
                    }

                    numText = macro.args.at(i+1);
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 5;
                        }
                        return "CodeMemInfo block length number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 5;
                        }
                        return "CodeMemInfo block length number format error";
                    }

                    if(num2!=0){
                        CodeMemBase.append(num);
                        CodeMemLength.append(num2);
                    }

                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 5;
                }
                return "CodeMemInfo Macro args length error";
            }
        }else if(macro.order=="DataMemInfo"){
            if(macro.args.length()%2==0){
                for(int i = 0;i<macro.args.length();i+=2){
                    QString numText = macro.args.at(i);
                    uint num = 0;
                    uint num2 = 0;
                    bool isSrc = 0;
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 6;
                        }
                        return "DataMemInfo base address number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 6;
                        }
                        return "DataMemInfo base address number format error";
                    }

                    numText = macro.args.at(i+1);
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 6;
                        }
                        return "DataMemInfo block length number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 6;
                        }
                        return "DataMemInfo block length number format error";
                    }

                    if(num2!=0){
                        DataMemBase.append(num);
                        DataMemLength.append(num2);
                    }

                }

            }else{
                if(errorCode!=NULL){
                    *errorCode = 6;
                }
                return "DataMemInfo Macro args length error";
            }
        }else if(macro.order=="StackInfo"){
            if(macro.args.length()==2){
                if(stackLength==0){
                    QString numText = macro.args.at(0);
                    uint num = 0;
                    uint num2 = 0;
                    bool isSrc = 0;
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 9;
                        }
                        return "StackInfo base address number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 9;
                        }
                        return "StackInfo base address number format error";
                    }

                    numText = macro.args.at(1);
                    if(numText.right(1)=="H"||numText.right(1)=="h"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,16);
                    }else if(numText.right(1)=="D"||numText.right(1)=="d"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,10);
                    }else if(numText.right(1)=="O"||numText.right(1)=="o"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,8);
                    }else if(numText.right(1)=="B"||numText.right(1)=="b"){
                        num2 = numText.left(numText.length()-1).toUInt(&isSrc,2);
                    }else{
                        if(errorCode!=NULL){
                            *errorCode = 9;
                        }
                        return "StackInfo size number format error";
                    }

                    if(isSrc==0){
                        if(errorCode!=NULL){
                            *errorCode = 9;
                        }
                        return "StackInfo size number format error";
                    }

                    if(num2==0){
                        if(errorCode!=NULL){
                            *errorCode = 9;
                        }
                        return "StackInfo size cannot be equal to zero";
                    }

                    if(num2!=0){
                        stackBase = num;
                        stackLength = num2;
                    }
                }else{
                    if(errorCode!=NULL){
                        *errorCode = 9;
                    }
                    return "StackInfo Macro duplicate definition";
                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 9;
                }
                return "StackInfo Macro args length error";
            }
        }else if(macro.order=="Define"){
            if(macro.args.length()==2){
                if(!Replaced.contains(macro.args.at(0))){
                    Replaced.append(macro.args.at(0));
                    replace.append(macro.args.at(1));
                }else{
                    if(errorCode!=NULL){
                        *errorCode = 7;
                    }
                    return "Define Macro duplicate definition";
                }
            }else{
                if(errorCode!=NULL){
                    *errorCode = 7;
                }
                return "Define Macro args length error";
            }
        }
    }


    if(MemoryArchModePtr!=NULL){
        *MemoryArchModePtr = MemoryArchMode;
    }
    if(InstArchModePtr!=NULL){
        *InstArchModePtr = InstArchMode;
    }
    if(InletAddressPtr!=NULL){
        *InletAddressPtr = InletAddress;
    }
    if(InletCodeBlockPtr!=NULL){
        *InletCodeBlockPtr = InletCodeBlock;
    }
    if(stackBasePtr!=NULL){
        *stackBasePtr = stackBase;
    }
    if(stackLengthPtr!=NULL){
        *stackLengthPtr = stackLength;
    }


    if(stackLength==0){
        if(errorCode!=NULL){
            *errorCode = 9;
        }
        return "No range set for stack";
    }
    //如果没设置Data范围，直接返回解析出错。如果是哈佛架构，没设置Code范围出错
    if(MemoryArchMode!="FN"&&CodeMemBase.length()==0){
        if(errorCode!=NULL){
            *errorCode = 5;
        }
        return "No range set for order block";
    }else if(DataMemBase.length()==0){
        if(errorCode!=NULL){
            *errorCode = 6;
        }
        return "No range set for data block";
    }

    //合并设置的Code/Data块范围
    if(MemoryArchMode!="FN"){
        mergeOverlapBlock(CodeMemBase,CodeMemLength,&CodeMemBase,&CodeMemLength);
    }
    mergeOverlapBlock(DataMemBase,DataMemLength,&DataMemBase,&DataMemLength);

    ///////////////////////////
    if(MemoryArchMode=="HD"){
        if(CodeMemBasePtr!=NULL){
            *CodeMemBasePtr = CodeMemBase;
        }
        if(CodeMemLengthPtr!=NULL){
            *CodeMemLengthPtr = CodeMemLength;
        }
    }
    if(DataMemBasePtr!=NULL){
        *DataMemBasePtr = DataMemBase;
    }
    if(DataMemLengthPtr!=NULL){
        *DataMemLengthPtr = DataMemLength;
    }

    //文本替换
    for(int i = 0;i<Replaced.length();i++){
        filterTxt.replace(Replaced[i],replace[i]);
    }

    if(errorCode!=NULL){
        *errorCode = 0;
    }

    return filterTxt;
}

//根据Data内存块的指令表分析出data块编译出的长度
uint getDataBlockSize(QList<Order>& orderList,QStringList& strTxt){
    uint size = 0;
    foreach(Order o,orderList){
        if(o.order=="UINT"||o.order=="uint"||
                o.order=="INT"||o.order=="int"||
                o.order=="FLOAT"||o.order=="float"){
            size+=4*o.args.length();
        }else if(o.order=="SHORT"||o.order=="short"||
                 o.order=="USHORT"||o.order=="ushort"){
            size+=2*o.args.length();
        }else if(o.order=="TYPE"||o.order=="type"||
                 o.order=="UTYPE"||o.order=="utype"){
            size+=o.args.length();
        }else if(o.order=="LONG"||o.order=="long"||
                 o.order=="ULONG"||o.order=="ulong"||
                 o.order=="DOUBLE"||o.order=="double"){
            size+=8*o.args.length();
        }else if(o.order=="STR"||o.order=="str"||
                 o.order=="STR(UTF8)"||o.order=="str(utf8)"){
            for(int i = 0;i<o.args.length();i++){
                QString tmp = getStringFromFlag(o.args[i],strTxt);
                tmp = tmp.mid(1,tmp.length()-2);
                size+=tmp.toUtf8().length();
            }
        }
    }
    return size;
}

//判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
bool judgeContainBlockSet(QList<uint>baseA,QList<uint>lenA,
                          QList<uint>baseB,QList<uint>lenB){
    //获取块集A/B中所有块的基址和尾址
    for(int i = 0;i<baseB.length();i++){
        lenB[i] = lenB[i]+baseB[i]-1;
    }
    for(int i = 0;i<baseA.length();i++){
        lenA[i] = lenA[i]+baseA[i]-1;
    }



    //依次判断块集中的每个块，这些块的基址和尾址是否能落在A中的同一个块里
    for(int i = 0;i<baseB.length();i++){

        //当前B中块的基址和尾址所落在的A块索引号，如果没落在A中任何块则为-1
        int baseIndex = -1;
        int endIndex = -1;

        for(int j = 0;j<baseA.length();j++){
            if(baseB[i]>=baseA[j] && baseB[i]<=lenA[j]){
                baseIndex = j;
            }
            if(lenB[i]>=baseA[j] && lenB[i]<=lenA[j]){
                endIndex = j;
            }
        }

        //但凡发现B中有一个块的基址/尾址没能落在A中任何一个块中，或者落在的块不同则直接返回非超集
        if(baseIndex==-1 || endIndex==-1 || baseIndex!=endIndex){
            return 0;
        }
    }
    return 1;
}

//判断内存块是否重叠[已带有自动排序功能]
bool judgeOverlapMemBlock(QList<MemoryBlock>& block){
    QList<uint>base,length;
    for(int i = 0;i<block.length();i++){
        if(!block[i].isAuto){
            base.append(block[i].base);
            length.append(block[i].length);
        }
    }

    //先对块进行排序
    for(int i=0;i<base.length();i++){
        for(int j=0;j<base.length()-1-i;j++){
            if(base[j]>base[j+1]){
                uint tmp = base[j];
                base[j] = base[j+1];
                base[j+1] = tmp;
                ////////////////////////////////
                tmp = length[j];
                length[j] = length[j+1];
                length[j+1] = tmp;
            }
        }
    }

    QList<bool> ls = getBlockOverlap(base,length);
    return ls.contains(1);
}

//去除掉已被占用的一个内存块
void removeOccupiedMemBlock(QList<uint>base,QList<uint>len,//剩下的物理内存块
                            uint occupiedBase,uint occupiedLen,//被占用的内存块
                            QList<uint> *remBase,QList<uint> *remLen
                            ){
    if(remBase==NULL || remLen==NULL){return;}
    if(occupiedLen==0){
        *remBase = base;
        *remLen = len;
    }

    for(int i = 0;i<base.length();i++){
        len[i] = base[i]+len[i]-1;
    }
    occupiedLen = occupiedBase+occupiedLen-1;


    QList<uint>retBase,retLen;
    for(int i = 0;i<base.length();i++){
        if(occupiedBase==base[i] && occupiedLen==len[i]){
            break;
        }else if(occupiedBase==base[i] && occupiedLen<len[i]){
            //如果要去除的块对齐了剩余区块的头部,直接重设头部的索引=去除块的下一个单元
            retBase.append(occupiedLen+1);
            retLen.append(len[i]);
        }else if(occupiedBase>base[i] && occupiedLen==len[i]){
            //如果要去除的块对齐的剩余区块的尾部，直接重设尾部的索引=去除块的前一个单元
            retBase.append(base[i]);
            retLen.append(occupiedBase-1);
        }else if(occupiedBase>base[i] && occupiedLen<len[i]){
            retBase.append(base[i]);
            retLen.append(occupiedBase-1);
            retBase.append(occupiedLen+1);
            retLen.append(len[i]);
        }else{
            retBase.append(base[i]);
            retLen.append(len[i]);
        }
    }
    for(int i = 0;i<retBase.length();i++){
        retLen[i] = retLen[i]-retBase[i]+1;
    }

    if(remBase!=NULL){
        *remBase = retBase;
    }
    if(remLen!=NULL){
        *remLen = retLen;
    }
}

//去除掉已被占用的多个内存块
void removeOccupiedMemBlocks(QList<uint>base,QList<uint>len,//剩下的物理内存块
                             QList<uint> occupiedBase,QList<uint> occupiedLen,//被占用的内存块
                             QList<uint> *remBase,QList<uint> *remLen){

    if(remBase==NULL || remLen==NULL){return;}
    if(occupiedBase.length()==0){
        *remBase = base;
        *remLen = len;
        return;
    }


    for(int i = 0;i<occupiedBase.length();i++){
        if(occupiedLen[i]!=0){
            removeOccupiedMemBlock(base,len,occupiedBase[i],occupiedLen[i],&base,&len);
        }
    }
    *remBase = base;
    *remLen = len;
}

//在物理内存区间中搜寻出能够放入指定内存块的区域/*返回值:0.分配错误 1.分配成功 2.分配位置跨越了多个段*/
int searchFreeMemarySpace(QList<uint> base,QList<uint> len,//剩下的物理物理内存区间
                          uint block_len,//要存放的内存块大小
                          uint*block_base//搜查到的能够放下该内存块的地址
                          ){
    if(block_base==NULL){
        return 0;
    }
    QList<int> freeMemIndex;//所以能够放入该内存块的物理内存区间索引号
    for(int i = 0;i<base.length();i++){
        if(len[i]>=block_len){
            freeMemIndex.append(i);
        }
    }

    if(freeMemIndex.length()==0){
        //所有物理内存区间都放不下，直接返回分配出错
        return 0;
    }
    if(block_len>65536){

        //内存块长度过长,超过了段大小，直接返回2，并随便从空闲物理内存区间中分一块给该内存块
        *block_base = base[freeMemIndex[0]];
        return 2;
    }
    foreach(int index,freeMemIndex){
        //获取当前物理内存区间基址/尾址所在的段号，以及段偏移地址
        uint baseSeg = (base[index])/65536;
        uint baseShift = (base[index])%65536;
        uint endSeg = (len[index]+base[index]-1)/65536;
        uint endShift = (len[index]+base[index]-1)%65536;

        if(endSeg-baseSeg==0){
            //如果基址和尾址在同一个段，判断该段能不能装下内存块。
            if((65536-baseShift)>=block_len){
                *block_base = baseSeg*65536+baseShift;
                return 1;
            }
        }
        else if(endSeg-baseSeg==1){
            //如果基址和尾址在相邻的段，判断这两个段能不能装下内存块
            if((65536-baseShift)>=block_len){
                *block_base = baseSeg*65536+baseShift;
                return 1;
            }else if(endShift>=block_len){
                *block_base = endSeg*65536;
                return 1;
            }
        }
        else if(endSeg-baseSeg>=2){
            //如果基址和尾址存在>=3个段，先判断基址在的段能不能装下，如果不能就装在第2个段
            if((65536-baseShift)>=block_len){
                *block_base = baseSeg*65536+baseShift;
                return 1;
            }
            *block_base = (baseSeg+1)*65536;
            return 1;
        }
    }

    //如果排查了所有的段，都没能找到能够放下该内存块的段，直接把内存块随便放在一个内存区间，并返回2
    *block_base = base[freeMemIndex[0]];
    return 2;
}

//内存分配信息:[用于记录在编译完成后，物理内存的利用情况]
class MemoryMallocInfo{
public:
    uint baseAddress;//起始地址
    uint endAddress;//结束地址
    int isUsage=0;//是否被使用[0为空，1为被常量区/代码区使用了，2为被栈区使用了]
    QString name;//如果被使用了，使用该内存的内存块名称
    QByteArray bin;//如果被使用了，该内存中存储的二进制数据
    int device =0;//当前内存所属的物理设备。如果是冯诺依曼架构就是0，
                  //如果是哈佛架构,指令内存为1，数据内存为2

    QString toString(){
        QString type;
        if(isUsage==0){
            type = "Free";
        }else if(isUsage==1){
            type = name;
        }else if(isUsage==2){
            type = "Stack";
        }

        QString dev;
        if(device==1){
            dev = ":CODE";
        }else if(device==2){
            dev = ":DATA";
        }
        QString base = QString::number(baseAddress,16);
        while (base.length()<8) {
            base.prepend("0");
        }
        base.prepend("0x");
        QString end = QString::number(endAddress,16);
        while (end.length()<8) {
            end.prepend("0");
        }
        end.prepend("0x");
        return "<"+base+"-"+end+">"+type+dev;
    }
};

//判断一个内存块是否跨越了多个内存段
bool isStrideMultipleSeg(uint base,uint length){
    uint base_seg = base / 65536;
    uint end_seg = (base+length-1)/65536;
    return (base_seg!=end_seg);
}


/*ErrorCode:
 * 1.导入外部源文件出错
 * 2.解析内存架构出错
 * 3.解析指令架构出错
 * 4.解析入口地址出错
 * 5.解析指令内存范围出错
 * 6.解析数据内存范围出错
 * 7.文本替换出错
 * 8.Data/Code块范围重叠[只有冯诺依曼cpu,才有可能出该问题]
 * 9.栈底地址/长度定义出错
 * 10.栈空间范围超出了物理内存寻址范围
 * 11.存在多个重名的内存块
 * 12.未知指令架构
 * 13.内存块解析出错
 * 14.自定义基址的内存块有重叠
 * 15.自定义基址的内存块超出了物理内存寻址范围
 * 16.重复定义入口程序块的地址
 * 17.未定义入口程序块
 * 18.入口程序块的内存空间已被占用
 * 19.自动为内存块分配基址出错(无连续内存空间分配给内存块)
 * 20.同一个内存块中有多个重名标记点
 * 21.内存块编译错误
 * 22.内存块命名字符串不符合规定
 * 23.标记点命名字符串不符合规定
 * 24.未知类型内存块
 * 25.程序员手动插入了空指令
 */
//编译ASM汇编文本
QList<MemoryMallocInfo> compileASM(QString txt,QString thisFilePath,QString *errorMessage=NULL,int * ErCode=NULL,
                      QStringList *warn=NULL,float*insertNull=NULL){//instNull程序的空占率(越低程序效率越高)

    QList<MemoryMallocInfo> memoryMallocInfoList;
    //去除注释
    txt = removeExpNote(txt);

    QStringList strTXT;//提取出的字符串
    //过滤掉字符串
    txt = extractString(txt,&strTXT);
    //解析出的内存架构类型
    QString MemoryArchMode;
    //解析出的指令架构类型
    QString InstArchMode;
    //解析出的程序入口信息
    uint InletAddress;
    QString InletCodeBlock;
    //解析出的物理内存设备的地址范围信息
    QList<uint> CodeMemBase;
    QList<uint> CodeMemLength;
    QList<uint> DataMemBase;
    QList<uint> DataMemLength;
    uint stackBase,stackLength;
    //宏定义解析出错码
    int errorCode=0;
    QString filterTxt = analysisMacro(txt,thisFilePath,&strTXT,&MemoryArchMode,&InstArchMode,&InletAddress,
                              &InletCodeBlock,&CodeMemBase,&CodeMemLength,&DataMemBase,&DataMemLength,
                                      &stackBase,&stackLength,&errorCode);
    if(errorCode!=0){
        //宏定义解析出错处理
        if(ErCode!=NULL){
            *ErCode = errorCode;
        }
        if(errorMessage!=NULL){
            *errorMessage = filterTxt;
        }
        return memoryMallocInfoList;
    }

    //根据栈底地址和栈大小给栈内存分配内存
    if(judgeContainBlockSet(DataMemBase,DataMemLength,{stackBase-stackLength},{stackLength})){
        //判断出物理内存空间能否放下栈
        removeOccupiedMemBlock(DataMemBase,DataMemLength,
                                stackBase-stackLength,stackLength,
                                &DataMemBase,&DataMemLength);
        MemoryMallocInfo stackInfo;
        stackInfo.baseAddress = stackBase-stackLength;
        stackInfo.endAddress = stackBase-1;
        stackInfo.isUsage = 2;
        if(MemoryArchMode=="HD"){
            stackInfo.device = 2;
        }else if(MemoryArchMode=="FN"){
            stackInfo.device = 0;
        }
        memoryMallocInfoList.append(stackInfo);
    }else{
        if(ErCode!=NULL){
            *ErCode = 10;
        }
        if(errorMessage!=NULL){
            *errorMessage = "Unable to allocate memory for stack";
        }
        return memoryMallocInfoList;
    }

    txt = filterTxt;
    //当前编译器仅支持OpenQinglin-023A的汇编，如果选择的指令架构非023A的直接返回编译出错
    if(InstArchMode!="023A"){
        if(ErCode!=NULL){
            *ErCode = 12;
        }
        if(errorMessage!=NULL){
            *errorMessage = "Unknown instruction arch";
        }
        return memoryMallocInfoList;
    }

    //去除多个空格和\t
    txt = dislodgeSpace(txt);

    //解析出内存块
    int isSrc = 0;
    QList<MemoryBlock> blockList = extractMemoryBlock(txt,&isSrc);
    if(isSrc==0){
        if(ErCode!=NULL){
            *ErCode = 13;
        }
        if(errorMessage!=NULL){
            *errorMessage = "Extract MemoryBlock error";
        }
        return memoryMallocInfoList;
    }

    //判断内存块的类型是否正确
    for(int i = 0;i<blockList.length();i++){
        QString type = blockList[i].type;
        if(type!="CODE" && type!="code" && type!="DATA" && type!="data"){
            if(ErCode!=NULL){
                *ErCode = 24;
            }
            if(errorMessage!=NULL){
                *errorMessage = type+":Unknown memory block type";
            }
            return memoryMallocInfoList;
        }
    }

    //判断是否有重复名字的内存块
    for(int i = 0;i<blockList.length();i++){

        for(int j=i+1;j<blockList.length();j++){
            if(blockList[i].name==blockList[j].name){
                //发现了名称重复的内存块，直接返回出错
                if(ErCode!=NULL){
                    *ErCode = 11;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "Duplicate memory block name:<\n"+
                            blockList[i].toString()+"\n"+blockList[j].toString()+"\n>";
                }
                return memoryMallocInfoList;
            }
        }
    }
    //判断内存块的名称是否符合要求[只能大英文字母或数字/下划线。且首字符必须为字母]
    for(int i = 0;i<blockList.length();i++){
        if(!nameIsConform(blockList[i].name)){
            if(ErCode!=NULL){
                *ErCode = 22;
            }
            if(errorMessage!=NULL){
                *errorMessage = "\'"+blockList[i].name+"\'does not comply with regulations";
            }
            return memoryMallocInfoList;
        }
    }

    QList<MemoryMark> memoryMark;//所有定义的标记
    MemoryMark stackMark;
    stackMark.markName = "StackBase";
    stackMark.address = stackBase;
    memoryMark.append(stackMark);
    //并解析出DATA、CODE内存块的指令表
    for(int i = 0;i<blockList.length();i++){
        if(blockList[i].type=="DATA" || blockList[i].type=="data" || blockList[i].type=="CODE" || blockList[i].type=="code"){
            blockList[i].orderList = analysisOrderGroup(blockList[i].data);
        }
    }

    //判断是否有空指令[对于高级汇编编译器，空指令由编译器自行插入。如果程序员插入空指令会报错]
    for(int i = 0;i<blockList.length();i++){
        if(blockList[i].type=="CODE" || blockList[i].type=="code"){
            foreach(Order order,blockList[i].orderList){
                if(order.order=="NULL"||order.order=="null"){
                    if(ErCode!=NULL){
                        *ErCode = 25;
                    }
                    if(errorMessage!=NULL){
                        *errorMessage = blockList[i].name+":Prohibit the use of NULL instructions";
                    }
                    return memoryMallocInfoList;
                }
            }
        }
    }

    int insertNullNumber = 0;
    int codeOrderNumber = 0;
    //插入空指令、重排指令顺序[解决数据依赖造成的数据冒险问题]
    for(int i = 0;i<blockList.length();i++){
        if(blockList[i].type=="CODE" || blockList[i].type=="code"){

            int nullNumber = 0;
            blockList[i].orderList = insertNULLOrder(blockList[i].orderList,&nullNumber);
            insertNullNumber+=nullNumber;
            codeOrderNumber+=blockList[i].orderList.length();


        }
    }

    if(insertNull!=NULL){
        *insertNull = (float)insertNullNumber/(float)codeOrderNumber;
    }

    //解析出Data/Code内存块的长度
    for(int i = 0;i<blockList.length();i++){
        if(blockList[i].type=="CODE" || blockList[i].type=="code"){
            //因为023A使用的是32位等长指令集，直接根据指令表中指令数*4即可
            blockList[i].length = blockList[i].orderList.length()*4;
            blockList[i].lenEffective = 1;
        }else if(blockList[i].type=="DATA" || blockList[i].type=="data"){
            blockList[i].length = getDataBlockSize(blockList[i].orderList,strTXT);
            blockList[i].lenEffective = 1;
        }
    }

    //判断非auto基址的内存块之间是否有重叠
    if(MemoryArchMode=="HD"){
        //如果为哈佛架构，Code和Data内存块单独判读是否重叠
        QList<MemoryBlock> blocks;
        foreach(MemoryBlock block,blockList){
            if(block.type=="DATA"||block.type=="data"){
                blocks.append(block);
            }
        }
        if(judgeOverlapMemBlock(blocks)){
            if(ErCode!=NULL){
                *ErCode = 14;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Overlapping of data Memory blocks";
            }
            return memoryMallocInfoList;
        }
        blocks.clear();
        foreach(MemoryBlock block,blockList){
            if(block.type=="CODE"||block.type=="code"){
                blocks.append(block);
            }
        }
        if(judgeOverlapMemBlock(blocks)){
            if(ErCode!=NULL){
                *ErCode = 14;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Overlapping of code Memory blocks";
            }
            return memoryMallocInfoList;
        }

    }else{
        //冯诺依曼架构，Code/Data全部合并在一起判断
        if(judgeOverlapMemBlock(blockList)){
            if(ErCode!=NULL){
                *ErCode = 14;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Overlapping Memory blocks";
            }
            return memoryMallocInfoList;
        }
    }

    //判断非auto基址的内存块是否有超出设置的物理内存寻址范围
    if(MemoryArchMode=="HD"){
        //如果为哈佛架构，Code和Data内存块分别和CODE、DATA物理内存范围进行判断
        QList<uint> B_base,B_len;
        foreach(MemoryBlock block,blockList){
            if((!block.isAuto) && (block.type=="DATA"||block.type=="data")){
                B_base.append(block.base);
                B_len.append(block.length);
            }
        }

        if(!judgeContainBlockSet(DataMemBase,DataMemLength,B_base,B_len)){
            if(ErCode!=NULL){
                *ErCode = 15;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Data Memory block defined at an illegal location";
            }
            return memoryMallocInfoList;
        }
        B_base.clear();
        B_len.clear();
        foreach(MemoryBlock block,blockList){
            if((!block.isAuto) && (block.type=="CODE"||block.type=="code")){
                B_base.append(block.base);
                B_len.append(block.length);
            }
        }

        if(!judgeContainBlockSet(CodeMemBase,CodeMemLength,B_base,B_len)){
            if(ErCode!=NULL){
                *ErCode = 15;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Code Memory block defined at an illegal location";
            }
            return memoryMallocInfoList;
        }

    }else if(MemoryArchMode=="FN"){
        //冯诺依曼架构，Code/Data全部合并在一起与DATA物理内存范围进行判断
        QList<uint> B_base,B_len;
        foreach(MemoryBlock block,blockList){
            if(!block.isAuto){
                B_base.append(block.base);
                B_len.append(block.length);
            }
        }
        if(!judgeContainBlockSet(DataMemBase,DataMemLength,B_base,B_len)){
            if(ErCode!=NULL){
                *ErCode = 15;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Memory block defined at an illegal location";
            }
            return memoryMallocInfoList;
        }
    }

    //去除掉已被非auto基址的内存块所占用的物理内存范围
    if(MemoryArchMode=="HD"){
        QList<uint> B_base,B_len;
        foreach(MemoryBlock block,blockList){
            if((!block.isAuto) && (block.type=="DATA"||block.type=="data")){
                B_base.append(block.base);
                B_len.append(block.length);

                if(isStrideMultipleSeg(block.base,block.length)){
                    if(warn!=NULL){
                        warn->append("<"+block.name+">Spanned multiple segments");
                    }
                }
            }
        }
        removeOccupiedMemBlocks(DataMemBase,DataMemLength,B_base,B_len,&DataMemBase,&DataMemLength);
        B_base.clear();
        B_len.clear();
        foreach(MemoryBlock block,blockList){
            if((!block.isAuto) && (block.type=="CODE"||block.type=="code")){
                B_base.append(block.base);
                B_len.append(block.length);
                if(isStrideMultipleSeg(block.base,block.length)){
                    if(warn!=NULL){
                        warn->append("<"+block.name+">Spanned multiple segments");
                    }
                }
            }
        }
        removeOccupiedMemBlocks(CodeMemBase,CodeMemLength,B_base,B_len,&CodeMemBase,&CodeMemLength);
    }else if(MemoryArchMode=="FN"){
        QList<uint> B_base,B_len;
        foreach(MemoryBlock block,blockList){
            if(!block.isAuto){
                B_base.append(block.base);
                B_len.append(block.length);
                if(isStrideMultipleSeg(block.base,block.length)){
                    if(warn!=NULL){
                        warn->append("<"+block.name+">Spanned multiple segments");
                    }
                }
            }
        }
        removeOccupiedMemBlocks(DataMemBase,DataMemLength,B_base,B_len,&DataMemBase,&DataMemLength);
    }

    //根据设定的程序入口内存块名及地址信息设置入口CODE内存块基址
    //如果定义了宏@Inlet 地址 内存块名; 那么入口CODE内存块地址必须为auto，否则会报入口地址重复定义的错
    //要么就不定义宏，入口CODE内存块的地址非auto,直接手动确定了。
    if(InletCodeBlock!=""){//不为空，说明入口地址通过@Inlet宏确定了
        //如果发现入口Code块基地址分配非auto报错:入口地址重复定义
        //如果发现@Inlet指定的Code内存块不存在，报错:未定义程序入口
        bool isExistInlet = 0;//是否查询到了入口Code内存块
        uint blockIndex;//查询到的入口Code内存块索引号
        for(int i = 0;i<blockList.length();i++){
            if((blockList[i].type=="CODE"||blockList[i].type=="code")&&(blockList[i].name==InletCodeBlock)){
                isExistInlet = 1;
                blockIndex = i;
                if(!blockList[i].isAuto){
                    //指定的入口Code块非auto分配基址，报错:重复定义入口内存块的基址
                    if(ErCode!=NULL){
                        *ErCode = 16;
                    }
                    if(errorMessage!=NULL){
                        *errorMessage = "Repeatedly defining the base address of the Inlet Code block";
                    }
                    return memoryMallocInfoList;
                }
                break;
            }
        }
        if(!isExistInlet){
            //未找到入口Code块
            if(ErCode!=NULL){
                *ErCode = 17;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Inlet Code block not found";
            }
            return memoryMallocInfoList;
        }

        //判断给入口Code内存块分配内存的地方是否被占用了，如果占用了报错:程序入口Code块内存分配失败
        uint base = InletAddress;
        uint length = blockList[blockIndex].length;

        //判断入口Code内存块分配的内存是否跨越了多个段,如果是报出警告
        if(isStrideMultipleSeg(base,length)){
            if(warn!=NULL){
                warn->append("Inlet CodeBlock Spanned multiple segments");
            }
        }


        if(MemoryArchMode=="HD"){
            if(!judgeContainBlockSet(CodeMemBase,CodeMemLength,{base},{length})){
                //所划定的入口程序块非剩余内存空间的子集，说明无可用内存供分配
                if(ErCode!=NULL){
                    *ErCode = 18;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "No free memory allocated for Inlet Code blocks";
                }
                return memoryMallocInfoList;
            }
            blockList[blockIndex].isAuto = 0;
            blockList[blockIndex].base = InletAddress;

            removeOccupiedMemBlock(CodeMemBase,CodeMemLength,base,length,&CodeMemBase,&CodeMemLength);
        }else if(MemoryArchMode=="FN"){
            if(!judgeContainBlockSet(DataMemBase,DataMemLength,{base},{length})){
                //所划定的入口程序块非剩余内存空间的子集，说明无可用内存供分配
                if(ErCode!=NULL){
                    *ErCode = 18;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "No free memory allocated for Inlet Code blocks";
                }
                return memoryMallocInfoList;
            }
            blockList[blockIndex].isAuto = 0;
            blockList[blockIndex].base = InletAddress;


            removeOccupiedMemBlock(DataMemBase,DataMemLength,base,length,&DataMemBase,&DataMemLength);
        }
    }else{
        //入口地址非@Inlet宏确定,查询所有Code内存块，如果都是auto，报错:未定义程序入口
        bool isExistInlet = 0;
        foreach(MemoryBlock block,blockList){
            if((block.type=="CODE"||block.type=="code")&&(!block.isAuto)){
                isExistInlet = 1;
                break;
            }
        }
        if(!isExistInlet){
            //未找到入口Code块
            if(ErCode!=NULL){
                *ErCode = 17;
            }
            if(errorMessage!=NULL){
                *errorMessage = "Inlet Code block not found";
            }
            return memoryMallocInfoList;
        }
    }

    /*注:内存块基址分配时会优先寻找完整的段进行分配。
        而内存块在定义时其大小也应该保持在0x00000000-0x0000ffff之间(0-64kb)
      原因:在cpu执行跳转/访存时，往往会用到立即数，但立即数有大小限制0-65535之间
        1.在内存读写时，地址为立即数的情况下，已R8寄存器的值作为高16位的段地址，立即数作为16位
          偏移地址。
          如果一个定义的内存块基地址正好是0x00010000的倍数，且大小0-64kb之间，那么就意味着只需要将
          r8段地址改为该内存块基地址，然后通过立即数给出的偏移地址就能够立刻读取该内存块中的任意地址上的数据了
        2.在程序跳转时，是立即数给出跳转偏移地址。假如当前程序所在的内存块基地址是0x00010000的倍数，且大小0-64kb之间，
          那么意味这可以跳转到当前内存块的任意地址后继续执行程序。(大部分函数的指令长度都不会高于64kb)。
          但是如果当前Code块不符合基地址是0x00010000的倍数，且大小0-64kb之间，很可能发生Code块跨越了两个内存段，
          造成程序跳转时的处理复杂，性能消耗增大。
          如果是Code内存块间跳转，一般是函数跳转，这种情况则使用寄存器给出跳转地址的方式进行长跳转。
      */
    //对于基地址为auto的内存块，自动分配基址
    //[需依赖于提供的CPU内存架构，Code/Data物理内存范围]


    if(MemoryArchMode=="HD"){
        for(int i = 0;i<blockList.length();i++){

            if(!blockList[i].isAuto){
                continue;
            }

            uint blockSaveAddress = 0;
            uint ret;
            if(blockList[i].type=="DATA"||blockList[i].type=="data"){
                ret = searchFreeMemarySpace(DataMemBase,DataMemLength,blockList[i].length,&blockSaveAddress);

            }else if(blockList[i].type=="CODE"||blockList[i].type=="code"){

                ret = searchFreeMemarySpace(CodeMemBase,CodeMemLength,blockList[i].length,&blockSaveAddress);
            }
            if(ret==0){
                if(ErCode!=NULL){
                    *ErCode = 19;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "<"+blockList[i].name+">Automatic allocation of base address failed";
                }
                return memoryMallocInfoList;
            }else{
                if(ret==2 && warn!=NULL){
                    warn->append("<"+blockList[i].name+">Spanned multiple segments");
                }
                blockList[i].isAuto = 0;
                blockList[i].base = blockSaveAddress;
                if(blockList[i].type=="DATA"||blockList[i].type=="data"){
                    removeOccupiedMemBlock(DataMemBase,DataMemLength,
                                            blockSaveAddress,blockList[i].length,
                                            &DataMemBase,&DataMemLength);
                }else if(blockList[i].type=="CODE"||blockList[i].type=="code"){
                    removeOccupiedMemBlock(CodeMemBase,CodeMemLength,
                                            blockSaveAddress,blockList[i].length,
                                            &CodeMemBase,&CodeMemLength);
                }
            }
        }
    }else if(MemoryArchMode=="FN"){
        for(int i = 0;i<blockList.length();i++){
            if(!blockList[i].isAuto){
                continue;
            }
            uint blockSaveAddress = 0;
            int ret = searchFreeMemarySpace(DataMemBase,DataMemLength,blockList[i].length,&blockSaveAddress);

            if(ret==0){
                if(ErCode!=NULL){
                    *ErCode = 19;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "<"+blockList[i].name+">Automatic allocation of base address failed";
                }
                return memoryMallocInfoList;
            }else{
                if(ret==2 && warn!=NULL){
                    warn->append("<"+blockList[i].name+">Spanned multiple segments");
                }

                blockList[i].isAuto = 0;
                blockList[i].base = blockSaveAddress;
                removeOccupiedMemBlock(DataMemBase,DataMemLength,
                                        blockSaveAddress,blockList[i].length,
                                        &DataMemBase,&DataMemLength);
            }
        }
    }

    //解析Data/Code块的标记定义
    for(int i = 0;i<blockList.length();i++){
        //判断标记名是否符合要求。标记名的要求与内存块名要求一样
        foreach(Order order,blockList[i].orderList){
            if(!order.isMark){
                continue;
            }
            if(!nameIsConform(order.markName)){
                if(ErCode!=NULL){
                    *ErCode = 23;
                }
                if(errorMessage!=NULL){
                    *errorMessage = blockList[i].name+"."+blockList[i].name+" does not comply with regulations";
                }
                return memoryMallocInfoList;
            }
        }

        if(blockList[i].type=="CODE" || blockList[i].type=="code"){
            memoryMark.append(analysisCodeMemoryMark(blockList[i]));
        }else if(blockList[i].type=="DATA" || blockList[i].type=="data"){
            memoryMark.append(analysisDataMemoryMark(blockList[i],strTXT));
        }
    }

    //判断是否在同一个内存块中存在重名标记点
    for(int i = 0;i<memoryMark.length();i++){
        for(int j = i+1;j<memoryMark.length();j++){
            if(memoryMark[i].markName == memoryMark[j].markName){
                if(ErCode!=NULL){
                    *ErCode = 20;
                }
                if(errorMessage!=NULL){
                    *errorMessage = "<"+memoryMark[i].markName+">Duplicate Definition";
                }
                return memoryMallocInfoList;
            }
        }
    }

    //解析标记引用
    for(int i = 0;i<blockList.length();i++){
        if(blockList[i].type=="CODE" || blockList[i].type=="code"||
           blockList[i].type=="DATA" || blockList[i].type=="data"){
            blockList[i].orderList = analysisMarkQuote(blockList[i],memoryMark);
        }
    }

    //将BIN/CODE块编译为2进制
    for(int i = 0;i<blockList.length();i++){
        int isSrc = 1;
        MemoryMallocInfo memoryMalloc;
         if(blockList[i].type=="CODE" || blockList[i].type=="code"){
             blockList[i].bin = compileCodeBlock(blockList[i].orderList,&isSrc);
             if(MemoryArchMode=="FN"){
                 memoryMalloc.device = 0;
             }else{
                 memoryMalloc.device = 1;
             }

         }else if(blockList[i].type=="DATA" || blockList[i].type=="data"){
             blockList[i].bin = compileDataBlock(blockList[i].orderList,strTXT,&isSrc);
             if(MemoryArchMode=="FN"){
                 memoryMalloc.device = 0;
             }else{
                 memoryMalloc.device = 2;
             }
         }
         if(!isSrc){
             QString str = blockList[i].type+"<"+blockList[i].name+">"+QString(blockList[i].bin);
             if(errorMessage!=NULL){
                 *errorMessage = str;
             }
             if(ErCode!=NULL){
                 *ErCode = 21;
             }
             return memoryMallocInfoList;
         }
         memoryMalloc.name = blockList[i].name;
         memoryMalloc.bin = blockList[i].bin;
         memoryMalloc.isUsage = 1;
         memoryMalloc.baseAddress = blockList[i].base;
         memoryMalloc.endAddress = blockList[i].base+blockList[i].length-1;
         memoryMallocInfoList.append(memoryMalloc);
    }

    MemoryMallocInfo memoryMalloc;
    memoryMalloc.isUsage = 0;
    if(MemoryArchMode=="FN"){
        for(int i = 0;i<DataMemBase.length();i++){
            memoryMalloc.device = 0;
            memoryMalloc.baseAddress = DataMemBase[i];
            memoryMalloc.endAddress = DataMemBase[i]+DataMemLength[i]-1;
            memoryMallocInfoList.append(memoryMalloc);
        }
    }else{
        for(int i = 0;i<DataMemBase.length();i++){
            memoryMalloc.baseAddress = DataMemBase[i];
            memoryMalloc.endAddress = DataMemBase[i]+DataMemLength[i]-1;
            memoryMalloc.device = 2;
            memoryMallocInfoList.append(memoryMalloc);
        }
        for(int i = 0;i<CodeMemBase.length();i++){
            memoryMalloc.baseAddress = CodeMemBase[i];
            memoryMalloc.endAddress = CodeMemBase[i]+CodeMemLength[i]-1;
            memoryMalloc.device = 1;
            memoryMallocInfoList.append(memoryMalloc);
        }
    }

    //将内存块已基地址进行从小到大排序
    for(int i = 0;i<memoryMallocInfoList.length();i++){
        for(int j = i+1;j<memoryMallocInfoList.length();j++){
            if(memoryMallocInfoList[i].baseAddress>memoryMallocInfoList[j].baseAddress){
                MemoryMallocInfo memoryMalloc = memoryMallocInfoList[i];
                memoryMallocInfoList[i] = memoryMallocInfoList[j];
                memoryMallocInfoList[j] = memoryMalloc;
            }
        }
    }
    return memoryMallocInfoList;
}

#endif // ANALYSISMEMORYBLOCK_H
