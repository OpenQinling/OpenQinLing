#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#include <QObject>
#include <QList>

class WORD;
class DWORD;
class QWORD;
typedef  QList<WORD> QWordArray;
typedef  QList<DWORD> QDwordArray;
typedef  QList<QWORD> QQwordArray;
//2byte数据类型
class WORD{
public:
    //c数据类型转换 为dword
    WORD();
    WORD(ushort d);
    WORD(short d);
    WORD(uchar d0,uchar d1);//dword={d0,d1}
    WORD& operator=(const WORD &d);


    //转换为c数据类型
    ushort toUShort();
    short toShort();

    //转换为char[2]
    QByteArray toByteArray();

    uchar& operator[](int i);

    //打印输出信息
    QString toHexString();
private:
    uchar data[2] = {0xff,0xff};
};
//4byte数据类型
class DWORD{
public:
    //c数据类型转换 为dword
    DWORD();
    DWORD(uint d);
    DWORD(float d);
    DWORD(int d);
    DWORD(WORD d0,WORD d1);//dword={d0,d1}
    DWORD(uchar d0,uchar d1,uchar d2,uchar d3);//dword={d0,d1,d2,d3}
    DWORD& operator=(const DWORD &d);


    //转换为c数据类型
    uint toUInt();
    int toInt();
    float toFloat();

    //转换为char[4]
    QByteArray toByteArray();
    QWordArray toWordArray();


    QString toHexString();

    //转换为
    uchar& operator[](int i);
private:
    uchar data[4] = {0xff,0xff,0xff,0xff};
};

//8byte数据类型
class QWORD{
public:
    //c数据类型转换 为dword
    QWORD();
    QWORD(unsigned long long d);
    QWORD(double d);
    QWORD(long long d);
    QWORD(DWORD d0,DWORD d1);
    QWORD(WORD d0,WORD d1,WORD d2,WORD d3);
    QWORD(uchar d0,uchar d1,uchar d2,uchar d3,
          uchar d4,uchar d5,uchar d6,uchar d7);
    QWORD& operator=(const QWORD &d);


    //转换为c数据类型
    unsigned long long toULong();
    long long toLong();
    double toDouble();

    //转换为char[4]
    QByteArray toByteArray();
    QWordArray toWordArray();
    QDwordArray toDWordArray();

    QString toHexString();

    //转换为
    uchar& operator[](int i);
private:
    uchar data[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
};


QDwordArray ByteToDwordArray(QByteArray array);
QWordArray ByteToWordArray(QByteArray array);
QQwordArray ByteToQwordArray(QByteArray array);
QDwordArray WordToDwordArray(QWordArray array);
QByteArray WordToByteArray(QWordArray array);
QQwordArray WordToQwordArray(QWordArray array);
QWordArray DwordToWordArray(QDwordArray array);
QByteArray DwordToByteArray(QDwordArray array);
QQwordArray DwordToQwordArray(QDwordArray array);
QWordArray QwordToWordArray(QQwordArray array);
QByteArray QwordToByteArray(QQwordArray array);
QDwordArray QwordToDwordArray(QQwordArray array);

#endif // DATASTRUCT_H
