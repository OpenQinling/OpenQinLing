#ifndef OPENQINLING_COMPILECMDACTUATOR_H
#define OPENQINLING_COMPILECMDACTUATOR_H

#include <QStringList>
QT_BEGIN_NAMESPACE
namespace OpenQinling {

//编译器终端命令执行器
//返回值:          执行结果(0为正确,其它值出错)
//actuatorDir:   命令执行器程序文件所在的系统目录
//workDir:       命令执行器的工作目录(启动命令执行器时的系统目录)
//cmdArgs:       指令参数的文本
int compileCmdActuator(QString actuatorExeDir,
                       QStringList cmdArgs);

}
QT_END_NAMESPACE





#endif // OPENQINLING_COMPILECMDACTUATOR_H
