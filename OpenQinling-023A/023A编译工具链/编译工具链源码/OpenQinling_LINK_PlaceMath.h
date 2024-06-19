#ifndef OPENQINLING_LINK_PLACEMATH_H
#define OPENQINLING_LINK_PLACEMATH_H

#include <QStringList>
#include "OpenQinling_LINK_Typedefs.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{
//获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
QList<bool> getBlockOverlap(QList<BlockInfo> memSpace);
//整合重叠的区域,并从小到大排序
QList<BlockInfo> integrationOverlapPlace(QList<BlockInfo> memSpace);
//判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
bool judgeContainBlockSet(QList<BlockInfo> memSpaceA,
                          QList<BlockInfo> memSpaceB);
//在块集A上减去块a
QList<BlockInfo> removeOccupiedMemBlock(QList<BlockInfo> memSpaceA,
                                        BlockInfo mema);

//在块集A上找到能放下块a的位置
BlockInfo searchFreeMemarySpace(QList<BlockInfo> memSpaceA,uint mema_size,bool *isScueess);


}}
QT_END_NAMESPACE

#endif // OPENQINLING_LINK_PLACEMATH_H
