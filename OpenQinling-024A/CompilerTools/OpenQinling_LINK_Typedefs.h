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




class BlockInfo{
public:
    //支持存放的内存块
    struct SupportType{
        bool supCODE,supConstDATA,supInitDATA,supZeroInitData;

        bool operator==(SupportType &a);
        bool operator!=(SupportType &a);


    }supportType;


    void setSupportType(QString typeText = QString());
    bool isSupportType(ASM_Compiler::BinaryMemBlock &block);
    //    代码块  常量数据块   初始化的变量数据块  非初始化变量块
    uint64_t baseAddress;
    uint64_t sizeBytes;
};

//链接脚本中解析出的数据
class LinkSprict{
public:
    //能够存放内存块的区域
    QList<BlockInfo> memSpace;
    //要检查的内存块
    QList<QString> check;
    //固定基地址的内存块
    QMap<QString,uint64_t> fixed;

    QString toString();
};





}}
QT_END_NAMESPACE






#endif // OPENQINLING_LINK_TYPEDEFS_H
