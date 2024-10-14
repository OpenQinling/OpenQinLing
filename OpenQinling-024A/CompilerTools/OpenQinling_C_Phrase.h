#ifndef PHRASE_H
#define PHRASE_H
#include <QString>
#include <QList>
#include <QDebug>
#include "OpenQinling_FormatText.h"

QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

class PhraseList;
//将c源码文本分割为词组
class Phrase{
public:
    enum Phrase_Type{
        Error,
        KeyWord,//关键字(const/int等)
        Identifier,//标识符(函数/结构体/变量名/自定义数据类型名等)
        Delimiter,//符号

        //预编译指令
        PrecompiledOrder_include,
        PrecompiledOrder_define,
        PrecompiledOrder_undef,
        PrecompiledOrder_ifdef,
        PrecompiledOrder_ifndef,
        PrecompiledOrder_if,
        PrecompiledOrder_elif,
        PrecompiledOrder_else,
        PrecompiledOrder_endif,
        PrecompiledOrder_error,
        PrecompiledOrder_warning,
        PrecompiledOrder_pragma,


        //子表达式
        SubExp_BRACE,//{}
        SubExp_SQBR,//[]
        SubExp_PAR,//()

        //常量数据
        ConstValue_INT32,//整型
        ConstValue_UINT32,//整型
        ConstValue_INT64,//整型
        ConstValue_UINT64,//整型
        ConstValue_FLOAT32,//浮点型
        ConstValue_FLOAT64,//浮点型
        ConstValue_CHAR,//字符
        ConstValue_STRING//字符串
    }PhraseType = Error;


    Phrase();
    Phrase(Phrase_Type type,QString txt);

    //是否是宏定义指令
    bool isPrecompiledOrder(){
        QList<Phrase_Type> precompiledOrders = {
            PrecompiledOrder_include,
            PrecompiledOrder_define,
            PrecompiledOrder_undef,
            PrecompiledOrder_ifdef,
            PrecompiledOrder_ifndef,
            PrecompiledOrder_if,
            PrecompiledOrder_elif,
            PrecompiledOrder_else,
            PrecompiledOrder_endif,
            PrecompiledOrder_error,
            PrecompiledOrder_warning,
            PrecompiledOrder_pragma,
        };
        return precompiledOrders.contains(PhraseType);
    }

    //是否是指定的符号
    bool isDelimiter(QString del);
    //是否是指定的关键字
    bool isKeyWord(QString del);
    //是否是一个常量
    bool isConstValue();
    //是否是一个标识符
    bool isIdentifier();
    //获取标识符文本内容
    QString getIdentifierText();
    //文本整体tab右移[num为右移次数]
    QString textRightShift(QString txt,int num);
    QString toString(int level = 0);


    //括号中的词组(仅子表达式有效)
    QList<Phrase> subPhraseList;

    int line;//词组所在的行
    int col;//词组所在的列
    QString srcPath;//词组所在的源码文件路径

    //词组内文本(仅常量数据,关键字,标识符/符号/预编译指令时有效)
    QString text;//存储预编译指令的指令的参数文本

    //如果是()括号,彻底展开()括号(展开所有子层)
    //subType:填 "[]" "()" "{}"
    QList<Phrase> expandAllSubPhrase(QString subType){
        QList<Phrase> tmp;
        if(!(subType=="()" || subType=="[]" || subType=="{}"))return tmp;


        if(this->isDelimiter(subType)){
            QList<Phrase> subPhrase = this->subPhraseList;
            for(int i = 0;i<subPhrase.length();i++){
                if(subPhrase[i].isDelimiter(subType)){
                    tmp.append(subPhrase[i].expandAllSubPhrase(subType));
                }else{
                    tmp.append(subPhrase[i]);
                }
            }
            return tmp;
        }
        tmp.append(*this);
        return tmp;
    }

};
//词组列
class PhraseList : public QList<Phrase>{
public:
    PhraseList(QList<Phrase> &phs);
    PhraseList();
    //判断是否存在任意的关键字
    bool isHaveKeyWord();
    //获取一段词组中存在多少个A符号在B符号之前
    int getDelimiterNum(QString delA,QString delB);
    int getDelimiterNum(QString delA);
    //判断词组中仅为关键字与标识符
    bool isOnlyHaveKeyWordAndIdentifier();
    //如果词组中全部都为关键字与标识符,提取出关键字/标识符的文本内容
    QStringList getKeyWordAndIdentifierText();
    //获取一段词组中存在多少个关键词A在符号B之前(如果不需要B作为限制,B传""即可)
    int getKeyWordNum(QString delA,QString delB);
    int getKeyWordNum(QString delA);
    //已一个符号来分割成多段数组
    QList<PhraseList> splitFromDelimiter(QString del);
    //判断一段词组中,是否存在dels中定义的任意一个关键字
    bool judgeIsHaveAnyKeyWord(QStringList dels);
    //去除掉一段词组中所有指定的关键词
    void removeKeyWord(QString del);
    //去除掉一段词组中所有指定的符号
    void removeDelimiter(QString del);
    //截取最左侧的一段数组,已指定的符号为结尾(包含符号本身),并在被截取的词组上删去这段内容
    PhraseList getPhraseLeftSec(QString delText);

    //将多个PhraseList,用一个符号进行连接
    static PhraseList joinFromDelimiter(QString del,QList<PhraseList> &phraseList){
        Phrase joinTmp;
        joinTmp.PhraseType = joinTmp.Delimiter;
        joinTmp.text = del;
        PhraseList ret;
        for(int i = 0;i<phraseList.length()-1;i++){
            ret += phraseList[i];
            ret += joinTmp;
        }
        ret += phraseList[phraseList.length()-1];
        return ret;
    }


    //将扁平化的token和树型token的互转
    //扁平化token适合预编译器的解析，树型token适合c语言编译器的解析
    //扁平化token中[、]、(、)、{、}符号都是一个个普通的独立符号
    //树型token中[、]、(、)、{、}两两组合为一个整体, 例如{、}看做是一个整体为{}符,原先{、}内部的包括的token都存放在这个{}子节点符的子token列表中
    PhraseList toTreeTokens();

    PhraseList toFlatTokens(){

    }
    //获取当前是否是树形token
    bool getIsTreeTokens(){
        PhraseList &thisPhr = *this;
        for(int i = 0;i<thisPhr.length();i++){
            if(thisPhr[i].isDelimiter("()")||
               thisPhr[i].isDelimiter("{}")||
               thisPhr[i].isDelimiter("[]")){
                return 1;
            }
        }
        return 0;
    }



    //将PhraseList转为文本
    //isAutoFormat:是否自动进行文本的格式化
    QString toSrcText(bool isAutoFormat = 1);


};

}}
QT_END_NAMESPACE


#endif // PHRASE_H
