#include "OpenQinling_CompileCmdActuator.h"
#include <QDebug>
#include "OpenQinling_Driver_Terminalparsing.h"
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include "OpenQinling_Compiler.h"
namespace OpenQinling {
#pragma execution_character_set("utf-8")

const static QString redColorTextBegin = "\033[31m";
const static QString greenColorTextBegin = "\033[32m";
const static QString yellowColorTextBegin = "\033[33m";
const static QString colorTextEnd = "\033[0m";

//帮助信息提示文本
const static QString helpPromptText = "\033[32m编译器支持的功能性命令(每次使用编译器都只可其中一种)\033[0m\r\n"
                                      "   -h     打印输出帮助信息 \r\n"
                                      "   -v     打印输出版本信息\r\n"
                                      "   -pre   C语言预编译宏定义为.i宏定义展开文件(参数1个,传入待文件路径)\r\n"
                                      "   -ir    将.c/.i文件编译为.ir中间表示语言文件(参数1个,传入待文件路径)\r\n"
                                      "   -asm   将.c/.i/.ir文件编译为.asm汇编语言文件(参数1个,传入待文件路径)\r\n"
                                      "   -lib   将.c/.i/.ir/.asm文件编译为.lib静态库文件(参数1-n个,传入待文件路径)\r\n"
                                      "   -info  打印输出.lib、.bin或.etb文件中符号表、内存分配状况等信息(参数1个,传入待获取信息的文件路径)\r\n"
                                      "   (不传入任何命令，就表示将代码、静态库链接为.etb、.bin、.coe可执行程序镜像文件)\r\n"
                                      "\r\n"
                                      "\033[32m编译器支持的附加限定命令\033[0m\r\n"
                                      "   -work   指定编译器的工作目录\r\n"
                                      "   -inc    指定C语言源码编译时头文件的搜索目录(参数1-n个,传入目录路径)\r\n"
                                      "   -ln     使用链接程序功能时指定链接脚本的路径(参数1个,传入脚本路径)\r\n"
                                      "   -out    指定链接/编译/合并后输出文件的路径(参数1个,传入输出路径)\r\n"
                                      "\r\n"
                                      "\033[32m各类型文件后缀代表的含义:\033[0m\r\n"
                                      "   .c      c语言文件(UTF8文本)\r\n"
                                      "   .i      c语言预编译后的文件(UTF8文本)\r\n"
                                      "   .ir     PSDL-IR中间表示语言文件(UTF8文本)\r\n"
                                      "   .asm    汇编语言文件(UTF8文本)\r\n"
                                      "   .lib    静态链接库文件(二进制文件)\r\n"
                                      "   .ln     链接脚本文件(UTF8文本)\r\n"
                                      "   .etb    可执行程序内存分配信息描述文件(ASCII文本标头+二进制数据)\r\n"
                                      "   .bin    程序二进制镜像文件\r\n";
//版本信息提示文本
const static QString versionPromptText = "\033[32mOpenQinling编译工具链:OpenQL-CC\033[0m\r\n"
                                   "适用CPU型号:  OpenQL-024A      \r\n"
                                   "版本:         1.0.0(测试版,暂不支持C、PSDL-IR语言)\r\n"
                                   "支持语言:      //C语言(c99标准)\r\n"
                                   "              //PSDL-IR中间表示语言\r\n"
                                   "              ASM汇编语言\r\n"
                                   "开源项目地址:  https://gitee.com/enlighten7328/OpenQinling\r\n"
                                   "作者抖音号:    I_am_a_big_fool4\r\n"
                                   "作者哔站号:    咋都是名称已存在\r\n"
                                   "作者邮箱号:    1969798169@qq.com\r\n"
                                   "[免责声明:该项目仅可用于学习、交流用途,因私自用于商业行为造成的任何后果作者概不负责]\r\n";



static QString getFunTypeOutputFileType(QString funType){
    if(funType == "-pre")return "i";
    else if(funType == "-ir")return "ir";
    else if(funType == "-asm")return "asm";
    else if(funType == "-lib")return "lib";
    else if(funType == "-bin")return "bin";
    else if(funType == "*")return "etb";
    else return QString();
}


//C语言预编译宏定义为.i宏定义展开文件
static int compilePre(QString srcPath,QString outPath,
                      QStringList includeDir,//#include""默认搜索的目录
                      QStringList stdIncludeDir//#include<>默认搜索的目录
                      ){

    if(QFileInfo(srcPath).completeSuffix().toUpper() != "C"){
        QString promptText = redColorTextBegin + "出错:根据源文件后缀名判断其非C语言源文件["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }else if(QFileInfo(outPath).completeSuffix().toUpper() != "I"){
        QString promptText = redColorTextBegin + "出错:根据输出文件后缀名判断其非宏定义展开的C语言源文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QFile file(srcPath);
    if(!file.open(QFile::ReadOnly)){
        QString promptText = redColorTextBegin + "出错:源文件打开失败["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QString srcText = file.readAll();


    bool suceess;
    QStringList promptText;
    QString outText = compileC_I(srcText,srcPath,includeDir,stdIncludeDir,suceess,promptText);

    for(int i = 0;i<promptText.length();i++){
        qDebug()<<promptText[i].toUtf8().data();
    }
    if(!suceess){
        QString promptText = redColorTextBegin + "出错:源文件编译出错["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QFile outFile(outPath);
    if(!outFile.open(QFile::WriteOnly)){
        QString promptText = redColorTextBegin + "出错:编译结果输出失败["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    outFile.resize(0);

    outFile.write(outText.toUtf8());
    return 0;
}


//将.c/.i文件编译为.ir中间表示语言文件
static int compileIR(QString srcPath,QString outPath,
                     QStringList includeDir,//#include""默认搜索的目录
                     QStringList stdIncludeDir//#include<>默认搜索的目录
                     ){


    QString suffix = QFileInfo(srcPath).completeSuffix().toUpper();
    if(suffix != "C" &&suffix != "I"){
        QString promptText = redColorTextBegin + "出错:根据源文件后缀名判断其非C语言源文件["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }else if(QFileInfo(outPath).completeSuffix().toUpper() != "IR"){
        QString promptText = redColorTextBegin + "出错:根据输出文件后缀名判断其非中间表示语言文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }



    QFile file(srcPath);
    if(!file.open(QFile::ReadOnly)){
        QString promptText = redColorTextBegin + "出错:源文件打开失败["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QString srcText = file.readAll();
    bool suceess;
    QStringList promptText;
    QString outText = compileC_IR(srcText,
                                  srcPath,includeDir,stdIncludeDir,suceess,promptText);

    for(int i = 0;i<promptText.length();i++){
        qDebug()<<promptText[i].toUtf8().data();
    }

    if(!suceess){
        QString promptText = redColorTextBegin + "出错:源文件编译出错["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }


    QFile outFile(outPath);
    if(!outFile.open(QFile::WriteOnly)){

        QString promptText = redColorTextBegin + "出错:编译结果输出失败["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    outFile.resize(0);

    outFile.write(outText.toUtf8());
    return 0;
}


//将.c/.i/.ir文件编译为.asm汇编语言文件
static int compileASM(QString srcPath,QString outPath,
                      QStringList includeDir,//#include""默认搜索的目录
                      QStringList stdIncludeDir//#include<>默认搜索的目录
                      ){
    QString suffix = QFileInfo(srcPath).completeSuffix().toUpper();
    if(suffix != "C" &&suffix != "I" && suffix != "IR"){
        QString promptText = redColorTextBegin + "出错:根据源文件后缀名判断其非C语言源文件或中间表示文件["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }else if(QFileInfo(outPath).completeSuffix().toUpper() != "ASM"){
        QString promptText = redColorTextBegin + "出错:根据输出文件后缀名判断其非汇编文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QFile file(srcPath);
    if(!file.open(QFile::ReadOnly)){
        QString promptText = redColorTextBegin + "出错:源文件打开失败["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QString srcText = file.readAll();

    //是c语言源码直接编译为汇编,需要先编译为中间表示语言
    QString irText;
    if(suffix == "C" || suffix == "I"){
        bool suceess;
        QStringList promptText;
        irText = compileC_IR(srcText,
                             srcPath,
                             includeDir,
                             stdIncludeDir,
                             suceess,
                             promptText);

        for(int i = 0;i<promptText.length();i++){
            qDebug()<<promptText[i].toUtf8().data();
        }

        if(!suceess){
            QString promptText = redColorTextBegin + "出错:C语言源文件编译出错["+srcPath+"]"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }else{
        irText = srcText;
    }


    //将IR代码编译为汇编语言文本
    bool suceess;

    QString outText = compileIR_ASM(irText,suceess);


    if(!suceess){
        QString promptText = redColorTextBegin + "出错:中间表示文件编译出错["+srcPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }


    QFile outFile(outPath);
    if(!outFile.open(QFile::WriteOnly)){

        QString promptText = redColorTextBegin + "出错:编译结果输出失败["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    outFile.resize(0);

    outFile.write(outText.toUtf8());
    return 0;
}


//将.c/.i/.ir/.asm/.lib文件编译合并为一个.lib静态库文件
static int compileLIB(QStringList srcPaths,QString outPath,
                      QStringList includeDir,//#include""默认搜索的目录
                      QStringList stdIncludeDir//#include<>默认搜索的目录
                      ){
    if(QFileInfo(outPath).completeSuffix().toUpper() != "LIB"){
        QString promptText = redColorTextBegin + "出错:根据输出文件后缀名判断其非静态库文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QList<ASM_Compiler::LibraryFile> libs;
    for(int i = 0;i<srcPaths.length();i++){
        QString &srcPath = srcPaths[i];

        QString suffix = QFileInfo(srcPath).completeSuffix().toUpper();
        if(suffix != "C" &&suffix != "I" && suffix != "IR" && suffix != "ASM" && suffix != "LIB"){
            QString promptText = redColorTextBegin + "出错:根据源文件后缀名判断其非C语言源文件、中间表示文件、汇编文件、静态库文件["+srcPath+"]"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QFile file(srcPath);
        if(!file.open(QFile::ReadOnly)){
            QString promptText = redColorTextBegin + "出错:源文件打开失败["+srcPath+"]"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QByteArray srcText = file.readAll();

        //是c语言源码直接编译为汇编,需要先编译为中间表示语言
        QString irText;
        if(suffix == "C" || suffix == "I"){
            bool suceess;
            QStringList promptText;
            irText = compileC_IR(srcText,
                                 srcPath,
                                 includeDir,
                                 stdIncludeDir,
                                 suceess,
                                 promptText);

            for(int i = 0;i<promptText.length();i++){
                qDebug()<<promptText[i].toUtf8().data();
            }

            if(!suceess){
                QString promptText = redColorTextBegin + "出错:C语言源文件编译出错["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
        }else if(suffix == "IR"){
            irText = srcText;
        }

        QString asmText;
        //将中间表示语言编译为汇编语言文件
        if(suffix == "C" || suffix == "I"  || suffix == "IR"){
            bool suceess;
            asmText = compileIR_ASM(irText,suceess);
            if(!suceess){
                QString promptText = redColorTextBegin + "出错:中间表示语言编译出错["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
        }else if(suffix == "ASM"){
            asmText = srcText;
        }

        ASM_Compiler::LibraryFile libFile;

        //将汇编语言编译为静态库
        if(suffix == "C" || suffix == "I"  || suffix == "IR" || suffix == "ASM"){
            bool suceess;
            QStringList promptText;
            libFile = compileASM_LIB(asmText,suceess,promptText,srcPath);
            for(int i = 0;i<promptText.length();i++){
                qDebug()<<promptText[i].toUtf8().data();
            }

            if(!suceess){
                QString promptText = redColorTextBegin + "出错:汇编语言编译失败["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
        }else{
            bool suceess;
            libFile.libData = ASM_Compiler::BinaryObject::objsfromFormatByteArray(srcText,&suceess);
            if(!suceess){
                QString promptText = redColorTextBegin + "出错:静态库文件信息解析失败["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
            libFile.fileName = srcPath;
        }

        libs.append(libFile);
    }

    bool suceess;
    QStringList promptText;
    ASM_Compiler::LibraryFile retLib = mergeLIB(libs,suceess,promptText);
    QByteArray bin = ASM_Compiler::BinaryObject::objsToFormatByteArray(retLib.libData);

    QList<ASM_Compiler::BinaryObject> objsds  =
    ASM_Compiler::BinaryObject::objsfromFormatByteArray(bin,NULL);

    for(int i = 0;i<promptText.length();i++){

        qDebug()<<promptText[i].toUtf8().data();
    }
    if(!suceess){
        QString promptText = redColorTextBegin + "出错:合并静态库文件失败"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    QFile outFile(outPath);
    if(!outFile.open(QFile::WriteOnly)){

        QString promptText = redColorTextBegin + "出错:编译结果输出失败["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    outFile.resize(0);

    outFile.write(bin);
    return 0;
}



//将代码、静态库链接为可执行程序
static int linkToExecFile(QStringList srcPaths,QString outPath,
                     QString lnkScriptPath,
                      QStringList includeDir,//#include""默认搜索的目录
                      QStringList stdIncludeDir//#include<>默认搜索的目录
                      ){
    QString suffix = QFileInfo(outPath).completeSuffix().toUpper();
    if(suffix != "ETB" && suffix != "BIN" && suffix != "COE"){
        QString promptText = redColorTextBegin + "出错:根据输出文件后缀名判断其非可执行程序信息描述文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }else if(QFileInfo(lnkScriptPath).completeSuffix().toUpper() != "LN"){
        QString promptText = redColorTextBegin + "出错:根据传入的脚本文件路径后缀名判断其非链接脚本文件["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }


    QFile scriptfile(lnkScriptPath);
    if(!scriptfile.open(QFile::ReadOnly)){
        QString promptText = redColorTextBegin + "出错:链接脚本文件打开失败["+lnkScriptPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    QString lnkScriptText = scriptfile.readAll();

    QList<ASM_Compiler::LibraryFile> libs;
    for(int i = 0;i<srcPaths.length();i++){
        QString &srcPath = srcPaths[i];

        QString suffix = QFileInfo(srcPath).completeSuffix().toUpper();
        if(suffix != "C" &&suffix != "I" && suffix != "IR" && suffix != "ASM" && suffix != "LIB"){
            QString promptText = redColorTextBegin + "出错:根据源文件后缀名判断其非C语言源文件、中间表示文件、汇编文件、静态库文件["+srcPath+"]"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QFile file(srcPath);
        if(!file.open(QFile::ReadOnly)){
            QString promptText = redColorTextBegin + "出错:源文件打开失败["+srcPath+"]"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QByteArray srcText = file.readAll();


        //是c语言源码直接编译为汇编,需要先编译为中间表示语言
        QString irText;
        if(suffix == "C" || suffix == "I"){
            bool suceess;
            QStringList promptText;
            irText = compileC_IR(srcText,
                                 srcPath,
                                 includeDir,
                                 stdIncludeDir,
                                 suceess,
                                 promptText);

            for(int i = 0;i<promptText.length();i++){
                qDebug()<<promptText[i].toUtf8().data();
            }

            if(!suceess){
                QString promptText = redColorTextBegin + "出错:C语言源文件编译出错["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();

                return -1;
            }
        }else if(suffix == "IR"){
            irText = srcText;
        }

        QString asmText;
        //将中间表示语言编译为汇编语言文件
        if(suffix == "C" || suffix == "I"  || suffix == "IR"){
            bool suceess;
            asmText = compileIR_ASM(irText,suceess);
            if(!suceess){
                QString promptText = redColorTextBegin + "出错:中间表示语言编译出错["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
        }else if(suffix == "ASM"){
            asmText = srcText;
        }

        ASM_Compiler::LibraryFile libFile;

        //将汇编语言编译为静态库
        if(suffix == "C" || suffix == "I"  || suffix == "IR" || suffix == "ASM"){
            bool suceess;
            QStringList promptText;
            libFile = compileASM_LIB(asmText,suceess,promptText,srcPath);
            for(int i = 0;i<promptText.length();i++){
                qDebug()<<promptText[i].toUtf8().data();
            }

            if(!suceess){
                QString promptText = redColorTextBegin + "出错:汇编语言编译失败["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
        }else{
            bool suceess;
            libFile.libData = ASM_Compiler::BinaryObject::objsfromFormatByteArray(srcText,&suceess);
            if(!suceess){
                QString promptText = redColorTextBegin + "出错:静态库文件信息解析失败["+srcPath+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
            libFile.fileName = srcPath;
        }

        libs.append(libFile);
    }

    bool suceess;
    QStringList promptText;
    LIB_StaticLink::ExecutableImage image = linkLIB(libs,lnkScriptText,suceess,promptText);




    QByteArray bin;

    if(suffix == "BIN"){
        bin = image.toBinFileData();
    }else if(suffix == "COE"){
        bin = image.toCoeFileData();
    }else{
        bin = image.toEtbFileData();
    }

    for(int i = 0;i<promptText.length();i++){
        qDebug()<<promptText[i].toUtf8().data();
    }

    if(!suceess){
        QString promptText = redColorTextBegin + "出错:链接为镜像文件失败"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }


    QFile outFile(outPath);
    if(!outFile.open(QFile::WriteOnly)){

        QString promptText = redColorTextBegin + "出错:编译结果输出失败["+outPath+"]"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }
    outFile.resize(0);
    outFile.write(bin);
    return 0;
}



//打印输出二进制数据
static QString binDataToString(QByteArray &bin,uint64_t div = 16){

    QString txt;


    uint64_t len = bin.length()/div;
    uint64_t rem = bin.length()%div;
    if(rem != 0){
        len += 1;
    }

    for(uint64_t i = 0;i<len;i++){
        QString tmp = QString::number((qlonglong)(div*i),16);
        while(tmp.length()<16){
            tmp.prepend('0');
        }
        tmp = tmp.right(16);
        tmp.prepend("0x");
        tmp.append(":");
        for(uint64_t j = 0;j<div;j++){
            QString byteText;
            if((i*div+j) >= (uint64_t)bin.length()){
                byteText = "--";
            }else{
                byteText = QString::number((uint8_t)bin[i*div+j],16);
                while(byteText.length()<2){
                    byteText.prepend('0');
                }
                byteText = byteText.right(2);
            }

            tmp += "  "+byteText.toUpper();
        }

        tmp += "   |   ";

        for(uint64_t j = 0;j<div;j++){
            QString asciiText;
            if((i*div+j) >= (uint64_t)bin.length()){
                asciiText = ".";
            }else{
                QChar c = bin[i*div+j];
                if(c.isDigit() || c.isLetter()){
                    asciiText += c;
                }else{
                    asciiText = ".";
                }
            }

            tmp += asciiText;
        }
        txt += tmp + "\r\n";
    }
    return txt;
}


//编译器终端命令执行器
//返回值:          执行结果
//actuatorDir:   命令执行器程序文件所在的系统目录
//workDir:       命令执行器的工作目录(启动命令执行器时的系统目录)
//cmdArgs:       指令参数的文本
int compileCmdActuator(QString actuatorExeDir,
                       QStringList cmdArgs){
    if(cmdArgs.length() == 0){
        qDebug()<<versionPromptText.toUtf8().data();
        qDebug()<<helpPromptText.toUtf8().data();
        return 0;
    }


    QList<Driver::TerminalCmd> cmds = Driver::TerminalCmd::parsingTerminalCmd(cmdArgs);
    const static QStringList allFuncCmdType= {"*","-pre","-ir","-asm","-lib","-h","-v","-info"};
    const static QStringList allLimitCmdType ={"-work","-inc","-out","-ln"};
    //检测是否定义了不支持的指令类型
    if(!Driver::TerminalCmd::isOnlyHaveTheseOrder(allFuncCmdType+allLimitCmdType,cmds)){

        QString promptText = redColorTextBegin + "出错:使用了编译器命令行不支持的指令类型"+colorTextEnd;
        promptText += "\r\n"+helpPromptText;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    //检测是否定义了多个功能性命令(功能性命令是互斥的)
    {
        int funcOrderCount = 0;//定义的-pre/-ir/-asm/-lib/-h/-v
        for(int i = 0;i<allFuncCmdType.length();i++){
            if(allFuncCmdType[i]=="*")continue;
            funcOrderCount += Driver::TerminalCmd::cmdOrderIsExist(allFuncCmdType[i],cmds);
        }
        if(funcOrderCount > 1){
            QString promptText = redColorTextBegin + "出错:使用了多个功能性命令"+colorTextEnd;
            promptText += "\r\n"+helpPromptText;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }

    //-h/-v指令的处理
    {
        bool isHelpOrder = 0;
        if((isHelpOrder = Driver::TerminalCmd::cmdOrderIsExist("-h",cmds))||
           Driver::TerminalCmd::cmdOrderIsExist("-v",cmds)){
            QString orderType = isHelpOrder ? "-h" : "-v";

            bool isError = !Driver::TerminalCmd::isOnlyHaveTheseOrder(QStringList({"*",orderType}),cmds);
            if(!isError){

                for(int i = 0;i<cmds.length();i++){
                    if(cmds[i].args.length() != 0){
                        isError = 1;
                        break;
                    }
                }
            }

            QString promptText;
            if(isError){
                promptText = yellowColorTextBegin +
                        "警告:打印版本信息/帮助信息时其它指令/参数全部失效" +
                        colorTextEnd +"\r\n";
            }

            if(isHelpOrder){
                promptText += helpPromptText;
            }else{
                promptText += versionPromptText;
            }

            qDebug()<<promptText.toUtf8().data();
            return 0;
        }
    }

    //处理-info指令
    {
        if((Driver::TerminalCmd::cmdOrderIsExist("-info",cmds))){
            bool isError = !Driver::TerminalCmd::isOnlyHaveTheseOrder(QStringList({"*","-info"}),cmds);
            if(!isError){
                for(int i = 0;i<cmds.length();i++){
                    if(cmds[i].order == "-info"){
                        if(cmds[i].args.length() != 1){
                            QString promptText = yellowColorTextBegin +
                                    "错误:必须指定且只能指定一个待打印信息的LIB文件或ETB文件" +
                                    colorTextEnd;
                            qDebug()<<promptText.toUtf8().data();
                            return -1;
                        }
                    }else if(cmds[i].args.length() != 0){
                        isError = 1;
                        break;
                    }
                }
            }

            QString promptText;
            if(isError){
                promptText = yellowColorTextBegin +
                        "警告:打印版本信息/帮助信息时其它指令/参数全部失效" +
                        colorTextEnd +"\r\n";
            }
            Driver::TerminalCmd cmd = Driver::TerminalCmd::getTerminalCmd("-info",cmds);

            QString suffix = QFileInfo(cmd.args[0]).completeSuffix().toUpper();
            if(suffix != "LIB" &&suffix != "ETB" &&suffix != "BIN"){
                QString promptText = redColorTextBegin + "出错:根据文件后缀名判断其非LIB文件和ETB、BIN文件["+cmd.args[0]+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }

            QFile file(cmd.args[0]);
            if(!file.open(QFile::ReadOnly)){
                QString promptText = redColorTextBegin + "出错:待打印信息的文件打开失败["+cmd.args[0]+"]"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }

            QByteArray fileData = file.readAll();
            QString fileInfoText;
            if(suffix == "LIB"){
                bool success;
                QList<ASM_Compiler::BinaryObject> objs = ASM_Compiler::BinaryObject::objsfromFormatByteArray(fileData,&success);
                if(!success){
                    QString promptText = redColorTextBegin + "出错:静态链接库文件解析失败["+cmd.args[0]+"]"+colorTextEnd;
                    qDebug()<<promptText.toUtf8().data();
                    return -1;
                }

                for(int i = 0;i<objs.length();i++){
                    fileInfoText += "OBJ"+QString::number(i+1)+":\r\n";
                    fileInfoText += objs[i].toString()+"\r\n";
                }
            }else if(suffix == "ETB"){
                LIB_StaticLink::ExecutableImage image;
                bool success = image.fromEtbFileData(fileData);
                if(!success){
                    QString promptText = redColorTextBegin + "出错:etb文件解析失败["+cmd.args[0]+"]"+colorTextEnd;
                    qDebug()<<promptText.toUtf8().data();
                    return -1;
                }
                fileInfoText += image.toString(1);
            }else{
                fileInfoText += binDataToString(fileData);
            }
            promptText += fileInfoText;
            qDebug()<<promptText.toUtf8().data();
            return 0;
        }

    }


    QStringList includeDir;//#include""默认搜索的目录
    QStringList stdIncludeDir;//#include<>默认搜索的目录
    QString outFilePath;
    QString lnkScriptPath;

    //解析-work命令,修改工作目录
    if(Driver::TerminalCmd::cmdOrderIsExist("-work",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-work",cmds);
        if(thisCmd.args.length() != 1){
            QString promptText = redColorTextBegin + "出错:设置工作目录必须指定一个路径"+colorTextEnd;
            promptText += "\r\n"+helpPromptText;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        if(!QDir::setCurrent(thisCmd.args[0])){
            QString promptText = redColorTextBegin + "出错:设置的工作目录不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }

    //解析-out命令，修改输出文件路径(默认情况下输出文件路径为 工作目录/a.后缀名)
    if(Driver::TerminalCmd::cmdOrderIsExist("-out",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-out",cmds);
        if(thisCmd.args.length() != 1){
            QString promptText = redColorTextBegin + "出错:必须指定一个输出路径"+colorTextEnd;
            promptText += "\r\n"+helpPromptText;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        outFilePath = thisCmd.args[0];

        QFileInfo info(outFilePath);
        if(info.isDir()){
            QString promptText = redColorTextBegin + "出错:指定的输出路径是一个文件夹"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }else{
        //自动生成输出路径
        QString funType = "*";
        for(int i = 0;i<allFuncCmdType.length();i++){
            if(allFuncCmdType[i] == "*")continue;
            else if(Driver::TerminalCmd::cmdOrderIsExist(allFuncCmdType[i],cmds)){
                funType = allFuncCmdType[i];
                break;
            }
        }

        if(funType=="*"){
            outFilePath = "a.etb";
        }else if(funType=="-lib"){
            outFilePath = "a.lib";
        }else{
            Driver::TerminalCmd thisCmd = Driver::TerminalCmd::getTerminalCmd(funType,cmds);
            if(thisCmd.args.length() != 1){
                QString promptText = redColorTextBegin + "出错:该功能只能指定一个参数"+colorTextEnd;
                qDebug()<<promptText.toUtf8().data();
                return -1;
            }
            outFilePath =QFileInfo(thisCmd.args[0]).baseName()+"."+getFunTypeOutputFileType(funType);
        }
    }

    QFile file(outFilePath);
    if(file.open(QFile::WriteOnly)){
        file.resize(0);
        file.close();
    }else{
        QString promptText = redColorTextBegin + "出错:无法创建输出文件"+colorTextEnd;
        qDebug()<<promptText.toUtf8().data();
        return -1;
    }

    //解析-inc命令,设置c语言头文件的搜索路径
    if(Driver::TerminalCmd::cmdOrderIsExist("-inc",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-inc",cmds);
        includeDir = thisCmd.args;


        QStringList openErrorDirs;
        for(int i = 0;i<includeDir.length();i++){
            if(!QFileInfo(includeDir[i]).isDir()){
                openErrorDirs +=includeDir[i];
            }
        }

        if(openErrorDirs.length()!=0){
            QString promptText = redColorTextBegin + "出错:以下的头文件目录无法被打开:"+colorTextEnd+"\r\n";
            promptText +=openErrorDirs.join("\r\n");
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }

    //解析-ln命令,设置链接脚本的搜索路径
    bool isHaveLinkOrder;
    if((isHaveLinkOrder = Driver::TerminalCmd::cmdOrderIsExist("-ln",cmds))){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-ln",cmds);
        if(thisCmd.args.length() != 1){
            QString promptText = redColorTextBegin + "出错:设置链接脚本的路径"+colorTextEnd;
            promptText += "\r\n"+helpPromptText;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        lnkScriptPath = thisCmd.args[0];

        if(!QFileInfo(lnkScriptPath).isFile()){
            QString promptText = redColorTextBegin + "出错:指定的链接脚本路径有误"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
    }



    Driver::TerminalCmd &defaultCmd = Driver::TerminalCmd::getTerminalCmd("*",cmds);
    if(Driver::TerminalCmd::cmdOrderIsExist("-pre",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-pre",cmds);
        if(defaultCmd.args.length() !=0 || thisCmd.args.length() !=1){
            QString promptText = redColorTextBegin + "出错:待编译文件指定不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QString srcFile = thisCmd.args[0];
        return compilePre(srcFile,outFilePath,includeDir,stdIncludeDir);
    }else if(Driver::TerminalCmd::cmdOrderIsExist("-ir",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-ir",cmds);
        if(defaultCmd.args.length() !=0 || thisCmd.args.length() !=1){
            QString promptText = redColorTextBegin + "出错:待编译文件指定不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        QString srcFile = thisCmd.args[0];
        return compileIR(srcFile,outFilePath,includeDir,stdIncludeDir);
    }else if(Driver::TerminalCmd::cmdOrderIsExist("-asm",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-asm",cmds);
        if(defaultCmd.args.length() !=0 || thisCmd.args.length() !=1){
            QString promptText = redColorTextBegin + "出错:待编译文件指定不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        QString srcFile = thisCmd.args[0];
        return compileASM(srcFile,outFilePath,includeDir,stdIncludeDir);
    }else if(Driver::TerminalCmd::cmdOrderIsExist("-lib",cmds)){
        Driver::TerminalCmd &thisCmd = Driver::TerminalCmd::getTerminalCmd("-lib",cmds);
        if(defaultCmd.args.length() !=0 || thisCmd.args.length() ==0){
            QString promptText = redColorTextBegin + "出错:待编译文件指定不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        QStringList srcFiles = thisCmd.args;
        return compileLIB(srcFiles,outFilePath,includeDir,stdIncludeDir);
    }else{
        if(defaultCmd.args.length() == 0){
            QString promptText = redColorTextBegin + "出错:待编译文件指定不正确"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }
        if(!isHaveLinkOrder){
            QString promptText = redColorTextBegin + "出错:未指定链接脚本"+colorTextEnd;
            qDebug()<<promptText.toUtf8().data();
            return -1;
        }

        QStringList srcFiles = defaultCmd.args;
        return linkToExecFile(srcFiles,outFilePath,lnkScriptPath,includeDir,stdIncludeDir);
    }
    return 0;
}
}
