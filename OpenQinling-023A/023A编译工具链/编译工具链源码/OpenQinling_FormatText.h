#ifndef OPENQINLING_FORMATTEXT_H
#define OPENQINLING_FORMATTEXT_H

#include <QString>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
//去除注释[支持#]
QString removeExpNote(QString txt);
//去除掉\t和多个空格以及首尾的空格
QString dislodgeSpace(QString txt);
//文本整体tab右移[num为右移次数]
QString textRightShift(QString txt,int num);
}
QT_END_NAMESPACE

#endif // OPENQINLING_FORMATTEXT_H
