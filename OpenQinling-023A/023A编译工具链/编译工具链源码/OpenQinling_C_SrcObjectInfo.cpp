#include "OpenQinling_C_SrcObjectInfo.h"
#include "OpenQinling_C_JudgestrtoolFunction.h"
#include "OpenQinling_C_Initblock.h"
#pragma execution_character_set("utf-8")
QT_BEGIN_NAMESPACE
namespace OpenQinling {
namespace C_Compiler{

////////////////////////////////////////////////////////////////////////////
/*                            GlobalInitValueTmp                          */
////////////////////////////////////////////////////////////////////////////
QString GlobalInitValueTmp::typeToString(){
    switch (valueType) {
        case ConstValueType_INT64:return "INT64";
        case ConstValueType_INT32:return "INT32";
        case ConstValueType_INT16:return "INT16";
        case ConstValueType_INT8:return "INT8";
        case ConstValueType_UINT64:return "UINT64";
        case ConstValueType_UINT32:return "UINT32";
        case ConstValueType_UINT16:return "UINT16";
        case ConstValueType_UINT8:return "UINT8";
        case ConstValueType_FLOAT:return "FLOAT32";
        case ConstValueType_DOUBLE:return "FLOAT64";
        case ConstValueType_POINTER:return "POINTER";
    default:return "ERROR";
    }
    return "ERROR";
}

QString GlobalInitValueTmp::toStirng(){
    if(valueType == ConstValueType_ERROR){
        return "<ERROR>";
    }

    QString str = "<"+typeToString()+":";

    if(valueType == ConstValueType_FLOAT || valueType==ConstValueType_DOUBLE){
        str += QString::number(floatData)+">";
        return str;
    }else if(factIsPointer){
        str+="["+pointerData_mark;
        if(pointerData_offset>0){
            str += "+"+QString::number(pointerData_offset);
        }else if(pointerData_offset<0){
            str += QString::number(pointerData_offset);
        }
        str+="]("+QString::number(pointerBytes)+")>";
        return str;
    }else{
        str += QString::number(intData)+">";
        return str;
    }
}

//数据类型转换
//pointerBytes:如果转换的是指针型,转出的指针对应的数据类型的字节数
GlobalInitValueTmp GlobalInitValueTmp::converType(ConstValueType converType,int pointerBytes){
    GlobalInitValueTmp tmp;
    if(converType==ConstValueType_ERROR || this->valueType==ConstValueType_ERROR)return tmp;


    if(converType==ConstValueType_POINTER){
        if(valueType==ConstValueType_FLOAT || valueType==ConstValueType_DOUBLE)return tmp;
        tmp.valueType = converType;
        tmp.factIsPointer = factIsPointer;

        if(factIsPointer){
            tmp.pointerBytes = pointerBytes;
            tmp.pointerData_mark = pointerData_mark;
            tmp.pointerData_offset = pointerData_offset;
        }else{
            tmp.intData = intData;
        }
    }else if(converType==ConstValueType_FLOAT || converType==ConstValueType_DOUBLE){
        if(valueType==ConstValueType_POINTER)return tmp;
        tmp.valueType = converType;
        if(valueType==ConstValueType_FLOAT || (valueType==ConstValueType_DOUBLE && converType==ConstValueType_DOUBLE)){
            //待转换数是float,转换为float或double
            //待转换数是double，要转换为double
            tmp.floatData = floatData;
        }else if(valueType==ConstValueType_DOUBLE && converType==ConstValueType_FLOAT){
            //待转换数是double,要转换为float
            tmp.floatData = (float)floatData;
        }else if(converType==ConstValueType_FLOAT){
            //待转换数是整数,要转换为float
            tmp.floatData = (float)intData;
        }else{
            //待转换数是整数,要转换为double
            tmp.floatData = intData;
        }
    }else if(factIsPointer){
        //指针转整数
        tmp.valueType = converType;
        tmp.factIsPointer = 1;
        tmp.pointerBytes = 1;
        tmp.pointerData_mark = pointerData_mark;
        tmp.pointerData_offset = pointerData_offset;
    }else{
        //整数/浮点数转整数
        long long median;
        if(valueType==ConstValueType_DOUBLE || valueType==ConstValueType_FLOAT){
            median = floatData;
        }else{
            median = intData;
        }

        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
    }
    return tmp;
}


GlobalInitValueTmp GlobalInitValueTmp::operator~(){
    GlobalInitValueTmp tmp;
    if(valueType==ConstValueType_ERROR ||
       valueType==ConstValueType_POINTER ||
       valueType==ConstValueType_DOUBLE ||
       valueType==ConstValueType_FLOAT ||
       factIsPointer)return tmp;


    if(valueType==ConstValueType_INT64){
        tmp.intData = ~(int64_t)this->intData;
    }else if(valueType==ConstValueType_INT32){
        tmp.intData = ~(int32_t)this->intData;
    }else if(valueType==ConstValueType_INT16){
        tmp.intData = ~(int16_t)this->intData;
    }else if(valueType==ConstValueType_INT8){
        tmp.intData = ~(int8_t)this->intData;
    }else if(valueType==ConstValueType_UINT64){
        tmp.intData = ~(uint64_t)this->intData;
    }else if(valueType==ConstValueType_UINT32){

        tmp.intData = ~(uint32_t)this->intData;

    }else if(valueType==ConstValueType_UINT16){
        tmp.intData = ~(uint16_t)this->intData;
    }else if(valueType==ConstValueType_UINT8){
        tmp.intData = ~(uint8_t)this->intData;
    }else{
        return tmp;
    }
    tmp.valueType = valueType;
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator!(){
    GlobalInitValueTmp tmp;
    if(valueType==ConstValueType_ERROR ||
       valueType==ConstValueType_POINTER ||
       factIsPointer)return tmp;
    bool median;
    if(valueType==ConstValueType_DOUBLE || valueType==ConstValueType_FLOAT){
        median = floatData;
    }else{
        median = intData;
    }

    tmp.valueType = ConstValueType_INT8;
    tmp.intData = !median;
    return tmp;
}


GlobalInitValueTmp GlobalInitValueTmp::operator+(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;

    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){

        if((this->valueType == ConstValueType_POINTER || this->factIsPointer) &&
           (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
            //2个都是指针,不符合语法要求
            return tmp;
        }else if(var.valueType == ConstValueType_DOUBLE || var.valueType == ConstValueType_FLOAT||
           this->valueType == ConstValueType_DOUBLE || this->valueType == ConstValueType_FLOAT){
            //偏移地址是浮点数,不符合语法要求
            return tmp;
        }

        GlobalInitValueTmp addressVar;
        GlobalInitValueTmp offsetVar;

        if(this->valueType == ConstValueType_POINTER || this->factIsPointer){
            addressVar = *this;
            offsetVar = var;
        }else{
            addressVar = var;
            offsetVar = *this;
        }


        if(addressVar.factIsPointer){
            if(addressVar.pointerBytes == 0)return tmp;

            tmp.valueType = addressVar.valueType>=offsetVar.valueType ? addressVar.valueType : offsetVar.valueType;

            tmp.factIsPointer = 1;

            tmp.pointerData_mark = addressVar.pointerData_mark;
            tmp.pointerBytes = addressVar.pointerBytes;
            tmp.pointerData_offset = addressVar.pointerData_offset + tmp.pointerBytes * offsetVar.intData;
        }else{
            ConstValueType converType = addressVar.valueType>=offsetVar.valueType ? addressVar.valueType : offsetVar.valueType;
            long long median = addressVar.intData + offsetVar.intData;

            if(converType==ConstValueType_INT8){
                median = (int8_t)median;
            }else if(converType==ConstValueType_INT16){
                median = (int16_t)median;
            }else if(converType==ConstValueType_INT32){
                median = (int32_t)median;
            }else if(converType==ConstValueType_INT64){
                median = (int64_t)median;
            }else if(converType==ConstValueType_UINT8){
                median = (uint8_t)median;
            }else if(converType==ConstValueType_UINT16){
                median = (uint16_t)median;
            }else if(converType==ConstValueType_UINT32){
                median = (uint32_t)median;
            }else if(converType==ConstValueType_UINT64){
                median = (uint64_t)median;
            }else{
                return tmp;
            }

            tmp.valueType = converType;
            tmp.intData = median;
        }
        return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){

        double median;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            median = this->floatData;
        }else{
            median = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            median += var.floatData;
        }else{
            median += var.intData;
        }
        tmp.valueType = this->valueType>var.valueType ? this->valueType : var.valueType;
        tmp.floatData = median;
        return tmp;
    }else{
        long long median = var.intData + this->intData;

        ConstValueType converType = this->valueType>var.valueType ? this->valueType : var.valueType;

        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
        return tmp;
    }
}
GlobalInitValueTmp GlobalInitValueTmp::operator-(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;

    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){

        if(var.valueType == ConstValueType_POINTER || var.factIsPointer){
            //被减数是指针,不符合语法要求
            return tmp;
        }else if(var.valueType == ConstValueType_DOUBLE || var.valueType == ConstValueType_FLOAT){
            //偏移地址是浮点数,不符合语法要求
            return tmp;
        }

        GlobalInitValueTmp addressVar = *this;
        GlobalInitValueTmp offsetVar = var;

        if(addressVar.factIsPointer){
            if(addressVar.pointerBytes == 0)return tmp;

            tmp.valueType = addressVar.valueType>=offsetVar.valueType ? addressVar.valueType : offsetVar.valueType;

            tmp.factIsPointer = 1;

            tmp.pointerData_mark = addressVar.pointerData_mark;
            tmp.pointerBytes = addressVar.pointerBytes;
            tmp.pointerData_offset = addressVar.pointerData_offset - tmp.pointerBytes * offsetVar.intData;
        }else{
            ConstValueType converType = addressVar.valueType>=offsetVar.valueType ? addressVar.valueType : offsetVar.valueType;
            long long median = addressVar.intData - offsetVar.intData;

            if(converType==ConstValueType_INT8){
                median = (int8_t)median;
            }else if(converType==ConstValueType_INT16){
                median = (int16_t)median;
            }else if(converType==ConstValueType_INT32){
                median = (int32_t)median;
            }else if(converType==ConstValueType_INT64){
                median = (int64_t)median;
            }else if(converType==ConstValueType_UINT8){
                median = (uint8_t)median;
            }else if(converType==ConstValueType_UINT16){
                median = (uint16_t)median;
            }else if(converType==ConstValueType_UINT32){
                median = (uint32_t)median;
            }else if(converType==ConstValueType_UINT64){
                median = (uint64_t)median;
            }else{
                return tmp;
            }
            tmp.valueType = converType;
            tmp.intData = median;
        }
        return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){

        double median;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            median = this->floatData;
        }else{
            median = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            median -= var.floatData;
        }else{
            median -= var.intData;
        }
        tmp.valueType = this->valueType>var.valueType ? this->valueType : var.valueType;
        tmp.floatData = median;
        return tmp;
    }else{
        long long median = this->intData-var.intData;

        ConstValueType converType = this->valueType>var.valueType ? this->valueType : var.valueType;
        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
        return tmp;
    }
}
GlobalInitValueTmp GlobalInitValueTmp::operator*(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if(this->valueType == ConstValueType_POINTER ||
       var.valueType == ConstValueType_POINTER ||
       this->factIsPointer || var.factIsPointer)return tmp;

    if(this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){

        double median;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            median = this->floatData;
        }else{
            median = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            median *= var.floatData;
        }else{
            median *= var.intData;
        }
        tmp.valueType = this->valueType>var.valueType ? this->valueType : var.valueType;
        tmp.floatData = median;
        return tmp;
    }else{
        long long median = var.intData * this->intData;

        ConstValueType converType = this->valueType>var.valueType ? this->valueType : var.valueType;

        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
        return tmp;
    }

}
GlobalInitValueTmp GlobalInitValueTmp::operator/(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if(this->valueType == ConstValueType_POINTER ||
       var.valueType == ConstValueType_POINTER ||
       this->factIsPointer || var.factIsPointer)return tmp;

    if(this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){

        double median;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            median = this->floatData;
        }else{
            median = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            median /= var.floatData;
        }else{
            median /= var.intData;
        }
        tmp.valueType = this->valueType>var.valueType ? this->valueType : var.valueType;
        tmp.floatData = median;
        return tmp;
    }else{
        long long median = this->intData/var.intData;

        ConstValueType converType = this->valueType>var.valueType ? this->valueType : var.valueType;

        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
        return tmp;
    }
}
GlobalInitValueTmp GlobalInitValueTmp::operator%(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if(this->valueType == ConstValueType_POINTER ||
       var.valueType == ConstValueType_POINTER ||
       this->factIsPointer || var.factIsPointer)return tmp;

    if(this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }else{
        long long median = this->intData / var.intData;

        ConstValueType converType = this->valueType>var.valueType ? this->valueType : var.valueType;

        if(converType==ConstValueType_INT8){
            median = (int8_t)median;
        }else if(converType==ConstValueType_INT16){
            median = (int16_t)median;
        }else if(converType==ConstValueType_INT32){
            median = (int32_t)median;
        }else if(converType==ConstValueType_INT64){
            median = (int64_t)median;
        }else if(converType==ConstValueType_UINT8){
            median = (uint8_t)median;
        }else if(converType==ConstValueType_UINT16){
            median = (uint16_t)median;
        }else if(converType==ConstValueType_UINT32){
            median = (uint32_t)median;
        }else if(converType==ConstValueType_UINT64){
            median = (uint64_t)median;
        }else{
            return tmp;
        }

        tmp.valueType = converType;
        tmp.intData = median;
        return tmp;
    }
}

GlobalInitValueTmp GlobalInitValueTmp::operator>(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset > var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData > var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA > medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData > var.intData;
    }
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator>=(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset >= var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData >= var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA >= medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData >= var.intData;
    }
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator==(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset == var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData == var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA == medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData == var.intData;
    }
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator!=(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset != var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData != var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA != medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData != var.intData;
    }
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator<(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset < var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData < var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA < medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData < var.intData;
    }
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator<=(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        if(this->factIsPointer && var.factIsPointer){
            //指针比较
            if(this->pointerData_mark != var.pointerData_mark) return tmp;
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->pointerData_offset <= var.pointerData_offset;
        }else if(!this->factIsPointer && !var.factIsPointer){
            //整数值比较
            tmp.valueType = ConstValueType_INT8;
            tmp.intData = this->intData <= var.intData;
        }else return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        //浮点数比较
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA <= medianB;
    }else{
        //整数值比较
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData <= var.intData;
    }
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator|(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)||
       this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }
    //整数值比较
    ConstValueType converType = this->valueType>=var.valueType ? this->valueType : var.valueType;
    long long median = this->intData | var.intData;

    if(converType==ConstValueType_INT8){
        median = (int8_t)median;
    }else if(converType==ConstValueType_INT16){
        median = (int16_t)median;
    }else if(converType==ConstValueType_INT32){
        median = (int32_t)median;
    }else if(converType==ConstValueType_INT64){
        median = (int64_t)median;
    }else if(converType==ConstValueType_UINT8){
        median = (uint8_t)median;
    }else if(converType==ConstValueType_UINT16){
        median = (uint16_t)median;
    }else if(converType==ConstValueType_UINT32){
        median = (uint32_t)median;
    }else if(converType==ConstValueType_UINT64){
        median = (uint64_t)median;
    }else{
        return tmp;
    }

    tmp.valueType = converType;
    tmp.intData = median;
    return tmp;
}
GlobalInitValueTmp GlobalInitValueTmp::operator&(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)||
       this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }
    //整数值比较
    ConstValueType converType = this->valueType>=var.valueType ? this->valueType : var.valueType;
    long long median = this->intData & var.intData;

    if(converType==ConstValueType_INT8){
        median = (int8_t)median;
    }else if(converType==ConstValueType_INT16){
        median = (int16_t)median;
    }else if(converType==ConstValueType_INT32){
        median = (int32_t)median;
    }else if(converType==ConstValueType_INT64){
        median = (int64_t)median;
    }else if(converType==ConstValueType_UINT8){
        median = (uint8_t)median;
    }else if(converType==ConstValueType_UINT16){
        median = (uint16_t)median;
    }else if(converType==ConstValueType_UINT32){
        median = (uint32_t)median;
    }else if(converType==ConstValueType_UINT64){
        median = (uint64_t)median;
    }else{
        return tmp;
    }

    tmp.valueType = converType;
    tmp.intData = median;
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator^(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)||
       this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }
    //整数值比较
    ConstValueType converType = this->valueType>=var.valueType ? this->valueType : var.valueType;
    long long median = this->intData ^ var.intData;

    if(converType==ConstValueType_INT8){
        median = (int8_t)median;
    }else if(converType==ConstValueType_INT16){
        median = (int16_t)median;
    }else if(converType==ConstValueType_INT32){
        median = (int32_t)median;
    }else if(converType==ConstValueType_INT64){
        median = (int64_t)median;
    }else if(converType==ConstValueType_UINT8){
        median = (uint8_t)median;
    }else if(converType==ConstValueType_UINT16){
        median = (uint16_t)median;
    }else if(converType==ConstValueType_UINT32){
        median = (uint32_t)median;
    }else if(converType==ConstValueType_UINT64){
        median = (uint64_t)median;
    }else{
        return tmp;
    }

    tmp.valueType = converType;
    tmp.intData = median;
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator||(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
            (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }


        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA || medianB;

    }else{
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData || var.intData;
    }
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator&&(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
            (var.valueType == ConstValueType_POINTER || var.factIsPointer)){
        return tmp;
    }else if(this->valueType == ConstValueType_DOUBLE ||
             this->valueType == ConstValueType_FLOAT ||
             var.valueType == ConstValueType_DOUBLE||
             var.valueType == ConstValueType_FLOAT){
        double medianA,medianB;
        if(this->valueType == ConstValueType_DOUBLE ||
           this->valueType == ConstValueType_FLOAT){
            medianA = this->floatData;
        }else{
            medianA = this->intData;
        }

        if(var.valueType == ConstValueType_DOUBLE ||
           var.valueType == ConstValueType_FLOAT){
            medianB = var.floatData;
        }else{
            medianB = var.intData;
        }


        tmp.valueType = ConstValueType_INT8;
        tmp.intData = medianA && medianB;

    }else{
        tmp.valueType = ConstValueType_INT8;
        tmp.intData = this->intData && var.intData;
    }
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator<<(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)||
       this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }

    ConstValueType converType = this->valueType;
    long long median = this->intData << var.intData;

    if(converType==ConstValueType_INT8){
        median = (int8_t)median;
    }else if(converType==ConstValueType_INT16){
        median = (int16_t)median;
    }else if(converType==ConstValueType_INT32){
        median = (int32_t)median;
    }else if(converType==ConstValueType_INT64){
        median = (int64_t)median;
    }else if(converType==ConstValueType_UINT8){
        median = (uint8_t)median;
    }else if(converType==ConstValueType_UINT16){
        median = (uint16_t)median;
    }else if(converType==ConstValueType_UINT32){
        median = (uint32_t)median;
    }else if(converType==ConstValueType_UINT64){
        median = (uint64_t)median;
    }else{
        return tmp;
    }

    tmp.valueType = converType;
    tmp.intData = median;
    return tmp;
}

GlobalInitValueTmp GlobalInitValueTmp::operator>>(GlobalInitValueTmp &var){
    GlobalInitValueTmp tmp;
    if((this->valueType == ConstValueType_POINTER || this->factIsPointer) ||
       (var.valueType == ConstValueType_POINTER || var.factIsPointer)||
       this->valueType == ConstValueType_DOUBLE ||
       this->valueType == ConstValueType_FLOAT ||
       var.valueType == ConstValueType_DOUBLE||
       var.valueType == ConstValueType_FLOAT){
        return tmp;
    }

    ConstValueType converType = this->valueType;
    long long median = this->intData >> var.intData;

    if(converType==ConstValueType_INT8){
        median = (int8_t)median;
    }else if(converType==ConstValueType_INT16){
        median = (int16_t)median;
    }else if(converType==ConstValueType_INT32){
        median = (int32_t)median;
    }else if(converType==ConstValueType_INT64){
        median = (int64_t)median;
    }else if(converType==ConstValueType_UINT8){
        median = (uint8_t)median;
    }else if(converType==ConstValueType_UINT16){
        median = (uint16_t)median;
    }else if(converType==ConstValueType_UINT32){
        median = (uint32_t)median;
    }else if(converType==ConstValueType_UINT64){
        median = (uint64_t)median;
    }else{
        return tmp;
    }

    tmp.valueType = converType;
    tmp.intData = median;
    return tmp;
}














////////////////////////////////////////////////////////////////////////////
/*                            FunctionInfo                                */
////////////////////////////////////////////////////////////////////////////
bool FunctionInfo::operator==(FunctionInfo&info){
    bool nameC = functionName==info.functionName;
    bool typeC = baseTypeIndex==info.baseTypeIndex;
    return nameC&&typeC;
}
bool FunctionInfo::operator!=(FunctionInfo&info){
    return !this->operator==(info);
}

QString FunctionInfo::toString(){
    QString temp = functionName;
    temp.append(baseTypeIndex.toString());
    if(isStatic){
        temp+="<Static>";
    }
    if(isOnlyStatement){
        temp+="<OnlyStatement>";
    }
    return temp;
}

////////////////////////////////////////////////////////////////////////////
/*                            SrcAllVarInfoTree                           */
////////////////////////////////////////////////////////////////////////////
SrcAllVarInfoTree::SrcAllVarInfoTree(){
    thisIndex.clear();
    thisIndex.append(0);
    rootNode = this;
}

//根据索引号获取当前树中任意一个节点的指针
SrcAllVarInfoTree*SrcAllVarInfoTree::getNode(IndexVN index){
    if(!index.isEmpty() && index[0]==0){
        SrcAllVarInfoTree * ptr = rootNode;
        index.removeAt(0);
        for(int i = 0;i<index.length();i++){
            ptr = &ptr->subNode[index[i]];
        }
        return ptr;
    }
    return NULL;
}

//获取当前对象在整个树中的索引号
IndexVN SrcAllVarInfoTree::getThisNodeIndex(){
    return thisIndex;
}

//新建一个节点
IndexVN SrcAllVarInfoTree::appendNewSubNode(){
    int i = subNode.length();
    SrcAllVarInfoTree newSubNode;
    newSubNode.rootNode = rootNode;
    newSubNode.parentNode = this;
    newSubNode.thisIndex = thisIndex;
    newSubNode.thisIndex.append(i);
    subNode.append(newSubNode);
    IndexVN index = thisIndex;
    index.append(i);
    return index;
}

//删除一个节点以及该节点下所有所有内容
bool SrcAllVarInfoTree::removeNode(IndexVN index){
    int i = index.getEndIndex();
    if(getNode(index)==NULL || i<0)return 0;
    if(getNode(index)->subNode.length()<=i)return 0;
    getNode(index)->subNode.removeAt(i);
    return 1;
}


//定义一个变量(返回定义成功后的变量索引号)
IndexVN SrcAllVarInfoTree::defineVar(VarDefineInfo &varInfo){
    if(varInfo.varName=="")return IndexVN();
    IndexVN index;
    for(int i = 0;i<thisVarTable.length();i++){
        if(thisVarTable[i].varName == varInfo.varName){
            return IndexVN();
        }
    }

    index = thisIndex;
    index.append(thisVarTable.length());
    thisVarTable.append(varInfo);
    return index;
}

//向上搜索一个变量
IndexVN SrcAllVarInfoTree::searchVar(QString varName){
    SrcAllVarInfoTree *parentNode = this;
    IndexVN index;
    while(parentNode!=NULL){
        for(int i = 0;i<parentNode->thisVarTable.length();i++){
            if(parentNode->thisVarTable[i].varName == varName){
                index = parentNode->thisIndex;
                index.append(i);
                return index;
            }
        }
        parentNode = parentNode->parentNode;
    }
    return IndexVN();
}

//获取一个变量的信息
VarDefineInfo* SrcAllVarInfoTree::getVar(IndexVN index){
    if(index.length()<2)return NULL;
    int varIndex = index.getEndIndex();
    SrcAllVarInfoTree* node = getNode(index);
    if(node==NULL)return NULL;
    if(node->thisVarTable.length()<=varIndex)return NULL;
    return &node->thisVarTable[varIndex];
}

//获取变量是全局变量还是局部变量,是全局变量返回1,局部变量0
bool SrcAllVarInfoTree::getVarIsInGlobal(IndexVN index){
    if(index.length()<2)return 0;
    if(index.length()==2 && index[0]==0)return 1;
    return 0;
}


//打印输出当前节点变量的信息
QString SrcAllVarInfoTree::thisNodeVarInfoToString(){
    QString temp;
    for(int i = 0;i<thisVarTable.length();i++){
        temp+=thisVarTable[i].toString()+"\r\n";

    }
    return temp;
}


////////////////////////////////////////////////////////////////////////////
/*                                VarDefineInfo                           */
////////////////////////////////////////////////////////////////////////////
QString VarDefineInfo::toString(){
    QString tmp = varName;
    tmp += baseTypeIndex.toString();
    if(arraySize.length()!=0){
        tmp+="Array<";
        for(int i = 0;i<arraySize.length();i++){
            tmp+="["+QString::number(arraySize[i])+"]";
        }
        tmp+=">";
    }
    for(int i = 0;i<pointerLevel;i++){
        tmp+="*";
    }
    if(isExtern){
        tmp+="<Extern>";
    }
    if(isStatic){
        tmp+="<Static>";
    }
    if(isConst){
        tmp+="<Const>";
    }
    return tmp;
}


////////////////////////////////////////////////////////////////////////////
/*                                SrcAllDataTypeTree                      */
////////////////////////////////////////////////////////////////////////////
//构建1个根节点
SrcAllDataTypeTree::SrcAllDataTypeTree(){
    thisIndex.append(0);
    rootNode = this;
    parentNode = NULL;
    thisNodeTable.thisIndex = thisIndex;
}
//创建一个子节点,并返回子节点的索引号
IndexVN SrcAllDataTypeTree::appendNewSubNode(){
    int i = subNode.length();
    SrcAllDataTypeTree newSubNode;
    newSubNode.rootNode = rootNode;
    newSubNode.parentNode = this;
    newSubNode.thisIndex = thisIndex;
    newSubNode.thisIndex.append(i);
    subNode.append(newSubNode);
    IndexVN index = thisIndex;
    index.append(i);
    return index;
}
//删除一个节点以及该节点下所有所有内容
bool SrcAllDataTypeTree::removeNode(IndexVN index){
    int i = index.getEndIndex();
    if(getNode(index)==NULL || i<0)return 0;
    if(getNode(index)->subNode.length()<=i)return 0;
    getNode(index)->subNode.removeAt(i);
    return 1;
}

//判断文本是否是一个枚举常量
bool SrcAllDataTypeTree::isEnumConstValue(QString txt){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        for(int i = 0;i<parentNode->thisNodeTable.length();i++){
            if(parentNode->thisNodeTable[i].baseType==DataBaseType_ENUM){
                if(parentNode->thisNodeTable[i].enumConstValue.contains(txt)){
                    return 1;
                }
            }
        }
        parentNode = parentNode->parentNode;
    }
    return 0;
}

//获取一个枚举常量的值
int SrcAllDataTypeTree::getEnumConstValue(QString txt,bool *isSuceess){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        for(int i = 0;i<parentNode->thisNodeTable.length();i++){
            if(parentNode->thisNodeTable[i].baseType==DataBaseType_ENUM){
                if(parentNode->thisNodeTable[i].enumConstValue.contains(txt)){
                    if(isSuceess!=NULL)*isSuceess = 1;
                    return parentNode->thisNodeTable[i].enumConstValue[txt];
                }
            }
        }
        parentNode = parentNode->parentNode;
    }
    if(isSuceess!=NULL)*isSuceess = 0;
    return 0;
}
//获取当前对象在整个树中的索引号
IndexVN SrcAllDataTypeTree::getThisNodeIndex(){
    return thisIndex;
}
//搜索一个定义的数据类型,存在则返回索引号
IndexVN SrcAllDataTypeTree::searchDataType(QStringList name){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        IndexVN index = parentNode->thisNodeTable.getTypeInfo(name);
        if(index.isEmpty()){
            parentNode = parentNode->parentNode;
        }else{
            return index;
        }
    }
    return IndexVN();
}
//向上搜寻一个非匿名结构体是否存在,存在返回索引号
IndexVN SrcAllDataTypeTree::searchStructType(QString name){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        int index = parentNode->thisNodeTable.getCustomSturctInfoIndex(name);
        if(index==-1){
            parentNode = parentNode->parentNode;
        }else{
            if(parentNode->thisNodeTable[index].baseType != DataBaseType_STRUCT)return IndexVN();
            IndexVN tableIndex = parentNode->thisIndex;
            tableIndex.append(index);
            return tableIndex;
        }
    }
    return IndexVN();
}
//向上搜寻一个非匿名共用体是否存在,存在返回索引号
IndexVN SrcAllDataTypeTree::searchUnionType(QString name){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        int index = parentNode->thisNodeTable.getCustomSturctInfoIndex(name);
        if(index==-1){
            parentNode = parentNode->parentNode;
        }else{
            if(parentNode->thisNodeTable[index].baseType != DataBaseType_UNION)return IndexVN();
            IndexVN tableIndex = parentNode->thisIndex;
            tableIndex.append(index);
            return tableIndex;
        }
    }
    return IndexVN();
}
//向上搜寻一个非匿名枚举是否存在,存在返回索引号
IndexVN SrcAllDataTypeTree::searchEnumType(QString name){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){
        int index = parentNode->thisNodeTable.getCustomSturctInfoIndex(name);
        if(index==-1){
            parentNode = parentNode->parentNode;
        }else{
            if(parentNode->thisNodeTable[index].baseType != DataBaseType_ENUM)return IndexVN();
            IndexVN tableIndex = parentNode->thisIndex;
            tableIndex.append(index);
            return tableIndex;
        }
    }
    return IndexVN();
}
//向上搜索一个变量类型映射
MappingInfo SrcAllDataTypeTree::searchMappingInfo(QStringList name,bool &status){
    SrcAllDataTypeTree *parentNode = this;
    while(parentNode!=NULL){

        if(!parentNode->thisNodeTable.typeNameMapping.contains(name)){
            parentNode = parentNode->parentNode;
        }else{
            status = 1;
            return parentNode->thisNodeTable.typeNameMapping[name];
        }
    }
    status = 0;
    return MappingInfo();
}
//向上搜素一个函数指针类型[函数指针类型的特点是其只会存储在根节点中]
IndexVN SrcAllDataTypeTree::searchFunctionType(DataTypeInfo &funTypeInfo){
    IndexVN index;
    index.append(0);
    for(int i = 0;i<rootNode->thisNodeTable.allDataType.length();i++){
        if(rootNode->thisNodeTable.allDataType[i]==funTypeInfo){
            index.append(i);
            return index;
        }
    }
    return IndexVN();
}
//获取一个子节点的引用
SrcAllDataTypeTree& SrcAllDataTypeTree::operator[](int i){
    return subNode[i];
}
//根据索引号获取一个数据类型信息对象的引用
DataTypeInfo*SrcAllDataTypeTree::getDataType(IndexVN index){
    if(index.length()>=2 && index[0]==0){
        SrcAllDataTypeTree * ptr = rootNode;
        index.removeAt(0);

        int typeIndex = index[index.length()-1];
        index.removeLast();

        for(int i = 0;i<index.length();i++){
            ptr = &ptr->operator[](index[i]);
        }
        return &ptr->thisNodeTable[typeIndex];
    }
    return NULL;
}
//根据索引号获取当前树中任意一个节点的指针
SrcAllDataTypeTree*SrcAllDataTypeTree::getNode(IndexVN index){
    if(!index.isEmpty() && index[0]==0){
        SrcAllDataTypeTree * ptr = rootNode;
        index.removeAt(0);
        for(int i = 0;i<index.length();i++){
            ptr = &ptr->operator[](index[i]);
        }
        return ptr;
    }
    return NULL;
}
//获取一个数据类型的字节数(传类型的索引号),错误返回-1
int SrcAllDataTypeTree::getTypeSize(IndexVN typeIndex){
    DataTypeInfo* typePtr = getDataType(typeIndex);
    if(typePtr==NULL){
        return -1;
    }
    return typePtr->dataBaseTypeBytes;
}
//在当前节点添加一个自定义数据类型,绑定到另一个数据类型上(相当于为数据类型起一个别名)
bool SrcAllDataTypeTree::appendNewTypeName(QString newName,QStringList bindTypeName,bool isDefaultConst,int pointerLevel){
    IndexVN bindTypeIndex = searchDataType(bindTypeName);
    if(bindTypeIndex.isEmpty())return 0;
    if(thisNodeTable.typeNameMapping.contains({newName})){
        if(thisNodeTable.typeNameMapping.value({newName}).index!=bindTypeIndex){
            return 0;
        }
    }
    int index = bindTypeIndex.getEndIndex();
    MappingInfo info = getNode(bindTypeIndex)->thisNodeTable.typeNameMapping[bindTypeName];
    info.isDefaultConst = isDefaultConst || info.isDefaultConst;
    info.pointerLevel += pointerLevel;
    bindTypeIndex.append(index);
    info.index = bindTypeIndex;
    thisNodeTable.typeNameMapping.insert({newName},info);
    return 1;
}
//在当前节点添加一个自定义数据类型,通过索引号的方式
bool SrcAllDataTypeTree::appendNewTypeName(QString newName,//类型名
                       IndexVN tableIndex,//绑定到的表
                       int typeIndex,//表中的哪个类型
                       bool isDefaultConst,int pointerLevel){
    SrcAllDataTypeTree* node = getNode(tableIndex);
    if(node==NULL)return 0;
    if(typeIndex>=node->thisNodeTable.length() || typeIndex<0){
        return 0;
    }
    if(thisNodeTable.typeNameMapping.contains({newName}))return 0;

    MappingInfo info;
    info.index = tableIndex;
    info.index.append(typeIndex);
    info.isDefaultConst = isDefaultConst;
    info.pointerLevel = pointerLevel;
    thisNodeTable.typeNameMapping.insert({newName},info);
    return 1;
}
//添加一个自定义的结构体/共用体/枚举类型(返回添加成功后的索引,如果失败返回空)
IndexVN SrcAllDataTypeTree::appendCustomSturctOrUnionOrEnum(DataTypeInfo&info){
    //判断要添加的是否已经存在,如果存在就返回其指针
    if(info.baseType != DataBaseType_STRUCT&&
       info.baseType != DataBaseType_UNION&&
       info.baseType != DataBaseType_ENUM&&
       info.baseType != DataBaseType_FUNPTR){
        return IndexVN();
    }
    IndexVN typeIndex = this->thisIndex;


    if(info.baseType == DataBaseType_STRUCT||
            info.baseType == DataBaseType_UNION||
            info.baseType == DataBaseType_ENUM){
        if(!info.isAnnony){
            if(thisNodeTable.customSturctMapping.contains(info.structName))return IndexVN();
            thisNodeTable.customSturctMapping.insert(info.structName,this->thisNodeTable.allDataType.length());
        }
    }

    typeIndex.append(this->thisNodeTable.allDataType.length());
    this->thisNodeTable.allDataType.append(info);
    return typeIndex;
}
//添加一个函数指针类型[函数指针类型的特点是其只会存储在根节点中,如果已经存在了该函数类型,直接返回类型的索引]
IndexVN SrcAllDataTypeTree::appendFunctionType(DataTypeInfo&info){
    IndexVN index;
    index.append(0);
    for(int i = 0;i<rootNode->thisNodeTable.allDataType.length();i++){
        if(rootNode->thisNodeTable.allDataType[i]==info){
            index.append(i);
            return index;
        }
    }
    index.append(rootNode->thisNodeTable.allDataType.length());
    rootNode->thisNodeTable.allDataType.append(info);
    return index;
}
//将一个类型的信息文本信息表现出来
QString SrcAllDataTypeTree::typeInfoToString(IndexVN typeIndex){
    return QString();
}

////////////////////////////////////////////////////////////////////////////
/*                                DataTypeInfoTable                       */
////////////////////////////////////////////////////////////////////////////

//根据类型名称搜索到类型信息存储的索引号,失败返回-1
IndexVN DataTypeInfoTable::getTypeInfo(QStringList name){
    if(typeNameMapping.contains(name)){
        return typeNameMapping[name].index;
    }
    return IndexVN();
}
//根据结构体/共用体/枚举的名称获取到自定义结构的信息索引号
int DataTypeInfoTable::getCustomSturctInfoIndex(QString name){

    if(customSturctMapping.contains(name)){
        return customSturctMapping[name];
    }
    return -1;
}

//判断一个数据类型是否在类型表中存在,存在返回索引号,不存在返回-1
int DataTypeInfoTable::isHaveType(DataTypeInfo&info){
    for(int i = 0;i<allDataType.length();i++){
        if(allDataType[i]==info){
            return i;
        }
    }
    return -1;
}


//根据索引号获取类型信息
DataTypeInfo& DataTypeInfoTable::operator[](int index){
    return allDataType[index];
}

//获取当前表总共有多少个类型
int DataTypeInfoTable::length(){
    return allDataType.length();
}

//打印出映射表的信息
QString DataTypeInfoTable::toStringMapInfo(){
    QString tmp;

    QList<QStringList> keys = typeNameMapping.keys();
    for(int i = 0;i<keys.length();i++){
        tmp+="["+keys[i].join(',')+"]="+
                typeNameMapping[keys[i]].index.toString()+
                (typeNameMapping[keys[i]].isDefaultConst==1 ? "<DefaultConst>" : "")+
                (typeNameMapping[keys[i]].pointerLevel!=0 ?
                "  pointerLevel:"+QString::number(typeNameMapping[keys[i]].pointerLevel) : "" )+"\r\n";
    }
    return tmp;
}
//打印出定义的结构体/共用体/枚举的名称到数据信息的映射表信息
QString DataTypeInfoTable::toStringCustomSturctMapInfo(){
    QString tmp;
    QStringList keys = customSturctMapping.keys();
    for(int i = 0;i<keys.length();i++){
        tmp+="["+keys[i]+"]="+QString::number(customSturctMapping[keys[i]])+"\r\n";
    }
    return tmp;
}

//打印数据类型向量表信息
QString DataTypeInfoTable::toStringDataTypeInfo(){
    QString tmp;
    for(int i = 0;i<allDataType.length();i++){
        tmp+="["+QString::number(i)+"]."+allDataType[i].toString()+"\r\n-------------------\r\n";
    }
    return tmp;
}


////////////////////////////////////////////////////////////////////////////
/*                                DataTypeInfo                            */
////////////////////////////////////////////////////////////////////////////
//判断两个类型是否相等
bool DataTypeInfo::operator==(const DataTypeInfo&other){
    if(funArgInfos.length()!=other.funArgInfos.length())return 0;
    if(attInfos.keys()!=other.attInfos.keys())return 0;

    for(int i = 0;i<funArgInfos.length();i++){
        if(funArgInfos[i]!=other.funArgInfos[i]){
            return 0;
        }
    }
    QStringList allKeys = attInfos.keys();
    for(int i = 0;i<allKeys.length();i++){
        if(attInfos[allKeys[i]]!=other.attInfos[allKeys[i]]){
            return 0;
        }
    }

    bool isEqu = 1;
    isEqu = isEqu && other.baseType == this->baseType;

    isEqu = isEqu && other.structName == this->structName;

    isEqu = isEqu && other.isAnnony == this->isAnnony;

    isEqu = isEqu && other.enumConstValue == this->enumConstValue;
    return isEqu;
}

//如果是结构体/共用体类型,根据索引号获取属性的信息
StructUnion_AttInfo DataTypeInfo::structGetAttFromIndex(int index,bool &status){
    status = 0;
    if(baseType!=DataBaseType_STRUCT && baseType!=DataBaseType_UNION)return StructUnion_AttInfo();

    QStringList attName = attInfos.keys();
    for(int i = 0;i<attName.length();i++){
        if(attInfos[attName[i]].attIndex == index){
            status = 1;
            return attInfos[attName[i]];
        }
    }
    return StructUnion_AttInfo();
}

//打印信息
QString DataTypeInfo::toString(){
    QString tmp;
    if(baseType==DataBaseType_FUNPTR){
        QString tmp = isStdcallFun ? "STDCALLFUN_PTR{\r\n" :"FUN_PTR{\r\n";
        if(funArgInfos.length()==0){
            return "ERROR";
        }
        tmp += "\tRETURN:Index>"+funArgInfos[0].typeIndex.toString()+"";
        for(int i = 0;i<funArgInfos[0].pointerLevel;i++){
            tmp+='*';
        }
        tmp+="\r\n";
        tmp += "\tARG:\r\n";
        for(int i = 1;i<funArgInfos.length();i++){
            StructUnion_AttInfo &att = funArgInfos[i];
            tmp += "\tIndex>"+att.typeIndex.toString();
            for(int i = 0;i<att.pointerLevel;i++){
                tmp+='*';
            }
            if(att.isConst){
                tmp+="<const>";
            }
            tmp+="\r\n";
        }

        tmp += "}";
        return tmp;
    }if(baseType==DataBaseType_ENUM){
        tmp = "ENUM";
        if(!isAnnony){
            tmp+= "(" + structName + "){\r\n";
        }else{
            tmp+="{\r\n";
        }
        QStringList allKey = enumConstValue.keys();
        for(int i = 0;i<allKey.length();i++){
            tmp += "\t"+allKey[i]+":"+QString::number(enumConstValue[allKey[i]])+"\r\n";
        }
        tmp+="}";
        return tmp;
    }else if(baseType==DataBaseType_STRUCT || baseType==DataBaseType_UNION){
        QString tmp;

        tmp = baseType==DataBaseType_STRUCT ? "STRUCT" : "UNION";

        if(!isAnnony){
            tmp+= "(" + structName + "){\r\n";
        }else{
            tmp+="{\r\n";
        }


        QStringList allKey = attInfos.keys();
        for(int i = 0;i<allKey.length();i++){
            StructUnion_AttInfo &att = attInfos[allKey[i]];

            tmp += "\t"+allKey[i]+": off>("+
                    QString::number(att.offset)+")type>"+
                    att.typeIndex.toString();
            if(att.arraySize.length()>0){
                tmp+="Array>";
            }
            for(int i = 0;i<att.arraySize.length();i++){
                tmp+="["+QString::number(att.arraySize[i])+"]";
            }
            for(int i = 0;i<att.pointerLevel;i++){
                tmp+='*';
            }
            if(att.isConst){
                tmp+="<const>";
            }
            tmp+="\r\n";
        }
        tmp+="}";
        return tmp;
    }else{
        switch (baseType) {
            case DataBaseType_INT:tmp= "INT";break;
            case DataBaseType_SHORT:tmp= "SHORT";break;
            case DataBaseType_CHAR:tmp= "BYTE";break;
            case DataBaseType_LONG:tmp= "LONG";break;
            case DataBaseType_UINT:tmp= "UINT";break;
            case DataBaseType_USHORT:tmp= "USHORT";break;
            case DataBaseType_UCHAR:tmp= "UBYTE";break;
            case DataBaseType_ULONG:tmp= "ULONG";break;
            case DataBaseType_FLOAT:tmp= "FLOAT";break;
            case DataBaseType_DOUBLE:tmp= "DOUBLE";break;
            case DataBaseType_VOID:tmp= "VOID";break;
            default:return "ERROR";
        }



        return tmp;
    }
}


////////////////////////////////////////////////////////////////////////////
/*                              StructUnion_AttInfo                       */
////////////////////////////////////////////////////////////////////////////
bool StructUnion_AttInfo::operator==(const StructUnion_AttInfo&other){
    bool isEqu = 1;
    isEqu = isEqu && other.arraySize == this->arraySize;
    isEqu = isEqu && other.pointerLevel == this->pointerLevel;
    isEqu = isEqu && other.offset == this->offset;
    isEqu = isEqu && other.isConst == this->isConst;
    isEqu = isEqu && other.typeIndex == this->typeIndex;
    return isEqu;
}

bool StructUnion_AttInfo::operator!=(const StructUnion_AttInfo&other){
    return !this->operator==(other);
}

////////////////////////////////////////////////////////////////////////////
/*                              SrcObjectInfo                             */
////////////////////////////////////////////////////////////////////////////
//添加一条提示信息
void SrcObjectInfo::appendPrompt(QString prompt,QString srcPath,int line,int col){
    PromptInfo info;
    info.prompt = prompt;
    info.srcPath = srcPath;
    info.line = line;
    info.col = col;
    allPromptInfos.append(info);
}

//获取当前正在解析的内容
CurrentAnalysisType SrcObjectInfo::getCurrentAnalysisType(){
    return currentAnalysisType[currentAnalysisType.length()-1];
}

//获取当前正在操作的数据类型表索引号
IndexVN SrcObjectInfo::getCurrentVarTableIndex(){
    return currentVarTableIndex;
}

//获取当前正在操作的变量定义表索引号
IndexVN SrcObjectInfo::getCurrentTypeTableIndex(){
    return currentTypeTableIndex;
}
//获取数据类型树的根节点指针
SrcAllDataTypeTree* SrcObjectInfo::getRootDataTypeInfoNode(){
    return &allDefineType;
}
SrcAllVarInfoTree* SrcObjectInfo::getRootVarDefineInfoNode(){
    return &allDefineVar;
}
//获取当前正在操作的数据类型树的节点指针
SrcAllDataTypeTree* SrcObjectInfo::getDataTypeInfoNode(){
    return getDataTypeInfoNode(getCurrentTypeTableIndex());
}
SrcAllVarInfoTree* SrcObjectInfo::getVarDefineInfoNode(){
    return getVarDefineInfoNode(getCurrentVarTableIndex());
}
//根据索引号获取数据类型树节点的指针
SrcAllDataTypeTree* SrcObjectInfo::getDataTypeInfoNode(IndexVN index){
    return allDefineType.getNode(index);
}
//根据索引号获取变量定义表的指针
SrcAllVarInfoTree* SrcObjectInfo::getVarDefineInfoNode(IndexVN index){
    return allDefineVar.getNode(index);
}
//搜索一个函数的索引号
int SrcObjectInfo::serachFunction(QString funName){
    for(int i = 0;i<allDefineFunction.length();i++){
        if(allDefineFunction[i].functionName == funName)return i;
    }
    return -1;
}
//添加一个函数信息,返回成功后的函数索引号,失败-1
int SrcObjectInfo::appendFunctionDefine(FunctionInfo&info){
    if(info.functionName == "")return -1;

    for(int i = 0;i<allDefineFunction.length();i++){
        if(allDefineFunction[i].functionName == info.functionName){
            if(info != allDefineFunction[i]){
                return -1;
            }else{
                if(!allDefineFunction[i].isOnlyStatement){
                    return -1;
                }
                allDefineFunction[i].isOnlyStatement = info.isOnlyStatement;
                allDefineFunction[i].isStatic = info.isStatic;
                return i;
            }
        }
    }

    //判断是否是变量名
    QList<VarDefineInfo>& thisVarTable = getVarDefineInfoNode()->thisVarTable;
    for(int i = 0;i<thisVarTable.length();i++){
        if(info.functionName==thisVarTable[i].varName)return -1;
    }

    //判断是否是一个枚举常量
    DataTypeInfoTable &thisDataTypeTable = getDataTypeInfoNode()->thisNodeTable;
    for(int i = 0;i<thisDataTypeTable.length();i++){
        if(thisDataTypeTable[i].baseType == DataBaseType_ENUM){
            if(thisDataTypeTable[i].enumConstValue.contains(info.functionName))return -1;
        }
    }
    if(!getDataTypeInfoNode()->thisNodeTable.getTypeInfo({info.functionName}).isEmpty())return 0;

    allDefineFunction.append(info);
    return allDefineFunction.length()-1;
}
//根据索引号获取函数指针
FunctionInfo*SrcObjectInfo::getFunctionInfo(int index){
    if(index<0 || index>=allDefineFunction.length())return NULL;
    return &allDefineFunction[index];
}
//初始化
SrcObjectInfo::SrcObjectInfo(){
    currentVarTableIndex.append(0);
    currentTypeTableIndex.append(0);
    currentAnalysisType.append(CurrentAnalysis_Global);
    rootMiddleNode.nodeType = "PSDL";
    PsdlIR_Compiler::MiddleNode_Att att;
    att.attName = "BODY";
    rootMiddleNode.atts.append(att);
}
//开始解析一个结构体的属性定义
//structInfo:当前数据结构的类型定义信息(对于属性中指向的类型指针可为无效)
bool SrcObjectInfo::startAnalysisSturctUnionAtt(){
    currentAnalysisType.append(CurrentAnalysis_SturctUnionAtt);
    currentTypeTableIndex = getDataTypeInfoNode()->appendNewSubNode();
    return 1;
}
//结构体的属性定义解析完成
bool SrcObjectInfo::endAnalysisSturctUnionAtt(){
    if(getCurrentAnalysisType()!=CurrentAnalysis_SturctUnionAtt&&
       getCurrentAnalysisType()!=CurrentAnalysis_Global)return 0;
    currentAnalysisType.removeLast();
    currentTypeTableIndex.removeLast();
    return 1;
}
//开始解析一个流程块
bool SrcObjectInfo::startAnalysisProcess(ProcessType currentProcessType){
    if(getCurrentAnalysisType()==CurrentAnalysis_SturctUnionAtt && getCurrentAnalysisType()==CurrentAnalysis_Global)return 0;
    else if(currentProcessType==funBodyProcess)return 0;
    this->currentProcessType.append(currentProcessType);
    if(currentProcessType==whileProcess){
        WhileBlockInfo whileBlockInfo;
        whileBlockInfo.beginMark = "@whileBegin"+QString::number(currentFuncHaveWhileCount);
        whileBlockInfo.endMark = "@whileEnd"+QString::number(currentFuncHaveWhileCount);
        currentFuncHaveWhileCount++;
        funcWhileBlockInfo.append(whileBlockInfo);
    }else if(currentProcessType==switchProcess){
        WhileBlockInfo whileBlockInfo;
        whileBlockInfo.isSwitch = 1;
        whileBlockInfo.endMark = "@switchEnd"+QString::number(currentFuncHaveWhileCount);
        currentFuncHaveWhileCount++;
        funcWhileBlockInfo.append(whileBlockInfo);
    }

    currentVarTableIndex = getVarDefineInfoNode()->appendNewSubNode();
    currentTypeTableIndex = getDataTypeInfoNode()->appendNewSubNode();
    currentAnalysisType.append(CurrentAnalysis_Process);
    return 1;
}
//流程块解析完成
bool SrcObjectInfo::endAnalysisProcess(){
    if(getCurrentAnalysisType()!=CurrentAnalysis_Process)return 0;
    if(allDefineVar.removeNode(currentVarTableIndex)==0)return 0;
    currentTypeTableIndex.removeLast();
    currentAnalysisType.removeLast();
    currentVarTableIndex.removeLast();

    ProcessType currentProcessType = this->currentProcessType.pop();
    if(currentProcessType==whileProcess || currentProcessType==switchProcess){
        funcWhileBlockInfo.pop();
    }
    return 1;
}
//开始解析一个函数
bool SrcObjectInfo::startAnalysisFunction(){
    if(getCurrentAnalysisType()==CurrentAnalysis_SturctUnionAtt||
       getCurrentAnalysisType()==CurrentAnalysis_Process||
       getCurrentAnalysisType()==CurrentAnalysis_Function)return 0;
    currentAnalysisType.append(CurrentAnalysis_Function);
    currentVarTableIndex = getVarDefineInfoNode()->appendNewSubNode();
    currentTypeTableIndex = getDataTypeInfoNode()->appendNewSubNode();
    this->currentProcessType.append(funBodyProcess);
    currentFuncHaveWhileCount = 0;
    currentFuncVarCount = 0;
    currentFuncTmpVarCount = 0;
    currentFuncLinkVarCount = 0;
    currentAnonyMarkCount = 0;
    defMarks.clear();
    jumpMarks.clear();
    return 1;
}
//函数解析完成
bool SrcObjectInfo::endAnalysisFunction(){
    if(getCurrentAnalysisType()!=CurrentAnalysis_Function)return 0;
    if(allDefineVar.removeNode(currentVarTableIndex)==0)return 0;
    currentTypeTableIndex.removeLast();
    currentAnalysisType.removeLast();
    currentVarTableIndex.removeLast();

    this->currentProcessType.pop();
    return 1;
}
//当前要定义一个数据类型,判断定义的名称是否合规
//A-Z/a-z/0-9/_构成的文本,不得为关键字[生成词组流时已经检查了,无需再检查]
//不得为当前变量表中存在的变量名
//如果当前是在全局解析中,不得为已存在的函数名
//不得为当前类型表中的枚举常量
bool SrcObjectInfo::judegDefTypeNameIsCopce(QString name){

    //判断是否是变量名
    QList<VarDefineInfo>& thisVarTable = getVarDefineInfoNode()->thisVarTable;
    for(int i = 0;i<thisVarTable.length();i++){
        if(name==thisVarTable[i].varName)return 0;
    }


    //判断是否是函数名
    if(getCurrentAnalysisType()==CurrentAnalysis_Global){
        for(int i = 0;i<allDefineFunction.length();i++){
            if(allDefineFunction[i].functionName==name){
                return 0;
            }
        }
    }

    //判断是否是一个枚举常量
    DataTypeInfoTable &thisDataTypeTable = getDataTypeInfoNode()->thisNodeTable;
    for(int i = 0;i<thisDataTypeTable.length();i++){
        if(thisDataTypeTable[i].baseType == DataBaseType_ENUM){
            if(thisDataTypeTable[i].enumConstValue.contains(name))return 0;
        }
    }
    return 1;
}
//当前要定义一个变量/枚举常量,判断定义的名称是否合规
//在judegDefTypeNameIsCopce的基础上
//不得为当前类型表中结构体/共用体/枚举体的名
//不得为已经存在的类型名
bool SrcObjectInfo::judegDefineVarNameIsCopce(QString name){
    if(!judegDefTypeNameIsCopce(name))return 0;
    DataTypeInfoTable &thisDataTypeTable = getDataTypeInfoNode()->thisNodeTable;
    if(!thisDataTypeTable.getTypeInfo({name}).isEmpty())return 0;
    return !thisDataTypeTable.customSturctMapping.contains(name);
}
//打印输出当前所有函数的信息
QString SrcObjectInfo::allFunctionInfotoString(){
    QString temp;
    for(int i = 0;i<allDefineFunction.length();i++){
        temp += allDefineFunction[i].toString()+"\r\n";
    }
    return temp;
}

//全局区中新增一个全局匿名字符串常量,返回常量名
QString SrcObjectInfo::appendAnonyConstStrValue(QString strValue,int&strByteSize){
    QString varName = "@AnonyStrValue"+QString::number(anonyStrValueCount);
    anonyStrValueCount++;

    QByteArray stringBytes = replaceTransChar(strValue,this->cstringCodeSystem);
    strByteSize = stringBytes.length();
    InitBlock initBlock(strByteSize,CPU_BIT,ramEnd);
    initBlock.insertData(0,stringBytes);

    PsdlIR_Compiler::MiddleNode varDef = initBlock.toPSDLVarDefineNode(varName,1,1);
    rootMiddleNode.getAttFromName("BODY")->subNodes.append(varDef);
    return varName;
}
}
}
QT_END_NAMESPACE
