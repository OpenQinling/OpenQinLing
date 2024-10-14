/**********************************************************/
//                  FPU 64位浮点乘法运算模块
/**********************************************************/

//浮点乘法(16个时钟-[0]解析出浮点数据结构 [1]-[14]进行计算 [15]格式化输出的数据)
module Multiply_Float64(
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

    reg [CONST_SIG_BASE-CONST_EXP_BASE:0]tmpCEXP = 0;//保存运算完毕后C的指数位应该偏移的值


    //保存最终的输出值
    reg [63:0] outputNumC_reg = 0;

    

    //保存运算时的两个A/B的真值
    reg [111:0] tmpAValue = 0,tmpBValue = 0;

    reg [111:0] tmpCValue = 0;//保存运算完毕后C的真值
    reg tmpCSign = 0;//保存运算完毕后C的符号

    reg [6:0] counter = 0;// 用于计数时钟周期

    //C的指数偏移值
    wire [CONST_SIG_BASE-CONST_EXP_BASE:0] numC_exp = numA_exp + numB_exp - 12'd1023;
    wire [CONST_SIG_BASE-CONST_EXP_BASE:0] numC_offset = tmpCEXP - 12'd1023 + tmpCValue[105];

    //判断A/B输入的数据是否是为0
    wire isAZero = numA == 0;
    wire isBZero = numB == 0;
    wire isCZero = numC_offset[CONST_SIG_BASE-CONST_EXP_BASE];
    //判断A/B输入的值是否是无限
    wire isAinfinity = numA_exp==11'b11111111111 & numA_num==0;
    wire isBinfinity = numB_exp==11'b11111111111 & numB_num==0;
    wire isCinfinity = ~numC_offset[CONST_SIG_BASE-CONST_EXP_BASE] & (numC_offset[CONST_SIG_BASE-CONST_EXP_BASE-1:0]>=1024);
    //判断A/B输入的值是否是错误值
    wire isAError = numA_exp==11'b11111111111 & numA_num!=0;
    wire isBError = numB_exp==11'b11111111111 & numB_num!=0;

    //将tmpBValue切分为8位宽总线到numB_tmp，counter选择那一块有效
    wire[7:0] numB_tmp;
    FU_ShareMod_SplitterBus#(
        .BUS_WIDTH(112),
        .SPLITTER_WIDTH(8)
    ) spl(
        .in_bus(tmpBValue),
        .out_select(counter),
        .out_bus(numB_tmp)
    );

    wire[111:0] resultTmp;
    FU_ShareMod_MultiplySpl#(
        .BUS_WIDTH(112)
    )  mulSpl(
        .a(tmpAValue),
        .b(numB_tmp),
        .c(resultTmp)
    );


    wire outputError = isAError | isBError | (isAZero & isBinfinity) | (isBZero & isAinfinity);
    wire outputInfinity = isAinfinity | isBinfinity;
    wire outputZero = isAZero || isBZero;


    wire isNotOper = ((outputError || outputInfinity || outputZero));
    assign isNowTickReady = isNotOper || (counter == 14);
    
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
            outputNumC_tmp[CONST_SIG_BASE] <= tmpCSign;
            if(isCinfinity)begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_EXP_BASE] = ~0;
                outputNumC_tmp[CONST_EXP_BASE-1:CONST_NUM_BASE] = 0;
            end else if(isCZero) begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_NUM_BASE] = 0;
            end else begin
                outputNumC_tmp[CONST_SIG_BASE-1:CONST_EXP_BASE] = tmpCEXP  + tmpCValue[105];
                outputNumC_tmp[CONST_EXP_BASE-1:CONST_NUM_BASE] = tmpCValue[105] ? 
                            tmpCValue[104 : CONST_EXP_BASE-CONST_NUM_BASE+1] + tmpCValue[CONST_EXP_BASE-CONST_NUM_BASE] :
                            tmpCValue[103 : CONST_EXP_BASE-CONST_NUM_BASE] + tmpCValue[CONST_EXP_BASE-CONST_NUM_BASE-1];
            end
        end
    end
    //运算流程:.
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
                tmpAValue <= {1'b1,numA_num};
                tmpBValue <= {1'b1,numB_num};
                tmpCSign <= numC_sign;
                tmpCEXP <= numC_exp;
                tmpCValue <= 0;
            end
        end else if(!isReadyReg)begin //运算过程中
            if(counter < 14)begin//运算数据
                tmpCValue <= tmpCValue + (resultTmp << (counter<<3));
                counter <= counter + 1;
            end else if(counter == 14)begin//格式化
                isReadyReg <= 1;
                counter <= 0;
            end
        end
    end
endmodule


