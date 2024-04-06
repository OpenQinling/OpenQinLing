#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <systemdependfunction.h>
#include <QDialog>
#include <QPushButton>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>
#include <QtGui/private/qzipwriter_p.h>
#include <iostream>

QJsonDocument settingJsonDoc;
QByteArray innerFlashData;
QString appDataDir;
void exitHandle(){
    //将当前的外部设备使用信息写入设置 DeviceList 节点中,并卸载设备的DLL
    QJsonObject rootObj = settingJsonDoc.object();
    if(rootObj.contains("DeviceList")){
        rootObj.remove("DeviceList");
    }

    QJsonArray deviceListNode;
    for(int i = 2;i<mainWindow->allUseDeviceMode.length();i++){
        DeviceMode &mode = mainWindow->allUseDeviceMode[i];
        QJsonObject devNode;
        devNode.insert("devName",mode.devName);
        devNode.insert("beginAddress",ToolFunction::transHexString(mode.beginAddress));
        devNode.insert("useInterNum",mode.useInterNum);
        devNode.insert("devFolderName",mode.devFolderName);
        devNode.insert("coreDllPath",mode.coreDllPath);
        deviceListNode.append(devNode);

        if(mode.BlackoutDevice!=NULL){
            mode.BlackoutDevice();
        }

        SystemDependFunction::UnloadDynamicLibrary(mode.dllHandler);
    }
    rootObj.insert("DeviceList",deviceListNode);
    settingJsonDoc.setObject(rootObj);

    //将设置信息写入settings.json
    QFile settingFile(appDataDir+"settings.json");
    settingFile.open(QFile::ReadWrite);
    settingFile.resize(0);
    QByteArray jsonText = settingJsonDoc.toJson();
    settingFile.write(jsonText);
    settingFile.close();

    //将内部FLSH数据保存入InnerFlashData.bin
    QFile innerflashDataFile(appDataDir+"/InnerFlashData.bin");
    innerflashDataFile.open(QFile::ReadWrite);
    innerflashDataFile.seek(0);
    innerflashDataFile.write(innerFlashData);
    innerflashDataFile.close();

    //删除Tmp文件夹
    QDir tmpDir(appDataDir+"/Tmp");
    tmpDir.removeRecursively();
    QApplication::quit();
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if(a.arguments().length()==3 || a.arguments().length()==4){
        qDebug()<<"--------------打包虚拟设备MOD模式--------------";
        QString packPath = QDir(a.arguments()[1]).absolutePath();
        if(!QFileInfo(packPath).isDir()){
            QMessageBox::critical(NULL,"错误","未指定正确的打包目录");
            qDebug()<<"错误:未指定正确的打包目录";
            return 0;
        }
        QString cwPth = QDir::currentPath();
        QDir::setCurrent(packPath);


        QString dllPath = QDir(a.arguments()[2]).absolutePath();
        if(!QFileInfo(dllPath).isFile()){
            QMessageBox::critical(NULL,"错误","未指定正确的核心DLL文件");
            qDebug()<<"错误:未指定正确的核心DLL文件";
            return 0;
        }
        if(ToolFunction::comPath(packPath,dllPath)!=packPath){
            QMessageBox::critical(NULL,"错误","指定的核心DLL文件不在打包目录中");
            qDebug()<<"错误:指定的核心DLL文件不在打包目录中";
            return 0;
        }
        void* dllHandler = SystemDependFunction::LoadDynamicLibrary(dllPath);
        if(dllHandler==0){
            QMessageBox::critical(NULL,"错误",("无法打开指定的核心DLL文件(注意程序编译的位数,当前虚拟SOC为"+QString::number(sizeof(void*)*8)+"位)"));
            qDebug()<<("错误:无法打开指定的核心DLL文件(注意程序编译的位数,当前虚拟SOC为"+QString::number(sizeof(void*)*8)+"位)").toUtf8().data();
            return 0;
        }

        ModeInfo(*getDllInfoFun)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8) = (ModeInfo(*)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8))SystemDependFunction::GetDynamicLibraryFunction(dllHandler,"GetModeRegisterTable");

        if(getDllInfoFun==0){
            QMessageBox::critical(NULL,"错误","无法在核心DLL文件中找到GetModeRegisterTable函数,请检查有无定义,或者是否是C++编译的而无extern\"C\"声明");
            qDebug()<<"错误:无法在核心DLL文件中找到GetModeRegisterTable函数,请检查有无定义,或者是否是C++编译的而无extern\"C\"声明";
            return 0;
        }
        ModeInfo modeInfo = getDllInfoFun(packPath.toUtf8().data(),QDir(packPath).relativeFilePath(dllPath).toUtf8().data());
        getDllInfoFun = 0;

        QString typeName = modeInfo.typeName;

        if(typeName==""){
            QMessageBox::critical(NULL,"错误","设备名不得为空");
            qDebug()<<"错误:设备名不得为空";
            return 0;
        }else if(typeName.length()>10){
            QMessageBox::critical(NULL,"错误","设备类型名不得超过10个字符");
            qDebug()<<"错误:设备类型名不得超过10个字符";
            return 0;
        }else if(typeName.toUpper()=="CPU"){
            QMessageBox::critical(NULL,"错误","非法的设备类型");
            qDebug()<<"错误:非法的设备类型";
        }



        dllPath = QDir(packPath).relativeFilePath(dllPath);
        QString targePath;
        if(argc==4){
            QDir::setCurrent(cwPth);
            targePath = QDir(a.arguments()[3]).absolutePath();
        }else{
            targePath = packPath+".pck";
        }
        if(ToolFunction::comPath(packPath,targePath)==packPath){
            return 0;
        }


        QZipWriter zip(targePath);

        zip.addDirectory("Pack");

        QDir::setCurrent(packPath);

        QFileInfo dllFileInfo(dllPath);

        QFileInfoList allPackageFile = QDir(packPath).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);
        for(int i = 0;i<allPackageFile.length();i++){
            QString packagePath = "Pack/"+QDir(packPath).relativeFilePath(allPackageFile[i].absoluteFilePath());
            if(allPackageFile[i].isDir()){
                zip.addDirectory(packagePath);
            }else if(allPackageFile[i].isFile()){
                QFile file(allPackageFile[i].absoluteFilePath());

                if(allPackageFile[i]==dllFileInfo){
                    dllPath = packagePath;
                }
                if(file.open(QFile::ReadOnly)){
                    zip.addFile(packagePath,file.readAll());
                    file.close();
                }else{
                    qDebug()<<"警告:"+QDir(packPath).relativeFilePath(allPackageFile[i].absoluteFilePath())+"文件无法打开";
                }
            }
        }
        QJsonDocument doc;
        QJsonObject rootJsonObj;
        rootJsonObj.insert("CoreDllPath",dllPath);
        doc.setObject(rootJsonObj);
        zip.addFile("ModInfo.json",doc.toJson());
        zip.close();
        QMessageBox::information(NULL,"成功","已生成包:"+targePath);
        qDebug()<<"已生成包:"<<targePath.toUtf8().data();
        a.quit();
        return 0;
    }
    qDebug()<<"--------------运行虚拟SOC模式--------------";

    static QSharedMemory * singleApp = new QSharedMemory(a.applicationFilePath());
    if(!singleApp->create(1)){
        qDebug()<<"虚拟SOC不可重复启动,否则可能造成文件损坏";
        QMessageBox::information(NULL,"OpenQinling-VritualSoc","该应用已启动,不可重复启动,否则可能造成应用文件损坏");
        a.quit();
        return -1;
    }

    appDataDir = QApplication::applicationDirPath()+"/Data/";
    QDir dir(a.applicationDirPath());
    if(!dir.exists("Data")){
        dir.mkdir("Data");
    }

    if(!dir.exists("Data\\Library")){
        dir.mkdir("Data\\Library");
    }

    if(!dir.exists("Data\\Devices")){
        dir.mkdir("Data\\Devices");
    }

    if(!dir.exists("Data\\Tmp")){
        dir.mkdir("Data\\Tmp");
    }

    qDebug()<<"读取设置配置文件中...";

    QFile settingFile(appDataDir+"settings.json");
    settingFile.open(QFile::ReadWrite);
    settingJsonDoc = QJsonDocument::fromJson(settingFile.readAll());
    if(settingJsonDoc.isEmpty() || settingJsonDoc.isArray()){
        QJsonObject rootNodeObj;
        settingJsonDoc.setObject(rootNodeObj);
    }
    settingFile.close();

    qDebug()<<"加载内置FLASH数据中...";

    QString path = appDataDir+"InnerFlashData.bin";
    QFile innerflashDataFile(path);
    bool isInit = !innerflashDataFile.exists();
    innerflashDataFile.open(QFile::ReadWrite);
    if(isInit){
        innerFlashData = QByteArray(65536,0x00);
        innerflashDataFile.write(innerFlashData);
    }else{
        innerFlashData = innerflashDataFile.readAll();
    }
    innerflashDataFile.close();

    //QObject::connect(&a,&QCoreApplication::aboutToQuit,exitHandle);

    MainWindow w;
    w.show();

    //启动外部设备
    for(int i = 2;i<w.allUseDeviceMode.length();i++){
        w.allUseDeviceMode[i].EnergizeDevice();
    }

    if(a.arguments().length()==2){
        w.appendDevice(a.arguments()[1]);
    }


    return a.exec();
}
