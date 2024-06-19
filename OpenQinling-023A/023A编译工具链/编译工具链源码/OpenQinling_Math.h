#ifndef OPENQINLING_ASM_MATH_H
#define OPENQINLING_ASM_MATH_H
#include <QList>
#include <QDebug>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
//uint从小到大排序
QList<uint> uintSortMinToMax(QList<uint> d);
//去除uint中的重复值
QList<uint> uintDeduplication(QList<uint> d);
//判断一个uint类型变量的值，是否是正好为(2^n),如果是,输出n的值。否则输出-1
int isPowerOfTwo(uint32_t x);


}
QT_END_NAMESPACE

#endif // OPENQINLING_ASM_MATH_H
