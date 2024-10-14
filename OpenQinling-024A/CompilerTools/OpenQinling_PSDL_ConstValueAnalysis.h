#ifndef OPENQINLING_PSDL_CONSTVALUEANALYSIS_H
#define OPENQINLING_PSDL_CONSTVALUEANALYSIS_H
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

//获取一个数至少要多少位才可以存储
uint getNumDigit(ulong num);

//解析基础常量定义格式
uint analysisStdConst(QString stdConstText,bool*isSuceess=NULL);


//将中级语言定义常量的文本解析转换为汇编语言定义常量的文本
//同时返回出中级语言定义的常量数据类型/常量位数
//txt:中级语言常量定义文本,
//ptrList:当前源文件中存在的指针标记名(解析函数/全局变量地址的常量时要用)
//
//type:常量数据类型
//size:常量
QString analysisConstValue(QString txt,QStringList&ptrList,bool&suceess,QString &type,uint &size);

}

}
QT_END_NAMESPACE

#endif // OPENQINLING_PSDL_CONSTVALUEANALYSIS_H
