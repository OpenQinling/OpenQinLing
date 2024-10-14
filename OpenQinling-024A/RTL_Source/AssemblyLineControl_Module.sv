

module AssemblyLineControl_Module(
        input wire clk,
        input wire rst,//所有模块全部restart
        input wire askStopNewTaskEnterAssemblyLine,//请求流水线阻止新任务进入流水线(在中断响应过程中都会保持1)
        input wire askCleanAllAssemblyLine,//将流水线中除回写模块外的其它模块全部restart:外部中断/软重启/异常/软中断会执行该操作 
        input wire askClean_FetchMod, //请求重启取指模块
        input wire askClean_InstParsingMod,//请求重启解析模块
        //控制各个模块的start接口
        output logic fetchOrder_Start,
        output logic analysis_Start,
        output logic execute_Start,
        //各个模块当前运行状态的信息获取接口(isReady)
        input logic analysis_Ready,
        input logic execute_Ready,
        input logic writeBack_Ready,

        //控制各个模块的clean接口
        output logic fetchOrder_Clean,
        output logic analysis_Clean,
        output logic execute_Clean,
        output logic writeBack_Clean
    );


    always_comb begin
        execute_Start = writeBack_Ready;
        analysis_Start = execute_Ready && writeBack_Ready;
        fetchOrder_Start = !askStopNewTaskEnterAssemblyLine && analysis_Ready && execute_Ready && writeBack_Ready;
        if(askCleanAllAssemblyLine)begin
            fetchOrder_Clean = 1;
            analysis_Clean  = 1;
            execute_Clean  = 1;
            writeBack_Clean = 0;
        end else begin
            fetchOrder_Clean = askClean_FetchMod;
            analysis_Clean  = askClean_InstParsingMod;
            execute_Clean  = 0;
            writeBack_Clean = 0;
        end
    end



endmodule