#ifndef STD_CODETXT_H
#define STD_CODETXT_H
#include <QString>
#include <QStringList>
#include <QDebug>
//去除注释[支持#]
QString removeExpNote(QString txt){
    QStringList list = txt.split('\n');//每行的文本
    QStringList out;
    foreach(QString line,list){
        QStringList d = line.split("#");
        if(d.length()>1){
            out.append(d.at(0)+"\n");
        }else{
            out.append(d.at(0)+"\n");
        }
    }
    return out.join("");
}

//获取字符串中某种字符所有非被引号包裹的字符地址
QList<uint> getNoQuoCharIndex(QString order,QChar ch){
    QList<uint> spaceIndex;//文本中所有空格的索引号
    QList<uint> quoIndex;//文本中双引号的索引地址(\"排除掉)
    for(int i = 0;i<order.length();i++){
        if(order.at(i)==ch){
            spaceIndex.append(i);
        }
        else if(order.at(i)=='\"'){
            if(i!=0){
                if(order.at(i-1)!='\\'){
                    quoIndex.append(i);
                }
            }else{
                quoIndex.append(i);
            }
        }
    }

    //判断出被引号所包裹的字符索引号
    /*
     * 判断标准:
     *   一个字符前面的引号数量为单数，并在其之后至少存在一个引号
     */
    QList<uint> quoSpaceIndex;//被引号包裹的空格的索引号
    foreach(uint space,spaceIndex){
        int last_number = 0;//在该空格前的引号数量
        int next_number = 0;//在该空格后的引号数量
        foreach(uint quo,quoIndex){
            if(quo<space){
                last_number++;
            }else{
                next_number++;
            }
        }
        if(last_number%2==1 && next_number>=1){
            quoSpaceIndex.append(space);
        }
    }

    //筛选出非被引号包裹的
    QList<uint> noQuoSpaceIndex;
    foreach(uint space,spaceIndex){
        int isRepeat = 0;
        foreach(uint quoSpace,quoSpaceIndex){
            isRepeat = isRepeat || (quoSpace==space);
        }
        if(!isRepeat){
            noQuoSpaceIndex.append(space);
        }
    }
    return noQuoSpaceIndex;
}

//去除字符串引号包裹外的字符
QString removeChar(QString order,QChar ch){
    QList<uint> noQuoSpaceIndex = getNoQuoCharIndex(order,ch);
    //字符串每减去一个字符，会让后面的空格索引位置都减1
    for(int i = 0;i<noQuoSpaceIndex.length();i++){
        order.remove(noQuoSpaceIndex.at(i)-i,1);
    }
    return order;
}

//替换被字符串引号包裹外的字符
QString replaceChar(QString order,QChar ch,QChar necChar){
    QList<uint> noQuoSpaceIndex = getNoQuoCharIndex(order,ch);
    //字符串每减去一个字符，会让后面的空格索引位置都减1
    for(int i = 0;i<noQuoSpaceIndex.length();i++){
        order.replace(noQuoSpaceIndex.at(i),1,necChar);
    }
    return order;
}

//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name){
    if(name=="" || name=="CODE" || name=="code"
       || name=="CODE" || name=="code" || name=="STACK" || name=="stack"){//名称不得为空或关键字
        return false;
    }



    foreach(QChar c,name){
        //如果name中有字符非 _ 且非 a-z A-Z 0-9的话就返回false
        if(c!=95 && !(c>=97 && c<=122) && !(c>=65 && c<=90) && !(c>=48 && c<=57)){
            return false;
        }
    }
    QChar c = name.at(0);
    //如果name的首字符非 a-z A-Z的话就返回false
    if(!(c>=65 && c<=90) && !(c>=97 && c<=122)){
        return false;
    }
    return true;
}
#endif // STD_CODETXT_H
