#include "OpenQinling_C_Precompile.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include "OpenQinling_C_LexicalAnalyzer.h"
#include "OpenQinling_DebugFunction.h"
#include "OpenQinling_C_JudgestrtoolFunction.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

//宏定义语法的token
struct DefineMacroToken{
    enum TokenType{
        TokenType_txt,//普通文本
        TokenType_Delimiter//符号
    }type;

    bool isDelimiter(QString delName){
        if(type == TokenType_Delimiter){
            return text==delName;
        }
        return 0;
    }
    QString text;
};

//去除文本的首尾空格
static QString removeSideSpace(QString str){
    //去除首尾空格
    while(str.length()!=0){
        if(str[0] == ' '||str[0] == '\t'){
            str.removeFirst();
        }else{
            break;
        }
    }

    while(str.length()!=0){
        int i = str.length()-1;
        if(str[i] == ' '||str[i] == '\t'){
            str.removeLast();
        }else{
            break;
        }
    }
    return str;
}

//导入头文件预编译指令
static bool analysisIncludePre(Phrase &thisPhrase,
                               PhraseList & headPhraseList,
                               SrcObjectInfo &objInfo){
    QString includePath = removeSideSpace(thisPhrase.text);


    if(includePath.length()<=2 ||
       (!(includePath[0]=='<' && includePath[includePath.length()-1]=='>') &&
       !(includePath[0] == '\"' && includePath[includePath.length()-1] == '\"'))){
        objInfo.appendPrompt("#include引用头文件格式不正确",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }

    bool isStdInclude = (includePath[0] == '<');
    includePath = includePath.mid(1,includePath.length()-2);

    QStringList searchDirs;//在哪些目录下搜索头文件
    if(isStdInclude){
        //在编译器标准头文件目录下搜索
        searchDirs.append(objInfo.stdIncludeDir);
        //在工作目录下搜索
        searchDirs.append("");
        //在外部指定目录搜索
        searchDirs.append(objInfo.includeDir);
    }else{
        //在工作目录下搜索
        searchDirs.append("");
        //在外部指定目录搜索
        searchDirs.append(objInfo.includeDir);
        //在编译器标准头文件目录下搜索
        searchDirs.append(objInfo.stdIncludeDir);
    }


    QFile headFile;
    QString headFilePath;

    bool isSearched = 0;
    foreach(QString dir,searchDirs){
        QFileInfo headFileInfo(QDir(dir),includePath);
        if(!headFileInfo.isFile()){
            continue;
        }
        headFilePath = headFileInfo.filePath();
        headFile.setFileName(headFilePath);
        isSearched = 1;
        break;
    }

    if(!isSearched){
        objInfo.appendPrompt("未能找到指定的头文件:"+includePath,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }else if(!headFile.open(QFile::ReadOnly)){
        objInfo.appendPrompt("头文件打开失败:"+includePath,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }


    QString fileSrcText = headFile.readAll();
    headFile.close();

    //调用词法分析器，将头文件源码转为token
    bool lexicalAnalysisSuceess;
    PhraseList srcExpList = lexicalAnalyzer_C(objInfo,
                                              headFilePath,
                                              fileSrcText,
                                              &lexicalAnalysisSuceess);

    if(!lexicalAnalysisSuceess){
        return 0;
    }

    //调用预编译器,解析预编译指令
    if(!analysisPrecompile(objInfo,srcExpList)){
        //预编译失败
        return 0;
    }

    //完成都文件的词法分析和预编译指令解析，将token返回
    headPhraseList = srcExpList;
    return 1;
}

//宏定义预编译指令
static bool analysisDefinePre(Phrase &thisPhrase,//当前宏的token
                              SrcObjectInfo &objInfo){
    QString defineExt = removeSideSpace(thisPhrase.text);
    QList<DefineMacroToken> tokens;
    bool isHaveTmp = 0;
    QString tmp;
    while(defineExt.length()!=0){
        int delCharcount;
        if(defineExt[0]==' ' || defineExt[0]=='\t'){
            if(isHaveTmp){
                DefineMacroToken token;
                token.type = token.TokenType_txt;
                token.text = tmp;
                tmp.clear();
                tokens.append(token);
                isHaveTmp = 0;
            }
            defineExt.remove(0,1);
            DefineMacroToken token;
            token.type = token.TokenType_Delimiter;
            token.text = " ";
            tokens.append(token);
        }else if((delCharcount = judgeFastCharsIsDelimiter(defineExt))){
            if(isHaveTmp){
                DefineMacroToken token;
                token.type = token.TokenType_txt;
                token.text = tmp;
                tmp.clear();
                tokens.append(token);
                isHaveTmp = 0;
            }
            DefineMacroToken token;
            token.type = token.TokenType_Delimiter;

            for(int i = 0;i<delCharcount;i++){
                token.text.append(defineExt[i]);
            }
            defineExt.remove(0,delCharcount);
            tokens.append(token);
        }else{
            isHaveTmp = 1;
            tmp.append(defineExt[0]);
            defineExt.remove(0,1);
        }
    }
    if(isHaveTmp){
        DefineMacroToken token;
        token.type = token.TokenType_txt;
        token.text = tmp;
        tmp.clear();
        tokens.append(token);
    }
    if(tokens.length() == 0 || tokens[0].type == DefineMacroToken::TokenType_Delimiter){
        objInfo.appendPrompt("宏定义语法有误",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }

    QString macroName = tokens[0].text;

    //判断当前宏定义是否已经被定义过了，如果是则报错退出
    if(objInfo.containsMacro(macroName)){
        objInfo.appendPrompt("重复定义了宏:"+macroName,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }

    tokens.removeFirst();
    QStringList macroArgs;
    bool isVariableArg = 0;
    if(tokens.length()>0 && tokens[0].isDelimiter(" ")){
        tokens.removeFirst();
    }else if(tokens.length()>0 && tokens[0].isDelimiter("(")){
        bool preIsArg = 0;
        tokens.removeFirst();
        while(tokens.length()!=0){
            if(preIsArg){//前一个token是参数
                if(tokens[0].isDelimiter(")")){
                    tokens.removeFirst();
                    break;
                }else if(!tokens[0].isDelimiter(",")){
                    objInfo.appendPrompt("宏定义参数语法有误,参数间必须用,分隔",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                    return 0;
                }

                preIsArg = 0;
                if(isVariableArg){
                    objInfo.appendPrompt("宏定义参数语法有误,在可变参数后不可在定义其它参数",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                    return 0;
                }
            }else if(!tokens[0].isDelimiter(" ")){//前一个token是符号
                if(tokens[0].isDelimiter("...")){
                    preIsArg = 1;
                    isVariableArg = true;
                }else if(tokens[0].type == DefineMacroToken::TokenType_Delimiter){
                    objInfo.appendPrompt("宏定义参数语法有误,参数名不能为空",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                    return 0;
                }else{
                    preIsArg = 1;
                    macroArgs.append(tokens[0].text);
                }
            }
            tokens.removeFirst();
        }
    }

    QStringList judgeIsHaveDup;
    //判断宏定义参数名称是否重复
    {
        for(int i = 0;i<macroArgs.length();i++){
            if(judgeIsHaveDup.contains(macroArgs[i])){
                objInfo.appendPrompt("宏定义参数语法有误,参数名称存在重复定义",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                return 0;
            }else{
                judgeIsHaveDup.append(macroArgs[i]);
            }
        }
    }

    DefineMacroInfo macroInfo;
    macroInfo.args = macroArgs;
    macroInfo.isVariableArg = isVariableArg;
    //将QList<DefineMacroToken>转为PhraseList
    QString txt;
    for(int i = 0;i<tokens.length();i++){
        txt += tokens[i].text;
    }
    bool success;
    macroInfo.value = lexicalAnalyzer_C(objInfo,thisPhrase.srcPath,txt,&success,1);
    objInfo.defineMacro(macroName,macroInfo);
    return 1;
}

//获取当前#if/#ifdef/#else到下一个#endif/#else的token数量(不包括#endif/#else以及当前的宏)
//-1表示失败。未找到当前流程块的结束符
static int getThisIfDefProcessBlock_tokenCount(PhraseList & allPhraseList,int beginIndex = 1){
    int level = 0;
    int count = 0;
    for(int i = beginIndex;i<allPhraseList.length();i++){
        Phrase &thisPhrase = allPhraseList[i];
        if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_if ||
           thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifndef||
           thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifdef){
            level+=1;
        }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_endif){
            if(level){
                level -= 1;
            }else{
                return count;
            }
        }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_else && level==0){
            return count;
        }
        count += 1;
    }
    return -1;
}

//解析#ifdef/#ifndef
static bool analysisIfdefinePre(PhraseList & allPhraseList,//从当前宏开始的所有token
                                SrcObjectInfo &objInfo){

    Phrase &thisPhrase = allPhraseList[0];
    bool isIfndef = thisPhrase.PhraseType == Phrase::PrecompiledOrder_ifndef;

    //当前流程块的判断是否为真
    bool isItTrue = objInfo.containsMacro(removeSideSpace(thisPhrase.text));
    if(isIfndef){
        isItTrue = !isItTrue;
    }
    int ifBlockCount = getThisIfDefProcessBlock_tokenCount(allPhraseList);//当前流程块内的token数
    if(ifBlockCount<0){
        objInfo.appendPrompt("宏定义判断流程块语法有误,未找到当前流程块的结束符",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return 0;
    }

    Phrase &ifblockEndPhrase = allPhraseList[ifBlockCount+1];//当前流程块的结束预编译指令token
    bool isHaveElseBlock = ifblockEndPhrase.PhraseType == Phrase::PrecompiledOrder_else;//是否还存在else块
    int elseifBlockCount = 0;

    if(isHaveElseBlock){
        elseifBlockCount = getThisIfDefProcessBlock_tokenCount(allPhraseList,ifBlockCount+2);//当前流程块内的token数
        if(elseifBlockCount<0){
            objInfo.appendPrompt("宏定义判断流程块语法有误,未找到当前流程块的结束符",ifblockEndPhrase.srcPath,ifblockEndPhrase.line,ifblockEndPhrase.col);
            return 0;
        }
        int endIndex = 2+ifBlockCount+elseifBlockCount;
        if(allPhraseList[endIndex].PhraseType != Phrase::PrecompiledOrder_endif){
            objInfo.appendPrompt("宏定义判断流程块语法有误,未找到当前流程块的结束符",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
            return 0;
        }
    }


    int elseBlockBeginIndex;
    //根据if流程块比较结果判断是否需要去除if流程块中的代码
    if(isItTrue){
        //只去除结束符
        allPhraseList.removeAt(ifBlockCount+1);
        elseBlockBeginIndex = ifBlockCount+1;
    }else{
        //去除所有
        int end = ifBlockCount+1;
        for(int i = 0;i<end;i++){
            allPhraseList.removeAt(1);
        }
        elseBlockBeginIndex = 1;
    }


    if(isHaveElseBlock){
        if(isItTrue){//去除所有
            int end = elseifBlockCount+1;
            for(int i = 0;i<end;i++){
                allPhraseList.removeAt(elseBlockBeginIndex);
            }
        }else{//只去除结束符
            allPhraseList.removeAt(elseifBlockCount+elseBlockBeginIndex);
        }
    }

    return 1;
}

//替换宏使用的参数,同时解析 #@ 和 #运算符。
//#@运算符，在替换时替换为 '宏传入参数'
//#运算符,在替换时替换为 "宏传入参数"
//一般情况下,就为直接进行替换
static void replaceDefineArgs(QString argName,PhraseList argValue,
                       PhraseList &retPhrs){
    PhraseList replase;
    for(int i = 0;i<retPhrs.length();i++){
        if((retPhrs[i].PhraseType == Phrase::KeyWord ||
           retPhrs[i].PhraseType == Phrase::Identifier)&&
           retPhrs[i].text == argName){
            if(replase.length()!=0 && replase[replase.length()-1].isDelimiter("#@")){//转为字符
                Phrase tmp;
                tmp.text = "\'"+argValue.toSrcText(0)+"\'";
                tmp.PhraseType = Phrase::ConstValue_CHAR;
                replase.removeLast();
                replase.append(tmp);
            }else if(replase.length()!=0 && replase[replase.length()-1].isDelimiter("#")){//转为字符串
                Phrase tmp;
                tmp.text = "\""+argValue.toSrcText(0)+"\"";
                tmp.PhraseType = Phrase::ConstValue_STRING;
                replase.removeLast();
                replase.append(tmp);
            }else{//普通替换操作
                replase.append(argValue);
            }
        }else{
            replase.append(retPhrs[i]);
        }
    }
    retPhrs = replase;
}

//解析##粘合指令
static void analysisAdhesionOrder(PhraseList &retPhrs){
    while(1){
        bool isHaveAdhesion = 0;
        for(int i = 0;i<retPhrs.length();i++){
            if(i+2 <retPhrs.length()){
                if((retPhrs[i].PhraseType == Phrase::Identifier ||
                   retPhrs[i].PhraseType == Phrase::KeyWord||
                    retPhrs[i].isConstValue())&&
                   (retPhrs[i+2].PhraseType == Phrase::Identifier ||
                   retPhrs[i+2].PhraseType == Phrase::KeyWord||
                   retPhrs[i+2].isConstValue())&&
                   retPhrs[i+1].isDelimiter("##")){
                    retPhrs[i].PhraseType = Phrase::Identifier;
                    retPhrs[i].text += retPhrs[i+2].text;
                    retPhrs.removeAt(i+1);
                    retPhrs.removeAt(i+1);
                    isHaveAdhesion = 1;
                }
            }
        }

        if(!isHaveAdhesion){
            break;
        }
    }

    for(int i = 0;i<retPhrs.length();i++){
        if(retPhrs[i].isDelimiter("##")){
            retPhrs.removeAt(i);
            i-=1;
        }
    }

}


static void setTokenSrcPath_line_col(PhraseList &retPhrs,QString path,int line,int col){
    for(int i = 0;i<retPhrs.length();i++){
        retPhrs[i].srcPath = path;
        retPhrs[i].line = line;
        retPhrs[i].col = col;
    }
}

//替换宏定义文本
static PhraseList analysisUseDefine(Phrase &thisPhrase,
                             QList<PhraseList> argsToken,//函数型宏定义的参数token
                             DefineMacroInfo macroInfo,
                             SrcObjectInfo &objInfo,
                             bool &success){
    success = 0;
    //如果是函数型宏定义，检查参数数量是否正确
    if(macroInfo.isVariableArg){
        success = argsToken.length()>=macroInfo.args.length();
    }else{
        success = macroInfo.args.length() == argsToken.length();
    }

    if(!success){
        objInfo.appendPrompt("宏定义替换传入的参数数量不正确",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        return PhraseList();
    }
    success = 0;

    success = 0;
    PhraseList retPhrs = macroInfo.value;

    //将宏定义传入的参数进行替换
    for(int i = 0;i<macroInfo.args.length();i++){
        replaceDefineArgs(macroInfo.args[i],
                          argsToken[0],
                          retPhrs);


        argsToken.removeFirst();
    }
    if(macroInfo.isVariableArg){
        //将末尾可变参数进行替换 __VA_ARGS__
        PhraseList vaArgsValue = PhraseList::joinFromDelimiter(",",argsToken);
        replaceDefineArgs("__VA_ARGS__",
                          vaArgsValue,
                          retPhrs);
    }

    //解析##指令,粘合2个标识符/关键字到一起
    analysisAdhesionOrder(retPhrs);
    //将替换出的代码token的行列值、所属文件，都设置为替换点的token
    setTokenSrcPath_line_col(retPhrs,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);

    success = 1;
    return retPhrs;
}

struct JudgeExpToken{
    enum{
        Error,//出错
        Number,//数值
        Operator,//运算符,
        Bracket,//括号
    }tokenType = Error;

    long long numberData;//数值型token的值
    QString operType;//运算符型token的类型
    QList<JudgeExpToken> subBracketTokens;//括号内的token

    bool isOperator(QString type){
        if(tokenType != Operator)return 0;
        return operType==type;
    }

    QString toString(){
        if(tokenType == Number){
            return QString::number(numberData);
        }else if(tokenType == Operator){
            return operType;
        }else if(tokenType == Bracket){
            QString txt;
            for(int i = 0;i<subBracketTokens.length();i++){
                txt += subBracketTokens[i].toString();
            }
            return "(" + txt + ")";
        }
        return "<ERROR>";
    }
};


static QList<JudgeExpToken> analysisJudgeExpTokens(SrcObjectInfo &objInfo,PhraseList &phraseList){
    QList<JudgeExpToken> expTokens;
    for(int i = 0;i<phraseList.length();i++){
        JudgeExpToken tmp;
        if(phraseList[i].isIdentifier() && phraseList[i].text == "defined" &&
           i+1<phraseList.length() && phraseList[i+1].isDelimiter("()")){
            PhraseList subPhr = phraseList[i+1].subPhraseList;
            if(subPhr.length()!=1)return QList<JudgeExpToken>();

            if(subPhr[0].PhraseType != Phrase::KeyWord &&
               subPhr[0].PhraseType != Phrase::Identifier)return QList<JudgeExpToken>();

            QString defName = subPhr[0].text;

            tmp.tokenType = tmp.Number;
            tmp.numberData = objInfo.containsMacro(defName);

            i += 1;
        }else if(phraseList[i].isDelimiter("()")){
            tmp.tokenType = tmp.Bracket;
            PhraseList subPhr = phraseList[i].subPhraseList;
            tmp.subBracketTokens = analysisJudgeExpTokens(objInfo,subPhr);
            if(tmp.subBracketTokens.length()==0){
                return QList<JudgeExpToken>();
            }
        }else if(phraseList[i].isConstValue()){
            tmp.tokenType = tmp.Number;
            if(phraseList[i].PhraseType == Phrase::ConstValue_FLOAT32 ||
               phraseList[i].PhraseType == Phrase::ConstValue_FLOAT64||
               phraseList[i].PhraseType == Phrase::ConstValue_STRING){
                return QList<JudgeExpToken>();
            }else if(phraseList[i].PhraseType == Phrase::ConstValue_CHAR){
                bool success;
                tmp.numberData = analysisCharConstValue(phraseList[i].text,success);
                if(!success){
                    return QList<JudgeExpToken>();
                }
            }else{
                tmp.numberData =  phraseList[i].text.toULongLong();
            }
        }else if(phraseList[i].PhraseType == Phrase::Delimiter){
            static const QStringList allDel = {
                "+","-","*","/","%",
                ">","<",">=","<=","==","!=",
                "&&","||","!",
                "&","|","^","~"
            };

            if(!allDel.contains(phraseList[i].text))return QList<JudgeExpToken>();
            tmp.tokenType = tmp.Operator;
            tmp.operType = phraseList[i].text;
        }else{
            return QList<JudgeExpToken>();
        }
        expTokens.append(tmp);
    }

    return expTokens;
}

static JudgeExpToken runJudgeExpTokens(QList<JudgeExpToken> operators){
    JudgeExpToken retValue;
    if(operators.length()==0)return retValue;

    //运算括号内表达式
    for(int i = 0;i<operators.length();i++){
        if(operators[i].tokenType == JudgeExpToken::Bracket){
            JudgeExpToken subValue = runJudgeExpTokens(operators[i].subBracketTokens);
            if(subValue.tokenType != JudgeExpToken::Number){
                return retValue;
            }

            operators[i] = subValue;
        }
    }

    JudgeExpToken zeroValue;
    zeroValue.tokenType = JudgeExpToken::Number;
    zeroValue.numberData = 0;
    //运算~ ! 正负号
    while(1){
        int operCount = 0;
        for(int i = operators.length()-1;i>=0;i--){
            if(operators[i].tokenType != JudgeExpToken::Number)continue;
            else if(i<=0)break;
            if(operators[i-1].isOperator("~")){
                operators[i].numberData = ~operators[i].numberData;
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("!")){
                operators[i].numberData = !operators[i].numberData;
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("+") &&
                     (i-1 == 0 || operators[i-2].tokenType != JudgeExpToken::Number)){
                operators[i-1] = operators[i];
            }else if(operators[i-1].isOperator("-") &&
                     (i-1 == 0 || operators[i-2].tokenType != JudgeExpToken::Number)){
                operators[i].numberData = zeroValue.numberData - operators[i].numberData;
                operators[i-1] = operators[i];
            }else{
                continue;
            }
            operators.removeAt(i);
            i--;
            operCount++;
        }
        if(operCount==0)break;
    }

    //运算*/%
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;



            if(operators[i+1].isOperator("*")){
                isOper = 1;
                tmp = operators[i].numberData*operators[i+2].numberData;
            }else if(operators[i+1].isOperator("/")){
                isOper = 1;
                tmp = operators[i].numberData/operators[i+2].numberData;
            }else if(operators[i+1].isOperator("%")){
                isOper = 1;
                tmp = operators[i].numberData%operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }

        if(operCount==0){
            break;
        }
    }

    //运算+-
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }
            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("+")){
                isOper = 1;
                tmp = operators[i].numberData+operators[i+2].numberData;
            }else if(operators[i+1].isOperator("-")){
                isOper = 1;
                tmp = operators[i].numberData-operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算<< >>
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("<<")){
                isOper = 1;
                tmp = operators[i].numberData<<operators[i+2].numberData;
            }else if(operators[i+1].isOperator(">>")){
                isOper = 1;
                tmp = operators[i].numberData>>operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算<= >= < >
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("<=")){
                isOper = 1;
                tmp = operators[i].numberData<=operators[i+2].numberData;
            }else if(operators[i+1].isOperator(">=")){
                isOper = 1;
                tmp = operators[i].numberData>=operators[i+2].numberData;
            }else if(operators[i+1].isOperator("<")){
                isOper = 1;
                tmp = operators[i].numberData<operators[i+2].numberData;
            }else if(operators[i+1].isOperator(">")){
                isOper = 1;
                tmp = operators[i].numberData>operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算== !=
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("==")){
                isOper = 1;
                tmp = operators[i].numberData==operators[i+2].numberData;
            }else if(operators[i+1].isOperator("!=")){
                isOper = 1;
                tmp = operators[i].numberData!=operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算&
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("&")){
                isOper = 1;
                tmp = operators[i].numberData&operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算^
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("^")){
                isOper = 1;
                tmp = operators[i].numberData^operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算|
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("|")){
                isOper = 1;
                tmp = operators[i].numberData|operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算&&
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("&&")){
                isOper = 1;
                tmp = operators[i].numberData&&operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    //运算||
    while(1){
        int operCount = 0;
        for(int i = 0;i<operators.length()-2;i++){
            if(operators[i].tokenType != JudgeExpToken::Number ||
               operators[i+2].tokenType != JudgeExpToken::Number){
                continue;
            }

            bool isOper = 0;
            long long tmp;

            if(operators[i+1].isOperator("||")){
                isOper = 1;
                tmp = operators[i].numberData||operators[i+2].numberData;
            }

            if(isOper){
                operators[i].numberData = tmp;
                operators.removeAt(i+1);
                operators.removeAt(i+1);
                operCount++;
            }
        }
        if(operCount==0){
            break;
        }
    }

    if(operators.length()!=1)return retValue;
    return operators[0];
}

//解析 #if #elif 中的判断语句(返回真假,如果解析出错默认就为假)
static bool analysisIf_Elif_JudgeExp(SrcObjectInfo &objInfo,QString expText){
    //解析为方便运算的token
    bool success;

    QString srcPath;
    PhraseList phraseList = lexicalAnalyzer_C(objInfo,srcPath,expText,&success,1);
    if(!success || phraseList.length()==0)return 0;

    PhraseList replaceList;
    //先检查语句中是否引用了其它的宏定义，如果有，先将所有宏定义都展开
    while(phraseList.length()!=0){
        Phrase &thisPhrase = phraseList[0];
        if(thisPhrase.isIdentifier() && thisPhrase.text == "defined" &&
           phraseList.length()>=4 && phraseList[1].isDelimiter("(")&&
           phraseList[3].isDelimiter(")")){//defined()运算符
            for(int i = 0;i<4;i++){
                replaceList.append(phraseList[0]);
                phraseList.removeFirst();
            }
        }else if((thisPhrase.isIdentifier() || thisPhrase.PhraseType ==Phrase::KeyWord)&&
                objInfo.containsMacro(thisPhrase.text)){//宏定义的使用
            Phrase thisPhraseInfo = thisPhrase;
            DefineMacroInfo macroInfo = objInfo.getMacroValue(thisPhraseInfo.text);

            phraseList.removeFirst();
            bool isFuncDefUse = phraseList.length()!=0 && phraseList[0].isDelimiter("(") &&
                                (macroInfo.args.length() != 0 || macroInfo.isVariableArg);


            QList<PhraseList> argsToken;
            if(isFuncDefUse){
                phraseList.removeFirst();
                //解析出调用函数型宏的传入参数
                int brackLevel = 1;//当前()的层级
                PhraseList thisArgTmp;
                while(phraseList.length()!=0){
                    if(phraseList[0].isDelimiter("(")){
                        brackLevel += 1;
                        thisArgTmp.append(phraseList[0]);
                    }else if(phraseList[0].isDelimiter(",") && brackLevel==1){
                        argsToken.append(thisArgTmp);
                        thisArgTmp.clear();
                    }else if(phraseList[0].isDelimiter(")")){
                        brackLevel -= 1;
                        if(brackLevel==0){
                            argsToken.append(thisArgTmp);
                            thisArgTmp.clear();
                            phraseList.removeFirst();
                            break;
                        }else{
                            thisArgTmp.append(phraseList[0]);
                        }
                    }else{
                        thisArgTmp.append(phraseList[0]);
                    }
                    phraseList.removeFirst();
                }

                if(brackLevel!=0){
                    return 0;
                }
            }


            bool success;
            PhraseList phr;

            //添加(

            Phrase brocketLeft;
            brocketLeft.PhraseType = Phrase::Delimiter;
            brocketLeft.text = "(";

            phr += brocketLeft;
            phr += analysisUseDefine(thisPhraseInfo,
                                     argsToken,
                                     macroInfo,
                                     objInfo,
                                     success);

            //添加)
            Phrase brocketRight;
            brocketRight.PhraseType = Phrase::Delimiter;
            brocketRight.text = ")";
            phr += brocketRight;


            if(!success){
                return 0;
            }else{


                phr.append(phraseList);
                phraseList = phr;
            }




        }else{
            replaceList.append(thisPhrase);
            phraseList.removeFirst();
        }
    }

    //将token转为方便后续数学运算的结构
    phraseList = replaceList.toTreeTokens();//数学运算适合于树形token
    replaceList.clear();



    QList<JudgeExpToken> expTokens = analysisJudgeExpTokens(objInfo,phraseList);
    if(expTokens.length() == 0)return 0;

    QString txt;
    for(int i = 0;i<expTokens.length();i++){
        txt += expTokens[i].toString();
    }


    JudgeExpToken result = runJudgeExpTokens(expTokens);

    if(result.tokenType != result.Number)return 0;


    return result.numberData;
}

struct IfProcessBlockInfo{
    enum{
       ProcessType_if,
       processType_else,
       processType_elif
    }processType;
    bool isExec;//该流程块是否有可能会被执行
    int beginIndex;//流程块起始索引号(#if/#else/#elif为开始)
    int nextBeginIndex;//下一个流程块起始的索引号
    int blockTokenCount;//流程块内token数量(不包括起始的预编译判断token)
};

//解析#if中的流程块信息
static IfProcessBlockInfo analysisIfProcessBlockInfo(PhraseList & allPhraseList,//从当前宏开始的所有token
                                                      int beginIndex,
                                                      SrcObjectInfo &objInfo,
                                                      bool&success){
    success = 0;
    Phrase &thisBlockBeginPhr = allPhraseList[beginIndex];
    IfProcessBlockInfo thisBlockInfo;
    thisBlockInfo.beginIndex = beginIndex;
    if(thisBlockBeginPhr.PhraseType == Phrase::PrecompiledOrder_if || thisBlockBeginPhr.PhraseType == Phrase::PrecompiledOrder_elif){
        thisBlockInfo.processType = thisBlockBeginPhr.PhraseType == Phrase::PrecompiledOrder_if ?
                                    IfProcessBlockInfo::ProcessType_if  :
                                    IfProcessBlockInfo::processType_elif;
        thisBlockInfo.isExec = analysisIf_Elif_JudgeExp(objInfo,thisBlockBeginPhr.text);
    }else if(thisBlockBeginPhr.PhraseType == Phrase::PrecompiledOrder_else){
        thisBlockInfo.processType = IfProcessBlockInfo::processType_else;
        thisBlockInfo.isExec = 1;
    }else{
        return IfProcessBlockInfo();
    }
    int level = 0;
    int count = 0;
    for(int i = beginIndex+1;i<allPhraseList.length();i++){
        Phrase &thisPhrase = allPhraseList[i];
        if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_if ||
           thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifndef||
           thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifdef){
            level+=1;
        }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_endif){
            if(level){
                level -= 1;
            }else{
                success = 1;
                break;
            }
        }else if((thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_else ||
                  thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_elif)&& level==0){
            success = 1;
            break;
        }
        count += 1;
    }
    thisBlockInfo.blockTokenCount = count;
    thisBlockInfo.nextBeginIndex = beginIndex + 1 + count;
    return thisBlockInfo;
}

//#if指令解析
static bool analysisIfPre(PhraseList & allPhraseList,//从当前宏开始的所有token
                          SrcObjectInfo &objInfo){
    Phrase thisPhrase = allPhraseList[0];
    //解析出#if中 各个 #if #elif #else #endif所构成的流程块的起始索引号、流程块内token数量
    //以及流程块是否需要执行
    QList<IfProcessBlockInfo> allProcessBlockInfo;
    int preBeginIndex = 0;
    for(int i = 0;i<allPhraseList.length();i++){
        bool success;
        IfProcessBlockInfo processInfo= analysisIfProcessBlockInfo(allPhraseList,
                                                    preBeginIndex,
                                                    objInfo,
                                                    success);
        if(!success){
            break;
        }
        preBeginIndex = processInfo.nextBeginIndex;
        allProcessBlockInfo.append(processInfo);
    }

    if(allProcessBlockInfo.length() == 0 ||
       allPhraseList[preBeginIndex].PhraseType != Phrase::PrecompiledOrder_endif){
        objInfo.appendPrompt("#if预编译指令解析失败,语法有误"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
    }else if(allProcessBlockInfo.length() >1 &&
        allProcessBlockInfo[allProcessBlockInfo.length()-1].processType == IfProcessBlockInfo::ProcessType_if){
         objInfo.appendPrompt("#if预编译指令解析失败,语法有误"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
     }

    for(int i = 1;i<allProcessBlockInfo.length()-1;i++){
        if(allProcessBlockInfo[i].processType == IfProcessBlockInfo::processType_else ||
           allProcessBlockInfo[i].processType == IfProcessBlockInfo::ProcessType_if){
            objInfo.appendPrompt("#if预编译指令解析失败,语法有误"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
        }
    }

    //判断流程块中哪一个流程块会最终被执行
    int execIndex = allProcessBlockInfo.length()-1;
    for(int i = 0;i<allProcessBlockInfo.length();i++){
        if(allProcessBlockInfo[i].isExec){
            execIndex = i;
            break;
        }
    }
    //去除掉除被执行流程块外的所有代码
    PhraseList execProTokens;
    if(allProcessBlockInfo[execIndex].blockTokenCount != 0){
        execProTokens += allPhraseList.mid(
                            allProcessBlockInfo[execIndex].beginIndex+1,
                            allProcessBlockInfo[execIndex].blockTokenCount);
    }
    execProTokens += allPhraseList.mid(
                        preBeginIndex+1
                        );

    allPhraseList = execProTokens;
    return 1;
}

//解析预编译指令
bool analysisPrecompile(SrcObjectInfo &objInfo,
                        PhraseList & phraseList){
    PhraseList phrases;
    bool isHaveError = 0;
    while(phraseList.length()!=0){
        Phrase &thisPhrase = phraseList[0];
        if(thisPhrase.isPrecompiledOrder()){
            if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_if){
                if(!analysisIfPre(phraseList,objInfo)){
                    isHaveError = 1;
                }
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifdef){
                if(!analysisIfdefinePre(phraseList,objInfo)){
                    isHaveError = 1;
                }
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_ifndef){
                if(!analysisIfdefinePre(phraseList,objInfo)){
                    isHaveError = 1;
                }
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_undef){
                objInfo.undefineMacro(removeSideSpace(thisPhrase.text));
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_define){
                if(!analysisDefinePre(thisPhrase,objInfo)){
                    isHaveError = 1;
                }
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_include){
                PhraseList headPhraseList;
                if(!analysisIncludePre(thisPhrase,headPhraseList,objInfo)){
                    isHaveError = 1;
                }else{
                    phrases.append(headPhraseList);
                }
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_error){
                objInfo.appendPrompt("#error:"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                isHaveError = 1;
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_warning){
                objInfo.appendPrompt("#warning:"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                phraseList.removeFirst();
            }else if(thisPhrase.PhraseType == thisPhrase.PrecompiledOrder_pragma){
                objInfo.appendPrompt("#pragma:当前编译器暂不支持任何#pragma预编译指令",thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                phraseList.removeFirst();
            }else{
                objInfo.appendPrompt("使用了不支持的预编译指令"+thisPhrase.text,thisPhrase.srcPath,thisPhrase.line,thisPhrase.col);
                isHaveError = 1;
                phraseList.removeFirst();
            }

        }else if((thisPhrase.isIdentifier() || thisPhrase.PhraseType ==Phrase::KeyWord)&&
                 objInfo.containsMacro(thisPhrase.text)){//当前是宏定义的使用，进行宏定义文本替换
            Phrase thisPhraseInfo = thisPhrase;
            DefineMacroInfo macroInfo = objInfo.getMacroValue(thisPhraseInfo.text);

            phraseList.removeFirst();
            bool isFuncDefUse = phraseList.length()!=0 && phraseList[0].isDelimiter("(") &&
                                (macroInfo.args.length() != 0 || macroInfo.isVariableArg);
            QList<PhraseList> argsToken;
            if(isFuncDefUse){
                phraseList.removeFirst();
                //解析出调用函数型宏的传入参数
                int brackLevel = 1;//当前()的层级
                PhraseList thisArgTmp;
                while(phraseList.length()!=0){
                    if(phraseList[0].isDelimiter("(")){
                        brackLevel += 1;
                        thisArgTmp.append(phraseList[0]);
                    }else if(phraseList[0].isDelimiter(",") && brackLevel==1){
                        argsToken.append(thisArgTmp);
                        thisArgTmp.clear();
                    }else if(phraseList[0].isDelimiter(")")){
                        brackLevel -= 1;
                        if(brackLevel==0){
                            argsToken.append(thisArgTmp);
                            thisArgTmp.clear();
                            phraseList.removeFirst();
                            break;
                        }else{
                            thisArgTmp.append(phraseList[0]);
                        }
                    }else{
                        thisArgTmp.append(phraseList[0]);
                    }
                    phraseList.removeFirst();
                }

                if(brackLevel!=0){
                    objInfo.appendPrompt("函数型宏定义的参数传递格式有误"+thisPhraseInfo.text,thisPhraseInfo.srcPath,thisPhraseInfo.line,thisPhraseInfo.col);
                    isHaveError = 1;
                    continue;
                }
            }
            bool success;
            PhraseList phr = analysisUseDefine(thisPhraseInfo,
                                               argsToken,
                                               macroInfo,
                                               objInfo,
                                               success);
            if(!success){
                isHaveError = 1;
            }else{
                phr.append(phraseList);
                phraseList = phr;
            }
        }else if(thisPhrase.isIdentifier() &&
                 thisPhrase.text == "__LINE__"){//__LINE__内置宏定义解析
            Phrase tmp;
            tmp.PhraseType = Phrase::ConstValue_INT32;
            tmp.text = QString::number(thisPhrase.line);
            phrases.append(tmp);
            phraseList.removeFirst();
        }else if(thisPhrase.isIdentifier() &&
                 thisPhrase.text == "__FILE__"){//__FILE__内置宏定义解析
            Phrase tmp;
            tmp.PhraseType = Phrase::ConstValue_STRING;
            tmp.text = "\""+createEscapeCharacter(thisPhrase.srcPath)+"\"";
            phrases.append(tmp);
            phraseList.removeFirst();
        }else{
            phrases.append(thisPhrase);
            phraseList.removeFirst();
        }
    }
    phraseList = phrases;
    return !isHaveError;
}
}}
QT_END_NAMESPACE
