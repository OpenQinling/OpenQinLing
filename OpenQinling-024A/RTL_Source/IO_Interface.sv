//CPU-Core连接ICache的接口(8080魔改)
interface FetchOrder_Interface();
    parameter ReadOrderWidth = 4; //一次最多可以读的指令数

    logic taskValid[ReadOrderWidth];//此时是否存在有效的取指任务
    logic [39:0] address;//读写的地址
    logic [31:0] readBus[ReadOrderWidth];//读写数据总线
    logic taskReady[ReadOrderWidth];//取指任务完成信号(1有效)[哪怕失败也要发送ready信号]
    logic taskError[ReadOrderWidth];//取指失败完成信号(1有效)
    modport master(
        input taskReady,taskError,readBus,
        output taskValid,address
    );
    modport slave (
        output taskReady,taskError,readBus,
        input taskValid,address
    );
endinterface

//CPU-Core连接DCache的接口(8080魔改)
interface IO_Interface();
    logic taskValid;//此时是否存在有效的读写任务
    logic [39:0] address;//读写的地址
    logic rwCtrl;//读写控制(0:读,1:写)
    logic [1:0]widthCtr;//位数控制(0:读1字节，1:读2字节，2:读4字节，3:读8字节)
    logic [63:0] readBus;//CPU从内存读数据总线
    logic [63:0] writeBus;//CPU向内存写数据总线
    logic taskReady;//读写任务完成信号(1有效)[哪怕失败也要发送ready信号]
    logic taskError;//读写失败完成信号(1有效)
    modport master(
        input taskReady,taskError,readBus,
        output taskValid,address,rwCtrl,widthCtr,writeBus
    );
    modport slave (
        output taskReady,taskError,readBus,
        input taskValid,address,rwCtrl,widthCtr,writeBus
    );
endinterface

/*  IO协议
写内存示例:
    起始时钟上升沿:
    等待taskReady=1
    taskValid<=1;
    address<=写地址;
    rwCtrl<=1;
    widthCtr<=写位数;
    ioBus<=写的数据;

    
    下一个时钟开始后每次的时钟上升沿执行{
        if(taskReady){
            if(taskError){
                写入失败，触发异常
            }else{
                完成写入
            }
            结束循环;
        }
    }
    
    address/rwCtrl/widthCtr/ioBus会在taskReady前始终保持不变
    如果taskValid在taskReady前变为0,视为强制停止此次读写。(如果此次写任务已经修改了内存，应当恢复)
*/