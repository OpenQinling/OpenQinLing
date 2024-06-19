#ifndef OPENQINLING_PSDL_FORMATTEXT_H
#define OPENQINLING_PSDL_FORMATTEXT_H
#include <QStringList>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name);

//判断定义的变量数据类型是否符合要求
bool varTypeIsConform(QString typeName);

//如果是指针类型变量,指针类型转换为无符号整型类型(具体根据不同的cpu位数来确定,32位cpu转为UINT)
QString pointerTypeConvert(QString typeName);

//获取变量数据类型的字节数
uint getVarTypeSize(QString typeName);

}
}
QT_END_NAMESPACE


#endif // OPENQINLING_PSDL_FORMATTEXT_H
