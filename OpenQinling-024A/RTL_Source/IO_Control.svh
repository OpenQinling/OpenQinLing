
`ifndef IO_Control_SVH
`define IO_Control_SVH

//读写功能枚举
typedef enum logic[3:0]{
    //[2]: 0读数据 1:写数据
    //[1:0]: 读写的数据类型

    //读模式
    IOModes__READ_UBYTE = 0,
    IOModes__READ_BYTE = 1,
    IOModes__READ_USHORT = 2,
    IOModes__READ_SHORT = 3,
    IOModes__READ_UINT = 4,
    IOModes__READ_INT = 5,
    IOModes__READ_FLOAT = 6,
    IOModes__READ_ULONG_LONG_DOUBLE = 7,
    
    //写模式
    IOModes__WRITE_UBYTE_BYTE = 8,
    IOModes__WRITE_USHORT_SHORT = 9,
    IOModes__WRITE_UINT_INT = 10,
    IOModes__WRITE_FLOAT = 11,
    IOModes__WRITE_ULONG_LONG_DOUBLE = 12
}IOModes_t;

`endif
