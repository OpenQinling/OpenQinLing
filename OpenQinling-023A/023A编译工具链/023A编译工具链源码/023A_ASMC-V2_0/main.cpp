#include <QCoreApplication>
#include <QDebug>
#include <asmc_typedef.h>
#include <FormatText.h>
#include <QFile>
#include "GrammaticalAnalysis.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <terminalCmdParsing.h>
#pragma execution_character_set("utf-8")

//支持的命令>
//默认命令 *: [传入要编译的汇编文件和要整合的二进制库文件] .asm后缀识别为汇编文件 .lib后缀识别为库文件
//指定编译项目地址命令 -p: 限定参数数量1个,也可以不使用该命令,自动将编译器可执行程序所在的目录作为项目根路径
//指定要编译的汇编文件 -a
//指定要整合的lib文件 -l
//指定最终编译/整合后输出的库文件路径 -o:限定参数数量1个，也可以不使用该命令，自动在项目路径下生成 项目文件夹名.lib文件
//检查一个库文件有哪些内容 -c: 支持一个参数，并且不允许使用其它任何指令(弹出警告，不报错)
//帮助 -h: 不支持任何参数，并且不允许使用其它任何指令(弹出警告，不报错)
//版本 -v: 打印输出程序版本信息
//包含的头文件目录-i: 程序编译过程中,头文件的搜索目录

QString asm_version = "023A_ASMC-V2.0";
void printHelp(){
    qDebug()<<"\033[32m注意:编译器必须在CMD/PowerShell环境下打开,不能双击exe打开\033[0m\r\n";
    qDebug()<<"\033[32m*\033[0m"
            <<"\t\t"<<"作用:传入要编译的汇编文件和要整合的二进制库文件\r\n\t\t\t(.asm后缀识别为汇编文件 .lib后缀识别为静态库文件)\r\n\t\t"<<
              "\t[*是默认命令，及不需要写-p/-a此类命令标识符，紧跟着程序调用后面的及为默认命令的参数]\r\n";
    qDebug()<<"\033[32m-p\033[0m"
            <<"\t\t"<<"作用:指定编译项目的根路径\r\n\t\t\t(限定参数数量1个,也可以不使用该命令,自动将编译器可执行程序所在的目录作为项目根路径)\r\n";
    qDebug()<<"\033[32m-a\033[0m"
            <<"\t\t"<<"作用:指定要编译的汇编文件(不强制后缀名)\r\n";
    qDebug()<<"\033[32m-l\033[0m"
            <<"\t\t"<<"作用:指定要整合的静态库文件(不强制后缀名)\r\n";
    qDebug()<<"\033[32m-i\033[0m"
            <<"\t\t"<<"作用:指定头文件的搜索目录\r\n";
    qDebug()<<"\033[32m-o\033[0m"
            <<"\t\t"<<"作用:指定最终编译/整合后的静态库文件的输出路径\r\n\t\t\t(限定参数数量1个，且至少1个)\r\n";
    qDebug()<<"\033[32m-h\033[0m"
            <<"\t\t"<<"作用:打印该汇编编译器使用命令帮助信息\r\n";
    qDebug()<<"\033[32m-c\033[0m"
            <<"\t\t"<<"作用:检查一个编译出的lib库文件的内容\r\n";
    qDebug()<<"\033[32m-v\033[0m"
            <<"\t\t"<<"作用:打印输出当前编译器的版本号\r\n";
}


//023A汇编编译器第二代
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeAt(0);

    QElapsedTimer timer;
    timer.start();

    QList<TerminalCmd> cmdList = parsingTerminalCmd(args);
    args.clear();
    if(cmdList.length()==1 && getTerminalCmd("*",cmdList).args.length()==0){
        qDebug()<<"\033[31m错误:\033[0m"<<"未提供编译器运行的必要参数和指令,自动执行了-h命令";
        printHelp();
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    if(!isOnlyHaveTheseOrder(QStringList({"*","-c","-p","-a","-l","-o","-h","-v","-i"}),cmdList)){
        qDebug()<<"\033[31m错误:\033[0m"<<"编译器使用了不支持的指令";
        qDebug()<<"该汇编编译器支持的指令:";
        printHelp();
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    QStringList asmFilesPath,libFilesPath;//要合并和编译的文件列表
    QString projectDirPath;//项目的文件夹路径
    QString outFilePath;//编译并合并后输出的二进制库文件

    ////////解析-h命令和-v命令//////////
    if(cmdOrderIsExist("-h",cmdList)){
        if(!isOnlyHaveTheseOrder({"*","-h"},cmdList) || getTerminalCmd("*",cmdList).args.length()!=0){
            qDebug()<<"\033[33m警告:\033[0m"<<"在使用了-h指令的情况下，其它指令以及参数将全部不生效";
        }
        printHelp();
         qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }else if(cmdOrderIsExist("-v",cmdList)){
        if(!isOnlyHaveTheseOrder({"*","-v"},cmdList) || getTerminalCmd("*",cmdList).args.length()!=0){
            qDebug()<<"\033[33m警告:\033[0m"<<"在使用了-v  指令的情况下，其它指令以及参数将全部不生效";
        }
        qDebug()<<"version = "<<asm_version.toUtf8().data();
        qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    ///////解析编译项目的路径/////////
    if(cmdOrderIsExist("-p",cmdList)){
        if(getTerminalCmd("-p",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"项目根目录必须指定一个，且只能指定一个\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        projectDirPath = getTerminalCmd("-p",cmdList).args[0];
    }else{
        projectDirPath = app.applicationDirPath();
    }
    projectDirPath = QFileInfo(projectDirPath).absoluteFilePath();

    if(!setProjectPath(projectDirPath)){
        //设置项目路径失败，报错
        qDebug()<<"\033[31m错误:\033[0m"<<"项目根目录设置失败，请检查路径是否正确\r\n";
         qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }
    QDir::setCurrent(projectDirPath);
    if(cmdOrderIsExist("-c",cmdList)){
        if(!isOnlyHaveTheseOrder({"*","-c","-p"},cmdList) || getTerminalCmd("*",cmdList).args.length()!=0){
            qDebug()<<"\033[33m警告:\033[0m"<<"在使用了-c  指令的情况下，-p外其它指令以及参数将全部不生效";
        }
        if(getTerminalCmd("-c",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"必须指定一个要检查的库文件\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        QString path = getTerminalCmd("-c",cmdList).args[0];

        path = getProjectRelativePath(path);

        QFile file(path);
        if(!file.open(QFile::ReadOnly)){
            qDebug()<<"\033[31m错误:\033[0m"<<"待解析静态库: \""<<getTerminalCmd("-c",cmdList).args[0].toUtf8().data()<<"\"无法打开";
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        qDebug()<<"开始解析: \""<<path.toUtf8().data()<<"\"";
        bool suceess;
        QByteArray fileData = file.readAll();
        QList<BinaryObject> bin = BinaryObject::objsfromFormatByteArray(fileData,&suceess);
        if(!suceess){
            qDebug()<<"\033[31m错误:\033[0m"<<"解析 \""<<path.toUtf8().data()<<"\" 出错\r\n";
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        qDebug()<<"解析完成: \""<<path.toUtf8().data()<<"\"\r\n";

        qDebug()<<("打印输出"+path+"静态库信息:").toUtf8().data();
        qDebug()<<"=========================================================";
        for(int i = 0;i<bin.length();i++){
            qDebug()<<(QString::number(i)+".").toUtf8().data();

            qDebug()<<bin[i].toString().toUtf8().data();

            qDebug()<<"=========================================================";
        }
        qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    ///////////解析编译时头文件的搜索目录/////////////
    includePaths.append(thisProjectPath);//默认会首先搜索项目目录
    if(cmdOrderIsExist("-i",cmdList)){
        foreach(QString path,getTerminalCmd("-i",cmdList).args){
            QFileInfo info(path);
            if(!info.isDir()){
                qDebug()<<"\033[31m错误:\033[0m"<<"找不到指定的头文件搜索目录("<<path.toUtf8().data()<<")\r\n";
                qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
                return 0;
            }
            path = info.absoluteFilePath();
            if(!includePaths.contains(path)){
                includePaths.append(path);
            }
        }
    }
    ////////解析要编译/整合的汇编和库文件路径//////////
    if(cmdOrderIsExist("-a",cmdList)){
        foreach(QString path,getTerminalCmd("-a",cmdList).args){
            path = getProjectRelativePath(path);
            if(!asmFilesPath.contains(path)){
                asmFilesPath.append(path);
            }
        }
    }
    if(cmdOrderIsExist("-l",cmdList)){
        foreach(QString path,getTerminalCmd("-l",cmdList).args){
            path = getProjectRelativePath(path);
            if(!libFilesPath.contains(path)){
                libFilesPath.append(path);
            }
        }
    }

    foreach(QString path,getTerminalCmd("*",cmdList).args){
        path = getProjectRelativePath(path);
        QFileInfo info(path);
        QString suffix = info.completeSuffix();
        if(suffix=="asm"){
            if(!asmFilesPath.contains(path)){
                asmFilesPath.append(path);
            }
        }else if(suffix=="lib"){
            if(!libFilesPath.contains(path)){
                libFilesPath.append(path);
            }
        }else{
            qDebug()<<"\033[31m错误:\033[0m"<<"待编译/整合的文件列表中存在未知的文件类型("<<suffix<<")\r\n";
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
    }

    /////////解析输出的路径/////////////
    if(cmdOrderIsExist("-o",cmdList)){
        if(getTerminalCmd("-o",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"输出路径必须指定一个，且只能指定一个\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        outFilePath = getTerminalCmd("-o",cmdList).args[0];
    }else{
        qDebug()<<"\033[31m错误:\033[0m"<<"未指定输出路径\r\n";
         qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    QList<LibraryFile> binaryLIB;//asm编译出的静态库，和导入的要整合的静态库
    foreach(QString path,asmFilesPath){
        //打开要编译的汇编文件
        QFile file(path);
        if(!file.open(QFile::ReadOnly)){
            qDebug()<<"\033[31m错误:\033[0m"<<"待编译文件: \""<<path.toUtf8().data()<<"\" 无法打开\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }

        qDebug()<<"开始编译: \""<<path.toUtf8().data()<<"\"";
        //开始编译导入的汇编文件
        QStringList prompts;
        bool suceess;
        BinaryObject bin = complieAsmSrc(file.readAll(),path,&suceess,&prompts);
        file.close();
        debugStringList(prompts);
        if(!suceess){
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }


        binaryLIB.append({path,{bin}});
        qDebug()<<"编译完成: \""<<path.toUtf8().data()<<"\"\r\n";
    }
    foreach(QString path,libFilesPath){
        QFile file(path);
        if(!file.open(QFile::ReadOnly)){
            qDebug()<<"\033[31m错误:\033[0m"<<"无法打开待解析静态库: \""<<path.toUtf8().data()<<"\"";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }

        qDebug()<<"开始解析: \""<<path.toUtf8().data()<<"\"";
        bool suceess;
        QByteArray fileData = file.readAll();
        file.close();
        QList<BinaryObject> bin = BinaryObject::objsfromFormatByteArray(fileData,&suceess);
        if(!suceess){
            qDebug()<<"\033[31m错误:\033[0m"<<"解析 \""<<path.toUtf8().data()<<"\" 出错\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        binaryLIB.append({path,bin});
        qDebug()<<"解析完成: \""<<path.toUtf8().data()<<"\"\r\n";
    }
    if(binaryLIB.length()==0){
        qDebug()<<"\033[31m错误:\033[0m"<<"未导入任何需要编译或要整合的文件\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    qDebug()<<"开始整合并生成静态库: \""<<getProjectRelativePath(outFilePath).toUtf8().data()<<"\"\r\n";
    QStringList prompts;
    bool isHaveDup = judgeHaveLibBlockDupName(binaryLIB,&prompts);
    debugStringList(prompts);
    if(isHaveDup){
        qDebug()<<"整合静态库失败: \""<<getProjectRelativePath(outFilePath).toUtf8().data()<<"\" \r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }
    QList<BinaryObject> allObjs;
    for(int j = 0;j<binaryLIB.length();j++){
        allObjs.append(binaryLIB[j].libData);
    }
    qDebug()<<"整合静态库成功: \""<<getProjectRelativePath(outFilePath).toUtf8().data()<<"\" \r\n";

    QByteArray bytes = BinaryObject::objsToFormatByteArray(allObjs);
    allObjs.clear();
    binaryLIB.clear();
    QFile file(outFilePath);
    if(!file.open(QFile::WriteOnly)){
        qDebug()<<"\033[31m错误:\033[0m"<<"无法将数据回写至目标路径: \""<<getProjectRelativePath(outFilePath).toUtf8().data()<<"\"\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }
    file.write(bytes);
    file.close();
    qDebug()<<"成功回写静态库数据: \""<<getProjectRelativePath(outFilePath).toUtf8().data()<<"\" \r\n";

    qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
    return 0;
}
