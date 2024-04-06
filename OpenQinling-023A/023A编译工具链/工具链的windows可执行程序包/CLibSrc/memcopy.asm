#作用:函数传递数组型变量时拷贝实参到形参
#参数: (uint 基地址,uint 长度)
#[sp]-传入的数组基地址
#[sp-4]-传入的数组长度
CODE-EXPORT funArgArrayCopy{
    PUSH DWORD,r1;
    PUSH DWORD,r2;
    PUSH DWORD,r3;
    PUSH DWORD,r4;
    SRD DWORD,r1,16d;#实参基址
    SRD DWORD,r2,20d;#实参字节数
    
    UADD r3,sp,24D;#形参基址
    UADD r4,r3,r2;#形参尾址+1
    while>
        ULC r3,r4;
    JNE [this.end];
        RD BYTE,r2,r1;
        WE BYTE,r2,r3;
        UADD r3,1d;
        UADD r1,1d;
        JMP [this.while];
    end>
    POP DWORD,r4;
    POP DWORD,r3;
    POP DWORD,r2;
    POP DWORD,r1;
    UADD SP,8d;
    RT;
};
