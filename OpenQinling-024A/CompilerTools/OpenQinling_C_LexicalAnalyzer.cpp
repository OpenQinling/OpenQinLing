#include "OpenQinling_C_LexicalAnalyzer.h"
#include "OpenQinling_C_JudgestrtoolFunction.h"
#include "OpenQinling_DebugFunction.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{
struct SrcToken{
    QString tokenText;//源码词组文本
    int line;//词组所在的行
    int col;//词组所在的列
    QString srcPath;//词组所在的源码文件路径
};

//将源码/或预编译指令分割为token
static QList<SrcToken> getToken(SrcObjectInfo &srcObjInfo,
                         QString &expressionText,
                         const QString &srcPath,
                         bool &suceess,
                         bool isMacroExp){
    QList<SrcToken> tokens;
    int l = 1;
    int c = 1;
    SrcToken tmp;
    tmp.srcPath = srcPath;
    bool isHaveTmpToken = 0;
    //分割token
    while(expressionText.length()!=0){
        int del = judgeFastCharsIsDelimiter(expressionText);
        if(del==0){
            isHaveTmpToken = 1;
            tmp.tokenText.append(expressionText[0]);
            expressionText.remove(0,1);
        }else{
            if(isHaveTmpToken){
                isHaveTmpToken = 0;
                tokens.append(tmp);
                tmp.tokenText.clear();
            }
            tmp.tokenText = expressionText.left(del);
            tokens.append(tmp);
            tmp.tokenText.clear();
            expressionText.remove(0,del);
        }
    }
    if(isHaveTmpToken){
        tokens.append(tmp);
    }
    isHaveTmpToken = 0;

    //解析出token的行列
    for(int i = 0;i<tokens.length();i++){
        tokens[i].line = l;
        tokens[i].col = c;

        if(tokens[i].tokenText=="\n" || tokens[i].tokenText=="\r\n"){
            l += 1;
            c = 1;
        }else{
            c += tokens[i].tokenText.length();
        }

    }

    QList<SrcToken> replaceTokens;
    bool isHaveError = 0;

    //合并字符串和字符的token
    bool isStr = 0,isChar = 0;
    for(int i = 0;i<tokens.length();i++){
        if(isStr || isChar){
            tmp.tokenText += tokens[i].tokenText;
            if(isStr && tokens[i].tokenText=="\"" && tmp.tokenText.right(2)!="\\\""){
                replaceTokens.append(tmp);
                isStr = 0;
            }else if(isChar && tokens[i].tokenText=="\'" && tmp.tokenText.right(2)!="\\\'"){
                replaceTokens.append(tmp);
                isChar = 0;
            }else if(tokens[i].tokenText=="\r\n" || tokens[i].tokenText=="\n"){
                replaceTokens.append(tmp);
                isChar = 0;
                isStr = 0;
                isHaveError = 1;
                srcObjInfo.appendPrompt("字符串/字符没有结束符",tokens[i].srcPath,tokens[i].line,tokens[i].col);
            }
        }else{
            tmp = tokens[i];

            if(tokens[i].tokenText=='\"'){
                isStr = 1;
            }else if(tokens[i].tokenText=='\''){
                isChar = 1;
            }else{
                replaceTokens.append(tmp);
            }
        }
    }

    //去除注释
    tokens.clear();
    bool isInExpNote = 0,//多行注释
         isInLineExpNote = 0;//单行注释
    for(int i = 0;i<replaceTokens.length();i++){
        bool isRemoveToken = 0;
        if(isInExpNote || isInLineExpNote){
            if(isInExpNote && replaceTokens[i].tokenText=="*/"){
                isInExpNote = 0;
            }else if(isInLineExpNote && (replaceTokens[i].tokenText=="\r\n"||replaceTokens[i].tokenText=="\n")){
                isInLineExpNote = 0;
            }
            isRemoveToken = 1;
        }else if(replaceTokens[i].tokenText=="//"){
            isInLineExpNote = 1;
            isRemoveToken = 1;
        }else if(replaceTokens[i].tokenText=="/*"){
            isInExpNote = 1;
            isRemoveToken = 1;
        }

        if(isRemoveToken){
            replaceTokens.removeAt(i);
            i--;
        }
    }

    //合并预处理指令的token
    if(!isMacroExp){
        tokens = replaceTokens;
        replaceTokens.clear();
        //合并预编译指令的token
        bool isInPrecompile = 0;
        SrcToken precompileTmpToken;
        for(int i = 0;i<tokens.length();i++){
            if(!isInPrecompile){
                if(tokens[i].tokenText == "#"){
                    isInPrecompile = 1;
                    precompileTmpToken = tokens[i];
                }else{
                    replaceTokens.append(tokens[i]);
                }
            }else{
                if(tokens[i].tokenText == "\\"){
                    //判断是否是换行连接符
                    int tokenCount = 0;
                    bool isConnector = 0;
                    for(int j = i+1;j<tokens.length();j++){
                        if(tokens[j].tokenText == "\r\n" ||
                           tokens[j].tokenText == "\n"){
                            tokenCount++;
                            isConnector = 1;
                            break;
                        }else if(tokens[j].tokenText == " "||
                                 tokens[j].tokenText == "\t"){
                            tokenCount++;
                        }else{

                            break;
                        }
                    }

                    if(isConnector){
                        i += tokenCount;
                        continue;
                    }
                }else if(tokens[i].tokenText == "\r\n" ||
                         tokens[i].tokenText == "\n"){
                    isInPrecompile = 0;
                    replaceTokens.append(precompileTmpToken);
                    replaceTokens.append(tokens[i]);
                    precompileTmpToken = SrcToken();
                    continue;
                }
                precompileTmpToken.tokenText.append(tokens[i].tokenText);
            }
        }
        if(isInPrecompile){
            replaceTokens.append(precompileTmpToken);
        }
    }

    bool lastIsNum = 0;//上一个token是否是纯数字
    tokens.clear();
    //合并浮点型初始值的token(浮点初始值存在. 会因为被解析为符号而分割为2个token)
    //合并逻辑: .之前的一个token为全数字,或者.后面的一个token首字符是数字
    for(int i = 0;i<replaceTokens.length();i++){

        if(replaceTokens[i].tokenText=="."){//当前可能是个小数点,如果是就进行合并

            bool nextIsNum = 0;//下一个token的首字符是否是纯数字 或 e
            if(i+1!=replaceTokens.length()){
                QChar c = replaceTokens[i+1].tokenText[0];
                const static QString num = "1234567890eE";
                if(num.contains(c) || replaceTokens[i+1].tokenText.toUpper()=="F"){
                    nextIsNum = 1;
                }
            }

            if(lastIsNum){
                tmp.tokenText += ".";
            }else{
                tmp = replaceTokens[i];
                tmp.tokenText.prepend('0');
            }
            if(nextIsNum){
                tmp.tokenText += replaceTokens[i+1].tokenText;
                i++;
            }

            if(nextIsNum || lastIsNum){
                tokens.append(tmp);
                nextIsNum = 0;
                lastIsNum = 0;
            }else{
                tokens.append(replaceTokens[i]);
            }
        }else{
            bool isNum = 1;
            for(int k = 0;k<replaceTokens[i].tokenText.length();k++){
                if(!replaceTokens[i].tokenText[k].isNumber()){
                    isNum = 0;
                    break;
                }
            }

            if(isNum && i+1!=replaceTokens.length() && replaceTokens[i+1].tokenText=="."){
                tmp = replaceTokens[i];
                lastIsNum = 1;
            }else{
                tokens.append(replaceTokens[i]);
            }
        }
    }

    //合并科学计数法表示的初始值 例如221.21e231
    for(int i = 0;i<tokens.length();i++){
        QChar c = tokens[i].tokenText[0];
        const static QString num = "1234567890eE";
        if(i+2<tokens.length() &&
           num.contains(c) && tokens[i].tokenText.right(1).toUpper()=='E' &&
           (tokens[i+1].tokenText=='+'||tokens[i+1].tokenText=='-')){

            tokens[i].tokenText += tokens[i+1].tokenText + tokens[i+2].tokenText;

            tokens.removeAt(i+1);
            tokens.removeAt(i+1);
        }
    }

    replaceTokens.clear();


    //将数值初始值标准化
    //c语言支持各种进制的整型表示方法,全部将其转换为10进制的表示
    //c语言支持 33.23f 21.3e32f等表示方法，全部转为221.32的形式
    for(int i = 0;i<tokens.length();i++){
        QString txt = tokens[i].tokenText;
        const static QString num = "1234567890";
        if(!num.contains(txt[0])){
            continue;
        }

        txt = txt.toUpper();
        txt.remove('_');
        if(((txt.contains('F')||txt.contains('E'))&&!txt.contains('X')) ||txt.contains('.')){
            //是一个浮点数,标准化浮点格式
            bool isDouble = !txt.contains('F');
            txt.remove("F");
            bool su;
            txt = QString::number(txt.toDouble(&su));
            if(isDouble){
                txt.append("D");
            }else{
                txt.append("F");
            }

            isHaveError = isHaveError || !su;
            if(!su){
                srcObjInfo.appendPrompt("浮点数初始值定义有误",tokens[i].srcPath,tokens[i].line,tokens[i].col);
            }
        }else{
            int count_L = 0;
            int count_U = 0;
            while(txt.right(1)=='L' || txt.right(1)=='U'){
                if(txt.right(1)=='L'){
                    count_L++;
                }else if(txt.right(1)=='U'){
                    count_U++;
                }else{
                    break;
                }
                txt.remove(txt.length()-1,1);
            }


            //是一个整数,标准化整数格式
            int numSys = 10;
            if(txt.length()>2 && txt.left(2)=="0X"){
                numSys = 16;
                txt.remove(0,2);
            }else if(txt.length()>2 && txt.left(2)=="0B"){
                numSys = 2;
                txt.remove(0,2);
            }else if(txt.length()>1 && txt[0]=='0'){
                numSys = 8;
                txt.remove(0,1);
            }


            count_U = count_U || (numSys != 10);

            bool isUnsigned = count_U;
            bool isLongLong = count_L==2;
            bool su = 0;
            unsigned long long num = 0;
            if(!isLongLong && !isUnsigned){
                num = txt.toInt(&su,numSys);
            }else if(!isLongLong && isUnsigned){
                num = txt.toUInt(&su,numSys);
            }
            if(!su && !isUnsigned){
                isLongLong = 1;
                num = txt.toLongLong(&su,numSys);
            }
            if(!su){
                isUnsigned = 1;
                isLongLong = 1;
                num = txt.toULongLong(&su,numSys);
            }
            txt = QString::number(num);
            if(isUnsigned)txt += "U";
            if(isLongLong)txt += "L";
            else txt += "I";

            isHaveError = isHaveError || !su;
            if(!su || count_U >1 || count_L>2){
                txt = "0";
                srcObjInfo.appendPrompt("整数初始值定义有误",tokens[i].srcPath,tokens[i].line,tokens[i].col);
            }
        }
        tokens[i].tokenText = txt;
    }

    //将\t替换为空格
    for(int i = 0;i<tokens.length();i++){
        if(tokens[i].tokenText=="\t"){
            tokens[i].tokenText=" ";
        }
    }

    //去除空格、换行
    replaceTokens.clear();
    for(int i = 0;i<tokens.length();i++){
        if(tokens[i].tokenText=="\r" ||
           tokens[i].tokenText=="\n" ||
           tokens[i].tokenText=="\r\n"||
           tokens[i].tokenText==" "){
            continue;
        }else{
            replaceTokens.append(tokens[i]);
        }
    }


    //将多个连续的字符串token,合并为1个
    for(int i = 0;i<replaceTokens.length();i++){

        if(replaceTokens[i].tokenText[0]!='\"' && replaceTokens[i].tokenText.right(1)!='\"')continue;
        while(i+1<replaceTokens.length() &&
              replaceTokens[i+1].tokenText[0]=='\"' && replaceTokens[i+1].tokenText.right(1)=='\"'){
            replaceTokens[i].tokenText.remove(replaceTokens[i].tokenText.length()-1,1);
            replaceTokens[i].tokenText.append(replaceTokens[i+1].tokenText.mid(1,replaceTokens[i+1].tokenText.length()-1));
            replaceTokens.removeAt(i+1);
        }


        //去除掉字符串中\0后的文本
        QStringList tmpList = replaceTokens[i].tokenText.split("\\0");
        replaceTokens[i].tokenText = tmpList[0];
        if(tmpList.length()>=2){
            replaceTokens[i].tokenText.append('\"');
        }
    }


    suceess = !isHaveError;
    return replaceTokens;
}

//c宏定义词组列表解析
static Phrase analysisPrecompilePhrase(SrcObjectInfo &srcObjInfo,
                                       SrcToken precompileToken,
                                       bool &suceess){
    suceess = 0;
    Phrase tmp;
    tmp.line = precompileToken.line;
    tmp.col = precompileToken.col;
    tmp.srcPath = precompileToken.srcPath;



    QString precompileText = precompileToken.tokenText;
    precompileText.remove(0,1);
    if(precompileText.length()==0){
        suceess = 0;
        return Phrase();
    }

    QString precompileType;
    while(precompileText.length()!=0){
        if(precompileText[0] == ' ' ||
           precompileText[0] == '\"'||
           precompileText[0] == '<'||
           precompileText[0] == '>'||
           precompileText[0] == '#'){
            break;
        }
        precompileType.append(precompileText[0]);
        precompileText.remove(0,1);
    }

    tmp.text = precompileText;

    if(precompileType == "if"){
        tmp.PhraseType = tmp.PrecompiledOrder_if;
    }else if(precompileType == "elif"){
        tmp.PhraseType = tmp.PrecompiledOrder_elif;
    }else if(precompileType == "else"){
        tmp.PhraseType = tmp.PrecompiledOrder_else;
    }else if(precompileType == "endif"){
        tmp.PhraseType = tmp.PrecompiledOrder_endif;
    }else if(precompileType == "ifdef"){
        tmp.PhraseType = tmp.PrecompiledOrder_ifdef;
    }else if(precompileType == "undef"){
        tmp.PhraseType = tmp.PrecompiledOrder_undef;
    }else if(precompileType == "define"){
        tmp.PhraseType = tmp.PrecompiledOrder_define;
    }else if(precompileType == "ifndef"){
        tmp.PhraseType = tmp.PrecompiledOrder_ifndef;
    }else if(precompileType == "include"){
        tmp.PhraseType = tmp.PrecompiledOrder_include;
    }else if(precompileType == "error"){
        tmp.PhraseType = tmp.PrecompiledOrder_error;
    }else if(precompileType == "warning"){
        tmp.PhraseType = tmp.PrecompiledOrder_warning;
    }else if(precompileType == "pragma"){
        tmp.PhraseType = tmp.PrecompiledOrder_pragma;
    }else{
        srcObjInfo.appendPrompt("定义了当前编译器不支持的预编译指令",tmp.srcPath,tmp.line,tmp.col);
        return tmp;
    }

    suceess = 1;
    return tmp;
}
//c源码解析为词组列表
static PhraseList analysisPhraseList(SrcObjectInfo &srcObjInfo,
                              QList<SrcToken> &tokens,
                              bool&stutes,
                              bool isMacroExp){
    bool isHaveError = 0;
    PhraseList plist;
    while(tokens.length()!=0){
        Phrase tmp;
        tmp.line = tokens[0].line;
        tmp.col = tokens[0].col;
        tmp.srcPath = tokens[0].srcPath;
        JudgeDefConstValueType constType;//如果是常量值的情况下,常量值的类型
        if(tokens[0].tokenText[0] == '#' &&
           tokens[0].tokenText != "##" &&
           tokens[0].tokenText != "#@"&&
           !isMacroExp){//宏定义解析
            bool success;
            tmp = analysisPrecompilePhrase(srcObjInfo,tokens[0],success);
            isHaveError = isHaveError || !success;
            tokens.removeFirst();
        }else if(judgeIsKeyword(tokens[0].tokenText)){//检查是否是关键字
            tmp.text = tokens[0].tokenText;
            tmp.PhraseType = Phrase::KeyWord;
            tokens.removeAt(0);
        }else if(judgeIsDelimiter(tokens[0].tokenText)){//检查是否是运算符
            tmp.text = tokens[0].tokenText;
            tmp.PhraseType = Phrase::Delimiter;
            tokens.removeAt(0);
        }else if(judgeIsConst(tokens[0].tokenText,constType)){//检查是否是常量
            tmp.text = tokens[0].tokenText;
            if(constType==JudgeDefConstValueType_String){
                tmp.PhraseType = Phrase::ConstValue_STRING;
            }else if(constType==JudgeDefConstValueType_Char){
                tmp.PhraseType = Phrase::ConstValue_CHAR;
                tmp.text.removeFirst();
                tmp.text.removeLast();
            }else if(constType==JudgeDefConstValueType_Float){
                tmp.PhraseType = Phrase::ConstValue_FLOAT32;
            }else if(constType==JudgeDefConstValueType_Double){
                tmp.PhraseType = Phrase::ConstValue_FLOAT64;
            }else if(constType==JudgeDefConstValueType_Int){
                tmp.PhraseType = Phrase::ConstValue_INT32;
            }else if(constType==JudgeDefConstValueType_Uint){
                tmp.PhraseType = Phrase::ConstValue_UINT32;
            }else if(constType==JudgeDefConstValueType_Ulong){
                tmp.PhraseType = Phrase::ConstValue_UINT64;
            }else if(constType==JudgeDefConstValueType_Long){
                tmp.PhraseType = Phrase::ConstValue_INT64;
            }
            tokens.removeAt(0);
        }else{//标识符
            tmp.text = tokens[0].tokenText;
            tmp.PhraseType = Phrase::Identifier;
            tokens.removeAt(0);
        }
        plist.append(tmp);
    }

    stutes = !isHaveError;
    return plist;
};



//C语言词法分析器
PhraseList lexicalAnalyzer_C(SrcObjectInfo &srcObjInfo,
                             QString &srcPath,
                             QString &src,
                             bool *sucess,
                             bool isMacroExp){
    bool getTokenStutes;
    //将源码分割为token
    QList<SrcToken> tokens = getToken(srcObjInfo,src,srcPath,getTokenStutes,isMacroExp);
    bool analysisPhraseListStutes;
    //分析token的词汇类型为phrase(表达式词汇元)
    PhraseList srcExpList = analysisPhraseList(srcObjInfo,tokens,analysisPhraseListStutes,isMacroExp);
    if(sucess!=NULL){
        *sucess = analysisPhraseListStutes && getTokenStutes;
    }
    return srcExpList;
}

}



}
QT_END_NAMESPACE
