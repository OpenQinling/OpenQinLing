`include "CoreMod_Interface.svh"
module DatBypass_Module(
    //当前物理寄存器的输出值
    input wire[63:0]allRegValue[31],
    //当前执行模块输出的操作信息和回写的值
    input wire CoreMod_OperationInfo_t executeOutput_OperInfo,
    input wire[63:0]resAValue,//如果resA存在回写需要，回写的值
    input wire[63:0]resBValue,//如果resB存在回写需要，回写的值
    //实际发给流水线前几个模块的寄存器真值
    output logic[63:0]outputRegValue[31]
);
    always_comb begin
        integer i;
        for(i = 0;i<31;i++)begin
            if(executeOutput_OperInfo.resA == i)begin
                outputRegValue[i] = resAValue;
            end else if(executeOutput_OperInfo.resB == i)begin
                outputRegValue[i] = resBValue;
            end else begin
                outputRegValue[i] = allRegValue[i];
            end
        end
    end
endmodule