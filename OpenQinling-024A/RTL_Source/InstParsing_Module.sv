`include "CoreMod_Interface.svh"
`include "InstParsing_Module.svh"
module InstParsing_Module(
    //时钟/重启信号
    input wire clk,rst,
    input wire clean, //清除当前正在运行的任务,将模块恢复到空闲状态
    input wire nextMod_isBeReady,//下一个模块是否已经即将完成
    input wire start,//开始执行一次取指任务的信号运行
    output wire ready,//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
    output wire running,//是否正在运行中
    output wire [39:0]runningAddress,//如果正在运行中,运行的指令地址
    output wire isBeReady,//在即将运行完成一个任务时发出，保持一个时钟
    //(当时钟上升沿该位为1,说明前一个模块传递来的任务已经在当前模块处理完毕。此时需要前一个模块更新传递来新任务,若无新任务就发送来一个无效任务,否则该模块将会重复执行已完成的任务)

    //寄存器组信息
    input wire[63:0]allRegValue[31],

    //如果当前是分支跳转指令,给出是否已经获取了最终的分支跳转结果
    output wire branchJmpPredictor_gotFinalResult,//是否已经获得了结果
    output wire branchJmpPredictor_finalResult,//最终结果是否要跳转

    //初步解析取到的指令的信息(取指模块取到并解析后发来的指令基本信息)
    input wire CoreMod_PrimaryInfo_t fetch_PrimaryInfo,
    output wire CoreMod_OperationInfo_t analysis_OperatInfo, //当前解析模块中正在处理的指令的操作数信息
    output wire CoreMod_PrimaryInfo_t operat_PrimaryInfo,   //下一个执行模块中正在处理的指令的基本信息
    output wire CoreMod_OperationInfo_t operat_OperatInfo   //下一个执行模块中正在处理的指令的操作数信息
);
    //是否正在运行中
    assign ready = 1;
    assign runningAddress = fetch_PrimaryInfo.orderAddress;
    assign running = (start && rst && fetch_PrimaryInfo.isValid && !clean);
    assign isBeReady = running;
    CoreMod_PrimaryInfo_t primaryInfo_Tmp;
    CoreMod_OperationInfo_t operatInfo_Tmp;
    assign analysis_OperatInfo = operatInfo_Tmp;

    wire [31:0] orderCode = fetch_PrimaryInfo.orderCode;
    wire [2:0] orderType = orderCode[31:29];

    

    //是否已经获得了最终的跳转结果
    assign branchJmpPredictor_gotFinalResult = running && //当前解析模块是否正在运行中
            (operat_OperatInfo.resA != RegisterBlockCode_R25_BJS && //当前BJS寄存器是否还不确定
            operat_OperatInfo.resB != RegisterBlockCode_R25_BJS)&&
            (fetch_PrimaryInfo.isBranchJump);//解析的是否是分支跳转指令
    //得到的跳转结果
    assign branchJmpPredictor_finalResult = allRegValue[RegisterBlockCode_R25_BJS] == 0;

    //assign primaryInfo_Tmp = fetch_PrimaryInfo;
    //解析输出的指令基础信息
    always_comb begin
        primaryInfo_Tmp = fetch_PrimaryInfo;
        //如果已经得到了分支预测的最终跳转结果,那将其分支跳转标识符置0,表示该指令已经分支跳转彻底完成了
        if(branchJmpPredictor_gotFinalResult)begin
            primaryInfo_Tmp.isBranchJump = 0;
        end
    end
    

    //解析输出的指令操作信息
    always_comb begin
        if(fetch_PrimaryInfo.isValid)begin
            //系统控制、跳转指令在取指阶段已经解析完成了，NOP指令无任何操作。此三类指令的信息直接原封不动保存到下一个模块
            if(orderType == InstTypeCode_IOStack && orderCode[28] == InstTypeCode_IOStackType_IO)begin //内存访问指令解析
                operatInfo_Tmp.operModType = OperationModType_Mem_Stack_IO;
                operatInfo_Tmp.operMod_FuncSelectCode = orderCode[27:24];//所操作模块的选择码
                //操作数的寄存器号
                operatInfo_Tmp.operA = orderCode[18:14];//操作数A(地址)
                operatInfo_Tmp.operB  = orderCode[27] ? orderCode[23:19] : RegisterBlockCode_IMME_INVALID;//操作数B(写入数据)
                operatInfo_Tmp.resA = orderCode[27] ? RegisterBlockCode_IMME_INVALID : orderCode[23:19];//结果A(读出数据)
                operatInfo_Tmp.resB = RegisterBlockCode_IMME_INVALID;
                //源操作数的立即数
                operatInfo_Tmp.operImme = orderCode[13] ? {~(51'b0),orderCode[12:0]} : orderCode[12:0];//最终读写地址= 操作数A指向的寄存器地址 + 立即数
                operatInfo_Tmp.isValid = 1;
            end else if(orderType == InstTypeCode_IOStack && orderCode[28] == InstTypeCode_IOStackType_SK)begin //出入栈指令解析
                operatInfo_Tmp.operModType = OperationModType_Stack_PushPop;
                operatInfo_Tmp.operMod_FuncSelectCode = orderCode[27:24];//所操作模块的选择码
                operatInfo_Tmp.operA = RegisterBlockCode_R26_SP;//操作数A(地址),固定为SP寄存器
                operatInfo_Tmp.operB  = orderCode[27] ? orderCode[23:19] : RegisterBlockCode_IMME_INVALID;//操作数B(写入数据)
                operatInfo_Tmp.resA = orderCode[27] ? RegisterBlockCode_IMME_INVALID : orderCode[23:19];//结果A(读出数据)
                operatInfo_Tmp.resB = RegisterBlockCode_R26_SP;//必然影响SP寄存器
                //源操作数的立即数
                operatInfo_Tmp.operImme = orderCode[18] ? {~(45'b0),orderCode[18:0]} : orderCode[18:0];//如果入栈时操作数A无效，入的就是立即数

                operatInfo_Tmp.isValid = 1;
            end else if(orderType == InstTypeCode_StackRW)begin //堆栈读写指令解析
                operatInfo_Tmp.operModType = OperationModType_Mem_Stack_IO;
                operatInfo_Tmp.operMod_FuncSelectCode = orderCode[27:24];//所操作模块的选择码
                //操作数的寄存器号
                operatInfo_Tmp.operA = RegisterBlockCode_R26_SP;//操作数A(地址)
                operatInfo_Tmp.operB  = orderCode[28] ? orderCode[24:20] : RegisterBlockCode_IMME_INVALID;//操作数B(写入数据)
                operatInfo_Tmp.resA = orderCode[28] ? RegisterBlockCode_IMME_INVALID : orderCode[24:20];//结果A(读出数据)
                operatInfo_Tmp.resB = RegisterBlockCode_IMME_INVALID;
                //源操作数的立即数
                operatInfo_Tmp.operImme = orderCode[19:0]; //最终读写地址= 操作数A指向的寄存器地址 + 立即数

                operatInfo_Tmp.isValid = 1;
            end else if(orderType == InstTypeCode_BaseOper || orderType == InstTypeCode_ExpOper)begin //基本运算、扩展运算指令解析
                automatic logic [4:0] subType = orderCode[28:24];
                if(orderType == InstTypeCode_BaseOper)begin
                    case(subType)
                    InstTypeCode_BaseOperType_ADD:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_AddSub_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_BaseOperType_SUB:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_AddSub_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_BaseOperType_MUL:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Multiply_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_BaseOperType_UDIV:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Division_UIntInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2'b00;
                    end
                    InstTypeCode_BaseOperType_IDIV:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Division_UIntInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2'b01;
                    end
                    InstTypeCode_BaseOperType_UREM:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Division_UIntInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2'b10;
                    end
                    InstTypeCode_BaseOperType_IREM:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Division_UIntInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2'b11;
                    end
                    //布尔运算
                    InstTypeCode_BaseOperType_BAND:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BoolOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_BaseOperType_BOR:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BoolOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_BaseOperType_BNOT:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BoolOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    //位运算
                    InstTypeCode_BaseOperType_AND:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_BaseOperType_OR:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_BaseOperType_NOT:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    InstTypeCode_BaseOperType_XOR:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 4;
                    end
                    InstTypeCode_BaseOperType_SAL:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 5;
                    end
                    InstTypeCode_BaseOperType_USOR:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 6;
                    end
                    InstTypeCode_BaseOperType_ISOR:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_BitOper_IntUInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 7;
                    end
                    //通用相等比较
                    InstTypeCode_BaseOperType_EC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Equal_All64Type;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_BaseOperType_NEC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Equal_All64Type;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    //无符号大小比较
                    InstTypeCode_BaseOperType_ULC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_UInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_BaseOperType_ULEC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_UInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    InstTypeCode_BaseOperType_UMC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_UInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_BaseOperType_UMEC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_UInt64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    //有符号大小比较
                    InstTypeCode_BaseOperType_ILC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_Int64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_BaseOperType_ILEC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_Int64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    InstTypeCode_BaseOperType_IMC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_Int64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_BaseOperType_IMEC:begin
                        operatInfo_Tmp.operModType = OperationModType_ALU_Compare_Int64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_BaseOperType_LDI8:begin
                        operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_BaseOperType_LDU16:begin
                        operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_BaseOperType_LDI16:begin
                        operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    InstTypeCode_BaseOperType_LDU32:begin
                        operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                        operatInfo_Tmp.operMod_FuncSelectCode = 4;
                    end
                    InstTypeCode_BaseOperType_LDI32:begin
                        operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                        operatInfo_Tmp.operMod_FuncSelectCode = 5;
                    end
                    default:begin
                        operatInfo_Tmp.isValid = 0;
                        operatInfo_Tmp.operModType = OperationModType_Mem_Stack_IO;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    endcase

                    operatInfo_Tmp.isValid = 1;
                end else begin
                    case(subType)
                    //四则运算
                    InstTypeCode_ExpOperType_FADD:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_AddSub_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_ExpOperType_FSUB:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_AddSub_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_ExpOperType_FMUL:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Multiply_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_ExpOperType_FDIV:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Division_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    //浮点大小比较
                    InstTypeCode_ExpOperType_FLC:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Compare_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_ExpOperType_FLEC:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Compare_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    InstTypeCode_ExpOperType_FMEC:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Compare_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    InstTypeCode_ExpOperType_FMC:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_Compare_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    //浮整转换
                    InstTypeCode_ExpOperType_FTI:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_TypeTrans_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 1;
                    end
                    InstTypeCode_ExpOperType_ITF:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_TypeTrans_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 2;
                    end
                    InstTypeCode_ExpOperType_UTF:begin
                        operatInfo_Tmp.operModType = OperationModType_FPU_TypeTrans_Float64;
                        operatInfo_Tmp.operMod_FuncSelectCode = 3;
                    end
                    default:begin
                        operatInfo_Tmp.isValid = 0;
                        operatInfo_Tmp.operModType = OperationModType_Mem_Stack_IO;
                        operatInfo_Tmp.operMod_FuncSelectCode = 0;
                    end
                    endcase
                end
                operatInfo_Tmp.operA = orderCode[18:14];//操作数A
                operatInfo_Tmp.operB = orderCode[13:9];//操作数B
                operatInfo_Tmp.resA = orderCode[23:19];//结果A
                operatInfo_Tmp.resB = RegisterBlockCode_IMME_INVALID;
                operatInfo_Tmp.operImme = orderCode[8] ? {~(56'b0),orderCode[7:0]} : orderCode[7:0];

                operatInfo_Tmp.isValid = 1;
            end else if(orderType == InstTypeCode_MovInit)begin //转移、初始化指令解析
                operatInfo_Tmp.operModType = OperationModType_Mov_LimitData_Init;
                operatInfo_Tmp.operMod_FuncSelectCode = orderCode[28:28] ? 6 : 0;//所操作模块的选择码
                if(orderCode[28:28] == InstTypeCode_MovInitType_MOV)begin
                    operatInfo_Tmp.operA = orderCode[22:18];
                    operatInfo_Tmp.operImme = orderCode[17] ? {~(46'b0),orderCode[17:0]} : orderCode[17:0];
                end else begin
                    operatInfo_Tmp.operA = orderCode[27:23];
                    operatInfo_Tmp.operImme = orderCode[22:0];
                end
                operatInfo_Tmp.operB  = RegisterBlockCode_IMME_INVALID;//操作数B(写入数据)
                operatInfo_Tmp.resA = orderCode[27:23];
                operatInfo_Tmp.resB = RegisterBlockCode_IMME_INVALID;
                operatInfo_Tmp.isValid = 1;
            end else begin
                operatInfo_Tmp = CoreMod_OperationInfo_NewInvalidValue();
            end
        end else begin
            operatInfo_Tmp = CoreMod_OperationInfo_NewInvalidValue();
        end
    end

    
    CoreMod_PrimaryInfo_t primaryInfo_Reg;
    CoreMod_OperationInfo_t operatInfo_Reg;
    assign operat_PrimaryInfo = primaryInfo_Reg;
    assign operat_OperatInfo = operatInfo_Reg;

    
    always_ff @(posedge clk or negedge rst)begin
        if(!rst || clean)begin
            primaryInfo_Reg <= CoreMod_PrimaryInfo_NewInvalidValue();
            operatInfo_Reg <= CoreMod_OperationInfo_NewInvalidValue();
        end else if(start)begin
            primaryInfo_Reg <= primaryInfo_Tmp;
            operatInfo_Reg <= operatInfo_Tmp;
        end else if(nextMod_isBeReady)begin
            primaryInfo_Reg <= CoreMod_PrimaryInfo_NewInvalidValue();
            operatInfo_Reg <= CoreMod_OperationInfo_NewInvalidValue();
        end
    end
endmodule