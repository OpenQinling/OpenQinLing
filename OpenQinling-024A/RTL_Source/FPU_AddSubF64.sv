/**********************************************************/
//                  FPU 64位浮点加减法运算模块
/**********************************************************/



//浮点加减法(3个时钟-[0]解析出浮点数据结构 [1]进行计算 [2]格式化输出的数据)
module AddSub_Float64(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC, //运算过程中的临时存储
    output logic isNowTickReady, //是否即将就绪
    //当前模块专有接口
    input logic funcSelect//0:加法 1:减法
);

    parameter CONST_NUM_BASE = 0;//float64数据结构中真值的bit位起始索引号
    parameter CONST_EXP_BASE = 52;//float64数据结构中浮点偏移值的bit位起始索引号
    parameter CONST_SIG_BASE = 63;//float64数据结构中索引号的bit位起始索引号

    reg isReadyReg = 1;

    wire numA_sign = numA[CONST_SIG_BASE];
    wire numB_sign = numB[CONST_SIG_BASE];

    wire [CONST_SIG_BASE-CONST_EXP_BASE-1:0] numA_exp = numA[CONST_SIG_BASE-1:CONST_EXP_BASE];
    wire [CONST_SIG_BASE-CONST_EXP_BASE-1:0] numB_exp = numB[CONST_SIG_BASE-1:CONST_EXP_BASE];

    wire [CONST_EXP_BASE-CONST_NUM_BASE-1:0] numA_num = numA[CONST_EXP_BASE-1:CONST_NUM_BASE];
    wire [CONST_EXP_BASE-CONST_NUM_BASE-1:0] numB_num = numB[CONST_EXP_BASE-1:CONST_NUM_BASE];


    //保存运算时A/B的符号
    reg tmpASign = 0,tmpBSign = 0,tmpCSign = 0;

    //保存运算时的两个A/B的真值
    reg [CONST_EXP_BASE-CONST_NUM_BASE+3:0] tmpAValue = 0,tmpBValue = 0;
    //保存运算完毕后C的真值
    reg [CONST_EXP_BASE-CONST_NUM_BASE+2:0] tmpCValue = 0;
    //计算出C的真值
    wire [CONST_EXP_BASE-CONST_NUM_BASE+3:0] operAValue = tmpASign ? -tmpAValue : tmpAValue,
                                            operBValue = tmpBSign ? -tmpBValue : tmpBValue,
                                            operCValue = funcSelect ? operAValue-operBValue : operAValue+operBValue;
    //保存运算时基础的偏移量(= max(a.e,b.e))
    reg [CONST_SIG_BASE-CONST_EXP_BASE-1:0] tmpBaseOffset = 0;
    reg counter = 1'd0;// 用于计数时钟周期

    wire[10:0] offset;
    wire isAllZero;
    wire isLeftOff;
    FloatOper_GetHighIndex#(
        .BUS_WIGHT(CONST_EXP_BASE-CONST_NUM_BASE+3),
        .INDEX_WIGHT(CONST_SIG_BASE-CONST_EXP_BASE),
        .BASE_INDEX(CONST_EXP_BASE-CONST_NUM_BASE+1)
    ) getHighIndex(
        .bus(tmpCValue),
        .index(offset),
        .isAllZero(isAllZero),
        .isLeftOff(isLeftOff)
    );


    wire [CONST_EXP_BASE-CONST_NUM_BASE:0] outputNumC_num= isLeftOff ? (tmpCValue>>offset) : (tmpCValue<<offset);


    assign isNowTickReady = (counter == 1);

    logic [63:0] outputNumC;
    assign numC = outputNumC;

    always_comb begin
        if(isAllZero)begin
            outputNumC <= 0;
        end else begin
            outputNumC[CONST_SIG_BASE] <= tmpCSign;
            outputNumC[CONST_SIG_BASE-1:CONST_EXP_BASE] <= isLeftOff ? tmpBaseOffset+offset : tmpBaseOffset-offset;
            outputNumC[CONST_EXP_BASE-1:CONST_NUM_BASE] <= outputNumC_num[CONST_EXP_BASE-CONST_NUM_BASE:1] + outputNumC_num[0:0];
        end
    end



    //运算流程:
    //时钟0: 计算出
    always @(posedge clk or negedge rst) begin
        if (!rst || clean) begin //重启运算器
            counter <= 0;
            isReadyReg <= 1;
        end else if (start & isReadyReg) begin //开启运算
            counter <= 0;
            isReadyReg <= 0;
            if(numA_exp > numB_exp)begin
                tmpBaseOffset <= numA_exp;
                tmpAValue <= {2'b01,numA_num,1'b0};
                tmpBValue <= {2'b01,numB_num,1'b0}>>(numA_exp-numB_exp);
            end else begin
                tmpBaseOffset <= numB_exp;
                tmpBValue <= {2'b01,numB_num,1'b0};
                tmpAValue <= {2'b01,numA_num,1'b0}>>(numB_exp-numA_exp);
            end
            tmpASign <= numA_sign;
            tmpBSign <= numB_sign;
            
        end else if(!isReadyReg)begin //运算过程中
            if(counter == 0)begin//运算数据
                tmpCValue <= operCValue[CONST_EXP_BASE-CONST_NUM_BASE+2] ? -operCValue : operCValue;
                tmpCSign <= operCValue[CONST_EXP_BASE-CONST_NUM_BASE+2];
                counter <= 1;
            end else if(counter == 1)begin//格式化
                isReadyReg <= 1;
                counter <= 0;
                tmpCValue<= 0;
                tmpCSign <= 0;
            end
        end
    end
endmodule


