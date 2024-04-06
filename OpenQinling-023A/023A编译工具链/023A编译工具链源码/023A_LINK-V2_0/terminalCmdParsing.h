#ifndef TERMINALCMDPARSING_H
#define TERMINALCMDPARSING_H
#include <QStringList>
#pragma execution_character_set("utf-8")
//一个终端命令
class TerminalCmd{
public:
    QString order;
    QStringList args;
};

//判断是否已经存在指定的命令
bool cmdOrderIsExist(QString order,QList<TerminalCmd> &terCmds){
    for(int i = 0;i<terCmds.length();i++){
        if(terCmds[i].order==order){
            return true;
        }
    }
    return false;
}

//根据命令名获取命令的索引
TerminalCmd& getTerminalCmd(QString order,QList<TerminalCmd> &terCmds){
    for(int i = 0;i<terCmds.length();i++){
        if(terCmds[i].order==order){
            return terCmds[i];
        }
    }
    return terCmds[0];
}

//判断所有的终端命令中，是否只存在限定的几种命令
bool isOnlyHaveTheseOrder(QStringList orders,QList<TerminalCmd> &terCmds){
    for(int i = 0;i<terCmds.length();i++){
        if(!orders.contains(terCmds[i].order)){
            return false;
        }
    }
    return true;
}

// 终端命令解析
//终端命令结构:  exe执行程序名 默认命令参数.... 命令1 命令1参数....  命令2 命令2参数.... 命令3 命令3参数....
//示例:   ./023A_ASMC-V1_0 "文件1路径" "文件2路径" "文件3路径" ...... -o "输出文件路径"
//      前3个是默认命令的参数[023A_ASMC-V1_0的默认命令为要编译的文件路径，.asm结尾的是要编译的汇编文件 .obj结尾的是要整合的二进制库文件]
//      -o后面是指定一个输出到的.obj文件地址
QList<TerminalCmd> parsingTerminalCmd(QStringList cmds){
    QList<TerminalCmd> terCmds;
    terCmds.append(TerminalCmd({"*",QStringList()}));

    QString currentOrder;//当前的命令
    foreach(QString cmd,cmds){
        if(cmd.at(0)=='-'){
            if(!cmdOrderIsExist(cmd,terCmds)){
                terCmds.append(TerminalCmd({cmd,QStringList()}));
            }
            currentOrder = cmd;
        }else if(terCmds.length()==1){
            //添加默认命令的参数
            terCmds[0].args.append(cmd);
        }else{
            //添加当前命令的参数
            getTerminalCmd(currentOrder,terCmds).args.append(cmd);
        }
    }
    return terCmds;
}

#endif // TERMINALCMDPARSING_H
