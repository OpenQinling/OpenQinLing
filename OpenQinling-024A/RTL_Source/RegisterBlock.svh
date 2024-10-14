`ifndef RegisterBlock_SVH
`define RegisterBlock_SVH
//CPU寄存器堆的寄存器编号
typedef enum logic[4:0]{
    //通用数据寄存器(DATA)
    RegisterBlockCode_R0_D0 = 0,
    RegisterBlockCode_R1_D1,
    RegisterBlockCode_R2_D2,
    RegisterBlockCode_R3_D3,
    RegisterBlockCode_R4_D4,
    RegisterBlockCode_R5_D5,
    RegisterBlockCode_R6_D6,
    RegisterBlockCode_R7_D7,
    RegisterBlockCode_R8_D8,
    RegisterBlockCode_R9_D9,
    RegisterBlockCode_R10_D10,
    RegisterBlockCode_R11_D11,
    RegisterBlockCode_R12_D12,
    RegisterBlockCode_R13_D13,
    RegisterBlockCode_R14_D14,
    RegisterBlockCode_R15_D15,
    RegisterBlockCode_R16_D16,
    RegisterBlockCode_R17_D17,
    RegisterBlockCode_R18_D18,
    RegisterBlockCode_R19_D19,

    //函数实参与返回值保存的寄存器(Parameter Value & Return Value)
    //在C/C++等高级语言调用函数时，024A支持前4个64位数据放在寄存器中传输/一个64位数据通过寄存器返回
    RegisterBlockCode_R20_RV,//函数返回值寄存器
    RegisterBlockCode_R21_PV0,//函数参数0寄存器
    RegisterBlockCode_R22_PV1,//函数参数1寄存器
    RegisterBlockCode_R23_PV2,//函数参数2寄存器
    RegisterBlockCode_R24_PV3,//函数参数3寄存器
    
    //分支跳转选择寄存器(Branch jump selection)
    RegisterBlockCode_R25_BJS,//分支跳转指令根据该寄存器中的值确定是否要跳转(JNE为0跳转，非0就不跳转)
    
    //CPU内部功能的寄存器(CPU System)
    RegisterBlockCode_R26_SP,//栈顶寄存器
    RegisterBlockCode_R27_TPC,//普通函数链接寄存器
    RegisterBlockCode_R28_IPC,//中断处理函数链接寄存器
    RegisterBlockCode_R29_PC,//指令地址寄存器[只读]
    RegisterBlockCode_R30_SYS,//CPU模式寄存器[只读]
    // SYS[0] CPU中断响应是否开启
    // SYS[1] 系统保护模式是否启用（保护模式只有进入中断处理时才有可能被关闭)
    //保护模式启用以后有以下限制: 1.无法再对SYS做修改

    //无效寄存器号
    RegisterBlockCode_IMME_INVALID//无效，或者表示使用的是纯立即数
}RegisterBlockCode;

`endif