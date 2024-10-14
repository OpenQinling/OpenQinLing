`include "RegisterBlock.svh"

`ifndef CoreMod_Interface_SVH
`define CoreMod_Interface_SVH
typedef enum logic[2:0]{
    CpuGotoAskType_NOTJMP = 0,
    CpuGotoAskType_JMP_JNE = 1,
    CpuGotoAskType_TPC = 2,
    CpuGotoAskType_IPC = 3
}CpuGotoAskTypeCode;

//CPU取指阶段传递给寄存器堆的跳转请求信息(仅取指模块与寄存器堆模块连接)
typedef struct packed {
    //跳转类型
    CpuGotoAskTypeCode jumpType;
    logic[39:0] jumpAddress;//直接跳转的地址(IT/RT/EIT无用)
}CoreMod_GotoAskInfo_t;

//模式调整选择码
typedef enum logic[2:0]{
    CpuPatternCtrlCode_EnableInterrupt = 0,//允许中断响应
    CpuPatternCtrlCode_DisableInterrupt = 1,//不允许中断响应
    CpuPatternCtrlCode_EnableProtect = 2//开启保护模式
}CpuPatternCtrlCode;
//CPU内核模块间必要传递的信息(所以模块间都需要连接)
typedef struct packed{
    logic isValid;//是否有效

    //基本信息
    logic [39:0] orderAddress;//当前模块正在执行的指令地址
    logic [31:0] orderCode;//当前模块正在执行的指令源码

    //CPU重启信号
    logic isRestart;//是否是重启请求指令
 
    //中断信息
    logic isHaveAbnormal;//当前执行的模块是否存在异常中断请求
    logic [7:0] abnormalCode;//如果存在中断请求，请求的号码

    //CPU模式控制信息
    logic isHavePatternCtrl;//是否是模式调整
    CpuPatternCtrlCode patternCtrlCode;//模式调整选择码

    //跳转信息
    logic isBranchJump;//是否是分支跳转
    logic branchJumpForecast;//分支预测的结果(0:不跳转,1:跳转)
    logic [39:0]branchJumpAddress;//如果分支跳转最终确定是需要跳转的话，其跳转地址
}CoreMod_PrimaryInfo_t;

function CoreMod_PrimaryInfo_t CoreMod_PrimaryInfo_NewInvalidValue();
    CoreMod_PrimaryInfo_t tmp;
    tmp.isValid = 0;
    tmp.orderAddress = 0;
    tmp.orderCode = 0;
    tmp.isRestart = 0;
    tmp.isHaveAbnormal = 0;
    tmp.abnormalCode = 0;
    tmp.isHavePatternCtrl = 0;
    tmp.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;
    tmp.isBranchJump = 0;
    tmp.branchJumpForecast = 0;
    tmp.branchJumpAddress = 0;
    return tmp;
endfunction


typedef enum logic[3:0]{
    OperationModType_Mem_Stack_IO, 
    OperationModType_Stack_PushPop,
    OperationModType_Mov_LimitData_Init,
    OperationModType_ALU_AddSub_IntUInt64,
    OperationModType_ALU_BitOper_IntUInt64,
    OperationModType_ALU_BoolOper_IntUInt64,
    OperationModType_ALU_Equal_All64Type,
    OperationModType_ALU_Compare_Int64,
    OperationModType_ALU_Compare_UInt64,
    OperationModType_ALU_Division_UIntInt64,
    OperationModType_ALU_Multiply_IntUInt64,
    OperationModType_FPU_AddSub_Float64,
    OperationModType_FPU_TypeTrans_Float64,
    OperationModType_FPU_Compare_Float64,
    OperationModType_FPU_Division_Float64,
    OperationModType_FPU_Multiply_Float64
}OperationModType;

//CPU指令解析步骤的后续模块间必要传递的信息
typedef struct packed{
    logic isValid;
    OperationModType operModType;//操作的模块类型
    logic [3:0] operMod_FuncSelectCode;//所操作模块的选择码

    //操作数的寄存器号
    logic [4:0] operA;//操作数A
    logic [4:0] operB;//操作数B
    logic [4:0] resA;//结果A
    logic [4:0] resB;//结果B(要么无效，如果有效那基本是SP)

    //源操作数的立即数
    logic [63:0] operImme;
    /*
    如果(operB_Valid==1 & operB==31) : operImme是operB的值
    如果(operB_Valid==0 & operA_Valid==1 & operA==31) : operImme是operA的值
    否则，operImme无效
    */
} CoreMod_OperationInfo_t;
function CoreMod_OperationInfo_t CoreMod_OperationInfo_NewInvalidValue();
    CoreMod_OperationInfo_t tmp;
    tmp.isValid = 0;
    tmp.operModType = OperationModType_Mem_Stack_IO;
    tmp.operMod_FuncSelectCode = 0;
    tmp.operImme = 0;
    tmp.operA = RegisterBlockCode_IMME_INVALID;
    tmp.operB = RegisterBlockCode_IMME_INVALID;
    tmp.resA = RegisterBlockCode_IMME_INVALID;
    tmp.resB = RegisterBlockCode_IMME_INVALID;
    return tmp;
endfunction
`endif