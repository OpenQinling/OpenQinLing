#include <QCoreApplication>
#include "OpenQinling_CompileCmdActuator.h"
//OpenQinling-024A编译工具链整合包
int main(int argc, char *argv[])
{
    //初始化QT,并获取必要的命令行参数信息
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();
    QString exeDir = QCoreApplication::applicationDirPath();
    //调用命令行指令执行器
    return OpenQinling::compileCmdActuator(exeDir,args);
}
