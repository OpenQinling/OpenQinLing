/**********************************************************/
//                  ALU 64位整数乘法运算模块
/**********************************************************/


//整数乘法,8个时钟tick完成一次运算(UINT64 INT64适用)
module Multiply_IntUInt64(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady
);
    reg isReadyReg = 1'b1;
    reg [5:0] counter = 6'd0;  // 用于计数时钟周期
    reg [63:0]tmpCValue = 64'd0;//运算过程中的临时存储
    assign numC = tmpCValue;

    //保存A/B临时的值
    reg [63:0]tmpAValue;
    reg [63:0]tmpBValue;

    
    //将tmpBValue切分为8位宽总线到numB_tmp，counter选择那一块有效
    wire[7:0] numB_tmp;
    FU_ShareMod_SplitterBus#(
        .BUS_WIDTH(64),
        .SPLITTER_WIDTH(8)
    ) spl(
        .in_bus(tmpBValue),
        .out_bus(numB_tmp),
        .out_select(counter)
    );

    wire[63:0] resultTmp;
    FU_ShareMod_MultiplySpl#(
        .BUS_WIDTH(64)
    )  mulSpl(
        .a(tmpAValue),
        .b(numB_tmp),
        .c(resultTmp)
    );


    assign isNowTickReady =  counter == 8;
    always @(posedge clk or negedge rst) begin
        if (!rst || clean) begin //重启运算器
            counter <= 6'd0;
            isReadyReg <= 1'b1;
            tmpCValue <= 64'd0;
        end else if (start & isReadyReg) begin //开启运算
            counter <= 6'd0;
            isReadyReg <= 1'b0;
            tmpAValue <= {64'd0,numA};
            tmpBValue <= numB;
            tmpCValue <= 64'd0;
        end else if(!isReadyReg && counter<=7)begin //运算过程中
            tmpCValue <= tmpCValue + (resultTmp << (counter<<3));
            counter <= counter + 1;
        end else if(counter == 8)begin
            isReadyReg <= 1'b1;
            counter <= 3'd0;
        end
    end
endmodule
