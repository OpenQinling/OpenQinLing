#ifndef OPENQINLING_PSDL_MIDDLENODE_H
#define OPENQINLING_PSDL_MIDDLENODE_H
#include <QStringList>
#include <QDebug>
#include "OpenQinling_FormatText.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace PsdlIR_Compiler{

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

            tmp = segmentStringThroughText(src,".",QStringList({";"}),segmentSuccess);
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
}


}
QT_END_NAMESPACE

#endif // OPENQINLING_PSDL_MIDDLENODE_H
