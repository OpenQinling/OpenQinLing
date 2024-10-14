/**********************************************************/
//                  ALU 64位整数基础运算模块
// 包含: 整数加法 整数减法 位运算 移位运算 整数大小比较运算 布尔运算 等于运算
/**********************************************************/
//整数加减法(INT64 UINT64适用)
module AddSub_IntUInt64 (
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic[63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input funcSelect//0是加法，1是减法
);
    assign isNowTickReady = 1;
    always_comb begin
        if(funcSelect) begin
            numC = numA - numB;
        end else begin
            numC = numA + numB;
        end
    end
endmodule

//整数位运算(INT64 UINT64适用)
module BitOper_IntUInt64 (
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA, //非运算无用
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input logic [2:0] funcSelect,//1:与 2:或 3:非 4:异或 5:左移 6:无符号右移 7:有符号右移 0无效(报错)
    output logic isInvailFuncSelect //无效功能选择
);
    assign isNowTickReady = 1;
    always_comb begin
        case (funcSelect)
            1: numC = numA & numB;
            2: numC = numA | numB;
            3: numC = ~numB;
            4: numC = numA ^ numB;
            5: numC = numA << numB;
            6: numC = numA >> numB;
            7: numC = (numA >> numB) | (~(64'hFFFFFFFFFFFFFFFF >> numB));
            default: numC = 0;
        endcase
        case (funcSelect)
            0: isInvailFuncSelect = 1;
            default: isInvailFuncSelect = 0;
        endcase
    end
endmodule

//整数布尔运算(INT64 UINT64适用)
module BoolOper_IntUInt64(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA, //非运算无用
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,
    
    //当前模块专有接口
    input logic [1:0] funcSelect,//1:与 2:或 3:非
    output logic isInvailFuncSelect //无效功能选择
);
    assign isNowTickReady = 1;
    always_comb begin
        case (funcSelect)
            1: numC = numA && numB;
            2: numC = numA || numB;
            3: numC = !numB;
            default: numC = 64'd0;
        endcase

        case (funcSelect)
            0: isInvailFuncSelect = 1'd1;
            default: isInvailFuncSelect = 1'd0;
        endcase
    end
endmodule

//比较两个数是否相等或不等(适用于任何64位类型)
module Equal_All64Type(
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input logic funcSelect//0:等于 1:不等于
);
    assign isNowTickReady = 1;
    wire isEqual = numA == numB;//A是否等于B
    always_comb begin
        case (funcSelect)
            0: numC = isEqual;
            1: numC = ~isEqual;
        endcase
    end
endmodule



//有符号整数大小比较(INT64适用)
module Compare_Int64 (
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input logic [1:0] funcSelect//0:大于 1:大于等于 2:小于 3:小于等于
);
    assign isNowTickReady = 1;
    wire isEqual = numA == numB;//A是否等于B
    reg isLess;//A是否小于B
    always_comb begin
        if (numA[63] & !numB[63]) begin //A是负数，B是正数或0
            isLess = 1;
        end else if (numB[63] & !numA[63]) begin //B是负数，A是正数或0
            isLess = 0;
        end else begin // A/B都是正数 或都是负数
            isLess = numA[62:0] < numB[62:0];
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

//无符号整数大小比较(UINT64适用)
module Compare_UInt64 (
    //ALU模块通用接口
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,
    input logic [63:0] numB,
    output logic [63:0] numC,
    output logic isNowTickReady,

    //当前模块专有接口
    input logic [1:0] funcSelect//0:大于 1:大于等于 2:小于 3:小于等于
);
    assign isNowTickReady = 1;
    always_comb begin
        case (funcSelect)
            0: numC = numA > numB;
            1: numC = numA >= numB;
            2: numC = numA < numB;
            3: numC = numA <= numB;
        endcase
    end
endmodule


//初始化/数据转移(所有数据类型适用)
module MovLimitDataInit_AllType(
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA,//MOV:初始值(源寄存器值) INIT:源寄存器值 LD:该位无用
    input logic [63:0] numB,//MOV:该位无用, INIT:该位放相或的初始值 LD:(源寄存器值)
    output logic [63:0] numC,
    output logic isNowTickReady,
    input logic  [2:0]funcSelect,
    output logic isInvailFuncSelect //无效功能选择
);
    /*funcSelect:
        0: MOV
        1: 限制为INT8类型
        2: 限制为UINT16类型
        3: 限制为INT16类型
        4: 限制为UINT32类型
        5: 限制为INT32类型
        6: INIT
    */
    assign isNowTickReady = 1;
    assign isInvailFuncSelect = funcSelect > 6;
    always_comb begin
        case (funcSelect)
            0: numC = numA;
            1: numC = {numB[7] ? ~(56'd0) : 56'd0,numB[7:0]};
            2: numC = {48'd0,numB[15:0]};
            3: numC = {numB[15] ? ~(48'd0) : 48'd0,numB[15:0]};
            4: numC = {32'd0,numB[31:0]};
            5: numC = {numB[31] ? ~(32'd0) : 32'd0,numB[31:0]};
            6: numC = (numA<<23) | numB;
            default: numC = 0;
        endcase
    end
endmodule