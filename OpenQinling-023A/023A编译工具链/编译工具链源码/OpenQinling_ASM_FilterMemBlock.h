#ifndef OPENQINLING_ASM_FILTERMEMBLOCK_H
#define OPENQINLING_ASM_FILTERMEMBLOCK_H
#include "OpenQinling_ASM_Typedefs.h"
#include <QHash>
#include <QList>
#include <QDebug>
#include "OpenQinling_DebugFunction.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{
//单个内存块使用的标记所属内存名称
typedef QStringList MarksBlockNames;

//根据名称获取内存块索引号
IndexV2 getBlockIndexFromName(QList<AsmSrc> &src,QString &name);
//查询allBlocks中有用的内存块
void inquireUsefulBlocks(MarksBlockNames &nodeUseBlock,QHash<QString,MarksBlockNames> &range,QStringList &usefulBlockNames);
//过滤掉无用的内存块
void filtrMemBlock(QList<AsmSrc> &asmSrcList);
}}
QT_END_NAMESPACE


#endif // OPENQINLING_ASM_FILTERMEMBLOCK_H
