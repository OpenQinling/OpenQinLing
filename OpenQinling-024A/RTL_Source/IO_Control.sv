`include "IO_Control.svh"

module IO_Control(
    //IO模块主要接口，与FU通用接口相同
    input logic clk,
    input logic rst,
    input logic clean,
    input logic start,
    input logic [63:0] numA, //[39:0]读写内存时的地址,其他位无用
    input logic [63:0] numB, //写内存时用来传递写入的值
    output logic [63:0] numC, //读内存时用来接收获取的值
    output logic isReady,
    //IO模块次要接口
    input logic[3:0] funcSelect, //读写模式
    output logic isError, //读写失败信号[裸机状态下就是访问外设超时,虚拟内存下就是缺页。出错后发出内存读写错误异常]
    //连接cpu数据缓存/MMU的接口
    IO_Interface.master ioInterface
);
    reg isReadyReg = 1;

    logic [63:0] readData;//读取的数据
    logic [63:0] writeData;//写入的数据(临时存储)
    logic [1:0] widthCtr;//读写的位宽
    logic readWriteCtrl;//读写控制(1:写)
    wire[63:0] mem_ReadBus = ioInterface.readBus;
    assign ioInterface.writeBus = readWriteCtrl ? writeData : 63'bz;
    assign ioInterface.address = numA[39:0];
    assign ioInterface.rwCtrl = readWriteCtrl;
    assign ioInterface.widthCtr = widthCtr;
    assign ioInterface.taskValid = (start & isReadyReg & !clean & rst) | ~isReadyReg;
    
    always_comb begin:GetReadData
        case(funcSelect)
            IOModes__READ_UBYTE:readData <= {56'd0,mem_ReadBus[7:0]};
            IOModes__READ_USHORT:readData <= {48'd0,mem_ReadBus[15:0]};
            IOModes__READ_UINT:readData <= {32'd0,mem_ReadBus[31:0]};
            IOModes__READ_ULONG_LONG_DOUBLE:readData <= mem_ReadBus;
            IOModes__READ_BYTE:readData <= mem_ReadBus[7] ? {56'hffffffffffffff,mem_ReadBus[7:0]} : mem_ReadBus;
            IOModes__READ_SHORT:readData <= mem_ReadBus[15] ? {56'hffffffffffff,mem_ReadBus[15:0]} : mem_ReadBus;
            IOModes__READ_INT:readData <= mem_ReadBus[31] ? {56'hffffffff,numB[31:0]} : mem_ReadBus;
            IOModes__READ_FLOAT:readData <= {mem_ReadBus[31],mem_ReadBus[30:23],3'd0,mem_ReadBus[22:0],29'd0};
            default:readData<=0;
        endcase
    end
    always_comb begin:GetWriteData
        if(funcSelect == IOModes__WRITE_FLOAT)begin
            writeData <= {numB[63],numB[62:55]+numB[54],numB[51:29]+numB[28]};
        end else begin
            writeData <= numB;
        end
    end
    always_comb begin:GetRwType
        readWriteCtrl = funcSelect[3:3];

        if(funcSelect == IOModes__READ_UBYTE |
            funcSelect == IOModes__READ_BYTE|
            funcSelect == IOModes__WRITE_UBYTE_BYTE)begin
            widthCtr = 0;
        end else if(funcSelect == IOModes__READ_USHORT |
            funcSelect == IOModes__READ_SHORT|
            funcSelect == IOModes__WRITE_USHORT_SHORT)begin
            widthCtr = 1;
        end else if(funcSelect == IOModes__READ_UINT |
            funcSelect == IOModes__READ_INT|
            funcSelect == IOModes__READ_FLOAT|
            funcSelect == IOModes__WRITE_UINT_INT|
            funcSelect == IOModes__WRITE_FLOAT)begin
            widthCtr = 2;
        end else begin
            widthCtr = 3;
        end
    end
    assign isError = ioInterface.taskError;
    assign numC = readData;
    assign isReady = ioInterface.taskReady;
    always_ff @(posedge clk or negedge rst)begin
        if(~rst || clean)begin
            isReadyReg <= 1;
        end else if(start & isReadyReg)begin
            if(!ioInterface.taskReady)begin
                isReadyReg <= 0;
            end
        end else if(!isReadyReg)begin
            if(ioInterface.taskReady)begin
                isReadyReg <= 1;
            end
        end
    end
endmodule