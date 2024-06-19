#ifndef OPENQINLING_ASM_TEXTFLAG_H
#define OPENQINLING_ASM_TEXTFLAG_H
#include <QString>
#include <QStringList>
#include <QDebug>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

//字符串提取器，并将原字符串的位置替换为标志号,标志号对应一个被提取的字符串，用于还原
//标志号的结构: $类型码 索引码 [类型为2位16进制数，索引号为5位16进制数]
//示例:   $01100a1  01是字符串的类型码，100a1对应字符串缓存库中第0x100a1位置的字符串
//isWindup是否去除成功了(如果不成功，说明存在一个字符串没有收尾) strStartp没有收尾的那个字符串定义的索引号
QString extractString(QString txt,QStringList* bank,bool*isWindup,int*strStartp=NULL);

//括号提取器[type:括号类型  0() 1[] 2{} 3<>]
//$1type 5位索引码[示例 $1100a1 11是[]括号的类型码]
QString extractParent(QString txt,uint type,QStringList* bank);

//获取一段字符串中，存在多少个指定类型的标志号
uint getFlagQuantity(QString text,QString typeCode);

//从标志号中提取出字符串
QString getStringFromFlag(QString flagTXT,QString typeCode,QStringList &str,bool*isSrceess=NULL);

//将一串文本中，所有标志号替换为原先的字符串
QString reductionString(QString text,QString typeCode,QStringList &str);

//判断一段文本是否是一个标志号
bool textIsFlag(QString text);

//从一段文本中分离出普通字符串和标志号(已$号进行分割)
QStringList separateFlag(QString text);

}}
QT_END_NAMESPACE

#endif // OPENQINLING_ASM_TEXTFLAG_H
