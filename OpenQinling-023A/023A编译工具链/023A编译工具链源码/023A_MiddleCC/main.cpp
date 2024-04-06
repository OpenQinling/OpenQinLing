#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <terminalCmdParsing.h>
#include <QFileInfo>
#include <QDir>
#include <GrammarticalAnalysis.h>
//支持的命令>
//默认命令 *: 传入要编译的中级语言源文件路径,限定参数数量1个
//指定编译项目地址命令 -p: 限定参数数量1个,也可以不使用该命令,自动将编译器可执行程序所在的目录作为项目根路径
//指定最终编译输出的汇编文件路径 -o:限定参数数量1个
//帮助 -h: 不支持任何参数，并且不允许使用其它任何指令(弹出警告，不报错)
//版本 -v: 打印输出程序版本信息
QString middlecc_version = "023A_MiddleCC-V2.0";
void printHelp(){
    qDebug()<<"\033[32m注意:编译器必须在CMD/PowerShell环境下打开,不能双击exe打开\033[0m\r\n";
    qDebug()<<"\033[32m*\033[0m"
            <<"\t\t"<<"作用:传入要编译的中级语言源文件路径\r\n\t\t\t(限定参数数量1个)\r\n";
    qDebug()<<"\033[32m-p\033[0m"
            <<"\t\t"<<"作用:指定编译项目的根路径\r\n\t\t\t(限定参数数量1个,也可以不使用该命令,自动将编译器可执行程序所在的目录作为项目根路径)\r\n";
    qDebug()<<"\033[32m-o\033[0m"
            <<"\t\t"<<"作用:指定最终编译生成的汇编文件的输出路径\r\n";
    qDebug()<<"\033[32m-h\033[0m"
            <<"\t\t"<<"作用:打印该汇编编译器使用命令帮助信息\r\n";
    qDebug()<<"\033[32m-v\033[0m"
            <<"\t\t"<<"作用:打印输出当前编译器的版本号\r\n";
}


QString thisProjectPath;
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

    QString srcFilesPath;//要编译的文件
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
        qDebug()<<"version = "<<middlecc_version.toUtf8().data();
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

    ///////解析输出路径/////////
    if(cmdOrderIsExist("-o",cmdList)){
        if(getTerminalCmd("-o",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"输出路径必须指定一个，且只能指定一个\r\n";
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        QString path = getTerminalCmd("-o",cmdList).args[0];
        outFilePath = getProjectRelativePath(path);
    }


    if(cmdOrderIsExist("*",cmdList)){
        if(getTerminalCmd("*",cmdList).args.length()!=1){
            qDebug()<<"\033[31m错误:\033[0m"<<"要编译的文件必须指定一个，且只能指定一个\r\n";
            qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
            return 0;
        }
        QString path = getTerminalCmd("*",cmdList).args[0];
        srcFilesPath = getProjectRelativePath(path);
    }

    ///////////////////////////////////////开始编译中级语言文件
    QFile file(srcFilesPath);
    if(!file.open(QFile::ReadOnly)){
        qDebug()<<"\033[31m错误:\033[0m"<<"待编译文件: \""<<srcFilesPath.toUtf8().data()<<"\" 无法打开\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }

    qDebug()<<"开始编译: \""<<srcFilesPath.toUtf8().data()<<"\"";
    //开始编译导入的汇编文件
    QStringList prompts;
    bool suceess = 1;

    QString asmText = complieMiddleSrc(file.readAll(),suceess);
    QFile file2(outFilePath);

    file.close();
    if(!suceess){
        qDebug()<<"\033[31m错误:\033[0m编译失败";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        exit(0);
    }
    qDebug()<<"编译完成: \""<<srcFilesPath.toUtf8().data()<<"\"\r\n";
    qDebug()<<"\033[32m任务成功:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();

    if(!file2.open(QFile::ReadWrite)){
        qDebug()<<"\033[31m错误:\033[0m"<<"待写入文件: \""<<outFilePath.toUtf8().data()<<"\" 无法打开\r\n";
        qDebug()<<"\033[31m任务失败:\033[0m"<<("耗时"+QString::number(timer.elapsed())+"ms").toUtf8().data();
        return 0;
    }
    file2.resize(0);
    file2.write(asmText.toUtf8());
    file2.close();

    exit(0);
    return app.exec();
}
