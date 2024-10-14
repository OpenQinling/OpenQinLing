#ifndef JUDGESTRTOOLFUNCTION_H
#define JUDGESTRTOOLFUNCTION_H


#include <QStringList>
#include "OpenQinling_C_SrcObjectInfo.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

//检查文本是否是关键字
bool judgeIsKeyword(QString txt);
//检查文本是否符合标识符的语法要求
bool judgeIsIdentifying(QString txt);
//判断是否是c语言基础数据类型的单词结构,如果时,将其单词顺序标准化(c语言标准数据类型的单词顺序是可变的,所以要对其标准化)
//如果不是c语言基础数据类型的单词结构,保持不变
QStringList stdFormatBaseTypeName(QStringList baseTypeName);


//判断前几个运算符,如果是运算符,返回运算符的字符数
int judgeFastCharsIsDelimiter(QString &delText);

//检查是否是符号
bool judgeIsDelimiter(QString txt);

enum JudgeDefConstValueType{
    JudgeDefConstValueType_String,
    JudgeDefConstValueType_Char,
    JudgeDefConstValueType_Int,
    JudgeDefConstValueType_Long,
    JudgeDefConstValueType_Ulong,
    JudgeDefConstValueType_Uint,
    JudgeDefConstValueType_Float,
    JudgeDefConstValueType_Double
};
//检查是否是一个常量值
bool judgeIsConst(QString &txt,JudgeDefConstValueType &constType);

//解析出字符常量定义的字符:传入去除''的字符常量定义文本
char analysisCharConstValue(QString charConstValue,bool&status);

//替换字符串中的转义字符
QByteArray replaceTransChar(QString strData);//

//将字符串中 " ' \ 换行等字符替换为 \起始的 转义字符
QString createEscapeCharacter(QString strData);


}}
QT_END_NAMESPACE

#endif // JUDGESTRTOOLFUNCTION_H
