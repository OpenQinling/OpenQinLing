#ifndef OPENQINLING_ASM_TRANSPLANT_H
#define OPENQINLING_ASM_TRANSPLANT_H

#include <QByteArray>
#include <QStringList>
#include "OpenQinling_ASM_Typedefs.h"

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

/********************
 * 汇编编译器移植接口
 ********************/

/////////////选择cpu的类型//////////////////
#define CPU_023A 1
//////////////////////////////////////////

//cpu架构的位数
#define CPU_Bits 64

//功能:获取自定义了哪些汇编指令
QStringList asmCustomInstructions();

//当前指令是否支持标记
bool thisCodeOrderIsSupportLabel(QString order,int argIndex,int argTotal);

//该指令支持的标记最大字节数
uint thisCodeOrderSupportMaxBits(QString order);

//标记引用语法中未声明higth,low。取默认的higth,low
void getMarkDefaultHL(QString order,uint &h,uint &l);

//确定order_offset_Byte、order_offset_Bit
QList<MarkQuote> markQuoteFromCodeOrder(QString order,
                                        int low,int higth,int offset,QString blockName,QString markName//analysisCodeBlockMarkQuote编译生成的
                                        );


//功能:编译一条汇编指令
//参数: name为汇编指令名称 args为指令参数 status返回编译情况[0成功 >=1失败]
//返回值:返回编译出的二进制码,失败则QByteArray长度为0
QByteArray compileCodeAsm(QString name,QStringList args,int *status);
/*
 * status失败类型
 * 1: 使用了指令集中未定义的指令
 * 2: 指令的参数数量不正确
 * 3: 使用了指令集中未定义的寄存器
 * 4: 使用了该参数位上禁止使用的寄存器
 * 5: 指令中立即数的大小超过了该指令允许的范围
 * 6: 在禁止使用立即数的参数位上使用了立即数
 */

}}
QT_END_NAMESPACE



#endif // OPENQINLING_ASM_TRANSPLANT_H
