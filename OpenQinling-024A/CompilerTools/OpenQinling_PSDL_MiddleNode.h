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
    QString toString(bool isRoot=1);
    MiddleNode();
    //从文本描述中解析出中级语言节点结构信息,并将当前节点对象作为根节点
    //src文本描述  suceess返回是否解析成功 level递归所用,无需传入任何值
    MiddleNode(QString &src,bool &suceess,bool level=0);
    //判断当前节点是否定义了除attNams外的其它属性
    bool areThereOnlyTheseAtt(QStringList attNams);
    //根据属性名,获取对应属性的对象指针(如果不存在返回NULL)
    MiddleNode_Att* getAttFromName(QString name);




private:
    //已一段文本对字符串进行分割为2段
    //前段通过函数返回，后段通过src返回 suceess返回是否分割成功(如果不成功，说明src在stopTexts前不存在指定的text)
    QString segmentStringThroughText(QString &src,QString text,QStringList stopTexts,bool &suceess);
    //提取出最近的大括号内的文本(带提取的文本不包括{首部)
    //suceess返回是否成功(如果不成功,说明找不到括号的结尾)
    QString extractParentContent(QString &src,bool &suceess);
    //字符串提取器，并将原字符串的位置替换为标志号,标志号对应一个被提取的字符串，用于还原
    //标志号的结构: $类型码 索引码 [类型为2位16进制数，索引号为5位16进制数]
    //示例:   $01100a1  01是字符串的类型码，100a1对应字符串缓存库中第0x100a1位置的字符串
    //isWindup是否去除成功了(如果不成功，说明存在一个字符串没有收尾) strStartp没有收尾的那个字符串定义的索引号
    QString extractString(QString txt,QStringList* bank,bool*isWindup,int*strStartp=NULL);
    //从标志号中提取出字符串
    QString getStringFromFlag(QString flagTXT,QString typeCode,QStringList &str,bool*isSrceess=NULL);
    //遍历所有节点的参数,如果参数是一个字符串的标志号，进行替换为原先的字符串
    void replaceNodeStr(MiddleNode &node,QStringList&strBlock);

};
}


}
QT_END_NAMESPACE

#endif // OPENQINLING_PSDL_MIDDLENODE_H
