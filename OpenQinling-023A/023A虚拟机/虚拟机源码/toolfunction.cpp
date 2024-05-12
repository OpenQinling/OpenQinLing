#include "toolfunction.h"

#include "systemdependfunction.h"
#include "mainwindow.h"
#include <QDialog>

QPixmap ToolFunction::crateIconMap(QString devType,QString devName,uint32_t beginAddress,uint32_t useMemLength,int useInterNum){
    QPixmap iconMap;
    //开始绘制图标pixmap
    QColor tspat(0,0,0,0);//透明
    QColor greyDegree(30,30,30,255);//深灰色
    QColor gree(50,255,50,255);//绿色
    QColor yellow(200,200,0,255);//黄色
    QColor grey(180,180,180,255);//灰色
    QColor black(0,0,0,255);//黑色
    QColor white(255,255,255,255);//白色
    uint endAddress = beginAddress+useMemLength-1;
    //使用的内存地址信息文本
    QString memAddText;
    if(devType!="CPU"){
        QString tmp = QString::number(beginAddress,16);
        while(tmp.length()<8){
            tmp.prepend("0");
        }
        tmp = tmp.right(8);
        memAddText.append(tmp+"-");
        tmp = QString::number(endAddress,16);
        while(tmp.length()<8){
            tmp.prepend("0");
        }
        tmp = tmp.right(8);
        memAddText.append(tmp);
    }

    QString interNumText;
    if(useInterNum!=-1 && useInterNum<=255){
        interNumText = QString::number(useInterNum);
        while(interNumText.length()<3){
            interNumText.prepend("0");
        }
        interNumText.prepend("INTER(");
        interNumText.append(")");
    }

    if(devType=="CPU"){
        iconMap = QPixmap(500,500);
        iconMap.fill(tspat);
        QPainter painter(&iconMap);
        QPen tmp(QColor(0,0,0,0));
        tmp.setWidth(0);
        painter.setPen(tmp);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        //绘制cpu底板
        painter.setBrush(greyDegree);
        painter.drawRoundedRect(0,0,500,500,10,10);



        //绘制cpu封装
        painter.setBrush(QColor(80,80,80,255));
        painter.drawRoundedRect(50,50,400,400,30,30);
        QLinearGradient grad(0,0,390,390);
        grad.setColorAt(0,QColor(200,200,200,255));
        grad.setColorAt(1,QColor(130,130,130,255));
        painter.setBrush(grad);
        painter.drawRoundedRect(60,60,380,380,25,25);


        QFont font;
        font.setPointSize(50);
        font.setWeight(200);
        painter.setFont(font);
        //绘制CPU封装文字
        painter.setPen(white);
        painter.drawText(161,251,"CORE");

        font.setPointSize(10);
        font.setWeight(200);
        painter.setFont(font);
        painter.drawText(170,280,"OpenQinling-023A");
        painter.drawText(170,300,"virtual CPU");

        //绘制正面标识符
        grad = QLinearGradient(0,0,10,10);
        grad.setColorAt(0,QColor(130,130,130,255));
        grad.setColorAt(1,QColor(150,150,150,255));
        painter.setPen(tmp);
        painter.setBrush(grad);
        painter.drawEllipse(QPoint(90,90),10,10);

        //绘制针脚
        painter.setBrush(yellow);
        painter.drawEllipse(QPoint(30,30),10,10);
        painter.drawEllipse(QPoint(30,470),10,10);
        painter.drawEllipse(QPoint(470,30),10,10);
        painter.drawEllipse(QPoint(470,470),10,10);
        painter.end();
    }else{
        iconMap = QPixmap(300,300);
        iconMap.fill(greyDegree);

        QPainter painter(&iconMap);
        QPen tmp(QColor(0,0,0,0));
        tmp.setWidth(0);
        painter.setPen(tmp);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        int textSize = 20;
        int textInterval = 0;
        int len = devName.length() * (textSize+textInterval);

        QFont font;
        font.setPointSize(textSize);
        font.setWeight(900);
        painter.setFont(font);
        //绘制虚拟芯片名称
        painter.setPen(white);
        painter.drawText(165-len/2,200-textSize/2,devName);

        //绘制虚拟芯片类型
        textSize = 40;
        textInterval = 0;
        len = devType.length() * (textSize+textInterval);
        font.setPointSize(textSize);
        painter.setFont(font);
        painter.drawText(160-len/2,160-textSize/2,devType);

        //绘制使用的内存地址信息
        font.setPointSize(15);
        painter.setFont(font);
        painter.drawText(40,280,memAddText);

        //绘制使用的中断号
        if(useInterNum!=-1){
            painter.drawText(90,240,interNumText);
        }

        //绘制正面标识符
        QLinearGradient grad = QLinearGradient(0,0,10,10);
        grad.setColorAt(0,QColor(130,130,130,255));
        grad.setColorAt(1,QColor(70,70,70,255));
        painter.setPen(tmp);
        painter.setBrush(grad);
        painter.drawEllipse(QPoint(20,20),10,10);
        painter.end();
    }
    return iconMap;
}

bool ToolFunction::getDevModInfo(QString packPath,QString coreLibPath,QString &typeName,QString &funInfo,uint32_t &memLen,bool &isUseInter){
    void *dllHandler = SystemDependFunction::LoadDynamicLibrary(packPath+"/"+coreLibPath);
    if(dllHandler==0){
        qDebug()<<"<出错,无法加载MOD的核心DLL>";
        return 0;
    }


    void(*ConfigModule)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8) = (void(*)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8))SystemDependFunction::GetDynamicLibraryFunction(dllHandler,"ConfigModule");
    if(ConfigModule!=0){
        qDebug()<<"启动MOD配置界面...";
        ConfigModule(packPath.toUtf8().data(),coreLibPath.toUtf8().data());
        qDebug()<<"MOD配置完成,尝试获取MOD信息...";
    }

    ModeInfo(*getDllInfoFun)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8) = (ModeInfo(*)(const char*packUnzipPath_utf8,const char*coreDllPath_utf8))SystemDependFunction::GetDynamicLibraryFunction(dllHandler,"GetModeRegisterTable");

    if(getDllInfoFun==0){
        qDebug()<<"<出错,找不到获取MOD注册表信息的接口函数>";
        SystemDependFunction::UnloadDynamicLibrary(dllHandler);
        return 0;
    }

    ModeInfo tmpMode = getDllInfoFun(packPath.toUtf8().data(),coreLibPath.toUtf8().data());
    typeName = tmpMode.typeName;
    funInfo = tmpMode.functionInfo;
    memLen = tmpMode.memLength;
    isUseInter = tmpMode.isHaveInterruptAsk;
    SystemDependFunction::UnloadDynamicLibrary(dllHandler);

    if(typeName=="" || typeName.toUpper()=="CPU" || typeName.length()>10){
        qDebug()<<"<出错,MOD的类型名称不合法>";
        return 0;
    }
    return 1;
}

QString ToolFunction::transHexString(uint32_t hexNum){
    QString tmp = QString::number(hexNum,16);
    while (tmp.length()<8) {
        tmp.prepend("0");
    }
    tmp.prepend("0x");
    return tmp;
}

//获取uint中[m:n]位置的数据
uint32_t ToolFunction::getBitsData(uint32_t d,int m,int n){
    if(m>31 || n>31 || m<0 || n<0 || m<n){
        return d;
    }
    uint32_t mask = ((1u <<(m-n+1))-1)<<n;
    return (d & mask)>>n;
}

//获取块重叠信息[返回发生重叠的块的编号]0为0/1是否重叠,1为1/2是否重叠.....注:需要先对块基址从小到大排序后才能用该函数
QList<bool> ToolFunction::getBlockOverlap(QList<BlockInfo> memSpace){
    if(memSpace.length()==0){
        return QList<bool>();
    }
    QList<bool> isOverlap;
    for(int i = 0;i<memSpace.length()-1;i++){
        if(memSpace[i].baseAdress+memSpace[i].sizeBytes>memSpace[i+1].baseAdress){
            isOverlap.append(1);
        }else{
            isOverlap.append(0);
        }
    }
    return isOverlap;
}

//整合重叠的区域,并从小到大排序
QList<BlockInfo> ToolFunction::integrationOverlapPlace(QList<BlockInfo> memSpace){
    if(memSpace.length()==0){
        return memSpace;
    }
    //先对块进行排序
    for(int i=0;i<memSpace.length();i++){
        for(int j=0;j<memSpace.length()-1-i;j++){
            if(memSpace[j].baseAdress>memSpace[j+1].baseAdress){
                BlockInfo tmp = memSpace[j];
                memSpace[j] = memSpace[j+1];
                memSpace[j+1] = tmp;
            }
        }
    }

    QList<BlockInfo> overlapPlace;
    //获取各相邻块是否有重叠
    QList<bool> overlap = getBlockOverlap(memSpace);

    //将length暂存块的结束地址
    for(int i=0;i<memSpace.length();i++){
        memSpace[i].sizeBytes += memSpace[i].baseAdress;
        memSpace[i].sizeBytes -= 1;
    }

    bool isInBlock = 0;
    BlockInfo info;
    for(int i=0;i<overlap.length();i++){
        if(!isInBlock){
            info.baseAdress = memSpace[i].baseAdress;
            isInBlock = true;
        }
        if(!overlap[i]){
            info.sizeBytes = memSpace[i].sizeBytes;
            overlapPlace.append(info);
            isInBlock = false;
        }
    }

    if(!isInBlock){
        info = memSpace[memSpace.length()-1];
        overlapPlace.append(info);
    }else{
        info.sizeBytes = memSpace[memSpace.length()-1].sizeBytes;
        overlapPlace.append(info);
    }
    for(int i = 0;i<overlapPlace.length();i++){
        overlapPlace[i].sizeBytes-=overlapPlace[i].baseAdress;
        overlapPlace[i].sizeBytes+=1;
    }
    return overlapPlace;
}

//使用多线程执行一个任务,所有线程都执行完该任务后函数退出(threadCount=0,默认使用cpu物理核心数相等的线程数)(stackSize=0 单个任务的堆栈大小,默认系统分配)
void ToolFunction::multithThreadExec(function<void(int threadIndex)>taskFun,int threadCount,uint32_t stackSize){
    if(taskFun==0)return;
    if(threadCount<=0){
        threadCount = QThread::idealThreadCount();
    }
    threadCount-=1;

    struct MultithThread:public QThread{

        int thisIndex = 0;
        function<void(int threadIndex)>taskFun;
        void run(){
            taskFun(thisIndex);
        }
    };

    QVector<MultithThread*> allThread(threadCount);
    for(int i = 0;i<threadCount;i++){
        allThread[i] = new MultithThread();
        allThread[i]->thisIndex = i;
        allThread[i]->taskFun = taskFun;
        allThread[i]->setStackSize(stackSize);
        allThread[i]->start();
    }

    taskFun(threadCount);

    bool isAllComplete = 0;
    while (!isAllComplete) {
        isAllComplete = 1;
        for(int i = 0;i<allThread.length();i++){
            if(!allThread[i]->isFinished()){
                isAllComplete = 0;
                break;
            }
        }
    }
    for(int i = 0;i<allThread.length();i++){
        delete allThread[i];
    }
    return;
}

//解压缩包到临时目录中
bool ToolFunction::unzipPackage(QString zipPath,QString&unzipPath,QString &coreLibPath){
    QFile file(zipPath);
    if(!file.exists())return 0;

    QZipReader read(zipPath);
    if(read.count()==0)return 0;

    QByteArray modInfoBin = read.fileData("ModInfo.json");
    QJsonDocument doc = QJsonDocument::fromJson(modInfoBin);

    if(!doc.isObject())return 0;
    if(!doc.object().contains("CoreDllPath"))return 0;

    QJsonValue value = doc.object().value("CoreDllPath");
    if(!value.isString())return 0;

    coreLibPath = value.toString();//lib在zip中的路径

    QDir tmpPack = QApplication::applicationDirPath()+"/Data/Tmp/TmpPack";
    unzipPath = tmpPack.path();
    tmpPack.removeRecursively();
    QDir tmpDir(QApplication::applicationDirPath()+"/Data/Tmp");
    tmpDir.mkdir("TmpPack");

    for(int i = 0;i<read.count();i++){
        if(read.entryInfoAt(i).isDir){
            tmpPack.mkpath(tmpPack.path()+"/"+read.entryInfoAt(i).filePath);
        }
    }

    bool isHaveCoreLib = 0;
    for(int i = 0;i<read.count();i++){
        if(read.entryInfoAt(i).isFile){
            if(read.entryInfoAt(i).filePath==coreLibPath){
                isHaveCoreLib = 1;
            }
            QFile file(tmpPack.path()+"/"+read.entryInfoAt(i).filePath);
            file.open(QFile::WriteOnly);
            file.write(read.fileData(read.entryInfoAt(i).filePath));
            file.close();
        }
    }

    if(!isHaveCoreLib){
        return 0;
    }
}


//判断块集A是否为块集B的超集[该算法需要先排除掉重叠块才能正常使用]
bool ToolFunction::judgeContainBlockSet(QList<BlockInfo> memSpaceA,
                          QList<BlockInfo> memSpaceB){
    if(memSpaceA.length()==0 && memSpaceB.length()==0){
        return 1;
    }else if(memSpaceA.length()==0 && memSpaceB.length()==1){
        return 0;
    }else if(memSpaceA.length()==1 && memSpaceB.length()==0){
        return 1;
    }


    //获取块集A/B中所有块的基址和尾址
    for(int i = 0;i<memSpaceB.length();i++){
        memSpaceB[i].sizeBytes = memSpaceB[i].sizeBytes+memSpaceB[i].baseAdress-1;
    }
    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].sizeBytes+memSpaceA[i].baseAdress-1;
    }



    //依次判断块集中的每个块，这些块的基址和尾址是否能落在A中的同一个块里
    for(int i = 0;i<memSpaceB.length();i++){

        //当前B中块的基址和尾址所落在的A块索引号，如果没落在A中任何块则为-1
        int baseIndex = -1;
        int endIndex = -1;

        for(int j = 0;j<memSpaceA.length();j++){
            if(memSpaceB[i].baseAdress>=memSpaceA[j].baseAdress && memSpaceB[i].baseAdress<=memSpaceA[j].sizeBytes){
                baseIndex = j;
            }
            if(memSpaceB[i].sizeBytes>=memSpaceA[j].baseAdress && memSpaceB[i].sizeBytes<=memSpaceA[j].sizeBytes){
                endIndex = j;
            }
        }

        //但凡发现B中有一个块的基址/尾址没能落在A中任何一个块中，或者落在的块不同则直接返回非超集
        if(baseIndex==-1 || endIndex==-1 || baseIndex!=endIndex){
            return 0;
        }
    }
    return 1;
}

//在块集A上减去块a
QList<BlockInfo> ToolFunction::removeOccupiedMemBlock(QList<BlockInfo> memSpaceA,
                                        BlockInfo mema){
    if(mema.sizeBytes==0){
        return memSpaceA;
    }

    for(int i = 0;i<memSpaceA.length();i++){
        if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){
            memSpaceA.removeAt(i);
            return memSpaceA;
        }
    }

    for(int i = 0;i<memSpaceA.length();i++){
        memSpaceA[i].sizeBytes = memSpaceA[i].baseAdress+memSpaceA[i].sizeBytes-1;
    }
    mema.sizeBytes = mema.baseAdress+mema.sizeBytes-1;


    QList<uint>retBase,retLen;
    QList<BlockInfo> ret;
    for(int i = 0;i<memSpaceA.length();i++){
        if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){



            break;
        }else if(mema.baseAdress==memSpaceA[i].baseAdress && mema.sizeBytes<memSpaceA[i].sizeBytes){
            //如果要去除的块对齐了剩余区块的头部,直接重设头部的索引=去除块的下一个单元
            retBase.append(mema.sizeBytes+1);
            retLen.append(memSpaceA[i].sizeBytes);
        }else if(mema.baseAdress>memSpaceA[i].baseAdress && mema.sizeBytes==memSpaceA[i].sizeBytes){
            //如果要去除的块对齐的剩余区块的尾部，直接重设尾部的索引=去除块的前一个单元
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(mema.baseAdress-1);
        }else if(mema.baseAdress>memSpaceA[i].baseAdress && mema.sizeBytes<memSpaceA[i].sizeBytes){
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(mema.baseAdress-1);
            retBase.append(mema.sizeBytes+1);
            retLen.append(memSpaceA[i].sizeBytes);
        }else{
            retBase.append(memSpaceA[i].baseAdress);
            retLen.append(memSpaceA[i].sizeBytes);
        }
    }


    for(int i = 0;i<retBase.length();i++){
        BlockInfo info;
        info.baseAdress = retBase[i];
        info.sizeBytes = retLen[i]-retBase[i]+1;
        ret.append(info);
    }
    return ret;
}

//在块集A上找到能放下块a的位置
BlockInfo ToolFunction::searchFreeMemarySpace(QList<BlockInfo> memSpaceA,uint mema_size,bool *isScueess){
     BlockInfo info;
    if(memSpaceA.length()==0){
        if(isScueess!=NULL){
            *isScueess = 0;
        }
        return info;
    }

    for(int i = 0;i<memSpaceA.length();i++){
        if(memSpaceA[i].sizeBytes >= mema_size){
            info.baseAdress = memSpaceA[i].baseAdress;
            info.sizeBytes = mema_size;
            if(isScueess!=NULL){
                *isScueess = 1;
            }
            return info;
        }
    }
    if(isScueess!=NULL){
        *isScueess = 0;
    }
    return info;
}


//复制目录以及目录下所有文件
bool ToolFunction::copyDir(const QString &srcPath, const QString &dstPath) {
    QDir srcDir(srcPath);
    QDir dstDir;
    if (!dstDir.exists(dstPath)) {
        dstDir.mkpath(dstPath);
    }

    QFileInfoList fileInfos = srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : fileInfos) {
        QString srcFilePath = fileInfo.absoluteFilePath();
        QString dstFilePath = dstPath + QDir::separator() + fileInfo.fileName();
        if (fileInfo.isFile()) {
            // 复制文件
            if (!QFile::copy(srcFilePath, dstFilePath)) {
                return false;
            }
        } else if (fileInfo.isDir()) {
            // 递归复制子目录
            if (!copyDir(srcFilePath, dstFilePath)) {
                return false;
            }
        }
    }
    return true;
}


//获取2个路径的公共路径
QString ToolFunction::comPath(QString aPath,QString bPath){
    QStringList aL =aPath.split("/");
    QStringList bL = bPath.split("/");

    QStringList comL;
    for(int i = 0;(i<aL.length() && i<bL.length());i++){
        if(aL[i]==bL[i]){
            comL.append(aL[i]);
        }else{
            break;
        }
    }
    return comL.join("/");
}
