#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#pragma execution_character_set("utf-8")
#include "GenerateMemoryMallocInfo.h"
#include "middleCompile.h"
int main(int argc, char *argv[])
{
    QStringList args;
    for(int i = 1;i<argc;i++){
        args.append(QString::fromLocal8Bit(argv[i]));
    }
    if(args.length()>=2 && args.length()<=3){
        if(args.at(0)=="-c"){//编译低级汇编文本成.bin文件
            //打开汇编文本文件
            QFile asm_file(args.at(1));
            if(!asm_file.open(QIODevice::ReadWrite)){
                qDebug()<<"\033[31mError:not fint asm file!\033[0m";
                return -1;
            }
            QString asm_txt = asm_file.readAll();//读取汇编文本
            //编译为2进制指令
            int errorCode = 0;
            QString errorMessage;
            QStringList warn;
            QList<MemoryMallocInfo> memInfo = compileASM(asm_txt,args.at(1),&errorMessage,&errorCode,&warn);
            QByteArray bin = inteMemory(memInfo);
            if(errorCode!=0){
                qDebug()<<"\033[31mCompile Error:"<<errorMessage<<"!\033[0m";
                return errorCode;
            }
            if(warn.length()!=0){
                qDebug()<<"\033[33mCompile Warn:\033[0m";
                foreach(QString s,warn){
                    qDebug()<<"\033[33m"<<s.toUtf8().data()<<"\033[0m"    ;
                }
            }
            //将2进制数据写回文件
            QFile bin_file;
            if(args.length()==2){//如果没指定二进制机器码的输出文件，就在汇编文件目录下创建一个同名.bin文件，存储输出的二进制码
                QFileInfo asm_fileinfo(asm_file);
                bin_file.setFileName(asm_fileinfo.absolutePath()+"\\"+asm_fileinfo.baseName()+".bin");
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }else{
                bin_file.setFileName(args.at(2));
                if(bin_file.exists()){
                    bin_file.remove();
                }
                bin_file.open(QIODevice::ReadWrite);
                bin_file.write(bin);
            }

            qDebug()<<"\033[32mMemory Usage:\033[0m";
            for(int i = 0;i<3;i++){
                foreach(MemoryMallocInfo info,memInfo){
                    if(info.device==i){
                        qDebug()<<info.toString().toUtf8().data();
                    }
                }
            }
            qDebug()<<"\033[32mCompile complete!\033[0m";
        }
        else if(args.at(0)=="-t"){//将二进制数据转换为TB测试文本
            if(args.length()!=2){
                qDebug()<<"\033[31mError:parameter format error\033[0m";
                return -1;
            }
            QFile asm_file(args.at(1));
            if(!asm_file.open(QIODevice::ReadWrite)){
                qDebug()<<"\033[31mError:not fint asm file!\033[0m";
                return -1;
            }

            QByteArray bin = asm_file.readAll();//读取二进制数据

            QList<MemoryMallocInfo> memInfo = deconMemory(bin);
            QStringList tb_txt = generateTbText(memInfo);
            qDebug()<<"------------------";
            if(tb_txt.length()==1){
                qDebug()<<"Memory DATA:\n"<<tb_txt[0].toUtf8().data();
            }else{
                qDebug()<<"DATA:\n"<<tb_txt[0].toUtf8().data();
                qDebug()<<"------------------\nCODE:\n"<<tb_txt[1].toUtf8().data();
            }
            qDebug()<<"\033[32mGenerate tb txt complete!\033[0m";
        }
        else if(args.at(0)=="-coe"){
            if(args.length()!=3){
                qDebug()<<"\033[31mError:parameter format error\033[0m";
                return -1;
            }
            QFile asm_file(args.at(1));
            if(!asm_file.open(QIODevice::ReadWrite)){
                qDebug()<<"\033[31mError:not fint asm file!\033[0m";
                return -1;
            }

            QByteArray bin = asm_file.readAll();//读取二进制数据

            QList<MemoryMallocInfo> memInfo = deconMemory(bin);

            qDebug()<<"\033[32mMemory Usage:\033[0m";
            foreach(MemoryMallocInfo info,memInfo){
                qDebug()<<info.toString().toUtf8().data();
            }

            if(generateCoeFile(args.at(2),memInfo)){
                qDebug()<<"\033[32mGenerate coe file complete!\033[0m";
            }else{
                return -1;
            }
        }
        else{
            qDebug()<<"\033[31mError:parameter format error\033[0m";
            return -1;
        }
    }
    else{
        qDebug()<<"\033[31mError:parameter format error\033[0;0m";
        return -1;
    }
    return 0;
}



