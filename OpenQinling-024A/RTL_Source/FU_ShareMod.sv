//将64位总线拆分为8宽度的子总线
module FU_ShareMod_SplitterBus
#(
    //可配置的模块属性配置
    parameter BUS_WIDTH = 64,       //输入总线的宽度
    parameter SPLITTER_WIDTH = 8,    //拆分的宽度
    //不可配置常量值
    localparam SPLITTER_COUNT = BUS_WIDTH/SPLITTER_WIDTH
)(
    input logic [BUS_WIDTH-1:0] in_bus,   //要被拆分的总线
    input logic [$clog2(SPLITTER_COUNT):0] out_select, //选择输出哪个总线信号
    output logic [BUS_WIDTH-1:0] out_bus //输出结果
);
    wire [BUS_WIDTH-1:0]outTmp[SPLITTER_COUNT];
    genvar i;
    generate
        for(i = 0;i<SPLITTER_COUNT;i=i+1)begin
            assign outTmp[i] = in_bus[(SPLITTER_WIDTH)*(i+1) -1:(SPLITTER_WIDTH)*i];
        end
    endgenerate
    assign out_bus = outTmp[out_select];
endmodule


//计算其中的部分乘法运算
module FU_ShareMod_MultiplySpl#(
    //可配置的模块常量属性
    parameter BUS_WIDTH = 64,       //输入总线的宽度
    parameter SPLITTER_WIDTH = 8    //拆分的宽度
)(
    input logic [BUS_WIDTH-1:0]a,
    input logic [SPLITTER_WIDTH-1:0] b,
    output logic [BUS_WIDTH-1:0]c
);
    wire [BUS_WIDTH-1:0] tmp[SPLITTER_WIDTH];
    genvar i;
    generate
        for(i=0;i<SPLITTER_WIDTH;i++)begin
            assign tmp[i] = b[i] ? a<<i : 0;
        end
    endgenerate
    always_comb begin
        integer i;
        c = 0;
        for(i = 0;i<SPLITTER_WIDTH;i = i + 1)begin
            c = c + tmp[i];
        end
    end
endmodule


//并行除法器中的部分并行除法逻辑(一次性处理8个bit)
module FU_ShareMod_DivisionParallDiv#(
    //可配置的模块常量属性
    parameter BUS_WIDTH = 64,       //输入总线的宽度
    parameter SPLITTER_WIDTH = 8    //拆分的宽度
)(
    input logic [BUS_WIDTH-1:0] numA,//被除数
    input logic [BUS_WIDTH-1:0] numB,//除数
    output logic [SPLITTER_WIDTH-1:0] numC,//结果
    output logic [BUS_WIDTH-1:0] rem //余数
);
    wire [BUS_WIDTH-2+SPLITTER_WIDTH:0] tmpA[SPLITTER_WIDTH];
    wire [BUS_WIDTH-2+SPLITTER_WIDTH:0] tmpB[SPLITTER_WIDTH];
    assign tmpA[0] = numA;

    genvar i;
    generate
        for(i=0;i<SPLITTER_WIDTH;i++)begin
            assign tmpB[i] = numB<<(SPLITTER_WIDTH-1-i);
        end
        for(i=0;i<SPLITTER_WIDTH;i++)begin
            assign numC[SPLITTER_WIDTH-1-i] = tmpA[i] >= tmpB[i];
            if(i == SPLITTER_WIDTH-1)begin
                assign rem = numC[SPLITTER_WIDTH-1-i] ? tmpA[i]-tmpB[i] : tmpA[i];
            end else begin
                assign tmpA[i+1] = numC[SPLITTER_WIDTH-1-i] ? tmpA[i]-tmpB[i] : tmpA[i];
            end
        end
    endgenerate
endmodule