#include "OpenQinling_Typedefs.h"
QT_BEGIN_NAMESPACE
namespace OpenQinling {


//打印输出
QString IndexVN::toString(){
    QString tmp;
    for(int i = 0;i<this->length();i++){
        tmp += "["+QString::number(this->at(i))+"]";
    }
    return tmp;
}

//判断是否为空
bool IndexVN::isEmpty(){
    return this->length()==0;
}

//将末尾的index提取出,并将其维度-1,失败返回-1
int IndexVN::getEndIndex(){
    if(this->length()==0)return -1;
    int end = this->at(this->length()-1);

    this->removeLast();
    return end;
}




}
QT_END_NAMESPACE
