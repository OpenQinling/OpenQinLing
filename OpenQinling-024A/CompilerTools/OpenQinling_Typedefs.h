#ifndef OPENQINLING_TYPEDEFS_H
#define OPENQINLING_TYPEDEFS_H
#include <QStringList>
#include <QDebug>
#include <QVector>
#include <QMap>
QT_BEGIN_NAMESPACE
namespace OpenQinling {


//用于在编译过程中的全局代码索引、定位
class IndexV2{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
};
class IndexV3{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
    int index3;//子指令索引号
};
class IndexV4{
public:
    int index1;//源文件索引号
    int index2;//全局指令索引号
    int index3;//子指令索引号
    int index4;//标记索引号
};

//n级索引号
class IndexVN:public QList<int> {
public:
    //打印输出
    QString toString();
    //判断是否为空
    bool isEmpty();
    //将末尾的index提取出,并将其维度-1,失败返回-1
    int getEndIndex();



};
}
QT_END_NAMESPACE
#endif // OPENQINLING_TYPEDEFS_H
