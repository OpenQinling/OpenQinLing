 #ifndef TERMINALPARSING_H
#define TERMINALPARSING_H
#include <QStringList>
#pragma execution_character_set("utf-8")

QT_BEGIN_NAMESPACE
namespace OpenQinling {

namespace Driver{
//一个终端命令
class TerminalCmd{
public:
    QString order;
    QStringList args;

    //判断是否已经存在指定的命令
    static bool cmdOrderIsExist(QString order,QList<TerminalCmd> &terCmds);

    //根据命令名获取命令的索引
    static TerminalCmd& getTerminalCmd(QString order,QList<TerminalCmd> &terCmds);

    //判断所有的终端命令中，是否只存在限定的几种命令
    static bool isOnlyHaveTheseOrder(QStringList orders,QList<TerminalCmd> &terCmds);

    // 终端命令解析
    //终端命令结构:  exe执行程序名 默认命令参数.... 命令1 命令1参数....  命令2 命令2参数.... 命令3 命令3参数....
    //示例:   ./023A_ASMC-V1_0 "文件1路径" "文件2路径" "文件3路径" ...... -o "输出文件路径"
    //      前3个是默认命令的参数[023A_ASMC-V1_0的默认命令为要编译的文件路径，.asm结尾的是要编译的汇编文件 .obj结尾的是要整合的二进制库文件]
    //      -o后面是指定一个输出到的.obj文件地址
    static QList<TerminalCmd> parsingTerminalCmd(QStringList cmds);
};







}}
QT_END_NAMESPACE




#endif // TERMINALPARSING_H
