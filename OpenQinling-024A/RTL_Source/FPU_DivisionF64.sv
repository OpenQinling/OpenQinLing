/**********************************************************/
//                  FPU 64位浮点除法运算模块
/**********************************************************/
//浮点除法法(16个时钟-[0]解析出浮点数据结构 [1]-[14]进行计算 [15]格式化输出的数据)
module Division_Float64(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC, //运算过程中的临时存储
    output logic isNowTickReady //是否就绪，可以开始下一次任务
);
    //float64数据结构定义
    parameter CONST_NUM_BASE = 0;//float64数据结构中真值的bit位起始索引号
    parameter CONST_EXP_BASE = 52;//float64数据结构中浮点偏移值的bit位起始索引号
    parameter CONST_SIG_BASE = 63;//float64数据结构中索引号的bit位起始索引号

    reg isReadyReg = 1;

    //运算参数解析数据结构出的内容
    //A/B的符号位
    wire numA_sign = numA[CONST_SIG_BASE];
    wire numB_sign = numB[CONST_SIG_BASE];
    wire numC_sign = numA_sign ^ numB_sign;
    //A/B的指数位
    wire [CONST_SIG_BASE-CONST_EXP_BASE:0] numA_exp = {1'd0,numA[CONST_SIG_BASE-1:CONST_EXP_BASE]};
    wire [CONST_SIG_BASE-CONST_EXP_BASE:0] numB_exp = {1'd0,numB[CONST_SIG_BASE-1:CONST_EXP_BASE]};
    //A/B的真值位
    wire [CONST_EXP_BASE-CONST_NUM_BASE-1:0] numA_num = numA[CONST_EXP_BASE-1:CONST_NUM_BASE];
    wire [CONST_EXP_BASE-CONST_NUM_BASE-1:0] numB_num = numB[CONST_EXP_BASE-1:CONST_NUM_BASE];

    //C的指数偏移值
    wire [CONST_SIG_BASE-CONST_EXP_BASE:0] numC_offset = numA_exp - numB_exp;

    reg [CONST_SIG_BASE-CONST_EXP_BASE:0]tmpCOffset = 0;//保存运算完毕后C的指数位应该偏移的值

    //判断A/B输入的数据是否是为0
    wire isAZero = numA == 0;
    wire isBZero = numB == 0;
    wire isCZero = tmpCOffset[CONST_SIG_BASE-CONST_EXP_BASE]  & (tmpCOffset[CONST_SIG_BASE-CONST_EXP_BASE-1:0]<1024);
    //判断A/B输入的值是否是无限
    wire isAinfinity = numA_exp==11'b11111111111 & numA_num==0;
    wire isBinfinity = numB_exp==11'b11111111111 & numB_num==0;
    wire isCinfinity = ~tmpCOffset[CONST_SIG_BASE-CONST_EXP_BASE] & (tmpCOffset[CONST_SIG_BASE-CONST_EXP_BASE-1:0]>=1024);
    //判断A/B输入的值是否是错误值
    wire isAError = numA_exp==11'b11111111111 & numA_num!=0;
    wire isBError = numB_exp==11'b11111111111 & numB_num!=0;
    //保存运算时的两个A/B的真值
    reg [111:0] tmpAValue = 0;
    reg [111:0]tmpBValue = 0;

    reg [111:0] tmpCValue = 0;//保存运算完毕后C的真值
    reg tmpCSign = 0;//保存运算完毕后C的符号

    reg [6:0] counter = 0;// 用于计数时钟周期
    parameter totalCounter = 56;

    reg[111:0] lastRem = 0;//保存上一阶段串行除法运行后的余数

    //将tmpBValue切分为8位宽总线到numB_tmp，counter选择那一块有效
    wire[1:0] numA_tmp;
    FU_ShareMod_SplitterBus#(
        .BUS_WIDTH(112),
        .SPLITTER_WIDTH(2)
    ) spl(
        .in_bus(tmpAValue),
        .out_bus(numA_tmp),
        .out_select(totalCounter-1-counter)
    );

    //进行一轮串行除法运算
    wire [1:0] currentResult;//此轮运算的结果
    wire [111:0]currentRem;//此轮运算的余数
    FU_ShareMod_DivisionParallDiv#(
        .BUS_WIDTH(112),
        .SPLITTER_WIDTH(2)
    )  parDiv(
        .numA({lastRem[109:0],numA_tmp}),
        .numB(tmpBValue),
        .numC(currentResult),
        .rem(currentRem)
    );

    wire outputError = isAError | isBError | (isAinfinity & isBinfinity) | (isAZero & isBZero);
    wire outputInfinity = isAinfinity |isBZero;
    wire outputZero = isAZero | isBinfinity;
    wire isNotOper =  (outputError || outputInfinity || outputZero);
    assign isNowTickReady = isNotOper || (counter == totalCounter);


    logic [63:0]outputNumC_tmp;
    assign numC = outputNumC_tmp;

    always_comb begin
        if(outputError)begin //直接输出错误
            outputNumC_tmp[CONST_SIG_BASE] = numC_sign;
            outputNumC_tmp[CONST_SIG_BASE-1:CONST_NUM_BASE] = ~0;
        end else if(outputInfinity)begin //直接输出无限
            outputNumC_tmp[CONST_SIG_BASE] = numC_sign;
            outputNumC_tmp[CONST_SIG_BASE-1:CONST_EXP_BASE] = ~0;
            outputNumC_tmp[CONST_EXP_BASE-1:CONST_NUM_BASE] = 0;
        end else if(outputZero)begin //直接输出0
            outputNumC_tmp[CONST_SIG_BASE] <= numC_sign;
            outputNumC_tmp[CONST_SIG_BASE-1:CONST_NUM_BASE] = 0;
        end else begin
            outputNumC_tmp[CONST_SIG_BASE] = tmpCSign;
            if(isCinfinity)begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_EXP_BASE] = ~0;
                outputNumC_tmp[CONST_EXP_BASE-1:CONST_NUM_BASE] = 0;
            end else if(isCZero) begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_NUM_BASE] = 0;
            end else begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_EXP_BASE] = 12'd1022 + tmpCOffset + tmpCValue[59];
                outputNumC_tmp[CONST_EXP_BASE-1:CONST_NUM_BASE] = tmpCValue[59] ? tmpCValue[58:7] + tmpCValue[6]:
                                                                    tmpCValue[57:6] + tmpCValue[5];
            end
        end
    end
    //运算流程:
    //时钟0: 计算出
    always @(posedge clk or negedge rst) begin
        if (!rst || clean) begin //重启运算器
            counter <= 0;
            isReadyReg <= 1;
        end else if (start & isReadyReg) begin //开启运算
            if(outputError || outputInfinity || outputZero)begin//不运算,直接输出
                counter <= 0;
                isReadyReg <= 1;
            end else begin //开始运算
                counter <= 0;
                isReadyReg <= 0;
                tmpAValue <= {1'b1,numA_num,59'd0};
                tmpBValue <= {1'b1,numB_num};
                tmpCSign <= numC_sign;
                tmpCOffset <= numC_offset;
            end
            tmpCValue <= 0;
            lastRem <= 0;
        end else if(!isReadyReg)begin //运算过程中
            if(counter < totalCounter)begin//运算数据
                tmpCValue <= (tmpCValue<<2) | {109'd0,currentResult};//将此轮串行运算结果存入
                lastRem <= currentRem;//保存余数
                counter <= counter + 1;
            end else if(counter == totalCounter)begin//格式化
                isReadyReg <= 1;
                counter <= 0;
            end
        end
    end
endmodule
