#ifndef MIDDLE_TYPEDEF_H
#define MIDDLE_TYPEDEF_H
#include <QStringList>
#include <FormatText.h>
#include <QDebug>
//中级语言节点
class MiddleNode;

//节点追加属性
class MiddleNode_Att{
public:
   QString attName;//追加属性名
   QList<MiddleNode> subNodes;//追加属性的子节点列表
};
class MiddleNode{
public:
    QString nodeType;//节点类型
    QStringList args;//节点必要参数
    QList<MiddleNode_Att> atts;//节点追加属性

    //将中级语言节点结构信息转为文本表示
    QString toString(bool isRoot=1){
        QString srcText = nodeType;
        if(args.length()!=0){
            srcText+="("+args.join(',')+")";
        }
        if(atts.length()==0){
            srcText = textRightShift(srcText,isRoot ? 0 : 1);
            srcText+=";\r\n";

            return srcText;
        }
        QString additionalArgText;
        foreach(MiddleNode_Att att,atts){
            additionalArgText+="."+att.attName;

            if(att.subNodes.length()==0){
                continue;
            }

            additionalArgText+="{\r\n";
            foreach(MiddleNode subNode,att.subNodes){
                additionalArgText+=subNode.toString(0);
            }
            additionalArgText+="}";
        }
        srcText += additionalArgText;
        srcText = textRightShift(srcText,isRoot ? 0 : 1);
        return srcText+";\r\n";
    }
    MiddleNode(){

    }
    //从文本描述中解析出中级语言节点结构信息,并将当前节点对象作为根节点
    //src文本描述  suceess返回是否解析成功 level递归所用,无需传入任何值
    MiddleNode(QString &src,bool &suceess,bool level=0){

        QStringList allStringDef;

        if(level==0){
            bool isWindup = 0;
            src = extractString(src,&allStringDef,&isWindup,NULL);
            if(isWindup==0){
                suceess = 0;
                return;
            }
        }


        src = removeExpNote(src);
        src = src.remove('\r');
        src = src.remove('\n');
        src = src.remove(' ');
        src = src.remove('\t');

        QString tmp;
        bool segmentSuccess;

        tmp = segmentStringThroughText(src,"(",{".",";"},segmentSuccess);

        if(segmentSuccess){
            nodeType = tmp;
        }else{
            tmp = segmentStringThroughText(src,".",{";"},segmentSuccess);
            if(segmentSuccess){
                nodeType = tmp;
                src.prepend('.');
                goto analysisAdditionalArgs;//跳过解析Args，直接开始解析analysisAdditionalArgs
            }else{
                tmp = segmentStringThroughText(src,";",{},segmentSuccess);
                if(segmentSuccess){
                    nodeType = tmp;
                    goto retHandle;
                }else{
                    //没有;号
                    suceess = 0;
                    return;
                }
            }
        }
        tmp = segmentStringThroughText(src,")",{";","{"},segmentSuccess);
        if(segmentSuccess){
            if(tmp!=""){
                args = tmp.split(',');
            }
        }else{
            //没有)号
            suceess = 0;
            return;
        }



    analysisAdditionalArgs:
        while(src[0]!=';'){
            tmp = segmentStringThroughText(src,".",{"{",";"},segmentSuccess);
            if(!segmentSuccess){
                //没有.号
                suceess = 0;
                return;
            }

            MiddleNode_Att att_Tmp;
            tmp = segmentStringThroughText(src,"{",{";","."},segmentSuccess);
            if(segmentSuccess){
                att_Tmp.attName = tmp.toUpper();
                tmp = extractParentContent(src,segmentSuccess);
                if(!segmentSuccess){
                    //找不到括号结尾}
                    suceess = 0;
                    return;
                }
                while(tmp.length()!=0 && tmp!=';'){
                    MiddleNode subNode(tmp,segmentSuccess,level+1);
                    if(!segmentSuccess){
                        //子节点解析失败
                        suceess = 0;
                        return;
                    }
                    att_Tmp.subNodes.append(subNode);
                }
                atts.append(att_Tmp);
            }else{
                tmp = segmentStringThroughText(src,".",{"{",";"},segmentSuccess);
                if(segmentSuccess){
                    att_Tmp.attName = tmp.toUpper();
                    src.prepend('.');
                    atts.append(att_Tmp);
                }else {
                    tmp = segmentStringThroughText(src,";",{},segmentSuccess);
                    if(segmentSuccess){
                        att_Tmp.attName = tmp.toUpper();
                        atts.append(att_Tmp);
                        src.prepend(';');
                    }else{
                        //没有分号
                        suceess = 0;
                        return;
                    }
                }
            }

        }
        src.remove(0,1);
    retHandle:
        nodeType = nodeType.toUpper();
        suceess = 1;


        if(level==0){
            replaceNodeStr(*this,allStringDef);
        }


        return;
    }

    //判断当前节点是否定义了除attNams外的其它属性
    bool areThereOnlyTheseAtt(QStringList attNams){
        for(int i = 0;i<atts.length();i++){
            if(!attNams.contains(atts[i].attName)){
                return true;
            }
        }
        return false;
    }

    //根据属性名,获取对应属性的对象指针(如果不存在返回NULL)
    MiddleNode_Att* getAttFromName(QString name){
        for(int i = 0;i<atts.length();i++){
            if(name==atts[i].attName){
                return &atts[i];
            }
        }
        return NULL;
    }



private:
    //文本整体tab右移[num为右移次数]
    QString textRightShift(QString txt,int num){
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

    //已一段文本对字符串进行分割为2段
    //前段通过函数返回，后段通过src返回 suceess返回是否分割成功(如果不成功，说明src在stopTexts前不存在指定的text)
    QString segmentStringThroughText(QString &src,QString text,QStringList stopTexts,bool &suceess){
        QStringList list;
        QString ret;
        list = src.split(text);

        foreach(QString stopText,stopTexts){
            if(list[0].contains(stopText)){
                suceess = 0;
                return QString();
            }
        }

        if(list.length()==1){
            suceess = 0;
            return QString();
        }



        suceess = 1;
        ret = list[0];
        list.removeFirst();
        src = list.join(text);
        return ret;
    }

    //提取出最近的大括号内的文本(带提取的文本不包括{首部)
    //suceess返回是否成功(如果不成功,说明找不到括号的结尾)
    QString extractParentContent(QString &src,bool &suceess){
        int level = 1;
        suceess = 0;
        QString parentContent;
        QString notParentContent;
        for(int i = 0;i<src.length();i++){
            if(!suceess){
                if(src[i]=='{')level++;
                else if(src[i]=='}')level--;

                if(level==0){
                    suceess = 1;
                    continue;
                }
                parentContent.append(src[i]);
            }else{
                notParentContent.append(src[i]);
            }
        }


        if(suceess){
            src = notParentContent;
            return parentContent;
        }
        return QString();
    }

    //字符串提取器，并将原字符串的位置替换为标志号,标志号对应一个被提取的字符串，用于还原
    //标志号的结构: $类型码 索引码 [类型为2位16进制数，索引号为5位16进制数]
    //示例:   $01100a1  01是字符串的类型码，100a1对应字符串缓存库中第0x100a1位置的字符串
    //isWindup是否去除成功了(如果不成功，说明存在一个字符串没有收尾) strStartp没有收尾的那个字符串定义的索引号
    QString extractString(QString txt,QStringList* bank,bool*isWindup,int*strStartp=NULL){
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


    //从标志号中提取出字符串
    QString getStringFromFlag(QString flagTXT,QString typeCode,QStringList &str,bool*isSrceess=NULL){
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
        return flagTXT;
    }



    //遍历所有节点的参数,如果参数是一个字符串的标志号，进行替换为原先的字符串
    void replaceNodeStr(MiddleNode &node,QStringList&strBlock){
        for(int i = 0;i<node.args.length();i++){
            node.args[i] = getStringFromFlag(node.args[i],"$01",strBlock);
        }
        for(int i = 0;i<node.atts.length();i++){
            MiddleNode_Att &thisAtt = node.atts[i];
            for(int j = 0;j<thisAtt.subNodes.length();j++){
                MiddleNode &thisNode = thisAtt.subNodes[j];
                replaceNodeStr(thisNode,strBlock);
            }
        }
    }

};



struct CodeSegGotoLabel{
    QString gotoLabelName;//跳转到的地址
    int orderIndex;//如果是跳转jne/jmp指令,指令的索引号
};
struct CodeSegGotoLabelList{
    QList<CodeSegGotoLabel> labels;

    int length(){
        return labels.length();
    }

    void append(QString label){
        labels.append({label,-1});
    }
    void append(QString gotoLabel,int index){
        labels.append({gotoLabel,index});
    }

    QString toString(){
        QStringList list;
        for(int i = 0;i<labels.length();i++){
            if(labels[i].orderIndex==-1){
                list.append("["+labels[i].gotoLabelName+"]");
            }else{
                list.append("["+labels[i].gotoLabelName+","+QString::number(labels[i].orderIndex)+"]");
            }

        }
        return "("+list.join(",")+")";
    }

    bool contains(QString name){
        for(int i = 0;i<labels.length();i++){
            if(labels[i].gotoLabelName==name){
                return 1;
            }
        }
        return 0;
    }
};

//代码段
//typedef QList<MiddleNode> CodeSegment;
struct CodeSegment:public QList<MiddleNode>{
    bool isHaveBeginLabel = 0;
    QString blockBeginLabelName;//其它段跳转到该段起始位置的标记
    bool isHaveGoto = 0;
    CodeSegGotoLabelList blockAllGotoLabels;//该段跳转到其它段的所有标记的名称
    QString blockEndGotoLabel;//段尾结束跳的标记的名称
    bool isVisible = 0;
};
//将当前段所可能跳转到的段isVisible置1
void searchMaybeGotoSegment(CodeSegment&thisSeg,QList<CodeSegment> &allSegment){
    if(!thisSeg.isHaveGoto)return;
    CodeSegGotoLabelList gotoLabel = thisSeg.blockAllGotoLabels;
    for(int i = 0;i<allSegment.length();i++){
        if(allSegment[i].isVisible || !allSegment[i].isHaveBeginLabel)continue;

        if(gotoLabel.contains(allSegment[i].blockBeginLabelName)){
            allSegment[i].isVisible = 1;
            searchMaybeGotoSegment(allSegment[i],allSegment);
        }
    }
}

//将函数中的指令分段
QList<CodeSegment> splitCodeSegment(QList<MiddleNode> &codeList){
    QList<CodeSegment> allSegment;
    CodeSegment tmp;
    QString thisBlockBeginLabel;
    tmp.clear();
    int thisSegBeginIndex = 0;
    for(int i = 0;i<codeList.length();i++){
        if(codeList[i].nodeType == "JNE" || codeList[i].nodeType == "JMP"){
            tmp.isHaveGoto = 1;
            QString gotoLabel = codeList[i].args[0].mid(6,codeList[i].args[0].length()-7);
            if(!tmp.blockAllGotoLabels.contains(gotoLabel)){
                tmp.blockAllGotoLabels.append(gotoLabel,i-thisSegBeginIndex);
            }
            if(codeList[i].nodeType == "JMP"){
                tmp.blockEndGotoLabel = gotoLabel;
            }
        }

        if(codeList[i].nodeType == "LABEL"){
            if(i>=1 && codeList[i-1].nodeType!="JMP"){
                tmp.isHaveGoto = 1;
                if(!tmp.blockAllGotoLabels.contains(codeList[i].args[0])){
                    tmp.blockAllGotoLabels.append(codeList[i].args[0]);
                }
                tmp.blockEndGotoLabel = codeList[i].args[0];
            }


            if(tmp.length()!=0){
                tmp.isHaveBeginLabel = allSegment.length()!=0;
                tmp.blockBeginLabelName = thisBlockBeginLabel;
                allSegment.append(tmp);
            }
            tmp = CodeSegment();
            tmp.append(codeList[i]);
            thisSegBeginIndex = i;
            thisBlockBeginLabel = codeList[i].args[0];
        }else{
            tmp.append(codeList[i]);
        }
    }
    if(tmp.length()!=0){
        tmp.isHaveBeginLabel = thisBlockBeginLabel!="";
        tmp.blockBeginLabelName = thisBlockBeginLabel;
        allSegment.append(tmp);
    }

    return allSegment;
}

//去除不可达代码
//allGotoLabel:函数中定义的所有标记
void removeUnreachableCode(QList<MiddleNode> &codeList,bool &isHaveReturnHandle){
    //所有的代码段
    QList<CodeSegment> allSegment;
    CodeSegment tmp;
    for(int i = 0;i<codeList.length();i++){
        if(codeList[i].nodeType == "LABEL"){
            allSegment.append(tmp);
            tmp.clear();
            tmp.append(codeList[i]);
        }else{
            tmp.append(codeList[i]);
        }
    }
    if(tmp.length()!=0){
        allSegment.append(tmp);
        tmp.clear();
    }

    //去除jmp后面的代码
    for(int i = 0;i<allSegment.length();i++){
        tmp.clear();
        for(int j = 0;j<allSegment[i].length();j++){
            tmp.append(allSegment[i][j]);
            if(allSegment[i][j].nodeType == "JMP"){
                break;
            }
        }
        allSegment[i] = tmp;
    }
    codeList.clear();
    for(int i = 0;i<allSegment.length();i++){
        codeList.append(allSegment[i]);
    }
    allSegment.clear();

    allSegment = splitCodeSegment(codeList);

    if(allSegment.length()==0)return;
    allSegment[0].isVisible = 1;
    searchMaybeGotoSegment(allSegment[0],allSegment);

    for(int i = 0;i<allSegment.length();i++){
        if(!allSegment[i].isVisible){
            allSegment.removeAt(i);
            i--;
        }
    }

    codeList.clear();
    isHaveReturnHandle = 0;
    for(int i = 0;i<allSegment.length();i++){
        if(allSegment[i].blockBeginLabelName=="*return*"){
            isHaveReturnHandle = 1;
        }
        codeList.append(allSegment[i]);
    }

}

//变量的使用情况
class VarUse{
public:
    QString type;       //变量数据类型
    int arrSize;    //数组大小
    QString varName;    //变量名
    bool isUse = 0;     //是否在代码中被使用过
    bool isArg;         //该变量的局部变量函数参数(函数参数为1)
    bool isRetrievedAddress = 0;//是否被取过地址
    bool isUseStack = 0;//变量是否在程序运行过程中使用堆栈空间去暂存(数组类型该参数无效)
    uint offset = 0;

    QString toString(){
        QString tmp = type + "(" + varName;
        if(type=="ARRAY"){
            tmp += ","+QString::number(arrSize);
        }
        tmp+= ")";
        if(isArg){
            tmp += ".ARG";
        }else{
            tmp += ".VAR";
        }

        if(isRetrievedAddress){
            tmp += "<RetrievedAddress>";
        }

        if(isUseStack){
            tmp += "<UseStack>";
        }
        return tmp;
    }
};

//获取一个指令中的操作数
class CodeOperand{
public:
    QString operText;//操作数的文本
    bool isValue;    //该操作数是否是一个变量
    bool rw;         //该操作数是源操作数还是目标操作数.源操作数为0，目标操作数为1

    QString toString(){
        return (rw ? "W:" : "R:") +operText + (isValue ? "<Value>" : "");
    }
};
bool thisOperIsValue(QString operText,QList<VarUse> &allFunValue){
    for(int i = 0;i<allFunValue.length();i++){
        if(allFunValue[i].varName==operText && allFunValue[i].type!="ARRAY"){
            return true;
        }
    }
    return false;
}
QList<CodeOperand> getCodeOperand(MiddleNode &code,QList<VarUse> &allFunValue){
    QString codeType = code.nodeType;
    QList<CodeOperand> oper;

    CodeOperand tmp;
    //有参数的指令,获取其中的源操作数变量/目标写入变量
    if(codeType=="ADD"||codeType=="SUB"||codeType=="MUL"||codeType=="DIV"||codeType=="REM"||
       codeType=="UADD"||codeType=="USUB"||codeType=="UMUL"||codeType=="UDIV"||
       codeType=="FADD"||codeType=="FSUB"||codeType=="FMUL"||codeType=="FDIV"||
       codeType=="NOT"||codeType=="OR"||codeType=="AND"||codeType=="XOR"||
       codeType=="SAL"||codeType=="SAR"||codeType=="MOV"||codeType=="INIT"||
       codeType=="ITF"||codeType=="UITF"||codeType=="FTI"||codeType=="BTD"||codeType=="WTD"){
        //[0]目标操作数,[1]/[2]源操作数
        for(int i = 0;i<code.args.length();i++){
            tmp.rw = i==0;
            tmp.operText = code.args[i];
            tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
            oper.append(tmp);
        }
    }else if(codeType=="INIT64"){
        //[0]/[1]目标操作数
        tmp.rw = 1;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="EC"||codeType=="NEC"||
             codeType=="LC"||codeType=="MC"||
             codeType=="LEC"||codeType=="MEC"||
             codeType=="UEC"||codeType=="UNEC"||
             codeType=="ULC"||codeType=="UMC"||
             codeType=="ULEC"||codeType=="UMEC"||
             codeType=="FEC"||codeType=="FNEC"||
             codeType=="FLC"||codeType=="FMC"||
             codeType=="FLEC"||codeType=="FMEC"){
        //[0]/[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="PUSH"){
        //[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);
    }else if(codeType=="RD" || codeType=="SRD"){
        //[1]目标操作数,[2]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

    }else if(codeType=="WE" || codeType=="SWE"){
        //[1]/[2]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);
    }else if(codeType=="ADDRESS"){

        //[0]目标操作数
        tmp.rw = 1;
        tmp.operText = code.args[0];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);


        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = 0;
        oper.append(tmp);


        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="ARRSTORE"){
        //[1]源操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[3];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="ARRLOAD"){
        //[1]目标操作数
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 1;
        tmp.operText = code.args[1];
        tmp.isValue = thisOperIsValue(tmp.operText,allFunValue);
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[2];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[3];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="COPYARR" || codeType=="STDCALL" || codeType=="CALL"){
        return oper;
    }else if(codeType=="PUSHARRAY" || codeType=="JNE" || codeType=="JMP" || codeType=="LABEL"){
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);
    }else if(codeType=="CLEAR"){
        tmp.rw = 0;
        tmp.operText = code.args[0];
        tmp.isValue = 0;
        oper.append(tmp);

        tmp.rw = 0;
        tmp.operText = code.args[1];
        tmp.isValue = 0;
        oper.append(tmp);
    }else{
        qDebug()<<"不支持的指令类型"<<codeType;

        return oper;
    }
    return oper;
}


//物理寄存器使用情况
class PhyRegUsage{
public:
    QString regName;//物理寄存器名
    bool isUsing = 0;//是否正被变量占用中
    bool isUpdate = 0;//存在寄存器中的变量数值是否被改变过
    VarUse * useVar=NULL;//使用该物理寄存器的局部变量信息

    bool isUsed = 0;//该寄存器是否在整个函数运行中有曾被占用过
};
//局部变量池[如果是fpu集成在cpu中的,那么函数指令编译时就让浮点变量与整型变量共用一个VariablePool,否则分别放在2个VariablePool]
class VariablePool{
public:
    struct OrderOper{
         QString type;//指令类型
         QList<CodeOperand> opers;//指令的参数信息


         QString toString(){
             QString tmp = type + " ";
             QStringList list;
             for(int i = 0;i<opers.length();i++){
                 list.append(opers[i].operText);
             }
             tmp.append(list.join(","))+";";
             return tmp;
         }

         //指令中是否使用了指定的变量
         bool isUseVar(QString varName){
             for(int i = 0;i<opers.length();i++){
                 if(opers[i].isValue && opers[i].operText == varName){
                     return 1;
                 }
             }
             return 0;
         }
         //如果变量使用存在,获取该变量是用于源操作数还是目标操作数(从指令操作数右至左搜索)
         //0:作源操作数 1:作目标操作数
         bool getFistUseVarType(QString varName){
             for(int i = opers.length()-1;i>=0;i--){
                 if(opers[i].isValue && opers[i].operText == varName){
                     return opers[i].rw;
                 }
             }
             return 0;
         }
         bool getLastUseVarType(QString varName){
             for(int i = 0;i<=opers.length();i++){
                 if(opers[i].isValue && opers[i].operText == varName){

                     return opers[i].rw;
                 }
             }
             return 0;
         }
    };

    struct OrderOpersSegs:public QList<OrderOper>{
        bool isHaveBeginLabel = 0;
        QString blockBeginLabelName;//其它段跳转到该段起始位置的标记
        bool isHaveGoto = 0;
        CodeSegGotoLabelList blockAllGotoLabels;//该段跳转到其它段的所有标记的名称
        QString blockEndGotoLabel;//段尾结束跳的标记的名称
        bool isVisible = 0;

        QString toString(){
            QString tmp;
            if(isHaveBeginLabel){
                tmp+="Begin["+blockBeginLabelName+"]\r\n";
            }
            if(isHaveGoto){
                tmp+="Goto["+blockAllGotoLabels.toString()+"]\r\n";
                tmp+="End["+blockEndGotoLabel+"]\r\n";
            }
            for(int i = 0;i<this->length();i++){
                tmp += this->operator[](i).toString()+"\r\n";
            }
            return tmp;

        }

        //判断当前段中是否使用了指定的变量,并返回首次使用该变量时是作为目标操作数还是源操作数
        //-1:未使用 0:作源操作数 1:作目标操作数
        int getFistUseVarType(QString varName){
            for(int i = 0;i<this->length();i++){
                if(this->operator[](i).isUseVar(varName)){
                    return this->operator[](i).getFistUseVarType(varName);
                }
            }
            return -1;
        }

        int getLastUseVarType(QString varName){
            for(int i = this->length()-1;i>=0;i--){
                if(this->operator[](i).isUseVar(varName)){
                    return this->operator[](i).getFistUseVarType(varName);
                }
            }
            return -1;
        }
    };


    //初始化局部变量池,传入cpu支持的所有通用运算寄存器的名称以及定义的所有局部变量
    //(funNode传入函数指令列表的节点属性
    VariablePool(QStringList regNames,
                 QList<VarUse>&vars,
                 QList<MiddleNode>&bodyCodeList,
                 QList<int> stackTopBuoy = QList<int>()){
        for(int i = 0;i<regNames.length();i++){
            PhyRegUsage reg;
            reg.regName = regNames[i];
            regs.append(reg);
        }
        this->vars = vars;
        QList<CodeSegment> tmp = splitCodeSegment(bodyCodeList);
        for(int i = 0;i<tmp.length();i++){
            OrderOpersSegs seg;
            for(int j = 0;j<tmp[i].length();j++){
                OrderOper oper;
                oper.type = tmp[i][j].nodeType;
                oper.opers = getCodeOperand(tmp[i][j],this->vars);
                seg.append(oper);
            }
            seg.isVisible = tmp[i].isVisible;
            seg.isHaveGoto = tmp[i].isHaveGoto;
            seg.isHaveBeginLabel = tmp[i].isHaveBeginLabel;
            seg.blockAllGotoLabels = tmp[i].blockAllGotoLabels;
            seg.blockEndGotoLabel = tmp[i].blockEndGotoLabel;
            seg.blockBeginLabelName = tmp[i].blockBeginLabelName;
            this->bodyCodeSegs.append(seg);
        }


        this->stackTopBuoy = stackTopBuoy;

        for(int i = 0;i<this->vars.length();i++){
            if(this->vars[i].isRetrievedAddress || this->vars[i].type=="ARRAY" || this->vars[i].isArg){
                this->vars[i].isUseStack = 1;
            }
        }
    }

    //根据名称,获取一个变量信息的对象。找不到返回NULL
    VarUse* getVarDefine(QString varName){
        for(int i = 0;i<vars.length();i++){
            if(vars[i].varName==varName){
                return &vars[i];
            }
        }
        return NULL;
    }


    //尝试为一个指令中所用到的一个变量分配所使用的物理寄存器,返回分配到的寄存器名
    //valueName变量名 proRegs禁止分配的寄存器
    //rwOper:申请使用该寄存器是完成 0.用于暂存指令的源操作数 1.用于暂存指令的目标操作数
    //codeIndex:当前指令是整个函数指令中的第几个(用于变量开销/临近值的计算)
    //proRegs:避开使用的寄存器
    //isBeenAlloc:返回是否执行该函数前已经被分配了。如果已经被分配了，那就无需再进行分配
    //isSeize:返回是否是抢占的其它变量使用的寄存器
    //seizeVar:返回被抢占变量的指针
    //seizeVarIsUpdate:返回抢占的变量是否在寄存器中改变过值
    //返回值:正常返回分配到使用权的物理寄存器指针。如果返回NULL说明分配出错
    PhyRegUsage* typeAllocReg(QString varName,
                         bool rwOper,
                         int segIndex,int offset,
                         QStringList proRegs,
                         bool &isBeenAlloc,
                         bool &isSeize,
                         VarUse**seizeVar,
                         bool &seizeVarIsUpdate){
        isSeize = 0;
        seizeVarIsUpdate =0;
        isBeenAlloc = 1;
        //判断该变量是否已经被分配了,如果是直接返回当前变量被分配到的寄存器名
        for(int i = 0;i<regs.length();i++){
            if(regs[i].isUsing){
                if(regs[i].useVar->varName==varName){
                    regs[i].isUpdate = regs[i].isUpdate||rwOper;
                    return &regs[i];
                }
            }
        }

        isBeenAlloc = 0;

        //如果该变量不存在,直接退出
        if(getVarDefine(varName)==NULL){
            return NULL;
        }

        //尝试寻找接下来开销值为0,且无需入栈保存的临时变量寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            if(regs[i].isUsing){
                VarUse* useVar = regs[i].useVar;
                if(getOverhead(useVar->varName,segIndex,offset)!=0)continue;//开销值不为0
                if(checkVarIsStoreToStack(useVar->varName,segIndex,offset))continue;//需要入栈保存
                //开销为0且不需要入栈保存
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }



        //尝试寻找未被分配的寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            if(!regs[i].isUsing){

                //找到了未被分配的寄存器,将该寄存器分配给当前变量
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }

        isSeize = 1;


        //当前找到的最优选项
        /*最优算法的参数:
         * 1.后续顺序代码段中该变量使用的次数最少
         * 2.距离下一次使用该变量间隔几条指令
         *
         * 要求使用次数最少，其次间隔的指令尽可能多
         *
         *
         */
        QString optimalReg;
        uint optimalProximity=0;
        uint optimalOverhead=0xffffffff;
        //寻找最优的暂存操作数的寄存器
        for(int i = 0;i<regs.length();i++){
            if(proRegs.contains(regs[i].regName))continue;
            //该寄存器当前使用的var
            VarUse* useVar = regs[i].useVar;
            uint proximity = getProximity(useVar->varName,segIndex,offset);//临近值
            uint overhead = getOverhead(useVar->varName,segIndex,offset);//开销值
            if(proximity==0xffffffff || overhead==0xffffffff)return NULL;
            if(proximity==0)continue;

            if((overhead<optimalOverhead)||
               (overhead==optimalOverhead && proximity>optimalProximity)){
                optimalReg = regs[i].regName;
                optimalProximity = proximity;
                optimalOverhead = overhead;
            }
        }

        if(optimalReg==""){
            //分配失败
            return NULL;
        }

        for(int i = 0;i<regs.length();i++){
            if(regs[i].regName == optimalReg){
                //找到了最优选择的寄存器,将该寄存器分配给当前变量
                if(seizeVar!=NULL){
                    *seizeVar = regs[i].useVar;
                }
                seizeVarIsUpdate = regs[i].isUpdate;
                regs[i].useVar = getVarDefine(varName);
                regs[i].isUsing = 1;
                regs[i].isUpdate = rwOper;
                regs[i].isUsed = 1;
                return &regs[i];
            }
        }
        return NULL;
    }

    //获取一个变量是否是数组
    bool varIsArray(QString varName){
        VarUse* var = getVarDefine(varName);
        if(var==NULL)return 0;
        return var->varName=="ARRAY";
    }

    //获取一个变量是否是要使用堆栈空间的(数组始终为是,被取过地址的变量也始终为是)
    bool getVarIsUseStackPlace(QString varName){
        VarUse* var = getVarDefine(varName);
        if(var==NULL)return 0;
        return var->isUseStack;
    }

    //在一个变量可能需要写入堆栈内存时判断该变量是否必须强制存在栈中
    //例如: sub a,b,c; 执行完毕后,继续执行其它指令时发现寄存器不够用了。
    //此时寄存器分配算法选择将a缓存到内存中,来让出寄存器使用权。此时判断a是否有必要一定得存入栈中,如果是非必要的,那么可以直接将a的数据给舍弃掉
    bool checkVarIsStoreToStack(QString varName,int segIndex,int offset){
        VarUse* useVar = getVarDefine(varName);
        if(useVar==NULL)return false;
        if(useVar->isRetrievedAddress)return 1;


        //搜索当前指令接下来的指令,是否有首次使用该变量作为指令的源操作数。如果有,说明该变量必须存入栈中
        for(int i = offset+1;i<bodyCodeSegs[segIndex].length();i++){
            if(!bodyCodeSegs[segIndex][i].isUseVar(varName))continue;
            bool useType = bodyCodeSegs[segIndex][i].getFistUseVarType(varName);
            if(useType){
                return 0;
            }else{
                useVar->isUseStack = 1;
                return 1;
            }
        }

        //搜索当前指令段可能跳转到的所有段,如果在段中首次使用该变量作为指令的源操作数,说明该变量必须存入栈中
        if(getAllSegIsStoreVar(varName,segIndex,offset)){
            useVar->isUseStack = 1;
            return 1;
        }

        return 0;
    }

    //在一个变量可能需要从堆栈中内存中加载时,判断该变量是否需要强制在内存中进行加载
    //例如: mov b,a;在执行该指令时,a的数据不在寄存器中。此时就要判断a是否一定得从栈中加载出来使用
    //如果a不是一个参数,并且a变量在此前并没有被赋过值,就说明该变量可以不去从栈中加载,而是直接使用寄存器中的无效值进行运算(写程序时坚决不能如此操作,否则将导致程序运行的不稳定)
    //反之,则a就必须从栈中加载保存的缓存值
    bool checkVarIsLoadFromStack(QString varName,int segIndex,int offset){
        VarUse* useVar = getVarDefine(varName);
        if(useVar==NULL)return false;
        if(useVar->isRetrievedAddress || useVar->isArg)return 1;


        //搜索当前指令接之前的指令,是否有首次使用该变量作为指令的目标操作数。如果有,说明该变量必须从栈中加载
        for(int i = offset-1;i>=0;i--){
            if(!bodyCodeSegs[segIndex][i].isUseVar(varName))continue;
            bool useType = bodyCodeSegs[segIndex][i].getLastUseVarType(varName);
            if(useType){
                useVar->isUseStack = 1;
                return 1;
            }
        }

        if(getAllSegIsLoadVar(varName,segIndex,offset)){
            useVar->isUseStack = 1;
            return 1;
        }
        return  0;
    }


    //生成其中一段指令的低级代码
    QList<MiddleNode> generateSegLowCode(OrderOpersSegs&segOrders,int segIndex,bool &status){
        QList<MiddleNode>replceBodyCodes;

        bool endIsJmpGoto = 0;//段末尾是否是jmp跳转
        QString jmpLabel;//如果是jmp跳转,jmp跳的地址

        for(int i = 0;i<segOrders.length();i++){
            status = 0;
            QString codeType = segOrders[i].type;//指令类型


            QList<MiddleNode> prefixOper;
            if(codeType=="JNE"){
                //jne是一大段中的子小段,所以也需要执行段尾数据保存的操作
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing&&regs[j].isUpdate){
                        if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                            prefixOper.append(generateLoadStoreValueCode(
                                                       1,
                                                       regs[j].useVar->varName,
                                                       regs[j].useVar->type,
                                                       regs[j].regName
                                                       ));
                        }

                        regs[j].isUpdate = 0;
                    }
                    regs[j].isUsing = 0;
                }
                replceBodyCodes.append(prefixOper);
                MiddleNode operNode;
                operNode.nodeType = codeType;
                operNode.args = QStringList({segOrders[i].opers[0].operText});
                replceBodyCodes.append(operNode);
                continue;
            }else if(codeType=="JMP"){
                endIsJmpGoto = 1;
                jmpLabel = segOrders[i].opers[0].operText;
                break;
            }else if(codeType=="CALL" || codeType=="STDCALL"){
                //将当前寄存器中所有变更过并且被取过地址的，则存入栈中的本体
                //将所有被取过地址的寄存器解除与变量的映射
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing){
                        if(!regs[j].useVar->isRetrievedAddress)continue;
                        if(regs[j].isUpdate){
                            if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                                prefixOper.append(generateLoadStoreValueCode(
                                                           1,
                                                           regs[j].useVar->varName,
                                                           regs[j].useVar->type,
                                                           regs[j].regName
                                                           ));
                                regs[j].isUpdate = 0;
                            }
                        }
                        regs[j].isUsing = 0;
                    }
                }
                replceBodyCodes.append(prefixOper);
                MiddleNode operNode;
                operNode.nodeType = codeType;
                replceBodyCodes.append(operNode);
                continue;
            }else if(segOrders[i].opers.length()==0){
                //没有参数的指令原样不变
                MiddleNode operNode;
                operNode.nodeType = codeType;
                replceBodyCodes.append(operNode);
                continue;
            }
            if(codeType=="RD" || codeType=="WE"){
                //将当前寄存器中所有变更过并且被取过地址的，存入栈中的本体。并设为没有变更过的
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing&&regs[j].isUpdate){
                        if(!regs[j].useVar->isRetrievedAddress)continue;

                        if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,i)){
                            prefixOper.append(generateLoadStoreValueCode(
                                                       1,
                                                       regs[j].useVar->varName,
                                                       regs[j].useVar->type,
                                                       regs[j].regName
                                                       ));
                        }

                        regs[j].isUpdate = 0;
                    }
                }
            }


            QList<CodeOperand> opers = segOrders[i].opers;
            QList<MiddleNode> loadStorVarCode;
            QStringList allOperReg;
            //将指令中使用的各个变量替换为分配到的寄存器
            for(int j = opers.length()-1;j>=0;j--){
                if(!opers[j].isValue)continue;
                bool isBeenAlloc;//当前变量是否已经在寄存器了,无需进行映射
                bool isSeize;    //未当前变量分配的寄存器是否是此前映射到了其它的变量
                VarUse* seizeVar;//如果当前变量是抢占了其它变量使用的寄存器,被抢占的变量名
                bool seizeVarHasUpdate;//被抢占的变量在寄存器中数据是否有过更改
                PhyRegUsage* allocReg = typeAllocReg(
                                opers[j].operText,
                                opers[j].rw,
                                segIndex,i,
                                allOperReg,
                                isBeenAlloc,
                                isSeize,
                                &seizeVar,
                                seizeVarHasUpdate);
                if(allocReg==NULL){
                    qDebug()<<"寄存器分配失败";
                }
                allOperReg.append(allocReg->regName);

                //在寄存器中则直接继续使用这个寄存器即可
                opers[j].operText = allocReg->regName;

                if(!isBeenAlloc && !isSeize){
                    //如果不在寄存器中,但是分配到了空闲的寄存器
                    //且当前变量是源操作数则将其从内存加载到寄存器。目标操作数则无需加载
                    if(!opers[j].rw){
                        if(checkVarIsLoadFromStack(allocReg->useVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       0,
                                                       allocReg->useVar->varName,
                                                       allocReg->useVar->type,
                                                       allocReg->regName
                                                       ));
                        }

                    }
                }else if(!isBeenAlloc && isSeize){


                    //如果不在寄存器中，且是抢占的另一个变量的寄存器使用权
                    //如果被抢占的变量在寄存器中有过变更,将变更的数据写入寄存器
                    if(seizeVarHasUpdate){
                        if(checkVarIsStoreToStack(seizeVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       1,
                                                       seizeVar->varName,
                                                       seizeVar->type,
                                                       allocReg->regName
                                                       ));
                        }
                    }
                    //当前变量是源操作数则将其从内存加载到寄存器。目标操作数则无需加载
                    if(!opers[j].rw){
                        if(checkVarIsLoadFromStack(seizeVar->varName,segIndex,i)){
                            loadStorVarCode.append(generateLoadStoreValueCode(
                                                       0,
                                                       allocReg->useVar->varName,
                                                       allocReg->useVar->type,
                                                       opers[j].operText
                                                       ));
                        }
                    }
                }

            }

            //生成指令的代码
            MiddleNode operNode;
            operNode.nodeType = codeType;
            for(int j = 0;j<opers.length();j++){
                operNode.args.append(opers[j].operText);
            }



            if(codeType=="WE"){
                //将当前寄存器中所有被取过地址的解除变量与其的映射
                for(int j = 0;j<regs.length();j++){
                    if(regs[j].isUsing){
                        if(!regs[j].useVar->isRetrievedAddress)continue;
                        regs[j].isUsing = 0;
                    }
                }
            }
            replceBodyCodes.append(prefixOper+loadStorVarCode);
            replceBodyCodes.append(operNode);
        }

        //段尾处理
        //将所有在寄存器中变更过数据的变量全部存入堆栈
        //同时将寄存器与变量的映射全部解除
        QList<MiddleNode> prefixOper;
        for(int j = 0;j<regs.length();j++){
            if(regs[j].isUsing&&regs[j].isUpdate){
                if(checkVarIsStoreToStack(regs[j].useVar->varName,segIndex,segOrders.length()-1)){
                    prefixOper.append(generateLoadStoreValueCode(
                                               1,
                                               regs[j].useVar->varName,
                                               regs[j].useVar->type,
                                               regs[j].regName
                                               ));
                }

                regs[j].isUpdate = 0;
            }
            regs[j].isUsing = 0;
        }
        replceBodyCodes.append(prefixOper);
        if(endIsJmpGoto){
            MiddleNode operNode;
            operNode.nodeType = "JMP";
            operNode.args = QStringList({jmpLabel});
            replceBodyCodes.append(operNode);
        }
        /////////////////////////////////////////////////
        status = 1;
        return replceBodyCodes;
    }


    //生成低级代码:将代码中使用的所有变量映射到寄存器,并生成读写堆栈变量本体的代码
    QList<MiddleNode> generateLowCode(bool &status){
        QList<MiddleNode>replceBodyCodes;
        for(int i = 0;i<bodyCodeSegs.length();i++){
            replceBodyCodes.append(generateSegLowCode(bodyCodeSegs[i],i,status));
            if(status==0)return replceBodyCodes;
        }
        return replceBodyCodes;
    }

    //获取所有在程序中使用到的寄存器名称
    QStringList getAllUsedRegNames(){
        QStringList names;
        for(int i = 0;i<regs.length();i++){
            if(regs[i].isUsed){
                names.append(regs[i].regName);
            }
        }
        return names;
    }



    QList<VarUse> vars;//所有局部变量
    QList<PhyRegUsage> regs;  //所有物理寄存器

    QList<OrderOpersSegs>bodyCodeSegs; //函数指令段
    QList<int> stackTopBuoy;  //栈顶浮动字节序列(与函数指令一一对应,记录了函数中每条指令时栈顶的浮动变化)

private:
    //已一个段为起始点,搜索出所有该段可能是从哪些段跳转来的,判断是否在这些段中是否有使用指定的变量为目标操作数
    bool seachAllSegIsLoadVar(QString varName,OrderOpersSegs&thisSeg,QList<OrderOpersSegs> &allSegment,int rootSegIndex,int rootLine){
        if(!thisSeg.isHaveBeginLabel)return 0;
        QString fromLabel = thisSeg.blockBeginLabelName;
        for(int i = 0;i<allSegment.length();i++){
            if(allSegment[i].isVisible || !allSegment[i].isHaveGoto)continue;
            if(allSegment[i].blockAllGotoLabels.contains(fromLabel)){
                int startSeachIndex = 0;
                if(allSegment[i].blockEndGotoLabel == fromLabel){
                    //从段末尾开始向上排查
                    startSeachIndex = allSegment[i].length()-1;

                }else{
                    //从段末尾,寻找到首个跳转到fromLabel的jne指令开始排查
                    for(int j = 0;j<allSegment[i].blockAllGotoLabels.length();j++){
                        if(allSegment[i].blockAllGotoLabels.labels[j].gotoLabelName != fromLabel)continue;

                        if(allSegment[i].blockAllGotoLabels.labels[j].orderIndex > startSeachIndex){
                            startSeachIndex = allSegment[i].blockAllGotoLabels.labels[j].orderIndex;
                        }
                    }
                }
                //搜索段中,是否有将该变量做目标操作数
                for(int j = startSeachIndex;j>=0;j--){
                    if(rootSegIndex == i && j==rootLine-1){
                        allSegment[i].isVisible = 1;
                        goto next;
                    }
                    if(!allSegment[i][j].isUseVar(varName)){
                        continue;
                    }
                    if(allSegment[i][j].getLastUseVarType(varName)){
                        return 1;
                    }
                }

                //向下递归搜索下一个段
                allSegment[i].isVisible = 1;
                if(seachAllSegIsLoadVar(varName,allSegment[i],allSegment,rootSegIndex,rootLine)){
                    return 1;
                }
                next:
                continue;
            }
        }

        return 0;
    }
    //判断当前所有的段中,是否有修改当前变量的值
    bool getAllSegIsLoadVar(QString varName,int rootSegIndex,int rootLine){
        for(int i = 0;i<bodyCodeSegs.length();i++){
            bodyCodeSegs[i].isVisible = 0;
        }
        return seachAllSegIsLoadVar(varName,bodyCodeSegs[rootSegIndex],bodyCodeSegs,rootSegIndex,rootLine);
    }
    //已一个段为起始点,搜索出所有该段可能跳转到的段,判断是否在段中首次使用该变量作为指令的源操作数
    bool seachAllSegIsStoreVar(QString varName,OrderOpersSegs&thisSeg,bool isSeachRoot,int thisSegIndex,QList<OrderOpersSegs> &allSegment,int rootSegIndex,int rootLine){

        if(!thisSeg.isHaveGoto)return 0;
        CodeSegGotoLabelList gotoLabel = thisSeg.blockAllGotoLabels;
        if(thisSegIndex == rootSegIndex){

            for(int i = 0;i<gotoLabel.length();i++){
                int index = gotoLabel.labels[i].orderIndex;
                if(index==-1){
                    index = thisSeg.length();
                }
                if(isSeachRoot? index<rootLine : index>rootLine){
                    gotoLabel.labels.removeAt(i);
                    i--;
                }
            }
        }
        for(int i = 0;i<allSegment.length();i++){
            if(allSegment[i].isVisible || !allSegment[i].isHaveBeginLabel)continue;
            if(gotoLabel.contains(allSegment[i].blockBeginLabelName)){
                int useType;
                if(rootSegIndex==i){
                    useType = -1;
                    for(int j = 0;j<=rootLine;j++){
                        if(allSegment[i][j].isUseVar(varName)){
                            useType = allSegment[i][j].getFistUseVarType(varName);
                            break;
                        }
                    }
                }else{
                    useType = allSegment[i].getFistUseVarType(varName);
                }
                if(useType==0){
                    //首次是用作源操作数,那无需向下搜索了,说明在该段中并首次使用该变量的数据,就必须要存入堆栈
                    return 1;
                }else if(useType==-1){
                    //如果全程都没有使用该操作数,向下搜索该段可能跳转到的段
                    allSegment[i].isVisible = 1;
                    if(seachAllSegIsStoreVar(varName,allSegment[i],0,i,allSegment,rootSegIndex,rootLine)){
                        return 1;
                    }
                }//如果首次是用作目标操作数,那无需向下搜索了,说明在该段中并不需要再次使用该变量的数据,可以不需要存入堆栈
            }
        }
        return 0;
    }
    //判断当前所有的段中,是否依赖当前段中对该变量所修改的数据
    bool getAllSegIsStoreVar(QString varName,int rootSegIndex,int rootLine){
        for(int i = 0;i<bodyCodeSegs.length();i++){
            bodyCodeSegs[i].isVisible = 0;
        }

        bool ret = seachAllSegIsStoreVar(varName,bodyCodeSegs[rootSegIndex],1,rootSegIndex,bodyCodeSegs,rootSegIndex,rootLine);
        //qDebug()<<varName<<ret<<rootSegIndex<<bodyCodeSegs[rootSegIndex].toString().toUtf8().data();
        return ret;
    }
    //获取指定的变量的开销值
    uint getOverhead(QString valueName,int segIndex,int offset){
        uint num = 0;
        if(segIndex==-1)return -1;
        for(int i = offset;i<bodyCodeSegs[segIndex].length();i++){
            if(bodyCodeSegs[segIndex][i].type=="JNE"){
                return num;
            }
            if(bodyCodeSegs[segIndex][i].isUseVar(valueName)){
                num++;
            }
        }
        return num;
    }

    //获取指定的变量的临近值(如果当前连续程序块内没有任何使用该变量的话，返回0xffffffff)
    uint getProximity(QString valueName,int segIndex,int offset){
        if(segIndex==-1)return -1;
        for(int i = offset;i<bodyCodeSegs[segIndex].length();i++){
            if(bodyCodeSegs[segIndex][i].type=="JNE"){
                return 0xffffffff;
            }
            if(bodyCodeSegs[segIndex][i].isUseVar(valueName)){
                return i;
            }
        }
        return 0xffffffff;
    }
    //获取一条指令属于哪个段的第几条
    int getThisCodeFromSegIndex(int lineIndex,int &segOffset){
        int allLine = 0;
        for(int i = 0;i<bodyCodeSegs.length();i++){
            segOffset = lineIndex-allLine;
            allLine += bodyCodeSegs[i].length();
            if(lineIndex<allLine){
                return i;
            }
        }
        return -1;
    }

    //生成从堆栈中变量与寄存器交换数据的指令
    //isStore:是将寄存器数据存入变量,还是从变量取到寄存器(0取出,1存入)
    //stackOffset:变量相对于栈顶的偏移
    //type:变量的数据类型
    //reg: 要取到的寄存器名
    MiddleNode generateLoadStoreValueCode(bool isStore,QString varName,QString type,QString reg){
        MiddleNode code;
        code.nodeType = isStore ? "SWE" : "SRD";
        QString rwBytes;
        if(type=="UBYTE"||type=="BYTE"){
            rwBytes = "BYTE";
        }else if(type=="USHORT"||type=="SHORT"){
            rwBytes = "WORD";
        }else if(type=="UINT"||type=="INT"||type=="FLOAT"){
            rwBytes = "DWORD";
        }
        QString rwAddress = "<offset:"+varName+">";
        code.args = QStringList({rwBytes,reg,rwAddress});
        return code;
    }



};




#endif // MIDDLE_TYPEDEF_H
