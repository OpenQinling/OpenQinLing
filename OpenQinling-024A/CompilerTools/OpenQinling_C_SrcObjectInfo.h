#ifndef SRCOBJECTINFO_H
#define SRCOBJECTINFO_H
#include "OpenQinling_Typedefs.h"
#include <math.h>
#include <iostream>
#include <QStack>
#include "OpenQinling_PSDL_MiddleNode.h"
#include "OpenQinling_C_AnalysisSrc.h"
#include "OpenQinling_C_Phrase.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

using namespace std;
//c语言的基础数据类型
enum DataBaseType{
    DataBaseType_VOID,   //void
    DataBaseType_CHAR,   //char
    DataBaseType_UCHAR,  //unsigned char
    DataBaseType_SHORT,  //short
    DataBaseType_USHORT, //unsigned short
    DataBaseType_ENUM,   //enum
    DataBaseType_INT,    //int/long
    DataBaseType_UINT,   //unsigned int
    DataBaseType_LONG,   //long long
    DataBaseType_ULONG,  //unsigned long long
    DataBaseType_FLOAT,  //float
    DataBaseType_DOUBLE, //double
    DataBaseType_FUNPTR, //函数指针
    DataBaseType_STRUCT,//struct
    DataBaseType_UNION  //union
    //结构体和共用体本质是一种数据类型,只是共用体的属性内存重叠
};

struct DataTypeInfo;
struct SrcAllDataTypeTree;

//变量、结构体属性的偏移量/数组维度信息
struct PointerArrayInfo{
    //示例: int * arr[10][20][30] : info = {30,20,10,-1}
    QList<int> info;//-1是指针,否则为数组

    //解引用 * []运算
    PointerArrayInfo dereference(){
        PointerArrayInfo tmp = *this;
        tmp.info.removeFirst();
        return tmp;
    }
    //取地址 &运算
    PointerArrayInfo getAddress(){
        PointerArrayInfo tmp = *this;
        tmp.info.prepend(-1);
        return tmp;
    }

    //判断第i个维度是否是指针(i越小,维度越高)
    bool isPointer(int i = 0){
        return info[i] < 0;
    }

    //判断第i个维度是否是数组(如果为0,就不是数组)
    int isArray(int i = 0){
        if(info[i] < 0)return 0;
        return info[i];
    }
};


//结构体/共用体属性信息、函数返回值/参数信息
struct StructUnion_AttInfo{
    int attIndex;//如果是结构体/共用体的属性,这是第几个定义的属性
    QList<int> arraySize;//如果是数组型,数组各维度的大小
    int pointerLevel = 0;//如果是指针型,指针的级数
    uint offset = 0;//属性相当于结构体基址的内存偏移量(共用体始终为0),如果是函数参数该类属性无效
    bool isConst = 0;//共用体/结构体当前属性是否是一个常量,如果是函数参数该类属性无效
    bool isPointerConst = 0;//是否是指针常量
    //属性指向的类型信息
    //[属性指向的类型只能是基础类型,不能够是一个派生类型,属性本身是否是派生类型由deriveType指定]
    IndexVN typeIndex;//可以是n级索引,前(n-1)级是属于哪一张表,最后1级指向表中的哪个属性

    bool operator==(const StructUnion_AttInfo&other);
    bool operator!=(const StructUnion_AttInfo&other);
};

//数据类型信息
struct DataTypeInfo{
    DataBaseType baseType;//基础类型
    bool isStdcallFun = 0;//如果是函数类型,是否是stdcall调用约定的可变参数函数

    uint dataBaseTypeBytes = 0;//该数据类型占用的字节数
    int maxAttSize = 0;//如果是结构体,结构体中最大的基础数据类型的总字节数
    /*struct{
     *      int a;
     *      struct{
     *          char a;
     *          long long b;
     *      }b;
     *}
     *最大的基础数据类型就是 b.b(long long)
     */

    //如果是enum/struct/union,结构名称是什么
    QString structName;
    bool isAnnony = 0;//是否匿名
    bool isOnlyStatement = 0;//如果是enum/struct/union,是否仅仅声明了,还没有定义
    //如果是此种情况,那么将限制该类型的使用[1.只能定义为指针类型变量/属性,2.定义的指针也无法访问任何结构内属性,3.不能通过sizeof()获取字节数]

    //变量基础类型是enum时,enum中各常量值是多少
    QMap<QString,int> enumConstValue;

    //如果变量基础类型是struct/union,结构中属性的信息
    QMap<QString,StructUnion_AttInfo> attInfos;

    //如果是函数指针类型,存储函数返回值即参数的信息
    QList<StructUnion_AttInfo> funArgInfos;
    //至少有1个,[0]为函数返回值类型,此后的n个就是函数参数类型信息

    //判断两个类型是否相等
    bool operator==(const DataTypeInfo&other);
    bool operator!=(const DataTypeInfo&other);

    //如果是结构体/共用体类型,根据索引号获取属性的信息
    StructUnion_AttInfo structGetAttFromIndex(int index,bool &status);//打印信息
    QString toString();

};

struct MappingInfo{
    IndexVN index;//allDataType的索引
    //以下是通过typedef自定义数据类型才有效的属性
    bool isDefaultConst = 0;//是常量、常量指针(指向的变量不可被修改)
    bool isDefaultPointerConst =0;//指针常量(指针的地址不可修改)
    //正常情况下类型都是变量,通过const关键字声明为常量
    //但也可在typedef中添加const关键字,此时新建的数据类型在创建变量时无论是否有const都是一个常量
    int pointerLevel = 0;//该类型是否默认为一个n级指针
    //如果通过该类型创建变量的话,最终要创建出来的变量的指针级数为= pointerLevel+创建指针时*的个数
    //如果创建出的变量指针级数=0,那么就是一个普通变量
};
//数据类型信息映射表
struct DataTypeInfoTable{
    IndexVN thisIndex;//当前节点的索引号
    //标识符与数据类型信息的映射表(如果是匿名结构体/共用体/枚举,那么就没有映射关系)
    //value为MappingInfo,映射信息
    //key由n个词组组成的标识符
    QMap<QStringList,MappingInfo> typeNameMapping;
    //对于c语言自带基础数据类型和通过typedef语句定义的类型会将映射信息存在此处

    //定义的结构体/共用体/枚举的名称到数据信息的映射表
    QMap<QString,int> customSturctMapping;

    //数据类型信息向量表
    QList<DataTypeInfo> allDataType;

    //根据类型名称搜索到类型信息存储的索引号,失败返回-1
    IndexVN getTypeInfo(QStringList name);
    //根据结构体/共用体/枚举的名称获取到自定义结构的信息索引号
    int getCustomSturctInfoIndex(QString name);
    //判断一个数据类型是否在类型表中存在,存在返回索引号,不存在返回-1
    int isHaveType(DataTypeInfo&info);
    //根据索引号获取类型信息
    DataTypeInfo& operator[](int index);
    //获取当前表总共有多少个类型
    int length();
    //打印出映射表的信息
    QString toStringMapInfo();
    //打印出定义的结构体/共用体/枚举的名称到数据信息的映射表信息
    QString toStringCustomSturctMapInfo();
    //打印数据类型向量表信息
    QString toStringDataTypeInfo();


};

//数据类型信息映射表树,实际程序中是由1张全局类型信息映射表和n张临时映射表组成的
//类型信息树的索引系统: 由IndexVN对象来表示,IndexVN是int序列,且一个有效的IndexVN不可为空,至少存在1级
//      例如对于根节点,其索引号为[0],根节点下的子节点就是[0][n],子节点n下的m节点,索引号为[0][n][m]
struct SrcAllDataTypeTree{
  /////////////////节点构建/////////////////////
    //构建1个根节点
    SrcAllDataTypeTree();
    //创建一个子节点,并返回子节点的索引号
    IndexVN appendNewSubNode();
    bool removeNode(IndexVN index);


    ///////////////////杂项功能/////////////////
    //判断文本是否是一个枚举常量
    bool isEnumConstValue(QString txt);
    //获取一个枚举常量的值
    int getEnumConstValue(QString txt,bool *isSuceess = NULL);

 //////////////////搜索功能//////////////////
    //获取当前对象在整个树中的索引号
    IndexVN getThisNodeIndex();
    //搜索一个定义的数据类型,存在则返回索引号
    IndexVN searchDataType(QStringList name);

    //向上搜寻一个非匿名结构体是否存在,存在返回索引号
    IndexVN searchStructType(QString name);
    //向上搜寻一个非匿名共用体是否存在,存在返回索引号
    IndexVN searchUnionType(QString name);
    //向上搜寻一个非匿名枚举是否存在,存在返回索引号
    IndexVN searchEnumType(QString name);
    //向上搜索一个变量类型映射
    MappingInfo searchMappingInfo(QStringList name,bool &status);
    //向上搜素一个函数指针类型[函数指针类型的特点是其只会存储在根节点中]
    IndexVN searchFunctionType(DataTypeInfo &funTypeInfo);



 ////////////////根据索引号获取对象信息////////////////
    //获取一个子节点的引用
    SrcAllDataTypeTree& operator[](int i);
    //根据索引号获取一个数据类型信息对象的引用
    DataTypeInfo*getDataType(IndexVN index);
    //根据索引号获取当前树中任意一个节点的指针
    SrcAllDataTypeTree*getNode(IndexVN index);
    //获取一个数据类型的字节数(传类型的索引号),错误返回-1
    int getTypeSize(IndexVN typeIndex);



 //////////////////增加类型定义//////////////////
    //在当前节点添加一个自定义数据类型,绑定到另一个数据类型上(相当于为数据类型起一个别名)
    bool appendNewTypeName(QString newName,QStringList bindTypeName,bool isDefaultConst,
                           bool isDefaultPointerConst,int pointerLevel);
    //在当前节点添加一个自定义数据类型,通过索引号的方式
    bool appendNewTypeName(QString newName,//类型名
                           IndexVN tableIndex,//绑定到的表
                           int typeIndex,//表中的哪个类型
                           bool isDefaultConst,
                           bool isDefaultPointerConst,int pointerLevel);
    //添加一个自定义的结构体/共用体/枚举类型(返回添加成功后的索引,如果失败返回空)
    IndexVN appendCustomSturctOrUnionOrEnum(DataTypeInfo&info);
    //添加一个函数指针类型[函数指针类型的特点是其只会存储在根节点中,如果已经存在了该函数类型,直接返回类型的索引]
    IndexVN appendFunctionType(DataTypeInfo&info);
    //添加一个匿名VOID类型()



  ////////////////////DEBUG功能///////////////////
    //将一个类型的信息文本信息表现出来
    QString typeInfoToString(IndexVN typeIndex);


 ///////////////////////属性////////////////////////
    //当前节点的索引号
    IndexVN thisIndex;
    //树的根节点指针
    SrcAllDataTypeTree *rootNode = this;
    //上级节点指针
    SrcAllDataTypeTree *parentNode = NULL;
    //当前节点的类型信息表
    DataTypeInfoTable thisNodeTable;
    //子节点的类型信息表
    QList<SrcAllDataTypeTree> subNode;
};

//全局常量初始值的类型
enum ConstValueType{
    ConstValueType_ERROR,//0

    ConstValueType_UINT8,//1
    ConstValueType_INT8,//2
    ConstValueType_UINT16,//3
    ConstValueType_INT16,//4
    ConstValueType_UINT32,//5
    ConstValueType_INT32,//6
    ConstValueType_UINT64,//7
    ConstValueType_INT64,//8

    ConstValueType_FLOAT,//9
    ConstValueType_DOUBLE,//10

    ConstValueType_POINTER,//11
};//当前结果的数据类型(数据类型不等于实际类型)
//全局变量初始值表达式运算过程中的临时变量
struct GlobalInitValueTmp{

    ConstValueType valueType = ConstValueType_ERROR;

    //例如 int p; (ulong)&p当前结果的类型是UINT64,但T实际其类型依然是一个指针

    //数据值的实际类型是否是指针(整型数据可能其实际值是指针,但浮点型数据其实际值只可能是浮点数)
    bool factIsPointer = 0;

    //数据的值
    int64_t intData;//整型数据且非指针的情况下使用该值
    double  floatData;//浮点型数据使用该值

    //如果类型是指针,指针所指向的数据类型的字节数
    int pointerBytes;

    //实际值是指针型数据的情况下使用的值
    QString pointerData_mark;//指针指向的标记名
    int pointerData_offset;//指针偏移地址

    QString typeToString();
    QString toStirng();
    //数据类型转换
    //pointerBytes:如果转换的是指针型,转出的指针对应的数据类型的字节数
    GlobalInitValueTmp converType(ConstValueType converType,int pointerBytes = 0);
    GlobalInitValueTmp operator~();
    GlobalInitValueTmp operator!();
    GlobalInitValueTmp operator+(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator-(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator*(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator/(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator%(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator>(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator>=(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator==(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator!=(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator<(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator<=(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator|(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator&(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator^(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator||(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator&&(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator<<(GlobalInitValueTmp &var);
    GlobalInitValueTmp operator>>(GlobalInitValueTmp &var);


};
//程序中变量的定义信息
struct VarDefineInfo{
    QString varName;//变量名称

    QString psdlName;//变量在转为psdl语言后,其对应的psdl变量的名称

    //变量属性信息
    //isExtern和isStatic不可同时为1
    bool isExtern = 0;//是否是外部导入的
    bool isStatic = 0;//是否为静态区变量
    //[对于全局变量来说,如果声明为静态,那么其在其他src中将无法被访问。如果是在函数中的局部变量声明为静态,那么实际该变量就成为了一个全局静态变量]
    bool isConst = 0;//是否为1个常量
    bool isPointerConst = 0;//是否为指针常量

    QList<PsdlIR_Compiler::MiddleNode_Att> extendAttributes;//psdl扩展属性


    //变量基础类型的索引号,指向SrcAllDataTypeTree中的一个变量信息
    IndexVN baseTypeIndex;
    QList<int> arraySize;//如果是数组派生型,数组各维度的大小
    int pointerLevel = 0;//如果是指针派生型,指针的级数

    bool isFuncVar = 0;//是否是函数中的动态局部变量

    bool globalVarIsInitValue = 0;//如果当前是一个c语言基础数据类型的全局变量(指针、整数、浮点数),是否有进行初始化值
    GlobalInitValueTmp globalInitValue;//如果当前是一个全局变量,存储变量的初始值
    bool isAutoArrSize = 0;//c语言定义数组型变量时,支持最高维度的数组的单元数可不写,通过写入的元素来自动求出总单元数,如果要用该功能isAutoArrSize==1

    QString toString();
};

//程序变量定义树
struct SrcAllVarInfoTree{
    SrcAllVarInfoTree();
    //根据索引号获取当前树中任意一个节点的指针
    SrcAllVarInfoTree*getNode(IndexVN index);
    //获取当前对象在整个树中的索引号
    IndexVN getThisNodeIndex();
    //新建一个节点
    IndexVN appendNewSubNode();
    //删除一个节点以及该节点下所有所有内容
    bool removeNode(IndexVN index);
    //定义一个变量(返回定义成功后的变量索引号)
    IndexVN defineVar(VarDefineInfo &varInfo);
    //向上搜索一个变量
    IndexVN searchVar(QString varName);
    //获取一个变量的信息
    VarDefineInfo* getVar(IndexVN index);
    //获取变量是全局变量还是局部变量,是全局变量返回1,局部变量0
    bool getVarIsInGlobal(IndexVN index);
    //打印输出当前节点变量的信息
    QString thisNodeVarInfoToString();



    //当前节点的索引号
    IndexVN thisIndex;
    //树的根节点指针
    SrcAllVarInfoTree *rootNode = this;
    //上级节点指针
    SrcAllVarInfoTree *parentNode = NULL;
    //子节点的类型信息表
    QList<SrcAllVarInfoTree> subNode;
    //当前节点的变量定义表
    QList<VarDefineInfo> thisVarTable;


};

//函数定义表
struct FunctionInfo{
    QString functionName;
    bool isStatic;//是否为静态函数[如果声明为静态,那么该函数就是当前src中私有的函数了]
    bool isOnlyStatement;//是否仅声明了
    //函数类型的索引号,指向SrcAllDataTypeTree中的一个变量信息
    IndexVN baseTypeIndex;
    //函数的定义,会在SrcAllDataTypeTree全局区中创建一个同结构函数指针类型

    QList<PsdlIR_Compiler::MiddleNode_Att> extendAttributes;//psdl扩展属性

    bool operator==(FunctionInfo&info);
    bool operator!=(FunctionInfo&info);
    QString toString();


};

enum CurrentAnalysisType{
    CurrentAnalysis_Global,//当前正在解析的是全局区
    CurrentAnalysis_Function,//当前正在解析的是函数
    CurrentAnalysis_Process,//当前正在解析的是流程块
    CurrentAnalysis_SturctUnionAtt,//当前正在解析的是结构体/共用体中的属性定义
};






//函数中循环块的信息(continue和break语句需要该信息)
struct WhileBlockInfo{
    bool isSwitch = 0;//是否是switch
    QString beginMark;//循坏块起始标记点(switch没有该属性)
    QString endMark;//循坏块结束标记点
};

//解析函数中指令，当前所在的流程块类型
enum ProcessType{
    funBodyProcess,//函数主体
    subProcess,// {}
    ifelseProcess,//选择流程块: if/else if/else/case
    switchProcess,//switch流程块
    whileProcess,//循环流程块:  while/for/do...while
};
//跳转标记点时,跳转的标记信息
struct JumpMarkInfo{
    QString markName;//goto语句跳转的标记名
    QString srcPath;//goto语句的源文件路径
    int line,col;//goto语句所在的源文件的行数列数
};


//宏定义信息
struct DefineMacroInfo{
    QStringList args;//宏定义的参数
    bool isVariableArg = 0;//是否为可变参数宏定义
    PhraseList value;//宏定义要替换为的文本token
};


//c源文件信息
struct SrcObjectInfo{
    QList<PromptInfo> allPromptInfos;//编译过程中警告/报错的信息
    QStringList allExterdAttributes;//所有支持的扩展属性

    int CPU_BIT = 32;//cpu的位数
    RamEndian ramEnd;//内存大小端(默认大端)

    //添加一条提示信息
    void appendPrompt(QString prompt,QString srcPath,int line,int col);

    //当前源码中所有的宏定义
    QMap<QString,DefineMacroInfo> defines;//所有的宏定义
    QStringList includeDir,stdIncludeDir;//头文件的搜索目录

    //添加一个宏定义
    bool defineMacro(QString name,DefineMacroInfo value);

    //取消一个宏定义
    void undefineMacro(QString name);

    //获取一个宏定义的信息
    DefineMacroInfo getMacroValue(QString name);

    //是否存在宏
    bool containsMacro(QString name);






    //源码中定义的函数
    QList<FunctionInfo> allDefineFunction;
    //源码中定义的所有变量
    SrcAllVarInfoTree allDefineVar;
    //源码中定义的所有数据类型
    SrcAllDataTypeTree allDefineType;
    //生成的MiddleNode
    PsdlIR_Compiler::MiddleNode rootMiddleNode;
    //当前源码位置的变量/类型表索引号
    IndexVN currentVarTableIndex;
    IndexVN currentTypeTableIndex;
    QList<CurrentAnalysisType> currentAnalysisType;//当前正在解析的内容
    //获取当前正在解析的内容
    CurrentAnalysisType getCurrentAnalysisType();
    //获取当前正在操作的数据类型表索引号
    IndexVN getCurrentVarTableIndex();
    //获取当前正在操作的变量定义表索引号
    IndexVN getCurrentTypeTableIndex();
    //获取数据类型树的根节点指针
    SrcAllDataTypeTree* getRootDataTypeInfoNode();
    SrcAllVarInfoTree* getRootVarDefineInfoNode();
    //获取当前正在操作的数据类型树的节点指针
    SrcAllDataTypeTree* getDataTypeInfoNode();
    SrcAllVarInfoTree* getVarDefineInfoNode();
    //根据索引号获取数据类型树节点的指针
    SrcAllDataTypeTree* getDataTypeInfoNode(IndexVN index);
    //根据索引号获取变量定义表的指针
    SrcAllVarInfoTree* getVarDefineInfoNode(IndexVN index);
    //搜索一个函数的索引号
    int serachFunction(QString funName);
    //添加一个函数信息,返回成功后的函数索引号,失败-1
    int appendFunctionDefine(FunctionInfo&info);
    //根据索引号获取函数指针
    FunctionInfo*getFunctionInfo(int index);
    //初始化
    SrcObjectInfo();
    //开始解析一个结构体的属性定义
    //structInfo:当前数据结构的类型定义信息(对于属性中指向的类型指针可为无效)
    bool startAnalysisSturctUnionAtt();
    //结构体的属性定义解析完成
    bool endAnalysisSturctUnionAtt();
    //开始解析一个流程块
    bool startAnalysisProcess(ProcessType currentProcessType);
    //流程块解析完成
    bool endAnalysisProcess();
    //开始解析一个函数
    bool startAnalysisFunction();
    //函数解析完成
    bool endAnalysisFunction();
    //当前要定义一个数据类型,判断定义的名称是否合规
    //A-Z/a-z/0-9/_构成的文本,不得为关键字[生成词组流时已经检查了,无需再检查]
    //不得为当前变量表中存在的变量名
    //如果当前是在全局解析中,不得为已存在的函数名
    //不得为当前类型表中的枚举常量
    bool judegDefTypeNameIsCopce(QString name);
    //当前要定义一个变量/枚举常量,判断定义的名称是否合规
    //在judegDefTypeNameIsCopce的基础上
    //不得为当前类型表中结构体/共用体/枚举体的名
    //不得为已经存在的类型名
    bool judegDefineVarNameIsCopce(QString name);
    //打印输出当前所有函数的信息
    QString allFunctionInfotoString();
    //全局区中新增一个全局匿名字符串常量,返回常量名
    QString appendAnonyConstStrValue(QString strValue,int&strByteSize);

    //定义一个变量(返回定义成功后的变量索引号)
    IndexVN defineVar(VarDefineInfo &varInfo);

    //创建一个匿名标记点
    QString defineAnonyMark();



    IndexVN defineValist();



    //在函数中添加一个独立的临时变量[生成一个独立的psdl名称,并在psdl的vars列表中添加该变量的定义](返回成功后的变量索引号)
    //baseType:临时变量的基础数据类型,pointerLevel:临时变量的指针级数 (临时变量不可能是一个数组)
    IndexVN defineFuncTmpVar(IndexVN baseType,
                             int pointerLevel,
                             QList<PsdlIR_Compiler::MiddleNode> &varsAtt);
    IndexVN defineFuncTmpVar(IndexVN baseType,
                             int pointerLevel,
                             QList<int>arraySize,
                             QList<PsdlIR_Compiler::MiddleNode> &varsAtt);
    //在函数中添加一个映射到另一变量的临时变量[psdl名称使用已存在的另一个变量的名称](返回成功后的变量索引号)
    //baseType:临时变量的基础数据类型,pointerLevel:临时变量的指针级数 (临时变量不可能是一个数组)
    IndexVN defineFuncLinkVar(IndexVN baseType,int pointerLevel,QString psdlName);


    ///////////函数编译过程中的临时数据存储////////////////
    QStack<ProcessType> currentProcessType;//当前正在解析的函数中流程块的类型
    QStack<WhileBlockInfo> funcWhileBlockInfo;//函数中循环块的信息(continue和break语句需要该信息)
    int currentFuncVarCount;//当前函数中的变量总数
    int currentFuncTmpVarCount;//当前函数中独立临时变量的总数
    int currentFuncLinkVarCount;//当前函数中映射临时变量的总数
    //函数中定义的变量psdl名称为 {变量名}_{currentFuncVarCount}
    int currentFuncHaveWhileCount;//当前函数中存在多少个循环块(循环块的begin/end标记点的名称依赖于该变量)
    //循环块的begin/end标记点名称为 @while{Begin/End}{currentFuncHaveWhileCount}
    int currentAnonyMarkCount;//当前函数中匿名跳转标记点的数量
    int anonyStrValueCount = 0;//全局匿名字符串常量的数量全局匿名字符串常量转为psdl名称为 @AnonyStrValue{anonyStrValueCount}
    QStringList defMarks;//所有定义的函数中的goto跳转点名称
    QList<JumpMarkInfo> jumpMarks;//goto语句跳转的所有点名称
};
}


}
QT_END_NAMESPACE


#endif // SRCOBJECTINFO_H
