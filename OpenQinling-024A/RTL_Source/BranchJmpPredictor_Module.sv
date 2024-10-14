`include "CoreMod_Interface.svh"
//分支跳转预测器
module BranchJmpPredictor_Module(
        input wire clk,rst,//系统时钟和重启信号

        //所有寄存器的信息
        input wire[63:0]allRegValue[31], //获取当前寄存器堆数据数据的接口 

        //连接取指模块的线路
        input wire isBranchJmpCode,//是否是取到的分支跳转指令(时序信号,在取指完成的那个时钟给出信号)
        input wire qequestBranchPrediction,//是否请求获取分支预测(是取到的分支跳转指令并且在取指阶段无法确定其最终走向)
        input wire[39:0] qequestJmpAddress,//请求获取分支预测结果的分支跳转指令要跳的地址(分支预测器会根据此信息匹配相应的历史数据缓存)
        output wire isJmp, //如果当前取指模块中是分支跳转指令。给出此次的预测结果,此次分支是否跳转

        //此2个信号当中只要有一个为真,就初始化分支结果缓存
        input wire askInterHandle,//当为1时表示CPU已经在中断响应处理流程中
        input wire askRestartHandle,//当为1时表示CPU已经在重启处理流程中

        //指令解析模块、执行模块发来的最终结果，以及其正在执行的分支指令的信息
        input wire analysis_gotFinalResult,//指令解析模块是否已经获得了分支跳转结果
        input wire execute_gotFinalResult,//指令执行模块是否已经获得了分支跳转结果
        input wire analysis_finalResult,//指令解析模块获得的最终结果
        input wire execute_finalResult,//指令执行模块获得的最终结果
        input wire CoreMod_PrimaryInfo_t analysisRunning_PrimaryInfo,//解析模块输出的指令信息
        input wire CoreMod_PrimaryInfo_t executeRunning_PrimaryInfo,//运算模块输出的指令信息

        //是否最终确定要恢复PC寄存器,向回写模块发出的恢复请求
        output wire isRecoverPC,//为1就是要恢复
        output wire [39:0] recoverAddress,//恢复的值

        //向流水线控制模块，发出的重启信号
        //首先判断执行模块,如果预测失败就重启指令解析、取指令模块
        //如果是指令解析模块就得到了，判断其如果预测失败仅重启取指令模块
        output logic askClean_FetchMod,
        output logic askClean_InstParsingMod
    );
    //获取2个模块是否是分支预测失败了
    wire analysis_isBranchLose = analysis_gotFinalResult && analysisRunning_PrimaryInfo.branchJumpForecast != analysis_finalResult;
    wire execute_isBranchLose = execute_gotFinalResult && executeRunning_PrimaryInfo.branchJumpForecast != execute_finalResult;

    //如果失败了，2个模块应该要给PC寄存器恢复的地址
    wire [39:0] analysis_recoverPC = analysis_finalResult ? analysisRunning_PrimaryInfo.branchJumpAddress : analysisRunning_PrimaryInfo.orderAddress;
    wire [39:0] execute_recoverPC = execute_finalResult ? executeRunning_PrimaryInfo.branchJumpAddress : executeRunning_PrimaryInfo.orderAddress;

    //向回写模块发出的PC寄存器恢复请求
    assign isRecoverPC = (analysis_isBranchLose || execute_isBranchLose);
    assign recoverAddress = execute_isBranchLose ? execute_recoverPC : analysis_recoverPC;

    //向流水线控制模块发出的重启信号
    always_comb begin
        if(execute_isBranchLose)begin
            askClean_FetchMod = 1;
            askClean_InstParsingMod = 1;
        end else if(analysis_isBranchLose)begin
            askClean_FetchMod = 1;
            askClean_InstParsingMod = 0;
        end else begin
            askClean_FetchMod = 0;
            askClean_InstParsingMod = 0;
        end
    end

    //饱和计数器默认初始状态 (默认状态 弱不跳转)
    wire [1:0] predictionHistoryCache_initState = 2'b01;
    /*
        0: 强不跳转
        1: 弱不跳转
        2: 弱跳转
        3: 强不跳转
    */

    //使用2位饱和计数器算法
    reg [1:0] predictionHistoryCache = predictionHistoryCache_initState;//分支预测的历史缓存数据
    assign isJmp = predictionHistoryCache>1;

    always_ff @(posedge clk or negedge rst)begin
        if(!rst || askInterHandle || askRestartHandle)begin
            predictionHistoryCache <= predictionHistoryCache_initState;
        end else if(execute_gotFinalResult || analysis_gotFinalResult)begin
            automatic logic [3:0] updateCacheData = predictionHistoryCache;
            if(execute_gotFinalResult)begin
                if(execute_finalResult)begin
                    updateCacheData = updateCacheData + 1;
                end else begin
                    updateCacheData = updateCacheData - 1;
                end
            end
            if(execute_isBranchLose && analysis_gotFinalResult)begin
                if(analysis_finalResult)begin
                    updateCacheData = updateCacheData + 1;
                end else begin
                    updateCacheData = updateCacheData - 1;
                end
            end
            if(updateCacheData[3])begin//更新结果为负数,取最低值0
                predictionHistoryCache <= 0;
            end else if(updateCacheData[2])begin//更新结果大于3,取最大值3
                predictionHistoryCache <= 3;
            end else begin //0-3之间，取原值
                predictionHistoryCache <= updateCacheData;
            end
        end else if(isBranchJmpCode && !qequestBranchPrediction)begin
            if(allRegValue[RegisterBlockCode_R25_BJS] && predictionHistoryCache!=2'b00)begin
                //历史缓存-1
                predictionHistoryCache <= predictionHistoryCache - 1;
            end else if(!allRegValue[RegisterBlockCode_R25_BJS] && predictionHistoryCache!=2'b11)begin
                //历史缓存+1
                predictionHistoryCache <= predictionHistoryCache + 1;
            end
        end
    end
endmodule