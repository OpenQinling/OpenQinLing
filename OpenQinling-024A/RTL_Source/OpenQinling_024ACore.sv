`include "CoreMod_Interface.svh"

module OpenQinling_024ACore(
    //时钟/硬重启信号
    input wire clk,rst,
    //所有程序员可见寄存器的信息
    output wire[63:0]allRegValue[31],
    //CPU内核连接外设/内设的接口
    output wire askInterHandle,//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
    output wire askRestartHandle,//连接所有内设和外设,当为1时表示CPU已经在重启处理流程中
    input wire allDeviceBeReadyInter,//为1表示所有内设都已经准备好进行中断响应
    input wire allDeviceBeReadyRestart,//为1表示所有内设和外设都已经准备好进行重启
    //中断模块的中断链接请求
    input wire interAsk,//外部中断请求
    input wire [7:0]interCode,//外部中断号
    //连接cpu数据缓存/MMU的接口
    IO_Interface.master ioInterface,
    //连接ICache的取指接口
    FetchOrder_Interface.master fetchFace
);
    //流水线控制信号
    wire askStopNewTaskEnterAssemblyLine;//请求流水线阻止新任务进入流水线(在中断响应过程中都会保持1)
    wire askCleanAllAssemblyLine;//将流水线中除回写模块外的其它模块全部restart:外部中断/软重启/异常/软中断会执行该操作 
    
    //控制各个模块的控制接口
    wire fetchOrder_Start,fetchOrder_Clean,fetchOrder_Running,fetchOrder_Ready;
    wire analysis_Start,analysis_Clean,analysis_Running,analysis_Ready,analysis_isBeReady;
    wire execute_Start,execute_Clean,execute_Running,execute_Ready,execute_isBeReady;
    wire writeBack_Clean,writeBack_Ready;

    //各个模块正在运行的指令的地址
    wire [39:0]fetchOrder_runAddress;
    wire [39:0]analysis_runAddress;
    wire [39:0]execute_runAddress;
    
    //各模块解析输出的指令信息
    wire CoreMod_PrimaryInfo_t fetchOrderOutput_PrimaryInfo;//取指模块输出的指令信息
    wire CoreMod_PrimaryInfo_t analysisOutput_PrimaryInfo;//解析模块输出的指令信息
    wire CoreMod_PrimaryInfo_t executeOutput_PrimaryInfo;//运算模块输出的指令信息
    
    wire CoreMod_OperationInfo_t analysisRunning_OperaInfo;//解析模块正在运行的指令的操作信息
    wire CoreMod_OperationInfo_t analysisOutput_OperaInfo;//解析模块输出的指令操作信息
    wire CoreMod_OperationInfo_t executeOutput_OperaInfo;//运算模块输出的指令操作信息
    
    //流水线模块间数据传输的接口
    wire CoreMod_GotoAskInfo_t fetchJumpAsk;//连接寄存器堆模块的接口(跳转时发出修改pc/tpc/ipc数据的请求)
    wire[63:0]resAValue;//如果resA存在回写需要，回写的值
    wire[63:0]resBValue;//如果resB存在回写需要，回写的值
    wire[63:0]regBlockValue[31];//cpu物理寄存器的数据

    //分支预测器给出的此次分支跳转结果
    wire branchJmpPredictor_isJmp;

    //各模块的分支预测指令，是否已经得到了最终结果，结果的值是多少
    wire analysis_branchJmpPredictor_gotFinalResult;//是否已经获得了结果
    wire analysis_branchJmpPredictor_finalResult;//最终结果是否要跳转
    wire execute_branchJmpPredictor_gotFinalResult;//是否已经获得了结果
    wire execute_branchJmpPredictor_finalResult;//最终结果是否要跳转

    //分支跳转预测失败的擦屁股信号
    wire branchJmpPredictor_isRecoverPC;//为1就是要恢复PC寄存器
    wire[39:0] branchJmpPredictor_recoverAddress;//恢复的值
    wire askClean_FetchMod; //请求重启取指模块
    wire askClean_InstParsingMod;//请求重启解析模块

    //数据旁路线
    DatBypass_Module datBypass(
        //当前物理寄存器的输出值
        .allRegValue(allRegValue),
        //当前执行模块输出的操作信息和回写的值
        .executeOutput_OperInfo(executeOutput_OperaInfo),
        .resAValue(resAValue),//如果resA存在回写需要，回写的值
        .resBValue(resBValue),//如果resB存在回写需要，回写的值
        //实际发给流水线前几个模块的寄存器真值
        .outputRegValue(regBlockValue)
    );

    wire isBranchJmpCode;
    wire qequestBranchPrediction;
    wire [39:0] qequestJmpAddress;
    //分支预测模块
    BranchJmpPredictor_Module branchJmpPredictor(
        .clk(clk),
        .rst(rst),
        .allRegValue(regBlockValue), //获取当前寄存器堆数据数据的接口 

        //连接取指模块的线路
        .isBranchJmpCode(isBranchJmpCode),//是否是取到的分支跳转指令(时序信号,在取指完成的那个时钟给出信号)
        .qequestBranchPrediction(qequestBranchPrediction),//是否请求获取分支预测(是取到的分支跳转指令并且在取指阶段无法确定其最终走向)
        .qequestJmpAddress(qequestJmpAddress),//请求获取分支预测结果的分支跳转指令要跳的地址(分支预测器会根据此信息匹配相应的历史数据缓存)
        .isJmp(branchJmpPredictor_isJmp), //如果当前取指模块中是分支跳转指令。给出此次的预测结果,此次分支是否跳转

        //此2个信号当中只要有一个为真,就初始化分支结果缓存
        .askInterHandle(askInterHandle),//当为1时表示CPU已经在中断响应处理流程中
        .askRestartHandle(askRestartHandle),//当为1时表示CPU已经在重启处理流程中

        //指令解析模块、执行模块发来的最终结果，以及其正在执行的分支指令的信息
        .analysis_gotFinalResult(analysis_branchJmpPredictor_gotFinalResult),//是否已经获得了结果
        .execute_gotFinalResult(execute_branchJmpPredictor_gotFinalResult),//是否已经获得了结果
        .analysis_finalResult(analysis_branchJmpPredictor_finalResult),
        .execute_finalResult(execute_branchJmpPredictor_finalResult),
        .analysisRunning_PrimaryInfo(fetchOrderOutput_PrimaryInfo),//解析模块输出的指令信息
        .executeRunning_PrimaryInfo(analysisOutput_PrimaryInfo),//运算模块输出的指令信息

        //是否最终确定要恢复PC寄存器,向回写模块发出的恢复请求
        .isRecoverPC(branchJmpPredictor_isRecoverPC),//为1就是要恢复
        .recoverAddress(branchJmpPredictor_recoverAddress),//恢复的值

        //向流水线控制模块，发出的重启信号
        //首先判断执行模块,如果预测失败就重启指令解析、取指令模块
        //如果是指令解析模块就得到了，判断其如果预测失败仅重启取指令模块
        .askClean_FetchMod(askClean_FetchMod),
        .askClean_InstParsingMod(askClean_InstParsingMod)
    );



    //执行模块操作的IO总线
    IO_Interface execute_IOBus();
    IO_Interface writeBack_IOBus();
    wire ioBus_select;//0: A接口有效  1:B接口有效
    IO_Interface_Select ioInterface_Select (
        .select(ioBus_select),
        .inInterfaceA(execute_IOBus),
        .inInterfaceB(writeBack_IOBus),
        .outputInterface(ioInterface)
    );

    //流水线启停控制模块
    AssemblyLineControl_Module assemblyLineControl(
        .clk(clk),
        .rst(rst),
        .askStopNewTaskEnterAssemblyLine(askStopNewTaskEnterAssemblyLine),
        .askCleanAllAssemblyLine(askCleanAllAssemblyLine),
        .askClean_FetchMod(askClean_FetchMod), //请求重启取指模块
        .askClean_InstParsingMod(askClean_InstParsingMod),//请求重启解析模块
        //控制各个模块的start接口
        .fetchOrder_Start(fetchOrder_Start),
        .analysis_Start(analysis_Start),
        .execute_Start(execute_Start),
        //各个模块当前运行状态的信息获取接口(isReady:当前是否就绪中或者是否即将就绪)
        .analysis_Ready(analysis_Running ? analysis_isBeReady :analysis_Ready),
        .execute_Ready(execute_Running ? execute_isBeReady :execute_Ready),
        .writeBack_Ready(writeBack_Ready),
        //控制各个模块的清空接口
        .fetchOrder_Clean(fetchOrder_Clean),
        .analysis_Clean(analysis_Clean),
        .execute_Clean(execute_Clean),
        .writeBack_Clean(writeBack_Clean)
    );

    

    //取指模块
    FetchOrderModule fetchOrder(
        //时钟/重启信号
        .clk(clk),
        .rst(rst),
        .clean(fetchOrder_Clean),
        .nextMod_isBeReady(analysis_isBeReady),
        //模块事务控制信号
        .start(fetchOrder_Start),//开始执行一次取指任务的信号
        .ready(fetchOrder_Ready),//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
        .running(fetchOrder_Running),//是否正在运行中
        .runningAddress(fetchOrder_runAddress),//如果正在运行中,运行的指令地址
        
        //初步解析取到的指令的信息(下一步传送到指令解析模块中)
        .orderInfo(fetchOrderOutput_PrimaryInfo),

        //后续流水线中指令的操作数信息(需要根据操作数使用状态来判断是否要暂停取指令、分支预测)
        .analysis_OperatInfo(analysisRunning_OperaInfo), //解析模块中正在处理的指令的操作数信息
        .operat_OperatInfo(analysisOutput_OperaInfo),   //执行模块中正在处理的指令的操作数信息
        
        //如果当前取到的是分支跳转指令,分支预测器给出的预测结果
        .isBranchJmpCode(isBranchJmpCode),//是否是取到的分支跳转指令(时序信号,在取指完成的那个时钟给出信号)
        .qequestBranchPrediction(qequestBranchPrediction),//是否请求获取分支预测(是取到的分支跳转指令并且在取指阶段无法确定其最终走向)
        .qequestJmpAddress(qequestJmpAddress),//请求获取分支预测结果的分支跳转指令要跳的地址(分支预测器会根据此信息匹配相应的历史数据缓存)
        .branchJmpPredictor_isJmp(branchJmpPredictor_isJmp),
        
        //跳转处理接口
        .allRegValue(regBlockValue), //获取当前寄存器堆数据数据的接口
        .fetchJumpAsk(fetchJumpAsk),//连接寄存器堆模块的接口(跳转时发出修改pc/tpc/ipc数据的请求)
        //连接ICache的取指接口
        .fetchFace(fetchFace)
    );

    //指令操作功能解析模块
    InstParsing_Module instParsing(
        //时钟/重启信号
        .clk(clk),
        .rst(rst),
        .clean(analysis_Clean),
        .nextMod_isBeReady(execute_isBeReady),
        .start(analysis_Start),//开始执行一次取指任务的信号运行
        .ready(analysis_Ready),//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
        .running(analysis_Running),//是否正在运行中
        .runningAddress(analysis_runAddress),//如果正在运行中,运行的指令地址
        .isBeReady(analysis_isBeReady),
        .allRegValue(regBlockValue),

        //如果当前是分支跳转指令,给出是否已经获取了最终的分支跳转结果
        .branchJmpPredictor_gotFinalResult(analysis_branchJmpPredictor_gotFinalResult),//是否已经获得了结果
        .branchJmpPredictor_finalResult(analysis_branchJmpPredictor_finalResult),//最终结果是否要跳转

        //初步解析取到的指令的信息(取指模块取到并解析后发来的指令基本信息)
        .fetch_PrimaryInfo(fetchOrderOutput_PrimaryInfo),

        .analysis_OperatInfo(analysisRunning_OperaInfo), //当前解析模块中正在处理的指令的操作数信息
        .operat_PrimaryInfo(analysisOutput_PrimaryInfo),   //下一个执行模块中正在处理的指令的基本信息
        .operat_OperatInfo(analysisOutput_OperaInfo)   //下一个执行模块中正在处理的指令的操作数信息
    );
    

    //指令功能执行模块(访存、运算、赋值等)
    Execute_Module execute(//时钟/重启信号
        .clk(clk),
        .rst(rst),
        .clean(execute_Clean),
        .start(execute_Start),//开始执行一次取指任务的信号
        .ready(execute_Ready),//是否已经运行完成上一个任务(允许执行下一个任务，以及当前输出的结果有效)
        .running(execute_Running),//是否正在运行中
        .runningAddress(execute_runAddress),//如果正在运行中,运行的指令地址
        .isBeReady(execute_isBeReady),
        
        .allRegValue(regBlockValue),//寄存器信息

        //如果当前是分支跳转指令,给出是否已经获取了最终的分支跳转结果
        .branchJmpPredictor_gotFinalResult(execute_branchJmpPredictor_gotFinalResult),//是否已经获得了结果
        .branchJmpPredictor_finalResult(execute_branchJmpPredictor_finalResult),//最终结果是否要跳转

        //输出的回写信息
        .resAValue(resAValue),//如果resA存在回写需要，回写的值
        .resBValue(resBValue),//如果resB存在回写需要，回写的值

        //解析的指令的信息(取指模块取到并解析后发来的指令基本信息)
        .this_PrimaryInfo(analysisOutput_PrimaryInfo),
        .this_OperatInfo(analysisOutput_OperaInfo),

        .next_PrimaryInfo(executeOutput_PrimaryInfo),   //下一个执行模块中处理的指令的基本信息
        .next_OperatInfo(executeOutput_OperaInfo),   //下一个执行模块中处理的指令的操作数信息);
        //连接cpu数据缓存/MMU的接口
        .ioInterface(execute_IOBus)
    );

    //指令回写模块、中断/重启处理
    WriteBack_Module writeBack(
        //时钟/重启信号
        .clk(clk),
        .rst(rst),
        .clean(writeBack_Clean),
        .ready(writeBack_Ready),

        //前几个模块的running信息
        .fetchOrderIsRunning(fetchOrder_Running),
        .analysisIsRunning(analysis_Running),
        .executeIsRunning(execute_Running),
        //前几个模块正在running的地址信息
        .fetchOrderRunningAddress(fetchOrder_runAddress),
        .analysisRunningAddress(analysis_runAddress),
        .executeRunningAddress(execute_runAddress),
        //前几个模块的ready信息
        .fetchOrderIsReady(fetchOrder_Ready),
        .analysisIsReady(analysis_Ready),
        .executeIsReady(execute_Ready),

        //前几个模块的输出信息
        .fetchOrderOutput_PrimaryInfo(fetchOrderOutput_PrimaryInfo),
        .analysisOutput_PrimaryInfo(analysisOutput_PrimaryInfo),
        .executeOutput_PrimaryInfo(executeOutput_PrimaryInfo),
        .executeOutput_OperInfo(executeOutput_OperaInfo),

        .resAValue(resAValue),//如果resA存在回写需要，回写的值
        .resBValue(resBValue),//如果resB存在回写需要，回写的值

        //取指阶段是否有短跳转的请求
        .fetchJumpAsk(fetchJumpAsk),//是否存在跳转请求
        //分支预测器是否预测失败，有要恢复PC寄存器的请求
        .branchJmpPredictor_isRecoverPC(branchJmpPredictor_isRecoverPC),//为1就是要恢复PC寄存器
        .branchJmpPredictor_recoverAddress(branchJmpPredictor_recoverAddress),//恢复的值
        //中断模块的中断链接请求
        .interAsk(interAsk),//外部中断请求
        .interCode(interCode),//外部中断号

        //CPU流水线总控制器接口
        .askStopNewTaskEnterAssemblyLine(askStopNewTaskEnterAssemblyLine),//请求流水线阻止新任务进入流水线(在中断响应过程中都会保持1)
        .askCleanAllAssemblyLine(askCleanAllAssemblyLine),//将流水线中除了当前回写模块外的其它模块全部restart:外部中断/软重启/异常/软中断会执行该操作 
        //流水线清空后恢复操作:(中断号0-15是异常中断,16-255根据中断信号来源确定是外部中断还是软件中断)
        //[外部中断:将流水线最后一个【running的 或 ready但输出有效的】模块所运行的指令地址存入tpc]
        //[异常中断:将当前回写模块正在执行的指令地址存入tpc]
        //[软中断:将当前回写模块正在执行的指令地址+4存入tpc]

        //连接cpu数据缓存/MMU的接口
        .ioBus_select(ioBus_select),
        .ioInterface(writeBack_IOBus),//通过该接口获取中断号对应处理函数的地址

        //CPU内核连接外设/内设的接口
        .askInterHandle(askInterHandle),//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
        .askRestartHandle(askRestartHandle),//连接所有内设和外设,当为1时表示CPU已经在重启处理流程中
        .allDeviceBeReadyInter(allDeviceBeReadyInter),//为1表示所有内设都已经准备好进行中断响应
        .allDeviceBeReadyRestart(allDeviceBeReadyRestart),//为1表示所有内设和外设都已经准备好进行重启

        //所有程序员可见寄存器的信息
        .allRegValue(allRegValue)
    );


    
endmodule