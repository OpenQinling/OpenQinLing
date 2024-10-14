`include "CoreMod_Interface.svh"
`include "InstParsing_Module.svh"
`include "IO_Control.svh"

module Execute_Module(
    //系统时钟/重启信号
    input wire clk,rst,
    input wire clean, //清除当前正在运行的任务,将模块恢复到空闲状态
    //模块事务控制信号
    input wire start,//开始执行一次取指任务的信号
    output wire ready,//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
    output wire running,//是否正在运行中
    output wire [39:0]runningAddress,//如果正在运行中,运行的指令地址
    output wire isBeReady,//在即将运行完成一个任务时发出，保持一个时钟
    //(当时钟上升沿该位为1,说明前一个模块传递来的任务已经在当前模块处理完毕。此时需要前一个模块更新传递来新任务,若无新任务就发送来一个无效任务,否则该模块将会重复执行已完成的任务)

    //寄存器信息
    input wire[63:0]allRegValue[31],

    //如果当前是分支跳转指令,给出是否已经获取了最终的分支跳转结果
    wire branchJmpPredictor_gotFinalResult,//是否已经获得了结果
    wire branchJmpPredictor_finalResult,//最终结果是否要跳转

    //输出的回写信息
    output wire[63:0]resAValue,//如果resA存在回写需要，回写的值
    output wire[63:0]resBValue,//如果resB存在回写需要，回写的值

    //解析的指令的信息(取指模块取到并解析后发来的指令基本信息)
    input wire CoreMod_PrimaryInfo_t this_PrimaryInfo,
    input wire CoreMod_OperationInfo_t this_OperatInfo,

    output  CoreMod_PrimaryInfo_t next_PrimaryInfo,   //下一个执行模块中处理的指令的基本信息
    output  CoreMod_OperationInfo_t next_OperatInfo,   //下一个执行模块中处理的指令的操作数信息);

    //连接cpu数据缓存/MMU的接口
    IO_Interface.master ioInterface
);
    logic [63:0] numA;
    logic [63:0] numB;
    logic[63:0] resA_Reg;
    logic[63:0] resB_Reg;

    CoreMod_PrimaryInfo_t next_PrimaryInfo_Reg;   //下一个执行模块中处理的指令的基本信息
    CoreMod_OperationInfo_t next_OperatInfo_Reg;   //下一个执行模块中处理的指令的操作数信息);
    assign next_PrimaryInfo = next_PrimaryInfo_Reg;
    assign next_OperatInfo = next_OperatInfo_Reg;
    assign resAValue = resA_Reg;
    assign resBValue = resB_Reg;

    logic addsubUI64_start,addsubUI64_isReady;
    logic [63:0]addsubUI64_res;
    AddSub_IntUInt64 addsubUI64(
        clk,
        rst,
        clean,
        addsubUI64_start,
        numA,
        numB,
        addsubUI64_res,
        addsubUI64_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic bitOperUI64_start,bitOperUI64_isReady,bitOperUI64_error;
    logic [63:0]bitOperUI64_res;
    BitOper_IntUInt64 bitOperUI64(
        //ALU模块通用接口
        clk,
        rst,
        clean,
        bitOperUI64_start,
        numA,
        numB,
        bitOperUI64_res,
        bitOperUI64_isReady,

        //当前模块专有接口
        this_OperatInfo.operMod_FuncSelectCode,//1:与 2:或 3:非 4:异或 5:左移 6:无符号右移 7:有符号右移 0无效(报错)
        bitOperUI64_error //无效功能选择
    );
    logic boolOperUI64_start,boolOperUI64_isReady,boolOperUI64_error;
    logic [63:0]boolOperUI64_res;
    BoolOper_IntUInt64 boolOperUI64(
        clk,
        rst,
        clean,
        boolOperUI64_start,
        numA,
        numB,
        boolOperUI64_res,
        boolOperUI64_isReady,
        this_OperatInfo.operMod_FuncSelectCode,
        boolOperUI64_error
    );
    logic equalAllType_start,equalAllType_isReady;
    logic [63:0]equalAllType_res;
    Equal_All64Type equalAllType(
        clk,
        rst,
        clean,
        equalAllType_start,
        numA,
        numB,
        equalAllType_res,
        equalAllType_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic compareI64_start,compareI64_isReady;
    logic [63:0]compareI64_res;
    Compare_Int64 compareI64(
        clk,
        rst,
        clean,
        compareI64_start,
        numA,
        numB,
        compareI64_res,
        compareI64_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic compareU64_start,compareU64_isReady;
    logic [63:0]compareU64_res;
    Compare_UInt64 compareU64(
        clk,
        rst,
        clean,
        compareU64_start,
        numA,
        numB,
        compareU64_res,
        compareU64_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic divisionUI64_start,divisionUI64_isReady,divisionUI64_error;
    logic [63:0]divisionUI64_res;
    Division_UIntInt64 divisionUI64(
        clk,
        rst,
        clean,
        divisionUI64_start,
        numA,
        numB,
        divisionUI64_res,
        divisionUI64_isReady,
        this_OperatInfo.operMod_FuncSelectCode,
        divisionUI64_error
    );
    logic multiplyUI64_start,multiplyUI64_isReady;
    logic [63:0]multiplyUI64_res;
    Multiply_IntUInt64 multiplyUI64(
        clk,
        rst,
        clean,
        multiplyUI64_start,
        numA,
        numB,
        multiplyUI64_res,
        multiplyUI64_isReady
    );
    logic addSubF64_start,addSubF64_isReady;
    logic [63:0]addSubF64_res;
    AddSub_Float64 addSubF64(
        clk,
        rst,
        clean,
        addSubF64_start,
        numA,
        numB,
        addSubF64_res,
        addSubF64_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic typeTransF64_start,typeTransF64_isReady,typeTransF64_error;
    logic [63:0]typeTransF64_res;
    TypeTrans_Float64 typeTransF64(
        clk,
        rst,
        clean,
        typeTransF64_start,
        numA,
        numB,
        typeTransF64_res,
        typeTransF64_isReady,
        this_OperatInfo.operMod_FuncSelectCode,
        typeTransF64_error
    );
    logic compareF64_start,compareF64_isReady;
    logic [63:0]compareF64_res;
    Compare_Float64 compareF64(
        clk,
        rst,
        clean,
        compareF64_start,
        numA,
        numB,
        compareF64_res,
        compareF64_isReady,
        this_OperatInfo.operMod_FuncSelectCode
    );
    logic divisionF64_start,divisionF64_isReady;
    logic [63:0]divisionF64_res;
    Division_Float64 divisionF64(
        clk,
        rst,
        clean,
        divisionF64_start,
        numA,
        numB,
        divisionF64_res,
        divisionF64_isReady
    );
    logic multiplyF64_start,multiplyF64_isReady;
    logic [63:0]multiplyF64_res;
    Multiply_Float64 multiplyF64(
        clk,
        rst,
        clean,
        multiplyF64_start,
        numA,
        numB,
        multiplyF64_res,
        multiplyF64_isReady
    );
    logic ioControl_start,ioControl_isReady,ioControl_error;
    logic [63:0]ioControl_res;
    IO_Control ioControl(
        clk,
        rst,
        clean,
        ioControl_start,
        numA,
        numB,
        ioControl_res,
        ioControl_isReady,
        this_OperatInfo.operMod_FuncSelectCode,
        ioControl_error,
        ioInterface //连接cpu数据缓存/MMU的接口
    );
    logic movLimitDataInitAllType_start,movLimitDataInitAllType_isReady,movLimitDataInitAllType_error;
    logic [63:0]movLimitDataInitAllType_res;
    MovLimitDataInit_AllType movLimitDataInitAllType(
        clk,
        rst,
        clean,
        movLimitDataInitAllType_start,
        numA,
        numB,
        movLimitDataInitAllType_res,
        movLimitDataInitAllType_isReady,
        this_OperatInfo.operMod_FuncSelectCode,
        movLimitDataInitAllType_error
    );

    reg isReadyReg = 1;
    assign ready = isReadyReg;
    assign running = (start && isReadyReg && this_PrimaryInfo.isValid) || ~isReadyReg;
    assign runningAddress = this_PrimaryInfo.orderAddress;
    logic isNowTickReady, isHaveError;

    //解析ready/error信号
    always_comb begin : analysisReadySignal
        if(this_OperatInfo.isValid)begin
            case(this_OperatInfo.operModType)
            OperationModType_Mem_Stack_IO:begin
                 isNowTickReady = ioControl_isReady;
                 isHaveError = ioControl_error;
            end
            OperationModType_Stack_PushPop:begin
                 isNowTickReady = ioControl_isReady;
                 isHaveError = ioControl_error;
            end
            OperationModType_Mov_LimitData_Init :begin
                isNowTickReady = movLimitDataInitAllType_isReady;
                isHaveError = movLimitDataInitAllType_error;
            end
            OperationModType_ALU_AddSub_IntUInt64:begin
                isNowTickReady = addsubUI64_isReady;
                isHaveError = 0;
            end
            OperationModType_ALU_BitOper_IntUInt64:begin
                isNowTickReady = bitOperUI64_isReady;
                isHaveError = bitOperUI64_error;
            end
            OperationModType_ALU_BoolOper_IntUInt64:begin
                isNowTickReady = boolOperUI64_isReady;
                isHaveError = boolOperUI64_error;
            end
            OperationModType_ALU_Equal_All64Type:begin
                isNowTickReady = equalAllType_isReady;
                isHaveError = 0;
            end
            OperationModType_ALU_Compare_Int64:begin
                isNowTickReady = compareI64_isReady;
                isHaveError = 0;
            end
            OperationModType_ALU_Compare_UInt64:begin
                isNowTickReady = compareU64_isReady;
                isHaveError = 0;
            end
            OperationModType_ALU_Division_UIntInt64:begin
                isNowTickReady = divisionUI64_isReady;
                isHaveError = divisionUI64_error;
            end
            OperationModType_ALU_Multiply_IntUInt64:begin
                isNowTickReady = multiplyUI64_isReady;
                isHaveError = 0;
            end
            OperationModType_FPU_AddSub_Float64:begin
                isNowTickReady = addSubF64_isReady;
                isHaveError = 0;
            end
            OperationModType_FPU_TypeTrans_Float64:begin
                isNowTickReady = typeTransF64_isReady;
                isHaveError = typeTransF64_error;
            end
            OperationModType_FPU_Compare_Float64:begin
                isNowTickReady = compareF64_isReady;
                isHaveError = 0;
            end
            OperationModType_FPU_Division_Float64:begin
                isNowTickReady = divisionF64_isReady;
                isHaveError = 0;
            end
            OperationModType_FPU_Multiply_Float64:begin
                isNowTickReady = multiplyF64_isReady;
                isHaveError = 0;
            end
            default:begin
                isNowTickReady = 1;
                isHaveError = 1;
            end
            endcase
        end else begin
            isNowTickReady = 1;
            isHaveError = 0;
        end
    end

    
    //解析start信号
    always_comb begin : analysisStartSignal
        if(this_OperatInfo.isValid && start)begin
            automatic OperationModType  operModType = this_OperatInfo.operModType;
            ioControl_start = operModType==OperationModType_Mem_Stack_IO || operModType==OperationModType_Stack_PushPop;
            addsubUI64_start = operModType==OperationModType_ALU_AddSub_IntUInt64;
            bitOperUI64_start = operModType==OperationModType_ALU_BitOper_IntUInt64;
            boolOperUI64_start = operModType==OperationModType_ALU_BoolOper_IntUInt64;
            equalAllType_start = operModType==OperationModType_ALU_Equal_All64Type;
            compareI64_start = operModType==OperationModType_ALU_Compare_Int64;
            compareU64_start = operModType==OperationModType_ALU_Compare_UInt64;
            divisionUI64_start = operModType==OperationModType_ALU_Division_UIntInt64;
            multiplyUI64_start = operModType==OperationModType_ALU_Multiply_IntUInt64;
            addSubF64_start = operModType==OperationModType_FPU_AddSub_Float64;
            typeTransF64_start = operModType==OperationModType_FPU_TypeTrans_Float64;
            compareF64_start = operModType==OperationModType_FPU_Compare_Float64;
            divisionF64_start = operModType==OperationModType_FPU_Division_Float64;
            multiplyF64_start = operModType==OperationModType_FPU_Multiply_Float64;
            movLimitDataInitAllType_start = operModType==OperationModType_Mov_LimitData_Init;
        end else begin
            ioControl_start = 0;
            addsubUI64_start = 0;
            bitOperUI64_start = 0;
            boolOperUI64_start = 0;
            equalAllType_start = 0;
            compareI64_start = 0;
            compareU64_start = 0;
            divisionUI64_start = 0;
            multiplyUI64_start = 0;
            addSubF64_start = 0;
            typeTransF64_start = 0;
            compareF64_start = 0;
            divisionF64_start = 0;
            multiplyF64_start = 0;
            movLimitDataInitAllType_start = 0;
        end
    end
    //解析numA信号
    always_comb begin : analysisNumASignal
        if(!this_OperatInfo.isValid)begin
            numA = 0;
        end else if(this_OperatInfo.operModType==OperationModType_Mem_Stack_IO)begin
            //内存读写操作,numA是地址  = numA寄存器 + 立即数
            automatic logic[63:0] offset = this_OperatInfo.operImme;
            numA = (this_OperatInfo.operA == RegisterBlockCode_IMME_INVALID) ? this_OperatInfo.operImme : allRegValue[this_OperatInfo.operA]+offset;
        end else if(this_OperatInfo.operModType==OperationModType_Stack_PushPop)begin
            //出入栈操作
            //数据入栈,numA地址  =  numA寄存器（必然为SP） - 入栈字节数
            //数据出栈,numA地址 = numA寄存器
            automatic logic[3:0] ioModeType = this_OperatInfo.operMod_FuncSelectCode;
            if(ioModeType ==  IOModes__WRITE_UBYTE_BYTE)begin
                numA = allRegValue[this_OperatInfo.operA] - 1;
            end else if(ioModeType ==  IOModes__WRITE_USHORT_SHORT)begin
                numA = allRegValue[this_OperatInfo.operA] - 2;
            end else if(ioModeType ==  IOModes__WRITE_UINT_INT ||
                        ioModeType == IOModes__WRITE_FLOAT)begin
                numA = allRegValue[this_OperatInfo.operA] - 4;
            end else if(ioModeType ==  IOModes__WRITE_ULONG_LONG_DOUBLE)begin
                numA = allRegValue[this_OperatInfo.operA] - 8;
            end else begin
                numA = allRegValue[this_OperatInfo.operA];
            end
        end else if(this_OperatInfo.operModType==OperationModType_Mov_LimitData_Init &&
                    this_OperatInfo.operMod_FuncSelectCode == 0)begin
            numA = (this_OperatInfo.operA == RegisterBlockCode_IMME_INVALID) ? this_OperatInfo.operImme : allRegValue[this_OperatInfo.operA];
        end else begin
            numA = (this_OperatInfo.operA == RegisterBlockCode_IMME_INVALID) ? 0 : allRegValue[this_OperatInfo.operA];
        end
    end
    //解析numB信号
    always_comb begin : analysisNumBSignal
        if(!this_OperatInfo.isValid)begin
            numB = 0;
        end else if(this_OperatInfo.operModType==OperationModType_Mem_Stack_IO)begin
            numB = (this_OperatInfo.operB == RegisterBlockCode_IMME_INVALID) ? 0 : allRegValue[this_OperatInfo.operB];
        end else if(this_OperatInfo.operModType==OperationModType_Mov_LimitData_Init &&
                    this_OperatInfo.operMod_FuncSelectCode == 6)begin
            numB = this_OperatInfo.operImme;
        end else begin
            numB = (this_OperatInfo.operB == RegisterBlockCode_IMME_INVALID) ? this_OperatInfo.operImme : allRegValue[this_OperatInfo.operB];
        end
    end
    
    logic [63:0]resA_Tmp,resB_Tmp;
    //解析resB的信号
    always_comb begin : analysisResBSignal
        if(this_OperatInfo.isValid && this_OperatInfo.operModType==OperationModType_Stack_PushPop)begin
            //出入栈操作
            //数据入栈,numB地址  =  numA
            //数据出栈,numB地址 = 寄存器组[numB的寄存器] + 出栈字节数
            automatic logic[3:0] ioModeType = this_OperatInfo.operMod_FuncSelectCode;
            if(ioModeType ==  IOModes__READ_UBYTE || ioModeType == IOModes__READ_BYTE)begin
                resB_Tmp = allRegValue[this_OperatInfo.operA] + 1;
            end else if(ioModeType == IOModes__READ_USHORT || ioModeType == IOModes__READ_SHORT)begin
                resB_Tmp = allRegValue[this_OperatInfo.operA] + 2;
            end else if(ioModeType ==  IOModes__READ_UINT ||
                        ioModeType == IOModes__READ_INT || ioModeType == IOModes__READ_FLOAT)begin
                resB_Tmp = allRegValue[this_OperatInfo.operA] + 4;
            end else if(ioModeType ==  IOModes__READ_ULONG_LONG_DOUBLE)begin
                resB_Tmp = allRegValue[this_OperatInfo.operA] + 8;
            end else begin
                resB_Tmp = numA;
            end
        end else begin
            resB_Tmp = 0;
        end
    end
    //解析resA的信号
    always_comb begin : analysisResASignal
        if(this_OperatInfo.isValid)begin
            case(this_OperatInfo.operModType)
            OperationModType_Mem_Stack_IO: resA_Tmp = ioControl_res;
            OperationModType_Stack_PushPop: resA_Tmp = ioControl_res;
            OperationModType_Mov_LimitData_Init : resA_Tmp = movLimitDataInitAllType_res;
            OperationModType_ALU_AddSub_IntUInt64: resA_Tmp = addsubUI64_res;
            OperationModType_ALU_BitOper_IntUInt64: resA_Tmp = bitOperUI64_res;
            OperationModType_ALU_BoolOper_IntUInt64: resA_Tmp = boolOperUI64_res;
            OperationModType_ALU_Equal_All64Type: resA_Tmp = equalAllType_res;
            OperationModType_ALU_Compare_Int64: resA_Tmp = compareI64_res;
            OperationModType_ALU_Compare_UInt64: resA_Tmp = compareU64_res;
            OperationModType_ALU_Division_UIntInt64: resA_Tmp = divisionUI64_res;
            OperationModType_ALU_Multiply_IntUInt64: resA_Tmp = multiplyUI64_res;
            OperationModType_FPU_AddSub_Float64: resA_Tmp = addSubF64_res;
            OperationModType_FPU_TypeTrans_Float64: resA_Tmp = typeTransF64_res;
            OperationModType_FPU_Compare_Float64: resA_Tmp = compareF64_res;
            OperationModType_FPU_Division_Float64: resA_Tmp = divisionF64_res;
            OperationModType_FPU_Multiply_Float64: resA_Tmp = multiplyF64_res;
            default: resA_Tmp = 0;
            endcase
        end else begin
            resA_Tmp = 0;
        end
    end

    CoreMod_PrimaryInfo_t next_PrimaryInfo_tmp;   //下一个执行模块中处理的指令的基本信息
    CoreMod_OperationInfo_t next_OperatInfo_tmp;   //下一个执行模块中处理的指令的操作数信息);
    
    
    //是否已经获得了最终的跳转结果(只要当前执行的是尚未完成的分支跳转指令,那么其在执行模块当中必然已经能够完成,得到最终跳转结果了)
    assign branchJmpPredictor_gotFinalResult = running ? (this_PrimaryInfo.isBranchJump) : 0;
    //得到的跳转结果
    assign branchJmpPredictor_finalResult = allRegValue[RegisterBlockCode_R25_BJS] == 0;
    
    //解析输出的Primary/Operat信息
    always_comb begin : analysisOutputOrderInfo
        if(this_PrimaryInfo.isValid)begin
            //判断有无出错,如果有出错OperatInfo输出无效,PrimaryInfo输出异常中断
            //如果无出错,原封不动的输出
            if(isHaveError)begin
                automatic OperationModType  operModType = this_OperatInfo.operModType;
                next_PrimaryInfo_tmp.isValid = 1;
                next_PrimaryInfo_tmp.orderAddress = this_PrimaryInfo.orderAddress;
                next_PrimaryInfo_tmp.orderCode = this_PrimaryInfo.orderCode;
                next_PrimaryInfo_tmp.isRestart = 0;
                next_PrimaryInfo_tmp.isHavePatternCtrl = 0;
                next_PrimaryInfo_tmp.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;
                next_PrimaryInfo_tmp.isBranchJump = 0;
                next_PrimaryInfo_tmp.branchJumpForecast = 0;
                next_PrimaryInfo_tmp.branchJumpAddress = 0;
                next_PrimaryInfo_tmp.isHaveAbnormal = 1;
                if(operModType == OperationModType_Mem_Stack_IO ||
                   operModType == OperationModType_Stack_PushPop)begin
                    //出错: 产生异常中断: 内存访问超时/缺页
                    next_PrimaryInfo_tmp.abnormalCode = SystemInterruptCode_MemoryAccessError;
                end else if(operModType == OperationModType_ALU_Division_UIntInt64)begin
                    //出错: 产生异常中断: 除法除数为0
                    next_PrimaryInfo_tmp.abnormalCode = SystemInterruptCode_DivisionOperationError;
                end else begin //出错: 产生异常中断:指令解析出错
                    next_PrimaryInfo_tmp.abnormalCode = SystemInterruptCode_AnalysisCodeError;
                end
                next_OperatInfo_tmp = CoreMod_OperationInfo_NewInvalidValue();
            end else begin
                next_PrimaryInfo_tmp = this_PrimaryInfo;
                //如果已经得到了分支预测的最终跳转结果,那将其分支跳转标识符置0,表示该指令已经分支跳转彻底完成了
                if(branchJmpPredictor_gotFinalResult)begin
                    next_PrimaryInfo_tmp.isBranchJump = 0;
                end
                next_OperatInfo_tmp = this_OperatInfo;
            end
        end else begin
            next_PrimaryInfo_tmp = CoreMod_PrimaryInfo_NewInvalidValue();
            next_OperatInfo_tmp = CoreMod_OperationInfo_NewInvalidValue();
        end
    end
    assign isBeReady = running ? isNowTickReady : 0;
    always_ff@(posedge clk or negedge rst)begin
        if(!rst || clean)begin
            isReadyReg <= 1;
            next_PrimaryInfo_Reg <= CoreMod_PrimaryInfo_NewInvalidValue();
            next_OperatInfo_Reg <= CoreMod_OperationInfo_NewInvalidValue();
            resA_Reg <= 0;
            resB_Reg <= 0;
        end else if(start && isReadyReg)begin
            if(isNowTickReady)begin
                next_PrimaryInfo_Reg <= next_PrimaryInfo_tmp;
                next_OperatInfo_Reg <= next_OperatInfo_tmp;
                resA_Reg <= resA_Tmp;
                resB_Reg <= resB_Tmp;
            end else begin
                isReadyReg <= 0;
                next_PrimaryInfo_Reg <= CoreMod_PrimaryInfo_NewInvalidValue();
                next_OperatInfo_Reg <= CoreMod_OperationInfo_NewInvalidValue();
            end
        end else if(~isReadyReg)begin
            if(isNowTickReady)begin
                isReadyReg <= 1;
                next_PrimaryInfo_Reg <= next_PrimaryInfo_tmp;
                next_OperatInfo_Reg <= next_OperatInfo_tmp;
                resA_Reg <= resA_Tmp;
                resB_Reg <= resB_Tmp;
            end else begin
                next_PrimaryInfo_Reg <= CoreMod_PrimaryInfo_NewInvalidValue();
                next_OperatInfo_Reg <= CoreMod_OperationInfo_NewInvalidValue();
            end
        end
    end
endmodule