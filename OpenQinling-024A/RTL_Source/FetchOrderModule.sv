`include "CoreMod_Interface.svh"
`include "InstParsing_Module.svh"
module FetchOrderModule(
    //时钟/重启信号
    input wire clk,rst,
    input wire clean,  //清除当前正在运行的任务,将模块恢复到空闲状态
    input wire nextMod_isBeReady,//下一个模块是否已经即将完成
    //模块事务控制信号
    input wire start,//开始执行一次取指任务的信号
    output wire ready,//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
    output wire running,//是否正在运行中
    output wire [39:0]runningAddress,//如果正在运行中,运行的指令地址
    //初步解析取到的指令的信息(下一步传送到指令解析模块中)
    output wire CoreMod_PrimaryInfo_t orderInfo,

    //后续流水线中指令的操作数信息(需要根据操作数使用状态来判断是否要暂停取指令、分支预测)
    input wire CoreMod_OperationInfo_t analysis_OperatInfo, //解析模块中正在处理的指令的操作数信息
    input wire CoreMod_OperationInfo_t operat_OperatInfo,   //执行模块中正在处理的指令的操作数信息
    
    //连接分支预测器
    output wire isBranchJmpCode,//是否是取到的分支跳转指令(时序信号,在取指完成的那个时钟给出信号)
    output wire qequestBranchPrediction,//是否请求获取分支预测(是取到的分支跳转指令并且在取指阶段无法确定其最终走向)
    output [39:0] qequestJmpAddress,//请求获取分支预测结果的分支跳转指令要跳的地址(分支预测器会根据此信息匹配相应的历史数据缓存)
    input wire branchJmpPredictor_isJmp,//分支预测器给出的预测结果(为1是要跳转,为0不要跳转)

    //跳转处理接口
    input wire[63:0]allRegValue[31], //获取当前寄存器堆数据数据的接口
    output  CoreMod_GotoAskInfo_t fetchJumpAsk,//连接寄存器堆模块的接口(跳转时发出修改pc/tpc/ipc数据的请求)
    //连接ICache的取指接口
    FetchOrder_Interface.master fetchFace
);
    //是否正在运行中
    reg isReadyReg = 1;
    assign ready = isReadyReg;
    assign running = (start && isReadyReg) || ~isReadyReg;
    assign runningAddress = fetchFace.address;

    //向取指总线输出信号
    assign fetchFace.taskValid[3] = 0;//024A内核每次只支持取一条指令，后3个取值请求始终为0
    assign fetchFace.taskValid[2] = 0;//024A内核每次只支持取一条指令，后3个取值请求始终为0
    assign fetchFace.taskValid[1] = 0;//024A内核每次只支持取一条指令，后3个取值请求始终为0
    assign fetchFace.taskValid[0] = start || running;//输出取指请求信号
    assign fetchFace.address = allRegValue[RegisterBlockCode_R29_PC];//输出取指信号
    
    reg isLoadCodeCompleteReg;//是否已经取到指令二进制码
    //取到的指令源码信息
    reg taskErrorReg;
    reg [31:0]readBusReg;
    reg [39:0]readAddressReg;
    wire taskError = isLoadCodeCompleteReg ? taskErrorReg : fetchFace.taskError[0];
    wire [31:0]readBus = isLoadCodeCompleteReg ? readBusReg : fetchFace.readBus[0];
    wire [39:0]readAddress = isLoadCodeCompleteReg ? readAddressReg : fetchFace.address;
    
    wire fetchIsReady = (running & fetchFace.taskReady[0]);
    wire isLoadCodeComplete = fetchIsReady || isLoadCodeCompleteReg;
    logic isFetchComplete;//是否已经取指令完毕(取到二进制码，并且如果是跳转/系统控制指令解析完毕后)

    //输出的解析信息
    CoreMod_PrimaryInfo_t outputOrderInfoReg;
    assign orderInfo = outputOrderInfoReg;
    CoreMod_PrimaryInfo_t orderInfoWire;


    //当前流水线中后续模块是否有对BJS寄存器的写入操作未执行完成
    wire bjsWriteNotFinished = (analysis_OperatInfo.resA == RegisterBlockCode_R25_BJS || 
                                        operat_OperatInfo.resA == RegisterBlockCode_R25_BJS||
                                        analysis_OperatInfo.resB == RegisterBlockCode_R25_BJS || 
                                        operat_OperatInfo.resB == RegisterBlockCode_R25_BJS);
    assign qequestBranchPrediction = bjsWriteNotFinished;
    //在ICache总线读写完成的处理,解析除orderInfo
    always_comb begin
        orderInfoWire.isValid = 1;
        orderInfoWire.orderCode = readBus;
        orderInfoWire.orderAddress = readAddress;

        //解析是否是软重启请求
        orderInfoWire.isRestart = !taskError &
                                    readBus[31:29] == InstTypeCode_SysCtrl &
                                    readBus[28:25] == InstTypeCode_SysCtrlType_Restart;

        //解析是否有中断请求
        if(taskError)begin
            orderInfoWire.isHaveAbnormal = 1;//当前执行的模块是否存在异常中断请求
            orderInfoWire.abnormalCode = SystemInterruptCode_MemoryAccessError;//如果存在中断请求，请求的号码
        end else if(!taskError &
            readBus[31:29] == InstTypeCode_SysCtrl &
            readBus[28:25] == InstTypeCode_SysCtrlType_SoftInt) begin//中断请求
            orderInfoWire.isHaveAbnormal = 1;//当前执行的模块是否存在异常中断请求
            orderInfoWire.abnormalCode = readBus[7:0]>=16 ? readBus[7:0] : SystemInterruptCode_AnalysisCodeError;//如果存在中断请求，请求的号码
        end else begin
            orderInfoWire.isHaveAbnormal = 0;//当前执行的模块是否存在异常中断请求
            orderInfoWire.abnormalCode = 0;//如果存在中断请求，请求的号码
        end

        //解析是否有分支跳转请求
        if(!taskError &
            readBus[31:29] == InstTypeCode_Jemp &
            readBus[28:28] == InstTypeCode_JempType_ShortJmp &
            readBus[27:27] == InstTypeCode_Jemp_ShortJmpType_JNE)begin
            //isBranchJump:是否是分支跳转 如果当前取到的是分支跳转指令,并且后续流水线中有对BJS寄存器的赋值为完成,就置为1,用于告知后续流水线模块此条分支跳转指令尚未运行完成(处于分支预测器推测的状态)
            orderInfoWire.isBranchJump = bjsWriteNotFinished;
            orderInfoWire.branchJumpForecast = branchJmpPredictor_isJmp;//分支预测的结果(0:不跳转,1:跳转) [BJS==0就要跳转]
            orderInfoWire.branchJumpAddress = {allRegValue[RegisterBlockCode_R29_PC][39:27],readBus[26:0]};//如果分支跳转最终确定是需要跳转的话，其跳转地址
        end else begin
            orderInfoWire.isBranchJump = 0;//是否是分支跳转
            orderInfoWire.branchJumpForecast = 0;//分支预测的结果(0:不跳转,1:跳转)
            orderInfoWire.branchJumpAddress = 0;//如果分支跳转最终确定是需要跳转的话，其跳转地址
        end

        //解析是否有模式修改请求
        if(!taskError &
            readBus[31:29] == InstTypeCode_Jemp &
            readBus[28:28] == InstTypeCode_JempType_LongJmp &
            readBus[27:0] == InstTypeCode_Jemp_LongJmpType_EIT)begin //开启中断响应
            orderInfoWire.isHavePatternCtrl = 1;//是否是模式调整
            orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;//模式调整选择码
        end else if(!taskError &
            readBus[31:29] == InstTypeCode_SysCtrl &
            readBus[28:25] == InstTypeCode_SysCtrlType_AccCtrl)begin //系统权限控制
            case (readBus[24:0])
                InstTypeCode_SysCtrl_AccCtrlType_DisableInt: begin //关闭中断
                    orderInfoWire.isHavePatternCtrl = 1;
                    orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_DisableInterrupt;
                end
                InstTypeCode_SysCtrl_AccCtrlType_EnableInt: begin //启用中断
                    orderInfoWire.isHavePatternCtrl = 1;
                    orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;
                end
                InstTypeCode_SysCtrl_AccCtrlType_EnableProtect: begin //开启保护
                    orderInfoWire.isHavePatternCtrl = 1;
                    orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_EnableProtect;
                end
                default: begin //未知权限控制类型，不执行任何操作
                    orderInfoWire.isHavePatternCtrl = 0;
                    orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;
                end
            endcase
        end else begin
            orderInfoWire.isHavePatternCtrl = 0;//是否是模式调整
            orderInfoWire.patternCtrlCode = CpuPatternCtrlCode_EnableInterrupt;//模式调整选择码
        end
    end

    wire isJempCode = isLoadCodeComplete && !taskError && (readBus[31:29] == InstTypeCode_Jemp);
    assign qequestJmpAddress = {allRegValue[RegisterBlockCode_R29_PC][39:27],readBus[26:0]};
    assign isBranchJmpCode = isJempCode && readBus[28:28] == InstTypeCode_JempType_ShortJmp && readBus[27:27] == InstTypeCode_Jemp_ShortJmpType_JNE;
    //发出取指完成信号： 一般命令在解析完orderInfo同时就发出
    // 跳转指令先完成(修改TPC/PC寄存器)，然后发出
    always_comb begin
        if(isJempCode)begin //读取完成,且非取指失败,是跳转指令
            if(readBus[28:28] == InstTypeCode_JempType_ShortJmp &
                (readBus[27:27] == InstTypeCode_Jemp_ShortJmpType_JMP || readBus[27:27] == InstTypeCode_Jemp_ShortJmpType_JNE))begin //普通短跳转(无依赖)
                automatic logic isJNE = readBus[27:27] == InstTypeCode_Jemp_ShortJmpType_JNE;
                isFetchComplete = 1;
                fetchJumpAsk.jumpType = CpuGotoAskType_JMP_JNE;
                
                if(((bjsWriteNotFinished && !orderInfoWire.branchJumpForecast)|| 
                   (!bjsWriteNotFinished && allRegValue[RegisterBlockCode_R25_BJS])) && isJNE)begin
                    //如果是分支跳转,并且BJS!=0，就不进行跳转,PC+4取下一条指令 (对于if(value){}的代码块来说,如果value不为0,就执行内部的程序,为0跳转到if结尾)
                    fetchJumpAsk.jumpAddress = allRegValue[RegisterBlockCode_R29_PC]+4;
                end else begin
                    //如果是分支跳转并且BJS==0，或者为立即跳转。直接将PC设置为跳转到的地址
                    fetchJumpAsk.jumpAddress = qequestJmpAddress;
                end
            end
            else if(readBus[28:28] == InstTypeCode_JempType_LongJmp &
                (readBus[27:0] == InstTypeCode_Jemp_LongJmpType_EIT||
                readBus[27:0] == InstTypeCode_Jemp_LongJmpType_IT))begin //中断函数退出(依赖IPC寄存器)
                
                isFetchComplete = !(analysis_OperatInfo.resA == RegisterBlockCode_R28_IPC || 
                                    operat_OperatInfo.resA == RegisterBlockCode_R28_IPC||
                                    analysis_OperatInfo.resB == RegisterBlockCode_R28_IPC || 
                                    operat_OperatInfo.resB == RegisterBlockCode_R28_IPC);
                fetchJumpAsk.jumpType = isFetchComplete ? CpuGotoAskType_IPC : CpuGotoAskType_NOTJMP;
                fetchJumpAsk.jumpAddress = allRegValue[RegisterBlockCode_R28_IPC];
            end
            else if(readBus[28:28] == InstTypeCode_JempType_LongJmp &
                readBus[27:0] == InstTypeCode_Jemp_LongJmpType_RT)begin //普通函数调用与退出(依赖TPC寄存器)
                isFetchComplete = !(analysis_OperatInfo.resA == RegisterBlockCode_R27_TPC || 
                                    operat_OperatInfo.resA == RegisterBlockCode_R27_TPC||
                                    analysis_OperatInfo.resB == RegisterBlockCode_R27_TPC || 
                                    operat_OperatInfo.resB == RegisterBlockCode_R27_TPC);
                fetchJumpAsk.jumpType = isFetchComplete ? CpuGotoAskType_TPC : CpuGotoAskType_NOTJMP;
                fetchJumpAsk.jumpAddress = allRegValue[RegisterBlockCode_R27_TPC];
            end 
            else begin //不进行任何处理
                isFetchComplete = 1;
                fetchJumpAsk.jumpType = CpuGotoAskType_JMP_JNE;
                fetchJumpAsk.jumpAddress = allRegValue[RegisterBlockCode_R29_PC]+4;
            end
        end
        else begin //非跳转指令,是否取指完成取决于是否已经取到指了
            isFetchComplete = isLoadCodeComplete;
            fetchJumpAsk.jumpType = isLoadCodeComplete ? CpuGotoAskType_JMP_JNE : CpuGotoAskType_NOTJMP;
            fetchJumpAsk.jumpAddress = allRegValue[RegisterBlockCode_R29_PC]+4;
        end
    end
    //当指令取到以后，存入寄存器缓存起来
    always_ff @(posedge clk or negedge rst)begin
        if(!rst || isFetchComplete || clean)begin
            isLoadCodeCompleteReg <= 0;
        end else if(fetchIsReady & !isLoadCodeCompleteReg)begin
            isLoadCodeCompleteReg <= 1;
            taskErrorReg <= fetchFace.taskError[0];
            readBusReg <= fetchFace.readBus[0];
            readAddressReg <= fetchFace.address;
        end
    end
    
    always_ff @(posedge clk or negedge rst)begin
        if(!rst  || clean)begin
            isReadyReg <= 1;
            outputOrderInfoReg <= CoreMod_PrimaryInfo_NewInvalidValue();
        end else if(start & isReadyReg)begin
            if(isFetchComplete)begin
                outputOrderInfoReg <= orderInfoWire;
            end else begin
                isReadyReg <= 0;
                if(nextMod_isBeReady)begin
                    outputOrderInfoReg <= CoreMod_PrimaryInfo_NewInvalidValue();
                end
            end
        end else if(~isReadyReg && isFetchComplete)begin
            isReadyReg <= 1;
            outputOrderInfoReg <= orderInfoWire;
        end else if(nextMod_isBeReady)begin
            outputOrderInfoReg <= CoreMod_PrimaryInfo_NewInvalidValue();
        end
    end
endmodule
