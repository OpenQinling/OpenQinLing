#include <QCoreApplication>
#include <terminalCmdParsing.h>
#include <QFile>
#include <GrammaticalAnalysis.h>
#include <DebugTool.h>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <compilerMemBlock.h>
#include <ProcedureImage.h>
#pragma execution_character_set("utf-8")

QString link_version = "023A_LINK-V2.0";
void printHelp(){
    qDebug()<<"\033[32m注意:链接器必须在CMD/PowerShell环境下打开,不能双击exe打开\033[0m\r\n";
    qDebug()<<"\033[32m*\033[0m"
            <<"\t\t"<<"作用:指定要链接的静态库文件\r\n"
            <<"\t\t\t[*是默认命令，及不需要写-l-p此类命令标识符，紧跟着程序调用后面的及为默认命令的参数]\r\n";;
    qDebug()<<"\033[32m-p\033[0m"
            <<"\t\t"<<"作用:指定编译项目的根路径\r\n\t\t\t(限定参数数量1个,也可以不使用该命令,自动将编译器可执行程序所在的目录作为项目根路径)\r\n";
    qDebug()<<"\033[32m-l\033[0m"
            <<"\t\t"<<"作用:指定链接时使用的链接脚本路径\r\n";
    qDebug()<<"\033[32m-o\033[0m"
            <<"\t\t"<<"作用:指定最终编译/整合后的静态库文件的输出路径\r\n";
    qDebug()<<"\033[32m-h\033[0m"
            <<"\t\t"<<"作用:打印该汇编编译器使用命令帮助信息\r\n";
    qDebug()<<"\033[32m-c\033[0m"
            <<"\t\t"<<"作用:检查一个编译出的lib库文件的内容\r\n";
    qDebug()<<"\033[32m-v\033[0m"
            <<"\t\t"<<"作用:打印输出当前编译器的版本号\r\n";
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeAt(0);

    QList<TerminalCmd> cmdList = parsingTerminalCmd(args);
    args.clear();

    QElapsedTimer timer;
    timer.start();

    if(cmdList.length()==1 && getTerminalCmd("*",cmdList).args.length()==0){
        qDebug()<<"\033[31m错误:\033[0m"<<"未提供链接器运行的必要参数和指令,自动执行了-h命令";
        printHelp();
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    if(!isOnlyHaveTheseOrder(QStringList({"*","-p","-l","-o","-h","-c","-v"}),cmdList)){
        qDebug()<<"\033[31m错误:\033[0m"<<"链接器使用了不支持的指令";
        qDebug()<<"该汇编链接器支持的指令:";
        printHelp();
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    QStringList libFilesPath;//要链接的静态库文件列表
    QString linkFilePath;//链接脚本文件目录
    QString projectDirPath;//项目的文件夹路径
    QString outFilePath;//编译并合并后输出的映像文件


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
        qDebug()<<"version = "<<link_version.toUtf8().data();
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

    ////////解析-c命令//////////
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
    ////////解析链接文件目录//////////
    if(cmdOrderIsExist("-l",cmdList)){
        if(getTerminalCmd("-l",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"链接脚本必须指定一个，且只能指定一个\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        QString path = getProjectRelativePath(getTerminalCmd("-l",cmdList).args[0]);
        QFileInfo info(path);
        QString suffix = info.completeSuffix();

        linkFilePath.append(path);
    }else{
        qDebug()<<"\033[31m错误:\033[0m"<<"链接脚本必须指定一个，且只能指定一个\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    ////////解析要链接的库文件目录////////////
    foreach(QString path,getTerminalCmd("*",cmdList).args){
        path = getProjectRelativePath(path);
        QFileInfo info(path);
        QString suffix = info.completeSuffix();

        if(!libFilesPath.contains(path)){
            libFilesPath.append(path);
        }
    }

    /////////解析输出的路径/////////////
    if(cmdOrderIsExist("-o",cmdList)){
        if(getTerminalCmd("-o",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"输出路径必须指定一个，且只能指定一个\r\n";
             qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        outFilePath = getProjectRelativePath(getTerminalCmd("-o",cmdList).args[0]);
    }else{
        qDebug()<<"\033[31m错误:\033[0m"<<"未指定输出路径\r\n";
         qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }
    ////////开始解析链接脚本////////////
    QFile linkFile(linkFilePath);
    LinkSprict sprictInfo;
    if(linkFile.open(QFile::ReadOnly)){
        QString linkString = linkFile.readAll();
        linkFile.close();
        QStringList prompts;
        qDebug()<<"开始解析链接脚本: \""<<linkFilePath.toUtf8().data()<<"\"";
        sprictInfo = linkGrammaticalAnalysis(linkString,&prompts);
        if(prompts.length()!=0){
            debugStringList(prompts);
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
    }else{
        qDebug()<<"\033[31m错误:\033[0m"<<"链接脚本无法打开\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    QList<LibraryFile> binaryLIB;//要链接的静态库
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
    qDebug()<<"开始链接所有库文件";
    bool isSueess = 0;
    QStringList pormpts;
    ExecutableImage exeImage = compilerMemBlock(binaryLIB,sprictInfo,&isSueess,&pormpts);
    debugStringList(pormpts);
    if(isSueess==0){
        return 0;
    }
    qDebug()<<"链接成功,生成可执行程序映像文件中\r\n";
    QFile outFile(outFilePath);
    if(outFile.open(QFile::ReadWrite)){
        QByteArray bin = exeImage.toEtbFileData();
        outFile.write(bin);
        outFile.close();
        qDebug()<<"成功回写可执行程序映像数据: \""<<outFilePath.toUtf8().data()<<"\" \r\n";
        qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
    }else{
        qDebug()<<"\033[31m错误:\033[0m无法创建可执行程序映像文件 \""+outFilePath+"\"";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
    }
    return 0;
}
