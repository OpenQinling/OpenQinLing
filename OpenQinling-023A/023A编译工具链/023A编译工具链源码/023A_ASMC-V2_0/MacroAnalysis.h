#ifndef MACROANALYSIS_H
#define MACROANALYSIS_H
#include <asmc_typedef.h>
#include <FormatText.h>
#include <TextFlag.h>
#include <QFile>
#include <QFileInfo>
#include <DebugTool.h>
#include "GrammarDetection.h"
#pragma execution_character_set("utf-8")
//判断一个文本中是否有@Import宏,如果有提取出来
QStringList getImportPath(QString txt,QStringList &strTXT,QString * filterTxt = NULL){
    //去除注释
    txt = removeExpNote(txt);
    //导入宏定义解析。导入宏是最特殊的宏，需要单独解析
    QStringList list = txt.split("\n");
    QStringList importPath;//当前文本中所有导入路径


    for(int i = 0;i<list.length();i++){
        QString str = list[i];

        str = dislodgeSpace(str);
        str = str.remove("\r");


        if(str.at(0)=="@"){//如果行首为@符表示就是宏定义的行
            QString order;//宏定义指令
            QStringList args;//宏定义参数
            //获取宏定义的指令与参数
            args = str.split(" ");

            order = args[0].mid(1,args[0].length()-1);
            args.removeFirst();
            if(order=="Import"){
                //读取导入宏指向的路径
                foreach(QString s,args){
                    QString stringText = getStringFromFlag(s,"$02",strTXT);
                    QFileInfo file(stringText.mid(1,stringText.length()-2));
                    importPath.append(file.absoluteFilePath());
                }
            }else if(filterTxt!=NULL){
                filterTxt->append(str+"\r\n");
            }
        }else if(filterTxt!=NULL){
            filterTxt->append(str+"\r\n");
        }

    }


    return importPath;
}

//递归查询当前工程所包含的所有文件路径
void queryAllImport(QStringList importPath,QStringList *strTXT,QStringList *queried,QStringList*queryError,QString *filterTxt=NULL){
    //importPath为当前文件所import的文件路径，所谓递归查询的根节点。queried为已经查询到的节点列表
    //对于已经排查过的节点，就不再深入进去排查，并且不重复记录。
    //queryError返回查询失败的路径
    if(queried==NULL){
        return;
    }
    foreach(QString path,importPath){
        QFile file(path);
        if(file.open(QFile::ReadOnly)){
            if(!queried->contains(path)){
                //如果能打开该路径指向的文件,且该路径还并未被查询过，就进入该路径节点下进行查询
                queried->append(path);//将该节点存入已查询集
                //从当前节点的文本中读取@Import的路径
                QString file_txt = file.readAll();
                bool isWinup;
                file_txt = extractString(file_txt,strTXT,&isWinup);
                QStringList thisNode = getImportPath(file_txt,*strTXT,filterTxt);
                file.close();
                queryAllImport(thisNode,strTXT,queried,queryError,filterTxt);
            }
        }else if(queryError!=NULL){
            queryError->append(path);
        }
    }
    return;
}

//文本中提取出所有的宏定义文本
QList<Macro> extractMacro(QString txt,QString path,QString*filterTxt,bool* isSrceess,QList<Prompt>* pormpts){
    if(filterTxt==NULL || isSrceess==NULL || pormpts==NULL){
        return QList<Macro>();
    }


    QStringList bank;
    bool isW;
    int startIndex;
    QString txt_t = extractString(txt,&bank,&isW,&startIndex);

    if(isW==0){
        *isSrceess = 0;
        *filterTxt = txt;

        Prompt prpt;
        prpt.isError = 1;
        prpt.content = "定义的字符串没有结尾的\"限制符";
        prpt.append(path,txt,getCharInStringLine(txt,startIndex),prpt.getStartline(0));
        pormpts->append(prpt);
        return QList<Macro>();
    }


    QStringList list = txt_t.split("\n");

    QStringList filter;
    QList<Macro> macroList;
    for(int j = 0;j<list.length();j++){
        QString tmp = list[j];
        tmp = tmp.append('\n');
        tmp = removeExpNote(tmp);
        tmp = tmp.remove('\n');
        tmp = tmp.remove('\r');
        tmp = dislodgeSpace(tmp);
        if(tmp.at(0)=="@"){

            Macro macro;
            QStringList args = tmp.split(" ");
            args.removeAll("");
            macro.line = j+1;
            macro.order = args.at(0).mid(1,args.at(0).length()-1);
            macro.args = args.mid(1,args.length()-1);
            for(int i = 0;i<macro.args.length();i++){
                macro.args[i] = reductionString(macro.args[i],"$01",bank);
            }
            macroList.append(macro);
            filter.append("\r");
        }else{
            filter.append(list[j]);
        }
    }
    *filterTxt = filter.join("\n");
    *filterTxt = reductionString(*filterTxt,"$01",bank);
    *isSrceess = 1;

    return macroList;
}


//解析宏定义,返回值是所有源文件的内容[注:仅包含path和asmSrcText的值]
QList<AsmSrc> analysisMacro(QString txt,QFileInfo path,bool* isSrceess,QStringList* promptStrs,QList<QFileInfo> *includedPath){
    if(isSrceess==NULL || promptStrs==NULL || includedPath==NULL)return QList<AsmSrc>();
    QString srcText = txt;
    QList<Prompt>pormpts;
    QList<AsmSrc> asmSrcList;

    //从源码中提取出宏定义。原宏定义地方用空格填充
    bool extractMacroSrceess;
    QList<Macro> macros = extractMacro(txt,path.absoluteFilePath(),&txt,&extractMacroSrceess,&pormpts);


    if(!extractMacroSrceess){
        //提取宏定义失败，报错退出
        for(int i = 0;i<pormpts.length();i++){
            QString pormptsText = grenratePromptText(pormpts[i]);
            promptStrs->append(pormptsText);
        }
        *isSrceess = 0;
        return QList<AsmSrc>();
    }
    AsmSrc src;
    src.path = path.absoluteFilePath();
    src.asmSrcText = txt;
    asmSrcList.append(src);

    bool macroIsScueess = true;//
    //判断宏定义类型和参数是否符合要求
    foreach(Macro macro,macros){
        if(macro.order=="INCLUDE"||macro.order=="include"){
            foreach(QString arg,macro.args){
                bool includeSuceess;
                QString tmp = getPathInStr(arg,&includeSuceess);
                //判断是否是一个描述路径的文本
                if(!includeSuceess){
                    //报错:include的非一个路径文本
                    Prompt prpt;
                    prpt.isError = 1;
                    prpt.content = "\""+tmp+"\" 引入的路径不合规";
                    prpt.append(path.absoluteFilePath(),txt,macro.line,macro.line);
                    QString pormptsText = grenratePromptText(prpt);
                    promptStrs->append(pormptsText);
                    macroIsScueess = 0;
                    continue;
                }

                bool isSerched = 0;//是否成功找到
                //tmp如果是相当路径,转换为绝对路径
                foreach(QString serchPath,includePaths){
                    QDir::setCurrent(serchPath);
                    QFileInfo info(tmp);
                    if(info.isFile()){
                        isSerched=1;
                        tmp = info.absoluteFilePath();
                        break;
                    }
                }
                QDir::setCurrent(thisProjectPath);
                if(!isSerched){
                    //报错:无法打开该路径
                    Prompt prpt;
                    prpt.isError = 1;
                    prpt.content = "\""+tmp+"\" 在所有头文件搜索路径中均未找到该头文件";
                    prpt.append(path.absoluteFilePath(),txt,macro.line,macro.line);
                    QString pormptsText = grenratePromptText(prpt);
                    promptStrs->append(pormptsText);
                    macroIsScueess = 0;
                    continue;
                }


                if(includedPath->contains(tmp)){
                    //警告:include已经引入过了
                    Prompt prpt;
                    prpt.isError = 0;
                    prpt.content = "\""+tmp+"\" 该路径已引入过了";
                    prpt.append(path.absoluteFilePath(),txt,macro.line,macro.line);
                    QString pormptsText = grenratePromptText(prpt);
                    promptStrs->append(pormptsText);
                    macroIsScueess = 1;
                    continue;
                }


                QFile file(tmp);
                if(file.open(QFile::ReadOnly)){
                    bool includeFileAnalysisSrceess;
                    QList<AsmSrc> asmSrcTmp = analysisMacro(file.readAll(),tmp,&includeFileAnalysisSrceess,promptStrs,includedPath);
                    if(includeFileAnalysisSrceess){
                        asmSrcList.append(asmSrcTmp);
                    }else{
                        macroIsScueess = 0;
                    }
                }else{
                    //报错:无法打开该路径
                    Prompt prpt;
                    prpt.isError = 1;
                    prpt.content = "\""+tmp+"\" 无法打开该路径指向的文件";
                    prpt.append(path.absoluteFilePath(),txt,macro.line,macro.line);
                    QString pormptsText = grenratePromptText(prpt);
                    promptStrs->append(pormptsText);

                    macroIsScueess = 0;
                    continue;
                }
            }
        }else{
            //未知宏定义
            Prompt prpt;
            prpt.isError = 1;
            prpt.content = "使用了不支持的宏定义";
            prpt.append(path.absoluteFilePath(),txt,macro.line,macro.line);
            QString pormptsText = grenratePromptText(prpt);
            promptStrs->append(pormptsText);
            macroIsScueess = 0;
        }
    }
    if(macroIsScueess==false){
        //解析失败了，退出函数
        *isSrceess = 0;
        return QList<AsmSrc>();
    }

    *isSrceess = 1;
    return asmSrcList;
}

#endif // MACROANALYSIS_H
