#include "systemdependfunction.h"
#include <QDebug>
#if _WIN32
#include "qt_windows.h"
//加载一个动态链接库
void* SystemDependFunction::LoadDynamicLibrary(QString soPath){
    HINSTANCE handler = LoadLibraryA(soPath.toLocal8Bit().data());
    return handler;
}

//卸载一个动态链接库
void SystemDependFunction::UnloadDynamicLibrary(void* handler){
    FreeLibrary((HINSTANCE)handler);
}

//获取动态链接库中的函数指针
void* SystemDependFunction::GetDynamicLibraryFunction(void* handler,QString functionName){
    return (void*)GetProcAddress((HINSTANCE)handler,functionName.toLocal8Bit().data());
}

#elif __linux__

#include <dlfcn.h>
//加载一个动态链接库
void* SystemDependFunction::LoadDynamicLibrary(QString soPath){
    void* handler = dlopen(soPath.toLocal8Bit().data());
    return handler;
}
//卸载一个动态链接库
void SystemDependFunction::UnloadDynamicLibrary(void* handler){
    dlclose(handler);
}

//获取动态链接库中的函数指针
void* SystemDependFunction::GetDynamicLibraryFunction(void* handler,QString functionName){
    return (void*)dlsym(handler,functionName.toLocal8Bit().data());
}

#endif




