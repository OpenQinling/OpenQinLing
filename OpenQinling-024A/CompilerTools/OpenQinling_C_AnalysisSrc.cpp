#include "OpenQinling_C_AnalysisSrc.h"

#include <OpenQinling_C_LexicalAnalyzer.h>
#include <OpenQinling_C_Grammarparsing.h>
#include <OpenQinling_PSDL_GrammarticalAnalysis.h>
#include <OpenQinling_DebugFunction.h>
#include <OpenQinling_C_Precompile.h>
#include <OpenQinling_C_JudgestrtoolFunction.h>

#include <QDateTime>

QT_BEGIN_NAMESPACE namespace OpenQinling {namespace C_Compiler{

//注册入c语言基础数据类型,以及关键字与类型信息的映射关系
static void registerCbaseDataType(DataTypeInfoTable &globalTable){
    struct CBaseTypeInfo{

        QList<QStringList> identifier;
        uint size;
        DataBaseType type;
    };
    QList<CBaseTypeInfo> cbaseType={
        {{
             {"int"},{"long"},{"signed","long"},{"signed","int"},{"long","int"},{"signed","long","int"}
         },4,DataBaseType_INT},
        {{
             {"short"},{"signed","short"},{"short","int"},{"signed","short","int"}
         },2,DataBaseType_SHORT},
        {{
             {"char"},{"signed","char"},
         },1,DataBaseType_CHAR},
        {{
             {"long","long"},{"signed","long","long"},{"long","long","int"},{"signed","long","long","int"}
         },8,DataBaseType_LONG},


        {{
             {"unsigned","long"},{"unsigned","int"},{"unsigned","long","int"}
         },4,DataBaseType_UINT},
        {{
             {"unsigned","short"},{"unsigned","short","int"}
         },2,DataBaseType_USHORT},
        {{
             {"unsigned","char"},
         },1,DataBaseType_UCHAR},
        {{
             {"unsigned","long","long"},{"unsigned","long","long","int"}
         },8,DataBaseType_ULONG},

        {{
             {"float"},
         },4,DataBaseType_FLOAT},

        {{
             {"long","double"},{"long","float"},{"double"}
         },8,DataBaseType_DOUBLE},
        {
            {{"void"}},0,DataBaseType_VOID
        }
    };

    for(int i = 0;i<cbaseType.length();i++){

        DataTypeInfo type;
        type.baseType = cbaseType[i].type;
        type.dataBaseTypeBytes = cbaseType[i].size;
        globalTable.allDataType.append(type);


        for(int j = 0;j<cbaseType[i].identifier.length();j++){
            MappingInfo mapInfo;
            mapInfo.index.append(0);
            mapInfo.index.append(globalTable.allDataType.length()-1);
            globalTable.typeNameMapping.insert(cbaseType[i].identifier[j],mapInfo);
        }
    }

}

//注册入汇编语言的内置宏定义
static void registerBuiltinMacro(SrcObjectInfo &objInfo,QString macroName,QString macroText){
    bool success;
    QString srcPath;
    PhraseList srcExpList = lexicalAnalyzer_C(objInfo,
                                              srcPath,
                                              macroText,
                                              &success);
    DefineMacroInfo info;
    info.value = srcExpList;

    objInfo.defineMacro(macroName,info);
}



QString compileSrc(QString src,//要编译的源码
                   QString srcPath,
                   CompileType compileType,//编译的功能类型
                   QStringList includeDir,//#include""默认搜索的目录
                   QStringList stdIncludeDir,//#include<>默认搜索的目录
                   QStringList exterdAttributes,//支持的扩展属性
                   bool&stutes,//编译结果
                   QList<PromptInfo> &prompts,//编译报错信息
                   CpuRegBitwide cpuBitwide,//cpu寄存器的位数
                   RamEndian ramEndian){//使用的内存大小端方式
    stutes = 0;
    //存储源码中所有信息的对象(定义的数据类型,变量,函数,以及缓存编译出的PSDL)
    SrcObjectInfo srcObjInfo;
    srcObjInfo.includeDir = includeDir;
    srcObjInfo.stdIncludeDir = stdIncludeDir;

    QDateTime currentTime = QDateTime::currentDateTime();

    //将c语言的基础数据类型导入到全局数据类型信息表中
    registerCbaseDataType(srcObjInfo.getDataTypeInfoNode()->thisNodeTable);
    stutes = 0;
    //添加__DATA__内置宏(Mmm dd yyyy 当前的日期信息)
    registerBuiltinMacro(srcObjInfo,"__DATE__","\""+currentTime.toString("mm dd yyyy")+"\"");
    //添加__TIME__内置宏(hh:mm:ss 当前的是时间信息)
    registerBuiltinMacro(srcObjInfo,"__TIME__","\""+currentTime.toString("hh:mm:ss")+"\"");

    //定义支持的扩展属性
    srcObjInfo.allExterdAttributes = exterdAttributes;
    srcObjInfo.ramEnd = ramEndian;
    if(cpuBitwide == CpuRegBitwide_8bit){
        srcObjInfo.CPU_BIT = 8;
    }else if(cpuBitwide == CpuRegBitwide_16bit){
        srcObjInfo.CPU_BIT = 16;
    }else if(cpuBitwide == CpuRegBitwide_32bit){
        srcObjInfo.CPU_BIT = 32;
    }else if(cpuBitwide == CpuRegBitwide_64bit){
        srcObjInfo.CPU_BIT = 64;
    }else{
        return QString();
    }


    //调用词法分析器，将c语言源码转为token
    bool lexicalAnalysisSuceess;
    PhraseList srcExpList = lexicalAnalyzer_C(srcObjInfo,
                                              srcPath,
                                              src,
                                              &lexicalAnalysisSuceess);

    if(!lexicalAnalysisSuceess){
        stutes = 0;
        prompts = srcObjInfo.allPromptInfos;
        return QString();
    }

    //调用预编译器,解析预编译指令
    if(!analysisPrecompile(srcObjInfo,srcExpList)){
        //预编译失败
        stutes = 0;
        prompts = srcObjInfo.allPromptInfos;
        return QString();
    }

    //判断如果仅需要预编译即可，那么直接将预编译后的token重新组合为c代码文本返回
    if(compileType == Precompile_I){
        stutes = 1;

        QString txt = srcExpList.toSrcText();

        //添加标头
        /*
         * ////////////////////////////////
         * 该文件由开源秦岭-C语言编译器编译
         * 完成时间:  xxxx年xx月xx日
         *
         * OpenQinling开源项目简介
         *
         */

        QDateTime currentTime = QDateTime::currentDateTime();

        QString title = "//////////////////////////////////////////////////////////////////////////////////////////\r\n";
        title +=        "//此已展开预编译指令的C语言文件由开源秦岭-C语言预处理编译器完成预处理命令解析\r\n";
        title +=        "//完成时间:"+currentTime.toString("yyyy年MM月dd日 - hh:mm:ss")+"\r\n";
        title +=        "//开源项目地址:https://gitee.com/enlighten7328/OpenQinling\r\n";
        title +=        "//作者抖音号:I_am_a_big_fool4\r\n";
        title +=        "//作者哔站号:咋都是名称已存在\r\n";
        title +=        "//作者邮箱号:1969798169@qq.com\r\n";
        title +=        "//[免责声明:该项目仅可用于学习、交流用途,因私自用于商业行为造成的任何后果作者概不负责]\r\n";
        title +=        "//////////////////////////////////////////////////////////////////////////////////////////\r\n";
        prompts = srcObjInfo.allPromptInfos;
        return title+txt;
    }


    //调用语法分析器，将token转为PSDL-IR源码
    analysisGrammatical_C(srcObjInfo,srcExpList,stutes);
    prompts = srcObjInfo.allPromptInfos;
    if(!stutes)return QString();

    //调用PSDL-IR源码优化器,优化生成的PSDL-IR指令


    stutes = 1;
    QString title = "########################################################################################\r\n";
    title +=        "#该IR语言文件由开源秦岭-C语言编译器完成编译\r\n";
    title +=        "#完成时间:"+currentTime.toString("yyyy年MM月dd日 - hh:mm:ss")+"\r\n";
    title +=        "#适用CPU位数:"+QString::number(srcObjInfo.CPU_BIT)+"\r\n";
    title +=        "#适用内存字节序:"+QString(ramEndian==CPURAM_BIG_ENDIAN ? "大端字节序" : "小端字节序")+"\r\n";
    title +=        "#使用的字符串编码格式:UTF8\r\n";
    title +=        "#开源项目地址:https://gitee.com/enlighten7328/OpenQinling\r\n";
    title +=        "#作者抖音号:I_am_a_big_fool4\r\n";
    title +=        "#作者哔站号:咋都是名称已存在\r\n";
    title +=        "#作者邮箱号:1969798169@qq.com\r\n";
    title +=        "#[免责声明:该项目仅可用于学习、交流用途,因私自用于商业行为造成的任何后果作者概不负责]\r\n";
    title +=        "########################################################################################\r\n";

    return title+srcObjInfo.rootMiddleNode.toString();
}

}}QT_END_NAMESPACE
