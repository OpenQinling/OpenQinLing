#ifndef OPENQINLING_LINK_MEMORYSET_H
#define OPENQINLING_LINK_MEMORYSET_H

#include <QStringList>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{
//内存数据翻转
bool memreverse(void*a,void*b,size_t size);
//一段内存数据右移(i:右移的位数)
QByteArray byteArrayRightShift(QByteArray bytes,int i);
}}
QT_END_NAMESPACE

#endif // OPENQINLING_LINK_MEMORYSET_H
