#ifndef OPENQINLING_ASM_FORMATTEXT_H
#define OPENQINLING_ASM_FORMATTEXT_H
#include <QStringList>
#include <QDebug>
#include "OpenQinling_ASM_Typedefs.h"
#include "OpenQinling_Math.h"
#include "OpenQinling_ASM_TextFlag.h"
#include "OpenQinling_ASM_Transplant.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
//去除括号内的内容，转为......
QString removeParentInside(QString txt,uint type);
//去除文本换行,并将文本以;号进行分割为List [lineFeed返回每个list单元产生换行的次数 preposeLine每个单元第几行真正开始是代码]
QStringList segment(QString txt,QList<uint> *lineFeed,QList<uint> * startLine);
//根据索引号分割字符串
QStringList splitStringFromIndex(QString str,QList<uint> index);
//格式化全局指令文本[指令文本结构:  指令操作符 参数1,参数2,参数n...{块指令文本内容};]
GlobalOrder formatOrderText(QString text,bool*status);
//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name);
//格式化块指令文本[块指令文本结构:  标记1>标记2> 指令操作符 参数1,参数2,参数n...]
//status:[0正常 1标记名为空 2标记名称不合规范 3指令结构不正确
AsmBlockCode formatBlockOrderText(QString text,int*status);
//判断一个字符是否是在""引号扩起的字符串之中
bool charIsInString(QString txt,int index);
//获取一个字符是在该段文本的第几行
int getCharInStringLine(QString txt,int index);
//替换字符串中的转义字符
QByteArray replaceTransChar(QString strData);
//判断该文本是否是描述的一个路径的字符串,是则返回路径内容
QString getPathInStr(QString txt,bool*status);
}}
QT_END_NAMESPACE
#endif // OPENQINLING_ASM_FORMATTEXT_H
