#ifndef OPENQINLING_LINK_TYPEDEFS_H
#define OPENQINLING_LINK_TYPEDEFS_H

#include <QStringList>
#include <QMap>
#include <OpenQinling_ASM_BinaryObjectLib.h>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace LIB_StaticLink{


#define DataMemBlock true;
#define CodeMemBlock false;
typedef ulong baseAddress;

class BlockInfo{
public:
    ulong baseAdress;
    ulong sizeBytes;
};

//链接脚本中解析出的数据
class LinkSprict{
public:
    //能够存放内存块的区域
    QList<BlockInfo> memSpace;
    //要检查的内存块
    QList<QString> checkData;
    QList<QString> checkCode;
    //固定基地址的内存块
    QMap<QString,baseAddress> fixed;

    QString toString();


};





}}
QT_END_NAMESPACE






#endif // OPENQINLING_LINK_TYPEDEFS_H
