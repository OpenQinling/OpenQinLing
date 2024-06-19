#include "OpenQinling_ASM_TextFlag.h"
#include "OpenQinling_ASM_FormatText.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace ASM_Compiler{

//字符串提取器，并将原字符串的位置替换为标志号,标志号对应一个被提取的字符串，用于还原
//标志号的结构: $类型码 索引码 [类型为2位16进制数，索引号为5位16进制数]
//示例:   $01100a1  01是字符串的类型码，100a1对应字符串缓存库中第0x100a1位置的字符串
//isWindup是否去除成功了(如果不成功，说明存在一个字符串没有收尾) strStartp没有收尾的那个字符串定义的索引号
QString extractString(QString txt,QStringList* bank,bool*isWindup,int*strStartp){
    if(bank==NULL || isWindup==NULL)return"";
    //txt是要提取字符串的文本，bank是被提取字符串缓存库
    QList<uint> quoIndex;// "号的索引地址(去除\")
    for(int i = 0;i<txt.length();i++){
        if(i!=0&&txt.at(i)=='\"' && txt.at(i-1)!='\\'){
            quoIndex.append(i);
        }else if(i==0&&txt.at(i)=='\"'){
            quoIndex.append(i);
        }
    }
    if(quoIndex.length()==0){
        *isWindup =true;
        return txt;
    }

    if(quoIndex.length()%2==1){
        *isWindup =false;
        if(strStartp!=NULL){
            *strStartp = quoIndex[quoIndex.length()-1];
        }
        return"";
    }

    bool isInStr = 0;
    uint quoIndex_index = 0;

    QString strTmp;//缓存提取出的文本
    QString retStr;//保存已经经过过滤将字符串替换为标志号的文本
    //提取出文本
    if(bank!=NULL){
        for(uint i=0;i<(uint)txt.length();i++){

           bool isRetStr = 0;
            //如果索引号碰上了
            if((int)quoIndex_index!=quoIndex.length()){
                if(i==quoIndex.at(quoIndex_index)){
                    if(isInStr){
                        isRetStr = 1;
                    }else{
                        isInStr = 1;
                    }
                    quoIndex_index++;
                }
            }


            if(isInStr){
                strTmp.append(txt.at(i));
            }else{
                retStr.append(txt.at(i));
            }

            if(isRetStr){
                isInStr = 0;
                QString num = QString::number(bank->length(),16);
                while(num.length()!=5){
                    num = "0"+num;
                }
                retStr.append("$01"+num);
                bank->append(strTmp);
                strTmp.clear();
            }
        }
        *isWindup =true;
        return retStr;
    }



    *isWindup =true;
    return txt;
}

//括号提取器[type:括号类型  0() 1[] 2{} 3<>]
//$1type 5位索引码[示例 $1100a1 11是[]括号的类型码]
QString extractParent(QString txt,uint type,QStringList* bank){

    char parent[2];
    if(type==1){
        parent[0] = '[';
        parent[1] = ']';
    }else if(type==0){
        parent[0] = '(';
        parent[1] = ')';
    }else if(type==2){
        parent[0] = '{';
        parent[1] = '}';
    }else if(type==3){
        parent[0] = '<';
        parent[1] = '>';
    }else{
        return txt;
    }

    QList<uint> quoIndex;
    bool isInParent = 0;
    for(int i = 0;i<txt.length();i++){
        if(isInParent && txt[i]==parent[1]){
            quoIndex.append(i);
            isInParent = 0;
        }else if(txt[i]==parent[0]){
            quoIndex.append(i);
            isInParent = 1;
        }
    }

    if(quoIndex.length()==0){
        return txt;
    }

    bool isInStr = 0;
    uint quoIndex_index = 0;
    QString strTmp;//缓存提取出的文本
    QString retStr;//保存已经经过过滤将字符串替换为标志号的文本
    //提取出文本
    if(bank!=NULL){
        for(uint i=0;i<(uint)txt.length();i++){
           bool isRetStr = 0;
            //如果索引号碰上了
            if(i==quoIndex.at(quoIndex_index)){
                if(isInStr){
                    isRetStr = 1;
                }else{
                    isInStr = 1;
                }
                quoIndex_index++;
            }


            if(isInStr){
                strTmp.append(txt.at(i));
            }else{
                retStr.append(txt.at(i));
            }
            if(isRetStr){
                isInStr = 0;
                QString num = QString::number(bank->length(),16);
                while(num.length()!=5){
                    num = "0"+num;
                }
                retStr.append("$1"+QString::number(type)+num);
                bank->append(strTmp);
                strTmp.clear();
            }
        }
        return retStr;
    }
    return txt;
}

//获取一段字符串中，存在多少个指定类型的标志号
uint getFlagQuantity(QString text,QString typeCode){
    QString tmp;
    uint q = 0;
    for(int i = 0;i<text.length();i++){
        if(i<(text.length()-7) && text.mid(i,3)==typeCode){
            q++;
        }
    }
    return q;
}

//从标志号中提取出字符串
QString getStringFromFlag(QString flagTXT,QString typeCode,QStringList &str,bool*isSrceess){
    if(flagTXT.left(3)==typeCode){
        QString number = flagTXT.right(5);
        bool isSrc = 0;
        int p = number.toInt(&isSrc,16);
        if(isSrceess!=NULL){
            *isSrceess = true;
        }
        return str.at(p);
    }
    if(isSrceess!=NULL){
        *isSrceess = false;
    }
    return "";
}

//将一串文本中，所有标志号替换为原先的字符串
QString reductionString(QString text,QString typeCode,QStringList &str){
    QString tmp;
    for(int i = 0;i<text.length();i++){
        if(i<(text.length()-7) && text.mid(i,3)==typeCode){
            tmp+=getStringFromFlag(text.mid(i,8),typeCode,str);
            i+=7;
        }else{
            tmp+=text.at(i);
        }
    }
    return tmp;
}

//判断一段文本是否是一个标志号
bool textIsFlag(QString text){
    if(text.length()==8 && text[0]=='$'){
        bool isSrc;
        text.right(7).toUInt(&isSrc,16);
        return isSrc;
    }
    return false;
}


//从一段文本中分离出普通字符串和标志号(已$号进行分割)
QStringList separateFlag(QString text){
    QList<uint> list;
    for(int i = 0;i<text.length();i++){
        if(textIsFlag(text.mid(i,8))){
            if(list.length()==0){
                list.append(i);
            }else if(list[list.length()-1]!=(uint)i){
                list.append(i);
            }
            list.append(i+8);
            i+=7;
        }
    }
    return splitStringFromIndex(text,list);
}


}}
QT_END_NAMESPACE
