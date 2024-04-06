#ifndef GRAMMARDETECTION_H
#define GRAMMARDETECTION_H
#include <FormatText.h>
#include <asmc_typedef.h>
#include <TextFlag.h>
#include <QDir>
#pragma execution_character_set("utf-8")
//获取一个全局指令定义的内存块，其子指令是在源码文件中的第几行开始定义的
int getSubBlockStartLine(QString src,int instStartLine,int instEndLine){
    QString instText = src.split('\n').mid(instStartLine-1,(instEndLine-1)-(instStartLine-1)+1).join('\n');
    QStringList tmp = instText.split('{');
    tmp.removeAt(tmp.length()-1);
    return tmp.join('{').count('\n')+instStartLine;
}

//获取全局指令中发出提示信息的起始、结束行数、子指令块开始行数
QList<int> getPromptLine(int orderIndex,QString src,QStringList &orderStrList,
                         QList<uint> &lineFeed,
                         QList<uint> &startLine,
                         QStringList &defBigParentStr){
    //需要参数: 产生该提示信息的全局指令索引号,全局指令列表,全局各指令的换行数量,全局指令CODE/DATA的内层文本
    int line = 1;
    for(int i = 0;i<orderIndex;i++){
        line += lineFeed[i];
    }
    for(int i = 0;i<orderIndex;i++){
        int quantity = 0;
        QString tmp = reductionString(orderStrList[i],"$12",defBigParentStr);
        for(int j = 0;j<tmp.length();j++){
            if(tmp[j] == '\n'){
                quantity++;
            }
        }
        line+=quantity;
    }

    QList<int> line_arr;
    line_arr.append(line+startLine[orderIndex]);

    line += lineFeed[orderIndex];

    int quantity = 0;
    QString tmp = reductionString(orderStrList[orderIndex],"$12",defBigParentStr);
    for(int j = 0;j<tmp.length();j++){
        if(tmp[j] == '\n'){
            quantity++;
        }
    }
    line+=quantity;
    line_arr.append(line);
    line_arr.append(getSubBlockStartLine(src,line_arr[0],line_arr[1]));
    return line_arr;
}

//获取子指令发出提示信息的起始、结束行数
QList<int> getBlockPromptLine(int orderIndex,
                              int blockStartLine,
                              QList<uint> &lineFeed,
                              QList<uint> &startLine){
    int line = 1;
    for(int i = 0;i<orderIndex;i++){
        line += lineFeed[i];
    }

    QList<int> line_arr;
    line_arr.append(blockStartLine+line+startLine[orderIndex]-1);
    line_arr.append(blockStartLine+line+lineFeed[orderIndex]-1);

    return line_arr;
}




QString thisProjectPath;
QStringList includePaths;
//用于生成提示信息时，文件的路径简化
bool setProjectPath(QString path){
    QFileInfo info(path);
    if(info.isDir()){
        thisProjectPath = info.absoluteFilePath();
        return 1;
    }
    return 0;
}

//获取一个绝对路径相对于指定的项目路径地址的相对路径
QString getProjectRelativePath(QString path){
    QFileInfo info(path);
    QDir dir(thisProjectPath);
    return dir.relativeFilePath(info.absoluteFilePath());
}

//生成指令的编译器输出提示文本信息
QString grenratePromptText(Prompt prompt){
    //提示类型文本
    QString type = prompt.isError ? "\033[31m错误:\033[0m" : "\033[33m警告:\033[0m";
    //提示处的代码内容
    QString codeText;
    //QStringList tmp = src.split('\n');

    for(int i = 0;i<prompt.length();i++){
        QString tmpCodeText;
        tmpCodeText = prompt.getSrcText(i).split('\n').mid(prompt.getStartline(i)-1,prompt.getEndline(i)-prompt.getStartline(i)+1).join('\n');
        tmpCodeText = removeParentInside(tmpCodeText,2);
        tmpCodeText = textRightShift(tmpCodeText,1);

        QString lineText = prompt.getStartline(i)==prompt.getEndline(i) ? "第"+QString::number(prompt.getStartline(i)):QString::number(prompt.getStartline(i))+"到"+QString::number(prompt.getEndline(i));
        codeText.append("\r\n    \033[34m"+getProjectRelativePath(prompt.getPath(i))+"\033[0m: "+lineText+"行\r\n\t"+tmpCodeText);
    }

    return (type+prompt.content+codeText);
}



#endif // GRAMMARDETECTION_H
