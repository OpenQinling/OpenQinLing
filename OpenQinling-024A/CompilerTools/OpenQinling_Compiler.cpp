#include "OpenQinling_Compiler.h"
#include "OpenQinling_C_AnalysisSrc.h"
#include "OpenQinling_PSDL_GrammarticalAnalysis.h"
#include "OpenQinling_ASM_GrammaticalAnalysis.h"
#include "OpenQinling_LINK_GrammaticalAnalysis.h"
#include "OpenQinling_LINK_CompileMemBlock.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {

//c源码编译,编译为MiddleNode的文本
QString complieC_IorIR(QString src,
                       QString srcPath,
                       C_Compiler::CompileType compileType,
                       QStringList includeDir,//#include""默认搜索的目录
                       QStringList stdIncludeDir,//#include<>默认搜索的目录
                       bool&stutes,
                       QStringList &promptText){
    QList<C_Compiler::PromptInfo> prompts;
    //将c语言源码编译为psdl中间语言代码
    QString psdlIR_text = C_Compiler::compileSrc(src,//要编译的源码
                                                 srcPath,
                                                 compileType,
                                                 includeDir,//#include""默认搜索的目录
                                                 stdIncludeDir,//#include<>默认搜索的目录
                                                 QStringList({"interrupt","drivercall","appcall","share"}),//支持的扩展属性
                                                 stutes,//编译结果
                                                 prompts,//编译报错信息
                                                 C_Compiler::CpuRegBitwide_32bit,//32位cpu
                                                 C_Compiler::CPURAM_BIG_ENDIAN//内存为大端格式
                                                );
    for(int i = 0;i<prompts.length();i++){
        QString promptsText = prompts[i].srcPath + "(";
        promptsText += QString::number(prompts[i].line);
        promptsText += "-";
        promptsText += QString::number(prompts[i].col);
        promptsText += "):"+prompts[i].prompt;
        promptText.append(promptsText);
    }

    if(!stutes){
        return QString();
    }
    stutes = 1;
    return psdlIR_text;
}


//c源码编译,编译为PSDL-IR代码文本
QString compileC_IR(QString src,
                    QString srcPath,
                    QStringList includeDir,//#include""默认搜索的目录
                    QStringList stdIncludeDir,//#include<>默认搜索的目录
                    bool&stutes,
                    QStringList &promptText){
    return complieC_IorIR(src,
                          srcPath,
                          C_Compiler::Compile_To_IR,
                          includeDir,
                          stdIncludeDir,
                          stutes,
                          promptText);
}

//将c源码中的预编译指令展开，生成展开后的c代码文本
QString compileC_I(QString src,
                   QString srcPath,
                   QStringList includeDir,//#include""默认搜索的目录
                   QStringList stdIncludeDir,//#include<>默认搜索的目录
                   bool&stutes,
                   QStringList &promptText){
    return complieC_IorIR(src,
                          srcPath,
                          C_Compiler::Precompile_I,
                          includeDir,
                          stdIncludeDir,
                          stutes,
                          promptText);
}

//将IR代码编译为汇编语言文本
QString compileIR_ASM(QString src,
                      bool&stutes){
    return PsdlIR_Compiler::complieMiddleSrc(src,stutes);
}

//将汇编语言编译为静态库
ASM_Compiler::LibraryFile compileASM_LIB(QString src,
                                         bool&stutes,
                                         QStringList &promptText,
                                         QString srcPath){

    ASM_Compiler::BinaryObject obj = ASM_Compiler::complieAsmSrc(src,srcPath,&stutes,&promptText);
    if(!stutes)return ASM_Compiler::LibraryFile();


    ASM_Compiler::LibraryFile lib;
    lib.fileName = srcPath;
    lib.libData = {obj};

    return lib;
}

//合并多个静态库为一个静态库
ASM_Compiler::LibraryFile mergeLIB(QList<ASM_Compiler::LibraryFile> libs,
                                   bool&stutes,
                                   QStringList &promptText){
    if(libs.length()==0 || ASM_Compiler::judgeHaveLibBlockDupName(libs,&promptText)){
        stutes = 0;
        return ASM_Compiler::LibraryFile();
    }
    stutes = 1;
    ASM_Compiler::LibraryFile lib;
    lib.fileName = libs[0].fileName;
    for(int i = 0;i<libs.length();i++){
        lib.libData += libs[i].libData;
    }
    return lib;
}

/*链接静态库为可执行文件
返回值:            链接完成的可执行程序的镜像数据
参数libs:         待链接的所有静态库
参数linkSprict:   指示如何链接的脚本代码文本*/
LIB_StaticLink::ExecutableImage linkLIB(QList<ASM_Compiler::LibraryFile> libs,
                                        QString linkSprict,
                                        bool&stutes,
                                        QStringList &promptText){
    LIB_StaticLink::LinkSprict sprictInfo =
            LIB_StaticLink::linkGrammaticalAnalysis(linkSprict,promptText,stutes);
    if(!stutes)return LIB_StaticLink::ExecutableImage();

    return LIB_StaticLink::compilerMemBlock(libs,sprictInfo,stutes,promptText);
}





}
QT_END_NAMESPACE
