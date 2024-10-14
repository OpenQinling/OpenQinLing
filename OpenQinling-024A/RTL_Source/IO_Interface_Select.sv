
//两个IO_Interface.slave 选择一个作为 IO_Interface.master输出
//执行模块、回写模块都有两个IO_Interface的操作接口,而CPU对外只有一个
//该模块用于选择最终是那个模块的接口连接到CPU对外的接口
//回写模块有优先占有权(用于读取中断处理函数的首地址),当回写模块不使用时接口才能被执行模块使用
module IO_Interface_Select(
    input wire select,//0: A接口有效  1:B接口有效
    IO_Interface.slave inInterfaceA,
    IO_Interface.slave inInterfaceB,
    IO_Interface.master outputInterface
);
always_comb begin
    if(~select)begin
        inInterfaceA.taskReady = outputInterface.taskReady;
        inInterfaceA.taskError = outputInterface.taskError;
        outputInterface.taskValid = inInterfaceA.taskValid;
        outputInterface.address = inInterfaceA.address;
        outputInterface.rwCtrl = inInterfaceA.rwCtrl;
        outputInterface.widthCtr = inInterfaceA.widthCtr;
        inInterfaceB.taskReady = 1;
        inInterfaceB.taskError = 1;
        outputInterface.writeBus = inInterfaceA.writeBus;
        inInterfaceA.readBus = outputInterface.readBus;
        inInterfaceB.readBus = 0;
    end else begin
        inInterfaceB.taskReady = outputInterface.taskReady;
        inInterfaceB.taskError = outputInterface.taskError;
        outputInterface.taskValid = inInterfaceB.taskValid;
        outputInterface.address = inInterfaceB.address;
        outputInterface.rwCtrl = inInterfaceB.rwCtrl;
        outputInterface.widthCtr = inInterfaceB.widthCtr;
        inInterfaceA.taskReady = 1;
        inInterfaceA.taskError = 1;
        outputInterface.writeBus = inInterfaceB.writeBus;
        inInterfaceB.readBus = outputInterface.readBus;
        inInterfaceA.readBus = 0;
    end

end


endmodule