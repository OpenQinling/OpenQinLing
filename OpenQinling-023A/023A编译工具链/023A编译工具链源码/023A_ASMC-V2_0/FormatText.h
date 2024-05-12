#ifndef FORMATTEXT_H
#define FORMATTEXT_H
#include <QStringList>
#include <QDebug>
#include <asmc_typedef.h>
#include <asmc_math.h>
#include <TextFlag.h>
#include <transplant.h>
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


//格式化全局指令文本[指令文本结构:  指令操作符 参数1,参数2,参数n...{块指令文本内容};]
GlobalOrder formatOrderText(QString text,bool*status){
    if(status==NULL)return GlobalOrder();
    text = dislodgeSpace(text);

    //去除,号两旁空格
    QString tmp;
    for(int i = 0;i<text.length();i++){
        if(text[i]==','){
            tmp.append(text[i]);
            //如果是,号，该字符写入缓存。然后跳过后续的空格
            if(text[i+1]==' '){
                i+=1;
            }
        }else if(text[i]!=' '){
            //如果不是空格也不是,号，正常写入缓存
            tmp.append(text[i]);
        }else if(text[i+1]!=','){
            //如果是空格。判断下一个字符是否是,号。如果是，该字符不写入缓存
            tmp.append(text[i]);
        }
    }
    text = tmp;
    QStringList list = text.split(' ');

    list.removeAll("");
    if(list.length()!=2){
        if(list.length()!=2){
            *status = 0;
        }
        return GlobalOrder();
    }
    GlobalOrder order;
    order.order = list.at(0);
    QStringList args = list.at(1).split(",");
    for(int i = 0;i<args.length();i++){
        if(textIsFlag(args[i])){
            *status = 0;
            return order;
        }
    }

    //判断最后一个参数是否有子指令。如果有。判断是否在参数末尾。如果不在参数末尾报错
    QStringList l = separateFlag(args[args.length()-1]);

    for(int i = 0;i<l.length()-1;i++){
        if(textIsFlag(l[i])){
            *status = 0;
            return order;
        }
    }
    if(l.length()!=1){
        order.subOrder = l[l.length()-1];
        l.removeAt(l.length()-1);
        args[args.length()-1] = l.join("");
    }
    order.args = args;
    *status = 1;
    return order;
}

//判断名称是否符合要求[只能为大小写英文字母或数字/下划线。且首字符必须为字母]]
bool nameIsConform(QString name){
    //判断名称是否符合编译器自带的指令类型
    if(name=="" || name=="CODE"|| name=="code"          ||
            name=="DATA"       || name=="data"          ||
            name=="DATA-CONST" || name=="data-const"    ||
            name=="DATA-NOINIT"|| name=="data-noinit"   ||
            name=="IMPORT"     || name=="import"        ||
            name=="CODE-EXPORT"|| name=="code-export"   ||
            name=="DATA-EXPORT"|| name=="data-export"   ||
            name=="DATA-EXPORT-CONST" || name=="data-export-const"  ||
            name=="DATA-EXPORT-NOINIT"|| name=="data-export-noinit" ||
            name=="DATA-CONST-EXPORT" || name=="data-const-export"  ||
            name=="DATA-NOINIT-EXPORT"|| name=="data-noinit-export" ||
            name=="INT"         ||name=="int"       ||
            name=="UINT"        ||name=="uint"      ||
            name=="SHORT"       ||name=="short"     ||
            name=="USHORT"      ||name=="ushort"    ||
            name=="LONG"        ||name=="long"      ||
            name=="ULONG"       ||name=="ulong"     ||
            name=="FLOAT"       ||name=="float"     ||
            name=="DOUBLE"      ||name=="double"    ||
            name=="STR"         ||name=="str"       ||
            name=="STR(UTF8)"   ||name=="str(utf8)" ||
            name=="ARR"         ||name=="arr"       ||
            name=="NULL"        ||name=="null"){
        //名称不得为空或关键字
        return false;
    }
    //判断名称是否符合编译器移植时自定义的指令类型
    foreach(QString CustomName,asmCustomInstructions()){
        if(CustomName==name){
            return false;
        }
    }

    foreach(QChar c,name){
        //如果name中有字符非 _ 且非 * 且非 a-z A-Z 0-9的话就返回false
        if(c!=95 && c!=42 && !(c>=97 && c<=122) && !(c>=65 && c<=90) && !(c>=48 && c<=57)){
            return false;
        }
    }
    QChar c = name.at(0);
    //如果name的首字符非_ 非* 非 a-z A-Z的话就返回false
    if(!(c>=65 && c<=90) && !(c>=97 && c<=122) && c!=95 && c!=42){
        return false;
    }
    return true;
}
//格式化块指令文本[块指令文本结构:  标记1>标记2> 指令操作符 参数1,参数2,参数n...]
//status:[0正常 1标记名为空 2标记名称不合规范 3指令结构不正确
AsmBlockCode formatBlockOrderText(QString text,int*status){
    AsmBlockCode code;
    if(status==NULL)return AsmBlockCode();


    QStringList tmp;

    //先解析出标记
    tmp = text.split('>');
    if(tmp.length()>1){
        //说明存在定义了标记.进行标记的解析

        for(int i = 0;i<tmp.length()-1;i++){
            if(tmp[i]==" " || tmp[i]==""){
                *status = 1;
                return AsmBlockCode();
            }
            tmp[i] = dislodgeSpace(tmp[i]);
            if(!nameIsConform(tmp[i])){
                *status = 2;
                return AsmBlockCode();
            }else if(!code.marks.contains(tmp[i])){

                code.marks.append(tmp[i]);
            }
        }
        text = tmp.last();
    }else{
        //未定义标记
        text = tmp[0];
    }

    text = dislodgeSpace(text);
    tmp = text.split(' ');
    QString order = tmp[0];//指令
    if(tmp.length()>=2){
        tmp.removeFirst();
        tmp = tmp.join(' ').split(',');//参数
        for(int i = 0;i<tmp.length();i++){
            tmp[i] = dislodgeSpace(tmp[i]);
            if(tmp[i].contains(' ')){
                *status = 3;
                return AsmBlockCode();
            }
        }

    }else{
        tmp.clear();
    }
    code.args = tmp;
    code.orderName = order;
    *status = 0;
    return code;
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
    if(strData.length()<2)return QByteArray();
    strData = strData.mid(1,strData.length()-2);

    QByteArray arr = strData.toUtf8();
    QByteArray seplaceStr;
    for(int i=0;i<arr.length();i++){
        if(arr.at(i)!='\\'){
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
    seplaceStr.append((char)0);
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
