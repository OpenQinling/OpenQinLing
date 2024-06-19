#ifndef CC_TYPEDEF_H
#define CC_TYPEDEF_H
#include <QStringList>
#include <QDebug>
#include <QVector>
#include <QMap>
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

}



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
    QString toString(){
        QString tmp;
        for(int i = 0;i<this->length();i++){
            tmp += "["+QString::number(this->at(i))+"]";
        }
        return tmp;
    }

    //判断是否为空
    bool isEmpty(){
        return this->length()==0;
    }

    //将末尾的index提取出,并将其维度-1,失败返回-1
    int getEndIndex(){
        if(this->length()==0)return -1;
        int end = this->at(this->length()-1);

        this->removeLast();
        return end;
    }

};
}
QT_END_NAMESPACE


#endif // CC_TYPEDEF_H
