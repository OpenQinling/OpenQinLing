/**********************************************************/
//                  FPU 64位浮点基础运算模块
// 包含: 浮点数大小比较 INT64转FLOAT64 UINT64转FLOAT64 FLOAT64转UINT64
/**********************************************************/


//FLOAT64和各种类型互转
module TypeTrans_Float64(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,//无用接口，但为了和其它ALU模块的接口统一，所以保留
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input logic [1:0] funcSelect, //1:FLOAT64->INT64 2:INT64->FLOAT64 3:UINT64->FLOAT64 0:无效
    output logic isError //无效功能选择
);
    parameter CONST_NUM_BASE = 0;//float64数据结构中真值的bit位起始索引号
    parameter CONST_EXP_BASE = 52;//float64数据结构中浮点偏移值的bit位起始索引号
    parameter CONST_SIG_BASE = 63;//float64数据结构中索引号的bit位起始索引号
    logic isInvailFuncSelect;
    assign isError = isInvailFuncSelect;
    //FLOAT64->INT64
    wire inFloat_sign = numB[CONST_SIG_BASE];
    wire [CONST_SIG_BASE-CONST_EXP_BASE-1:0] inFloat_exp = numB[CONST_SIG_BASE-1:CONST_EXP_BASE];
    wire inFloat_offset = (inFloat_exp >= 1023 & inFloat_exp<1075) ? 1075-inFloat_exp : 0;
    wire [CONST_EXP_BASE-CONST_NUM_BASE-1:0] inFloat_num = numB[CONST_EXP_BASE-1:CONST_NUM_BASE];
    wire [63:0] float64RealValue = {1'b1,inFloat_num}>>(52-inFloat_exp+1023);
    wire [63:0] outputINT64 = inFloat_sign ? -float64RealValue : float64RealValue;

    //INT64->FLOAT64
    wire isInputMinus =  (funcSelect==2 & numB[63]);//是否是输入的负数
    wire [63:0] inputIntValue = (isInputMinus ? -inFloat_num : inFloat_num);//输入的真值
    wire [10:0] index;
    wire isAllZero;
    FloatOper_GetHighIndex#(
        .BUS_WIGHT(64),
        .INDEX_WIGHT(11),
        .BASE_INDEX(0)
    ) getHighIndex(
        .bus(inputIntValue),
        .index(index),
        .isAllZero(isAllZero)
    );
    wire [63:0] outputNum = inputIntValue<<(64 - index);//输出的浮点数真值
    wire [63:0] outputFloatValue = {isInputMinus,index+11'd1023,outputNum[63:12]+outputNum[11]};
    assign isNowTickReady = 1;
    always_comb begin
        case (funcSelect)
            1: numC= outputINT64;
            2: numC= outputFloatValue;
            3: numC= outputFloatValue;
            default: numC= 64'd0;
        endcase
        case (funcSelect)
            0: isInvailFuncSelect= 1'd1;
            default: isInvailFuncSelect= 1'd0;
        endcase
    end
endmodule

//有符号整数大小比较(FLOAT64适用)
module Compare_Float64 (
    //ALU模块通用接口
    input clk,
    input rst,
    input clean,
    input start,
    input [63:0] numA,
    input [63:0] numB,
    output logic[63:0] numC,
    output isNowTickReady,

    //当前模块专有接口
    input [1:0] funcSelect//0:大于 1:大于等于 2:小于 3:小于等于
);
    parameter CONST_NUM_BASE = 0;//float64数据结构中真值的bit位起始索引号
    parameter CONST_EXP_BASE = 52;//float64数据结构中浮点偏移值的bit位起始索引号
    parameter CONST_SIG_BASE = 63;//float64数据结构中索引号的bit位起始索引号

    assign isNowTickReady = 1;
    wire isEqual = numA == numB;//A是否等于B
    logic isLess;//A是否小于B
    always_comb begin
        if (numA[CONST_SIG_BASE] & !numB[CONST_SIG_BASE]) begin //A是负数，B是正数或0
            isLess <= 1;
        end else if (numB[CONST_SIG_BASE] & !numA[CONST_SIG_BASE]) begin //B是负数，A是正数或0
            isLess <= 0;
        end else if (numA[CONST_SIG_BASE-1:CONST_EXP_BASE] < numB[CONST_SIG_BASE-1:CONST_EXP_BASE])begin // A/B都是正数 或都是负数,判断A和B的指数位
            isLess <= 1;
        end else if (numA[CONST_EXP_BASE-1:CONST_NUM_BASE] < numB[CONST_EXP_BASE-1:CONST_NUM_BASE])begin // A/B都是正数 或都是负数,判断A和B的指数位
            isLess <= 1;
        end else begin
            isLess <= 0;
        end
    end

    always_comb begin
        case (funcSelect)
            0: numC = !isLess & !isEqual;
            1: numC = !isLess;
            2: numC = isLess & !isEqual;
            3: numC = isLess | isEqual;
        endcase
    end
endmodule

//浮点运算完成后，获取[m:0]中从高到低，首个逻辑1的位是第几个
module FloatOper_GetHighIndex#(
    parameter BUS_WIGHT = 64,
    parameter INDEX_WIGHT = 11,
    parameter BASE_INDEX = BUS_WIGHT-1
) (
    input [BUS_WIGHT-1:0] bus,
    output [INDEX_WIGHT-1:0] index,//输出偏移的值
    output isAllZero,//是否传入的值全为0
    output isLeftOff//是否是左移(默认是向右偏移)
);
    reg [INDEX_WIGHT-1:0]index_tmp;
    assign isLeftOff = index_tmp > BASE_INDEX;
    assign index = (isLeftOff ? index_tmp-BASE_INDEX : BASE_INDEX-index_tmp);
    assign isAllZero = bus==0;

    integer i = 0;
    logic [BUS_WIGHT-1 : 0]isT;
    always_comb begin
        if(isAllZero)begin
            index_tmp = 0;
        end else begin
            isT = 0;
            for (i = BUS_WIGHT-1; i >= 0; i = i - 1) begin
                if (bus[i] == 1'b1 && isT==0) begin
                    index_tmp = i;
                    isT[i] = 1;
                end
            end
        end
    end
endmodule