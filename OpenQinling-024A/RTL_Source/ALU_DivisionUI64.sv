/**********************************************************/
//                  ALU 64位整数除法运算模块
/**********************************************************/
//整形除法/取余(INT64/UINT64) 8个周期运算完成
module Division_UIntInt64 (
    //ALU模块通用接口
    input logic clk, 
    input logic rst, 
    input logic clean,
    input logic start, 
    input logic [63:0] numA, 
    input logic [63:0] numB, 
    output logic [63:0] numC, //运算过程中的临时存储
    output logic isNowTickReady, //是否在当前这个时钟沿就完成运算
    //当前模块专有接口
    input logic [1:0]funcSelect,//[1]={0:返回除法运算结果，1:取余结果} [0]={0:UINT,1:INT}
    output logic isError //是否发生了除以0的错误
);
    reg isReadyReg = 1;
    reg [5:0] counter = 0; // 用于计数时钟周期
    parameter totalCounter = 16;
    reg [63:0]tmpAValue = 0;//保存A临时的值
    reg [63:0]tmpBValue = 0;//保存B临时的值
    reg tmpASign,tmpBSign;//保存A/B原值的符号位
    reg [1:0]tmpFuncSelect = 0;//保存任务模式
    reg[63:0] lastRem = 0;//保存上一阶段串行除法运行后的余数
    reg [63:0]tmpCValue = 0;//保存除法结果的值


    //除法运算结果和余数
    wire [63:0] outputResult = funcSelect[0]&(tmpASign^tmpBSign) ? -tmpCValue : tmpCValue;
    wire [63:0] outputRem = funcSelect[0]&(tmpASign|tmpBSign) ? -lastRem : lastRem;
    //最终输出结果
    assign numC = tmpFuncSelect[1] ? outputRem : outputResult;


    //获取此次轮串行除法运算的被除数
    wire[3:0] numA_tmp;
    FU_ShareMod_SplitterBus#(
        .BUS_WIDTH(64),
        .SPLITTER_WIDTH(4)
    ) spl(
        .in_bus(tmpAValue),
        .out_bus(numA_tmp),
        .out_select(totalCounter-1-counter)
    );
    //进行一轮串行除法运算
    wire [3:0] currentResult;//此轮运算的结果
    wire [63:0]currentRem;//此轮运算的余数
    FU_ShareMod_DivisionParallDiv#(
        .BUS_WIDTH(64),
        .SPLITTER_WIDTH(4)
    ) parDiv(
        .numA({lastRem[59:0],numA_tmp}),
        .numB(tmpBValue),
        .numC(currentResult),
        .rem(currentRem)
    );

    assign isError = numB == 0;
    assign isNowTickReady = isError || counter == totalCounter;
    
    always @(posedge clk or negedge rst) begin
        if (!rst || clean) begin //重启运算器
            tmpCValue <= 0;
            counter <= 0;
            isReadyReg <= 1;
        end else if (start & isReadyReg) begin //开启运算
            if(numB != 0)begin //输入除数为0，直接报错停止运算
                tmpCValue <= 0;
                isReadyReg <= 0;
                tmpAValue<= funcSelect[0]&numA[63] ? -numA : numA;
                tmpBValue<= funcSelect[0]&numB[63] ? -numB : numB;
                tmpASign <= numA[63];
                tmpBSign <= numB[63];
                lastRem<=0;
                tmpFuncSelect<=funcSelect;
            end
            counter <= 0;
        end else if(!isReadyReg && counter<=totalCounter)begin //运算过程中
            tmpCValue <= (tmpCValue<<4) | {59'd0,currentResult};//将次轮串行运算结果存入
            lastRem <= currentRem;//保存余数
            counter <= counter + 1;
        end else if(counter==totalCounter)begin
            isReadyReg <= 1;
            counter <= 0;
        end
    end
endmodule

