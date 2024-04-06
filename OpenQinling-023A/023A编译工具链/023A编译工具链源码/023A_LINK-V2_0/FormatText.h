#ifndef FORMATTEXT_H
#define FORMATTEXT_H
#include <QStringList>
#include <QDebug>
#include <asmc_math.h>
#pragma execution_character_set("utf-8")
////////////对文本信息格式化函数////////////////////
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

//去除括号内的内容，转为......
QString removeParentInside(QString txt,uint type){
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
    for(uint i = 0;i<txt.length();i++){
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
    for(int i=0;i<txt.length();i++){
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
            retStr.append(QString(parent[0])+"......"+QString(parent[1]));
            strTmp.clear();
        }
    }
    return retStr;
    return txt;
}

//去除掉\t和多个空格以及首尾的空格
QString dislodgeSpace(QString txt){
    QString tmp;
    //去除\t
    foreach(QChar c,txt){
        if(c=="\t"){
            tmp.append(" ");
        }else{
            tmp.append(c);
        }
    }
    //将多个空格合并为1个空格
    QString ret;
    bool isSpace = 0;
    for(int i = 0;i<tmp.length();i++){
        QChar c = tmp.at(i);
        if(!isSpace && c==" "){
            ret.append(" ");

            isSpace = 1;
        }else if(c!=" "){
            ret.append(c);
            isSpace = 0;
        }
    }
    //去除首尾的空格
    while(ret.at(0)==" "){
        ret.remove(0,1);
    }
    while(ret.right(1)==" "){
        ret.remove(ret.length()-1,1);
    }
    return ret;
}


//去除文本换行,并将文本以;号进行分割为List [lineFeed返回每个list单元产生换行的次数 preposeLine每个单元第几行真正开始是代码]
QStringList segment(QString txt,QList<uint> *lineFeed,QList<uint> * startLine){
    if(lineFeed==NULL && startLine==NULL)return QStringList();
    QStringList list = txt.split(";");

    for(int i = 0;i<list.length();i++){
        int quantity = 0;
        for(int j = 0;j<list[i].length();j++){
            if(list[i][j]!='\n' && list[i][j]!='\r' && list[i][j]!=' '){
                break;
            }
            if(list[i][j]=='\n'){
                quantity++;
            }
        }
        startLine->append(quantity);


        quantity = 0;
        for(int j = 0;j<list[i].length();j++){
            if(list[i][j] == '\n'){
                quantity++;
            }
        }
        lineFeed->append(quantity);
        list[i].replace("\n"," ");
        list[i].remove('\r');
    }
    return list;
}

//文本整体tab右移[num为右移次数]
QString textRightShift(QString txt,int num){
    QString tmp;
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

//根据索引号分割字符串
QStringList splitStringFromIndex(QString str,QList<uint> index){
    //索引号去除重复值并从小到大排序
    index.append(0);
    index.append(str.length());
    index = uintSortMinToMax(index);
    index = uintDeduplication(index);
    if(index[index.length()-1]!=(uint)str.length()){
        return QStringList();
    }
    QStringList list;
    for(int i = 0;i<index.length()-1;i++){
        int s = index[i];
        int l = index[i+1] - index[i];
        list.append(str.mid(s,l));
    }
    return list;
}

//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name){
    //判断名称是否符合编译器自带的指令类型
    if(name=="" || name=="CODE" || name=="code"||
            name=="DATA" || name=="data" ||
            name=="IMPORT" || name=="import" ||
            name=="CODE-EXPORT" || name=="DATA-EXPORT"||
            name=="code-export" || name=="data-export"||
            name=="INT"||name=="int"||
            name=="UINT"||name=="uint"||
            name=="SHORT"||name=="short"||
            name=="USHORT"||name=="ushort"||
            name=="LONG"||name=="long"||
            name=="ULONG"||name=="ulong"||
            name=="FLOAT"||name=="float"||
            name=="DOUBLE"||name=="double"||
            name=="STR"||name=="str"||
            name=="STR(UTF8)"||name=="str(utf8)"||
            name=="ARR"||name=="arr"||
            name=="NULL"||name=="null"){
        //名称不得为空或关键字
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
    if(!(c>=65 && c<=90) && !(c>=97 && c<=122) && c!=95){
        return false;
    }
    return true;
}

//判断一个字符是否是在""引号扩起的字符串之中
bool charIsInString(QString txt,int index){
    QList<int> quoIndex;// "号的索引地址(去除\")
    for(int i = 0;i<txt.length();i++){
        if(i!=0&&txt.at(i)=='\"' && txt.at(i-1)!='\\'){
            quoIndex.append(i);
        }else if(i==0&&txt.at(i)=='\"'){
            quoIndex.append(i);
        }
    }
    if(quoIndex.length()%2==1){
        quoIndex.append(txt.length());
    }
    for(int i = 0;i<quoIndex.length();i+=2){
        if(index>quoIndex[i] && index<quoIndex[i+1]){
            return 1;
        }
    }
    return 0;
}

//获取一个字符是在该段文本的第几行
int getCharInStringLine(QString txt,int index){
    return txt.left(index).count('\n')+1;
}

//替换字符串中的转义字符
QByteArray replaceTransChar(QString strData){
    QByteArray arr = strData.toUtf8();
    QByteArray seplaceStr;
    for(int i=0;i<strData.length();i++){
        if(strData.at(i)!='\\'){
            seplaceStr.append(arr.at(i));
            continue;
        }

        if(i!=arr.length()-1){
            if(arr.at(i+1)=='a'){
                seplaceStr.append('\a');
                i++;
            }else if(arr.at(i+1)=='b'){
                seplaceStr.append('\b');
                i++;
            }else if(arr.at(i+1)=='f'){
                seplaceStr.append('\f');
                i++;
            }else if(arr.at(i+1)=='n'){
                seplaceStr.append('\n');
                i++;
            }else if(arr.at(i+1)=='r'){
                seplaceStr.append('\r');
                i++;
            }else if(arr.at(i+1)=='t'){
                seplaceStr.append('\t');
                i++;
            }else if(arr.at(i+1)=='v'){
                seplaceStr.append('\v');
                i++;
            }else if(arr.at(i+1)=='\\'){
                seplaceStr.append('\\');
                i++;
            }else if(arr.at(i+1)=='?'){
                seplaceStr.append('\?');
                i++;
            }else if(arr.at(i+1)=='\''){
                seplaceStr.append('\'');
                i++;
            }else if(arr.at(i+1)=='\"'){
                seplaceStr.append('\"');
                i++;
            }else if(arr.at(i+1)=='0'){
                seplaceStr.append('\0');
                i++;
            }else if(arr.at(i+1)=='x' && i<=arr.length()-3){
                QChar hex[2] = {arr.at(i+2),arr.at(i+3)};
                bool isHexSrceess;
                uchar d = QString(hex,2).toUInt(&isHexSrceess,16);
                if(isHexSrceess){
                    seplaceStr.append(d);
                    i+=3;
                }else{
                    seplaceStr.append(arr.at(i));
                }
            }
        }
    }
    return seplaceStr;
}

//判断该文本是否是描述的一个路径的字符串,是则返回路径内容
QString getPathInStr(QString txt,bool*status){
    if(status==NULL)return QString();
    if(txt[0]!='\"' || txt[txt.length()-1]!='\"'){
        *status = 0;
        return QString();
    }
    txt = replaceTransChar(txt.mid(1,txt.length()-2));
    *status = 1;
    return txt;
}

#endif // FORMATTEXT_H
