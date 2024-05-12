#ifndef SYSTEMDEPENDFUNCTION_H
#define SYSTEMDEPENDFUNCTION_H
#include <QString>
//系统依赖函数
namespace SystemDependFunction
{
    //加载一个动态链接库
    void* LoadDynamicLibrary(QString soPath);

    //卸载一个动态链接库
    void UnloadDynamicLibrary(void* handler);

    //获取动态链接库中的函数指针
    void* GetDynamicLibraryFunction(void* handler,QString functionName);
};

#endif // SYSTEMDEPENDFUNCTION_H
