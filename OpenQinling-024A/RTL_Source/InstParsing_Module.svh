`ifndef InstParsing_Module_SVH
`define InstParsing_Module_SVH


//024A系统内置异常中断号
typedef enum logic[7:0]{
    SystemInterruptCode_AnalysisCodeError = 0,//解析编码出错
    SystemInterruptCode_DivisionOperationError = 1,//除法运算出错
    SystemInterruptCode_MemoryAccessError = 2//内存访问出错
}SystemInterruptCode;

//024A指令大类编号
typedef enum logic[2:0]{
    InstTypeCode_SysCtrl = 0, //系统控制指令
    InstTypeCode_IOStack = 1, //全局内存读写/出入栈指令
    InstTypeCode_StackRW = 2, //堆栈读写指令
    InstTypeCode_BaseOper = 3,//基本整数运算指令(整数四则、布尔运算、按位运算、移位运算、INT64/UINT64大小比较、通用相等比较)
    InstTypeCode_ExpOper = 4, //扩展运算指令(浮点四则、浮整转换、FLOAT64大小比较、三角函数等)
    InstTypeCode_MovInit = 5, //转移/赋值指令
    InstTypeCode_Jemp = 6,    //程序跳转、函数调用/返回指令
    InstTypeCode_NOP = 7      //保留，无用(用于在024A指令集基础上自定义指令)
}InstTypeCode;

////////////////////////////////初始化转移数据相关/////////////////////////////////////
typedef enum logic[0:0] {
    InstTypeCode_MovInitType_MOV = 0, //用于完成寄存器间数据转移、(INT16/INT8/UINT16/UINT8)数据初始化(1个tick)
    InstTypeCode_MovInitType_INIT = 1//用于完成64位数据初始化(3个tick)
}InstTypeCode_MovInitType;



////////////////////////////////系统控制相关/////////////////////////////////////
//024A 系统控制指令 的子类编号
typedef enum logic[3:0] {
    InstTypeCode_SysCtrlType_Restart = 0, //重启
    InstTypeCode_SysCtrlType_SoftInt = 1, //软中断
    InstTypeCode_SysCtrlType_AccCtrl = 2 //权限控制
}InstTypeCode_SysCtrlType;
//024A 权限控制 的子类编号
typedef enum logic[24:0] {
    InstTypeCode_SysCtrl_AccCtrlType_EnableInt = 0,  //启用中断响应
    InstTypeCode_SysCtrl_AccCtrlType_DisableInt = 1, //关闭中断响应
    InstTypeCode_SysCtrl_AccCtrlType_EnableProtect = 2 //启用保护模式
}InstTypeCode_SysCtrl_AccCtrlType;

////////////////////////////////全局内存读写/出入栈指令相关/////////////////////////
typedef enum logic {
    InstTypeCode_IOStackType_IO = 0, //内存读写
    InstTypeCode_IOStackType_SK = 1 //堆栈读写
}InstTypeCode_IOStackType;


////////////////////////////////程序跳转相关//////////////////////////////////////
//024A 程序跳转、函数调用/返回指令 的子类编号
typedef enum logic {
    InstTypeCode_JempType_ShortJmp = 0, //短跳转
    InstTypeCode_JempType_LongJmp = 1 //长跳转
}InstTypeCode_JempType;

//024A 程序短跳转指令 的子类编号
typedef enum logic {
    InstTypeCode_Jemp_ShortJmpType_JMP = 0, //直接跳转
    InstTypeCode_Jemp_ShortJmpType_JNE = 1 //分支跳转
}InstTypeCode_Jemp_ShortJmpType;

//024A 程序长跳转指令 的子类编号
typedef enum logic[26:0] {
    InstTypeCode_Jemp_LongJmpType_RT = 0, //函数调用和返回
    InstTypeCode_Jemp_LongJmpType_IT = 1, //中断处理退出
    InstTypeCode_Jemp_LongJmpType_EIT = 2 //中断处理退出同时开启中断响应
}InstTypeCode_Jemp_LongJmpType;

//////////////////////////////////基本运算相关///////////////////////////////
typedef enum logic[4:0] {
    //四则运算
    InstTypeCode_BaseOperType_ADD,
    InstTypeCode_BaseOperType_SUB,
    InstTypeCode_BaseOperType_MUL,
    InstTypeCode_BaseOperType_UDIV,
    InstTypeCode_BaseOperType_IDIV,
    InstTypeCode_BaseOperType_UREM,
    InstTypeCode_BaseOperType_IREM,
    //布尔运算
    InstTypeCode_BaseOperType_BAND,
    InstTypeCode_BaseOperType_BOR,
    InstTypeCode_BaseOperType_BNOT,
    //位运算
    InstTypeCode_BaseOperType_AND,
    InstTypeCode_BaseOperType_OR,
    InstTypeCode_BaseOperType_NOT,
    InstTypeCode_BaseOperType_XOR,
    InstTypeCode_BaseOperType_SAL,
    InstTypeCode_BaseOperType_USOR,
    InstTypeCode_BaseOperType_ISOR,
    //通用相等比较
    InstTypeCode_BaseOperType_EC,
    InstTypeCode_BaseOperType_NEC,
    //无符号大小比较
    InstTypeCode_BaseOperType_ULC,
    InstTypeCode_BaseOperType_ULEC,
    InstTypeCode_BaseOperType_UMEC,
    InstTypeCode_BaseOperType_UMC,
    
    //有符号大小比较
    InstTypeCode_BaseOperType_ILC,
    InstTypeCode_BaseOperType_ILEC,
    InstTypeCode_BaseOperType_IMEC,
    InstTypeCode_BaseOperType_IMC,

    //位限制
    InstTypeCode_BaseOperType_LDI8,
    InstTypeCode_BaseOperType_LDU16,
    InstTypeCode_BaseOperType_LDI16,
    InstTypeCode_BaseOperType_LDU32,
    InstTypeCode_BaseOperType_LDI32
}InstTypeCode_BaseOperType;

typedef enum logic[4:0] {
    //四则运算
    InstTypeCode_ExpOperType_FADD,
    InstTypeCode_ExpOperType_FSUB,
    InstTypeCode_ExpOperType_FMUL,
    InstTypeCode_ExpOperType_FDIV,
    //浮点大小比较
    InstTypeCode_ExpOperType_FLC,
    InstTypeCode_ExpOperType_FLEC,
    InstTypeCode_ExpOperType_FMEC,
    InstTypeCode_ExpOperType_FMC,
    //浮整转换
    InstTypeCode_ExpOperType_FTI,
    InstTypeCode_ExpOperType_ITF,
    InstTypeCode_ExpOperType_UTF
}InstTypeCode_ExpOperType;

`endif


/*024A指令集 RISC[31:0]等长指令
[31:29] 指令大类编号 [28:0]各类指令自行分配区域

大类编号[2:0]:
    0. 系统控制指令
    1. 全局内存读写/出入栈指令
    2. 堆栈读写指令
    3. 基本整数运算指令(整数四则、布尔运算、按位运算、移位运算、INT64/UINT64大小比较、通用相等比较)
    4. 扩展运算指令(浮点四则、浮整转换、FLOAT64大小比较、三角函数等)
    5. 转移/赋值指令
    6. 程序跳转、函数调用/返回指令
    7. 保留，默认无用(NOP指令)

0.系统控制指令类型
    [28:25] 系统控制指令子类型 [24:0]子类型指令自行分配区域
    子类型编号:
        0:  重启CPU
            [24:0]保留无用
        1:  软中断请求
            [24:8]保留无用  [7:0]软中断号
            中断号分配策略:
                0-15: 强制用于处理器运行异常的中断:
                    0: 指令解析有误
                    1: 除法/取余时除数为0
                    2: 内存读写无响应/缺页
                    3-15: 保留
                16-255: 自由分配给软中断或外部中断
        2:  权限控制
            [24:0] 权限控制类型:
                (中断默认关闭)
                0:  启用中断响应
                1:  关闭中断响应
                (系统保护模式默认关闭)
                2:  启用系统保护
                (其它编号保留)
        3-15:   保留(无用)

1.全局内存读写/出入栈指令类型
    [28] 读写内存/出入栈选择 0:内存读写 1:出入栈
    [27:24] 读写类型:  
        [27]读还是写: 0读 1写
        读数据                      写数据 
        0.R-UINT8                   8.W-UINT8/INT8
        1.R-INT8                    9.W-UINT16/INT16
        2.R-UINT16                  10.W-UINT32/INT32
        3.R-INT16                   11.W-FLOAT32
        4.R-UINT32                  12.W-UINT64/INT64/FLOAT64
        5.R-INT32                   13-15:保留
        6.R-FLOAT32
        7.R-UINT64/INT64/FLOAT64
    
    0.内存读写: 
        [23:19] 读写数据的寄存器号
        [18:14] 读写地址的寄存器号
        [13:0]  读写地址的立即数(如果地址寄存器有效，最终读写地址=地址寄存器+立即数，否则读写地址等于立即数。正负8KB,自动扩展符号位)
    1.出入栈:
        [23:19] 读写数据的寄存器号
        [18:0]  如果是入栈，且数据寄存器号无效，则最终写入的数据为INT19类型(自动扩展符号位)
    
2.堆栈读写指令
    [28:25] 读写类型: 
        [28]读还是写: 0读 1写
        读数据                      写数据 
        0.R-UINT8                   8.W-UINT8/INT8
        1.R-INT8                    9.W-UINT16/INT16
        2.R-UINT16                  10.W-UINT32/INT32
        3.R-INT16                   11.W-FLOAT32
        4.R-UINT32                  12.W-UINT64/INT64/FLOAT64
        5.R-INT32                   13-15:保留
        6.R-FLOAT32
        7.R-UINT64/INT64/FLOAT64
    [24:20] 读写数据的寄存器号
    [19:0]  读写的堆栈地址立即数(最终读写位置等于SP+立即数 正1MB范围)

3.基本整数运算指令(整数四则、布尔运算、按位运算、移位运算、INT64/UINT64大小比较、通用相等比较)
    [28:24] 子指令类型:
        ----------------------四则运算--------------------
        0.加法ADD          1.减法SUB          2.乘法MUL          3.无符号除法UDIV
        4.有符号除法IDIV    5.无符号取余UREM     6.有符号取余IREM
        ----------------------布尔运算--------------------
        7.布尔与BAND        8.布尔或BOR        9.布尔非BNOT
        -----------------------位运算---------------------
        10.位与AND         11.位或OR         12.位非NOT         13.位异或XOR
        14.左移SAL         15.无符号右移USOR       16.有符号右移ISOR
        ----------------------相等比较--------------------
        17.通用相等比较EC  18.通用不相等比较NEC
        --------------------无符号大小比较-----------------
        19.小于ULC         20.小于等于ULEC     21.大于等于UMEC      22.大于UMC
        --------------------有符号大小比较-----------------
        23.小于ILC         24.小于等于ILEC     25.大于等于IMEC      26.大于IMC
        --------------------位限制----------------
        27.LDI8             28.LDU16
        29.LDI16            30.LDU32
        31.LDI32

    [23:19] 目标寄存器号
    [18:14] 参数A寄存器号   (布尔非/位非,该位无用)
    [13:9]  参数B寄存器号
    [8:0]   参数B的立即数(INT9类型，可为正负255,自动扩展为INT64)
    
4.扩展运算指令(浮点四则、浮整转换、FLOAT64大小比较、(三角函数、指数、对数 【该内核暂不支持】)等)
    [28:24] 子指令类型:
        --------------------浮点四则运算-------------------
        0.加法FADD          1.减法FSUB          2.乘法FMUL          3.除法FDIV
        --------------------浮点大小比较-------------------
        4.小于FLC         5.小于等于FLEC     6.大于等于FMEC      7.大于FMC
        ----------------------浮整转换---------------------
        8.FLOAT64转INT64 FTI    9.INT64转FLOAT64 ITF
        10.UINT64转FLOAT64 UTF
        -------------------【11-31】保留-------------------
    [23:19] 目标寄存器号
    [18:14] 参数A寄存器号   (浮整转换,该位无用)
    [13:9]  参数B寄存器号
    [8:0]   参数B的立即数(INT9类型，可为正负255,自动扩展为INT64)

5.转移/赋值指令
    [28:28] 子指令类型:
    0.MOV数据转移指令(初始化INT18的值,正负131072)
        [27:23] 目标寄存器号
        [22:18] 源寄存器号
        [17:0]  初始值立即数
    --------------------------------------------------------
    1.INIT 64位初始化(3次完成)、40位地址初始化(2次完成)指令[先左移目标寄存器23位,随后与23位的初始化值相或]
        [27:23] 目标寄存器号
        [22:0] 初始值

    基于MOV / INIT指令组合而成的伪指令:
        IT64 : 连续执行3次INIT指令 用于完成一个64位值的初始化
        IT40 : 执行1次MOV指令+1次INIT指令,用于完成INT40/UINT40的数据类型的初始化
                (可用于完成一个内存地址指针的初始化>=40bit的整形数据初始化)

6. 程序跳转、函数调用/返回指令
    [28:28]  子指令类型:
    0.程序段内的短跳转 (可在128MB范围内跳转)
        [27:27] 子指令类型
        0.JMP 立即跳转
            [26:0]跳转地址立即数  最终地址= {PC[39:27],立即数[26:0]}
        1.JNE 分支跳转
            [26:0]跳转地址立即数  最终地址= {PC[39:27],立即数[26:0]} 由BJS寄存器确定是否要跳转
    1.函数间的调用、退出
        [27:0]确定是哪种链接指令
        0.RT 普通函数调用、退出 (对调PC的值+4后存入TPC,同时TPC的值写入PC)
            在调用函数时，先将TPC设置为函数地址，再调用该指令完成跳转
            在函数退出时，直接调用该指令，便能回到原调用点
        1.IT  一般中断退出 (IPC的值写入PC)
        2.EIT 一般中断退出 (IPC的值写入PC，同时设置SYS寄存器开启中断响应)
    【3保留无用】

*/