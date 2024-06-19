#include "OpenQinling_C_Phrase.h"
#include <QDebug>
#include "OpenQinling_DebugFunction.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{


Phrase::Phrase(){


}

Phrase::Phrase(Phrase_Type type,QString txt){
    PhraseType = type;
    text = txt;
}

//是否是指定的符号
bool Phrase::isDelimiter(QString del){
    if(del=="{}"){
        return PhraseType==SubExp_BRACE;
    }else if(del=="[]"){
        return PhraseType==SubExp_SQBR;
    }else if(del=="()"){
        return PhraseType==SubExp_PAR;
    }

    if(PhraseType!=Delimiter){
        return 0;
    }
    return text==del;
}
//是否是指定的关键字
bool Phrase::isKeyWord(QString del){
    if(PhraseType!=KeyWord){
        return 0;
    }
    return text==del;
}
//是否是一个常量
bool Phrase::isConstValue(){
    if(PhraseType==ConstValue_INT32
            ||PhraseType==ConstValue_INT64
            ||PhraseType==ConstValue_UINT32
            ||PhraseType==ConstValue_UINT64
            || PhraseType==ConstValue_FLOAT64
            || PhraseType==ConstValue_FLOAT32
            || PhraseType==ConstValue_CHAR
            || PhraseType==ConstValue_STRING){
        return 1;
    }
    return 0;
}
//是否是一个标识符
bool Phrase::isIdentifier(){
    return PhraseType==Identifier;
}
//获取标识符文本内容
QString Phrase::getIdentifierText(){
    if(PhraseType!=Identifier){
        return QString();
    }
    return text;
}

//文本整体tab右移[num为右移次数]
QString Phrase::textRightShift(QString txt,int num){
    QString tmp;
    for(int j = 0;j<num;j++){
        tmp.append('\t');
    }
    for(int i = 0;i<txt.length();i++){
        tmp.append(txt[i]);
        if(txt[i]=='\n'){
            for(int j = 0;j<num;j++){
                tmp.append('\t');
            }
        }
    }
    return tmp;
}

QString Phrase::toString(int level){
    QString tmp;
    if(isPrecompiledOrder()){
        QString type;
        if(PhraseType == PrecompiledOrder_if){
            type = "if";
        }else if(PhraseType == PrecompiledOrder_elif){
            type = "elif";
        }else if(PhraseType == PrecompiledOrder_else){
            type = "else";
        }else if(PhraseType == PrecompiledOrder_endif){
            type = "endif";
        }else if(PhraseType == PrecompiledOrder_ifdef){
            type = "ifdef";
        }else if(PhraseType == PrecompiledOrder_undef){
            type = "undef";
        }else if(PhraseType == PrecompiledOrder_define){
            type = "define";
        }else if(PhraseType == PrecompiledOrder_ifndef){
            type = "ifndef";
        }else if(PhraseType == PrecompiledOrder_include){
            type = "include";
        }else if(PhraseType == PrecompiledOrder_error){
            type = "error";
        }else if(PhraseType == PrecompiledOrder_warning){
            type = "warning";
        }else if(PhraseType == PrecompiledOrder_pragma){
            type = "pragma";
        }

        tmp = "<预编译指令(" +type+ "):["+text+"]>";
    }else if(PhraseType==KeyWord){
        tmp= "<关键字:"+text+">";
    }else if(PhraseType==ExtendKeyWord){
        tmp = "<扩展关键字:{\r\n";
        foreach(Phrase phr,subPhraseList){
            tmp += phr.toString(1);
            tmp+="\r\n";
        }
        tmp+="}>";
    }else if(PhraseType==Identifier){
        tmp= "<标识符:"+text+">";
    }else if(PhraseType==Delimiter){
        tmp= "<符号:["+text+"]>";
    }else if(PhraseType==ConstValue_INT32){
        tmp= "<整型INT32:"+text+">";
    }else if(PhraseType==ConstValue_INT64){
        tmp= "<整型INT64:"+text+">";
    }else if(PhraseType==ConstValue_UINT32){
        tmp= "<整型UINT32:"+text+">";
    }else if(PhraseType==ConstValue_UINT64){
        tmp= "<整型UINT64:"+text+">";
    }else if(PhraseType==ConstValue_FLOAT32){
        tmp= "<浮点FLOAT32:"+text+">";
    }else if(PhraseType==ConstValue_FLOAT64){
        tmp= "<浮点FLOAT64:"+text+">";
    }else if(PhraseType==ConstValue_CHAR){
        tmp= "<字符:"+text+">";
    }else if(PhraseType==ConstValue_STRING){
        tmp= "<字符串:"+text+">";
    }else if(PhraseType==SubExp_BRACE){
        tmp = "<花括号:{\r\n";
        foreach(Phrase phr,subPhraseList){
            tmp += phr.toString(1);
            tmp+="\r\n";
        }
        tmp+="}>";
    }else if(PhraseType==SubExp_SQBR){
        tmp = "<中括号:[\r\n";
        foreach(Phrase phr,subPhraseList){
            tmp += phr.toString(1);
            tmp+="\r\n";
        }
        tmp+="]>";
    }else if(PhraseType==SubExp_PAR){
        tmp = "<小括号:(\r\n";
        foreach(Phrase phr,subPhraseList){
            tmp += phr.toString(1);
            tmp+="\r\n";
        }
        tmp+=")>";
    }


    return textRightShift(tmp,level);
}


PhraseList::PhraseList(QList<Phrase> &phs){
    this->clear();
    for(int i = 0;i<phs.length();i++){
        this->append(phs[i]);
    }
}

PhraseList::PhraseList(){

}

//判断是否存在任意的关键字
bool PhraseList::isHaveKeyWord(){
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).PhraseType == Phrase::KeyWord){
            return 1;
        }
    }
    return 0;
}


//获取一段词组中存在多少个A符号在B符号之前
int PhraseList::getDelimiterNum(QString delA,QString delB){
    int num = 0;
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isDelimiter(delB)){
            break;
        }else if(this->operator[](i).isDelimiter(delA)){
            num++;
        }
    }
    return num;
}
int PhraseList::getDelimiterNum(QString delA){
    int num = 0;
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isDelimiter(delA)){
            num++;
        }
    }
    return num;
}

//判断词组中仅为关键字与标识符
bool PhraseList::isOnlyHaveKeyWordAndIdentifier(){
    for(int i = 0;i<this->length();i++){
        Phrase &tmp = this->operator[](i);
        if(tmp.PhraseType != Phrase::KeyWord && tmp.PhraseType!=Phrase::Identifier){
            return 0;
        }
    }
    return 1;
}

//如果词组中全部都为关键字与标识符,提取出关键字/标识符的文本内容
QStringList PhraseList::getKeyWordAndIdentifierText(){
    QStringList text;
    for(int i = 0;i<this->length();i++){
        Phrase &tmp = this->operator[](i);
        if(tmp.PhraseType != Phrase::KeyWord && tmp.PhraseType!=Phrase::Identifier){
            return QStringList();
        }
        text.append(tmp.text);
    }
    return text;
}

//获取一段词组中存在多少个关键词A在符号B之前(如果不需要B作为限制,B传""即可)
int PhraseList::getKeyWordNum(QString delA,QString delB){
    int num = 0;
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isDelimiter(delB)){
            break;
        }else if(this->operator[](i).isKeyWord(delA)){
            num++;
        }
    }
    return num;
}
int PhraseList::getKeyWordNum(QString delA){
    int num = 0;
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isKeyWord(delA)){
            num++;
        }
    }
    return num;
}

//已一个符号来分割成多段数组
QList<PhraseList> PhraseList::splitFromDelimiter(QString del){
     QList<PhraseList> list;

     PhraseList tmp;

     for(int i = 0;i<this->length();i++){
         if(this->operator[](i).isDelimiter(del)){
             list.append(tmp);
             tmp.clear();
         }else{
             tmp.append(this->operator[](i));
         }
     }
     if(tmp.length()!=0){
         list.append(tmp);
     }

     return list;
}

//判断一段词组中,是否存在dels中定义的任意一个关键字
bool PhraseList::judgeIsHaveAnyKeyWord(QStringList dels){
    for(int i = 0;i<this->length();i++){
        if(!(this->operator[](i).PhraseType==Phrase::KeyWord))continue;
        if(dels.contains(this->operator[](i).text)){
            return 1;
        }
    }

    return 0;
}

//去除掉一段词组中所有指定的关键词
void PhraseList::removeKeyWord(QString del){

    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isKeyWord(del)){
            this->removeAt(i);
            i--;
        }
    }
}

//去除掉一段词组中所有指定的符号
void PhraseList::removeDelimiter(QString del){
    for(int i = 0;i<this->length();i++){
        if(this->operator[](i).isDelimiter(del)){
            this->removeAt(i);
            i--;
        }
    }
}

//截取最左侧的一段数组,已指定的符号为结尾(包含符号本身),并在被截取的词组上删去这段内容
PhraseList PhraseList::getPhraseLeftSec(QString delText){
    PhraseList list;

    while(this->length()!=0){
        list.append(this->operator[](0));
        if(this->operator[](0).isDelimiter(delText)){
            this->removeFirst();
            break;
        }
        this->removeFirst();
    }

    return list;
}


//将扁平化的token和树型token的互转
//扁平化token适合预编译器的解析，树型token适合c语言编译器的解析
//扁平化token中[、]、(、)、{、}符号都是一个个普通的独立符号
//树型token中[、]、(、)、{、}两两组合为一个整体, 例如{、}看做是一个整体为{}符,原先{、}内部的包括的token都存放在这个{}子节点符的子token列表中
PhraseList PhraseList::toTreeTokens(){
    enum CurrentType{
        CurrentType_None,
        CurrentType_LetterBracket,//()
        CurrentType_MiddleBracket,//[]
        CurrentType_LargeBracket//{}
    }currentType = CurrentType_None;
    PhraseList &thisPhr = *this;
    int currentBracketLevel = 0;


    PhraseList retPhr;


    Phrase tmpPhr;
    for(int i = 0;i<thisPhr.length();i++){
        if(thisPhr[i].isDelimiter("(") && currentType==CurrentType_None){
            tmpPhr.PhraseType = Phrase::SubExp_PAR;
            tmpPhr.srcPath = thisPhr[i].srcPath;
            tmpPhr.line = thisPhr[i].line;
            tmpPhr.col = thisPhr[i].col;

            currentType = CurrentType_LetterBracket;
            currentBracketLevel = 1;
        }else if(thisPhr[i].isDelimiter("[") && currentType==CurrentType_None){
            tmpPhr.PhraseType = Phrase::SubExp_SQBR;
            tmpPhr.srcPath = thisPhr[i].srcPath;
            tmpPhr.line = thisPhr[i].line;
            tmpPhr.col = thisPhr[i].col;
            currentType = CurrentType_MiddleBracket;
            currentBracketLevel = 1;
        }else if(thisPhr[i].isDelimiter("{") && currentType==CurrentType_None){
            tmpPhr.PhraseType = Phrase::SubExp_BRACE;
            tmpPhr.srcPath = thisPhr[i].srcPath;
            tmpPhr.line = thisPhr[i].line;
            tmpPhr.col = thisPhr[i].col;
            currentType = CurrentType_LargeBracket;
            currentBracketLevel = 1;
        }else if((thisPhr[i].isDelimiter(")") && currentType==CurrentType_LetterBracket)||
                 (thisPhr[i].isDelimiter("]") && currentType==CurrentType_MiddleBracket)||
                 (thisPhr[i].isDelimiter("}") && currentType==CurrentType_LargeBracket)){
            if(currentBracketLevel == 1){
                PhraseList subPhr = tmpPhr.subPhraseList;
                tmpPhr.subPhraseList = subPhr.toTreeTokens();
                currentType = CurrentType_None;
                retPhr.append(tmpPhr);
                tmpPhr = Phrase();
            }else{
                tmpPhr.subPhraseList.append(thisPhr[i]);
                currentBracketLevel--;
            }
        }else if(currentType==CurrentType_None){
            retPhr.append(thisPhr[i]);
        }else{
            if((thisPhr[i].isDelimiter("(") && currentType==CurrentType_LetterBracket)||
               (thisPhr[i].isDelimiter("[") && currentType==CurrentType_MiddleBracket)||
               (thisPhr[i].isDelimiter("{") && currentType==CurrentType_LargeBracket)){
                currentBracketLevel++;
            }
            tmpPhr.subPhraseList.append(thisPhr[i]);
        }
    }

    if(currentType==CurrentType_LetterBracket){
        Phrase tmp;
        tmp.PhraseType = Phrase::Delimiter;
        tmp.text = "(";
        tmp.srcPath = tmpPhr.srcPath;
        tmp.line = tmpPhr.line;
        tmp.col = tmpPhr.col;
        retPhr.append(tmp);
        retPhr.append(tmpPhr.subPhraseList);
    }else if(currentType==CurrentType_LargeBracket){
        Phrase tmp;
        tmp.PhraseType = Phrase::Delimiter;
        tmp.text = "[";
        tmp.srcPath = tmpPhr.srcPath;
        tmp.line = tmpPhr.line;
        tmp.col = tmpPhr.col;
        retPhr.append(tmp);
        retPhr.append(tmpPhr.subPhraseList);
    }else if(currentType==CurrentType_MiddleBracket){
        Phrase tmp;
        tmp.PhraseType = Phrase::Delimiter;
        tmp.text = "{";
        tmp.srcPath = tmpPhr.srcPath;
        tmp.line = tmpPhr.line;
        tmp.col = tmpPhr.col;
        retPhr.append(tmp);
        retPhr.append(tmpPhr.subPhraseList);
    }
    return retPhr;
}


//将PhraseList转为文本
//isAutoFormat:是否自动进行文本的格式化
QString PhraseList::toSrcText(bool isAutoFormat){
    PhraseList thisPhrase;
    if(isAutoFormat){//如果要进行文本的格式化操作,必须先转为树型token,方便添加缩进/换行符
        thisPhrase = this->toTreeTokens();
    }else{
        thisPhrase = *this;
    }
    QString txt;
    bool preIsDel = 1;
    for(int i = 0;i<thisPhrase.length();i++){
        QString tmpText;
        if(thisPhrase[i].isDelimiter("()")){
            PhraseList tmp = thisPhrase[i].subPhraseList;
            tmpText +=("("+tmp.toSrcText()+")");
            preIsDel = 1;
        }else if(thisPhrase[i].isDelimiter("[]")){
            PhraseList tmp = thisPhrase[i].subPhraseList;
            tmpText +=("["+tmp.toSrcText()+"]");
            preIsDel = 1;
        }else if(thisPhrase[i].isDelimiter("{}")){
            PhraseList tmp = thisPhrase[i].subPhraseList;
            QString subText = textRightShift(tmp.toSrcText(),1);
            tmpText +=("{\r\n"+subText+"}\r\n");
            preIsDel = 1;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_INT32){
            tmpText = preIsDel ? "" : " ";
            tmpText += thisPhrase[i].text;
            preIsDel = 0;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_INT64){
            tmpText = preIsDel ? "" : " ";
            tmpText += thisPhrase[i].text+"ll";
            preIsDel = 0;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_UINT32){
            tmpText = preIsDel ? "" : " ";
            tmpText += thisPhrase[i].text+"u";
            preIsDel = 0;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_UINT64){
            tmpText = preIsDel ? "" : " ";
            tmpText += thisPhrase[i].text+"ull";
            preIsDel = 0;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_FLOAT32){
            tmpText = preIsDel ? "" : " ";
            tmpText += thisPhrase[i].text + "f";
            preIsDel = 0;
        }else if(thisPhrase[i].PhraseType == Phrase::ConstValue_FLOAT64){
            tmpText = preIsDel ? "" : " ";
            QString tmp = thisPhrase[i].text;
            if(!tmp.contains(".")){
                tmp += ".0";
            }
            tmpText += tmp;
            preIsDel = 0;
        }else if(thisPhrase[i].isDelimiter(";")){
            tmpText += thisPhrase[i].text+"\r\n";
            preIsDel = 1;
        }else{
            bool thisIsDel = thisPhrase[i].PhraseType == Phrase::Delimiter;
            if(thisIsDel){
                tmpText += thisPhrase[i].text;
            }else{
                tmpText = preIsDel ? "" : " ";
                tmpText += thisPhrase[i].text;
            }

            preIsDel = thisIsDel;
        }
        txt.append(tmpText);
    }
    if(!isAutoFormat){
        txt.remove("\r");
        txt.remove("\n");
        txt.remove("\t");
    }
    return txt;
}

}}
QT_END_NAMESPACE

