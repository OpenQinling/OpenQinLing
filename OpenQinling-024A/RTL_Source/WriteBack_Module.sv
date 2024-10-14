`include "CoreMod_Interface.svh"

module WriteBack_Module(
    //时钟/重启信号
    input wire clk,rst,
    input wire clean,
    output wire ready,

    //前几个模块的running信息
    input wire fetchOrderIsRunning,
    input wire analysisIsRunning,
    input wire executeIsRunning,
    //前几个模块正在running的地址信息
    input wire [39:0]fetchOrderRunningAddress,
    input wire [39:0]analysisRunningAddress,
    input wire [39:0]executeRunningAddress,
    //前几个模块的ready信息
    input wire fetchOrderIsReady,
    input wire analysisIsReady,
    input wire executeIsReady,

    //前几个模块的输出信息
    input wire CoreMod_PrimaryInfo_t fetchOrderOutput_PrimaryInfo,//取指模块输出的指令信息
    input wire CoreMod_PrimaryInfo_t analysisOutput_PrimaryInfo,//解析模块输出的指令信息
    input wire CoreMod_PrimaryInfo_t executeOutput_PrimaryInfo,//运算模块输出的指令信息
    input wire CoreMod_OperationInfo_t executeOutput_OperInfo,
    input wire[63:0]resAValue,//如果resA存在回写需要，回写的值
    input wire[63:0]resBValue,//如果resB存在回写需要，回写的值

    //取指阶段是否有短跳转的请求
    input wire CoreMod_GotoAskInfo_t fetchJumpAsk,//是否存在跳转请求
    //分支预测器是否预测失败，有要恢复PC寄存器的请求
    input wire branchJmpPredictor_isRecoverPC,//为1就是要恢复PC寄存器
    input wire[39:0] branchJmpPredictor_recoverAddress,//恢复的值
    //中断模块的中断链接请求
    input wire interAsk,//外部中断请求
    input wire [7:0]interCode,//外部中断号

    //CPU流水线总控制器接口
    output logic askStopNewTaskEnterAssemblyLine = 0,//请求流水线阻止新任务进入流水线(在中断响应过程中都会保持1)
    output logic askCleanAllAssemblyLine = 0,//将流水线中除了当前回写模块外的其它模块全部restart:外部中断/软重启/异常/软中断会执行该操作 
    //流水线清空后恢复操作:(中断号0-15是异常中断,16-255根据中断信号来源确定是外部中断还是软件中断)
    //[外部中断:将流水线最后一个【running的 或 ready但输出有效的】模块所运行的指令地址存入ipc]
    //[异常中断:将当前回写模块正在执行的指令地址存入ipc]
    //[软中断:将当前回写模块正在执行的指令地址+4存入ipc]

    //连接cpu数据缓存/MMU的接口
    output wire ioBus_select,//0: 执行模块占用内存总线  1:中断处理模块占用内存总线
    IO_Interface.master ioInterface,//通过该接口获取中断号对应处理函数的地址

    //CPU内核连接外设/内设的接口
    output logic askInterHandle = 0,//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
    output logic askRestartHandle = 0,//连接所有内设和外设,当为1时表示CPU已经在重启处理流程中
    input wire allDeviceBeReadyInter,//为1表示所有内设都已经准备好进行中断响应
    input wire allDeviceBeReadyRestart,//为1表示所有内设和外设都已经准备好进行重启

    //所有程序员可见寄存器的信息
    output wire[63:0]allRegValue[31]
);

    //cpu寄存器组的初始值(SP=65536,PC=2048,其它默认0)
    wire [63:0]cpuPhysicalRegsInitValue[31];
    generate
        genvar i;
        for(i = 0;i<31;i++)begin
            if(i == RegisterBlockCode_R29_PC)begin
                assign cpuPhysicalRegsInitValue[i] = 2048;
            end else if(i == RegisterBlockCode_R26_SP)begin
                assign cpuPhysicalRegsInitValue[i] = 65536;
            end else begin
                assign cpuPhysicalRegsInitValue[i] = 0;
            end
        end
    endgenerate

    //cpu的物理寄存器组(对程序员可见)
    reg [63:0]cpuPhysicalRegs[31] = cpuPhysicalRegsInitValue;
    assign allRegValue = cpuPhysicalRegs;
    assign ready = 1;

    //中断响应的处理流程
    logic [1:0] responseInterStep = 0;
    logic [7:0] interCodeReg = 0;
    assign ioBus_select = responseInterStep != 0;
    //0: 检查是否有中断请求,以及当前cpu状态是否允许响应中断；
    //     第一步：向内设发出准备中断处理的信号(内设包括: SysTick、MMU、Cache等)
    //     第二步：清空流水线,并且请求流水线清空+阻止新任务进入流水线执行
    //     第三步: 找到中断处理后应当恢复的PC地址，存入到IPC中
    //1: 等待所有内设的响应中断处理信号(在这个过程中应对至少实现关闭MMU虚拟地址映射功能，实现使用物理地址访问内存(否则必然无法正确读到中断向量表,可能直接造成系统自重启的严重不可恢复的错误)。可以选自性的关闭缓存器，关闭系统定时器)
    //      控制IO模块,根据中断号找到目标中断向量内存地址读取其中响应函数的地址数据
    //2: 等待IO读取模块的响应,如若读取失败直接选择重启CPU(中断向量表读取失败为不可恢复的严重错误,进入CPU重启响应的处理流程中)
    //     如果读取成功,将读取到的地址存入PC,并允许流水线进入新的任务

    //CPU重启响应的处理流程
    logic responseRestartStep = 0;
    //0: 检查是否有重启请求,以及当前cpu状态是否允许响应重启；
    //    第一步：如果允许,向所有内设和外设发出重启处理信号
    //    第二步：清空流水线,并且请求流水线清空+阻止新任务进入流水线执行
    //    第三步: 将CPU所有物理寄存器恢复初始值
    //1: 等待所有内设和外设响应了重启信号
    //    如果全部响应了,重新允许流水线进入新的任务

    //操作内存总线的接口
    logic ioTaskValid = 0;//此时是否存在有效的读写任务
    logic [39:0] ioAddress = 0;//读写的地址
    assign ioInterface.taskValid = ioTaskValid;
    assign ioInterface.address = ioAddress;
    assign ioInterface.rwCtrl = 0;
    assign ioInterface.widthCtr = 3;
    assign ioInterface.writeBus = 0;

    //是否有外部中断
    wire isExtInter = interAsk && interCode>=16;
    //是否有系统异常或软件中断
    wire isSysAbnormal_SoftInter = executeOutput_PrimaryInfo.isHaveAbnormal && executeOutput_PrimaryInfo.isValid;

    always_ff @(posedge clk or negedge rst)begin
        if(~rst)begin
            //硬重启CPU
            responseInterStep <= 0;
            responseRestartStep <= 0;
            askInterHandle <= 0;
            askRestartHandle <= 0;
            cpuPhysicalRegs <= cpuPhysicalRegsInitValue;
            askStopNewTaskEnterAssemblyLine <= 0;
            askCleanAllAssemblyLine <= 0;
            interCodeReg <= 0;
            ioTaskValid <= 0;
            ioAddress <= 0;
        end else if(responseRestartStep != 0)begin //重启CPU中
            if(allDeviceBeReadyRestart)begin
                responseRestartStep <= 0;
                askRestartHandle <= 0;
                askCleanAllAssemblyLine <= 0;
                askStopNewTaskEnterAssemblyLine <= 0;
            end
        end else if(!cpuPhysicalRegs[RegisterBlockCode_R30_SYS][1] && executeOutput_PrimaryInfo.isValid && executeOutput_PrimaryInfo.isRestart)begin
            //开始响应CPU的软重启
            responseRestartStep <= 1;
            askRestartHandle <= 1;
            cpuPhysicalRegs <= cpuPhysicalRegsInitValue;
            askCleanAllAssemblyLine <= 1;
            askStopNewTaskEnterAssemblyLine <= 1;
        end else if(responseInterStep != 0)begin //CPU中断响应中
            if(responseInterStep == 1)begin
                if(allDeviceBeReadyInter)begin
                    responseInterStep <= 2;
                    ioTaskValid <= 1;
                    ioAddress <= interCodeReg<<3;
                end
            end else if(responseInterStep == 2)begin
                if(ioInterface.taskReady)begin
                    ioTaskValid <= 0;
                    ioAddress <= 0;
                    if(ioInterface.taskError)begin  //访问中断向量表出错,进入软重启流程
                        responseInterStep <= 0;
                        responseRestartStep <= 1;
                        askRestartHandle <= 1;
                        cpuPhysicalRegs <= cpuPhysicalRegsInitValue;
                    end else begin //访存成功
                        responseInterStep <= 3;
                        cpuPhysicalRegs[RegisterBlockCode_R29_PC] <= ioInterface.readBus[39:0];
                    end
                end
            end else if(responseInterStep == 3)begin
                responseInterStep <= 0;
                askCleanAllAssemblyLine <= 0;
                askStopNewTaskEnterAssemblyLine <= 0;
            end
        end else if(cpuPhysicalRegs[RegisterBlockCode_R30_SYS][0] && (isExtInter || isSysAbnormal_SoftInter))begin//开始响应中断处理
            //是否存在外部中断,立即执行中断响应。假如此时存在系统异常/软件中断,不对此两种中断进行任何处理。
            //系统将在外部中断处理完毕后重新在此运行一遍产生系统异常/软件中断的指令
            responseInterStep <= 1;
            askCleanAllAssemblyLine <= 1;
            askStopNewTaskEnterAssemblyLine <= 1;

            //缓存中断号
            if(isExtInter)begin
                interCodeReg <= interCode;
            end else begin
                interCodeReg <= executeOutput_PrimaryInfo.abnormalCode;
            end

            //设置CPU SYS寄存器,关闭保护模式和中断响应
            cpuPhysicalRegs[RegisterBlockCode_R30_SYS] <= 0;
            //获取中断处理完成后应该回到的程序位置
            if(isExtInter)begin//将流水线最后一个模块所运行的指令地址存入ipc
                if(executeOutput_PrimaryInfo.isValid && executeIsReady)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= executeOutput_PrimaryInfo.orderAddress;
                end else if(executeIsRunning)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= executeRunningAddress;
                end else if(analysisOutput_PrimaryInfo.isValid && analysisIsReady)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= analysisOutput_PrimaryInfo.orderAddress;
                end else if(analysisIsRunning)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= analysisRunningAddress;
                end else if(fetchOrderOutput_PrimaryInfo.isValid && fetchOrderIsReady)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= fetchOrderOutput_PrimaryInfo.orderAddress;
                end else if(fetchOrderIsRunning)begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= fetchOrderRunningAddress;
                end else begin
                    cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= cpuPhysicalRegs[RegisterBlockCode_R29_PC];
                end
            end else if(executeOutput_PrimaryInfo.abnormalCode >= 16)begin//是软件中断就将当前回写模块正在执行的指令地址+4存入ipc
                cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= executeOutput_PrimaryInfo.orderAddress + 4;
            end else begin//是异常就将当前回写模块正在执行的指令地址存入ipc
                cpuPhysicalRegs[RegisterBlockCode_R28_IPC] <= executeOutput_PrimaryInfo.orderAddress;
            end
        end else begin
            //回写寄存器/程序跳转
            integer i;
            for(i = 0;i<31;i++)begin
                if(i == RegisterBlockCode_R29_PC)begin
                    if(branchJmpPredictor_isRecoverPC)begin
                        cpuPhysicalRegs[i] <= branchJmpPredictor_recoverAddress;
                    end else if(fetchJumpAsk.jumpType != CpuGotoAskType_NOTJMP)begin
                        cpuPhysicalRegs[i] <= fetchJumpAsk.jumpAddress;
                    end
                end else if(i == RegisterBlockCode_R27_TPC  & fetchJumpAsk.jumpType == CpuGotoAskType_TPC)begin
                    //函数链接请求，修改tpc为pc+4
                    cpuPhysicalRegs[i] <= cpuPhysicalRegs[RegisterBlockCode_R29_PC]+4;
                end else if(fetchJumpAsk.jumpType == CpuGotoAskType_IPC & i == RegisterBlockCode_R28_IPC)begin
                    //中断链接请求，修改ipc
                    cpuPhysicalRegs[i] <= cpuPhysicalRegs[RegisterBlockCode_R29_PC]+4;
                end else if(i == RegisterBlockCode_R30_SYS & executeOutput_PrimaryInfo.isHavePatternCtrl && executeOutput_PrimaryInfo.isValid)begin
                    //CPU权限修改
                    if(executeOutput_PrimaryInfo.patternCtrlCode == CpuPatternCtrlCode_EnableInterrupt)begin
                        //开启中断响应
                        CpuPatternCtrlCode_EnableInterrupt: cpuPhysicalRegs[i] <= {cpuPhysicalRegs[i][1],1'b1};
                    end else if(executeOutput_PrimaryInfo.patternCtrlCode == CpuPatternCtrlCode_EnableProtect)begin
                        //开启保护模式
                        CpuPatternCtrlCode_EnableProtect: cpuPhysicalRegs[i] <= {1'b1,cpuPhysicalRegs[i][0]};
                    end else if(executeOutput_PrimaryInfo.patternCtrlCode == CpuPatternCtrlCode_DisableInterrupt && 
                                !cpuPhysicalRegs[RegisterBlockCode_R30_SYS][1])begin 
                        //关闭中断响应,必须要非保护模式下才能有效执行,保护模式下无视该指令
                        CpuPatternCtrlCode_DisableInterrupt: cpuPhysicalRegs[i] <= {cpuPhysicalRegs[i][1],1'b0};
                    end
                end else if(i != RegisterBlockCode_IMME_INVALID & i != RegisterBlockCode_R30_SYS && executeOutput_OperInfo.isValid)begin
                    //写回运算结果
                    if(i == executeOutput_OperInfo.resA)begin
                        cpuPhysicalRegs[i] <= resAValue;
                    end else if(i == executeOutput_OperInfo.resB)begin
                        cpuPhysicalRegs[i] <= resBValue;
                    end
                end
            end //for循环 遍历处理每一个寄存器的写入请求
        end//判断是中断处理还是 回写寄存器/程序跳转
    end//判断是时钟上升沿执行的程序 还是 重启执行的程序
    
endmodule