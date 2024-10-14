
interface SingleChannelIO;
	logic taskValid;//此时是否存在有效的读写任务
	logic [39:0] address;//读写的地址
	logic rwCtrl;//读写控制(0:读,1:写)
	logic [7:0] readBus;//CPU从内存读数据总线
	logic [7:0] writeBus;//CPU向内存写数据总线
	logic taskReady;//读写任务完成信号(1有效)[哪怕失败也要发送ready信号]
	logic taskError;//读写失败完成信号(1有效)
	modport master(
		input taskReady,taskError,readBus,
		output taskValid,address,rwCtrl,writeBus
	);
	modport slave (
		output taskReady,taskError,readBus,
		input taskValid,address,rwCtrl,writeBus
	);
endinterface


//将IO_Interface FetchOrder_Interface的信号转换为以字节为单位单通道读写内存的信号
//[FetchOrder_Interface只支持一次读取一条指令,不支持写入任务的回滚]
module SingleChannelIO_TransMod(input logic clk,//时钟
						input logic rst,//硬重启
						//对于askInterHandle请求信号，如果当前正在执行写内存任务执行到了一半，应当要将内存恢复到开始执行此次写任务前的状态。
						//当前模块暂不支持该写内存任务的回滚功能
						input askInterHandle,//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
						input askRestartHandle,//连接所有内设和外设,当为1时表示CPU已经在重启处理流程中
						output beReadyInter,//返回1表示已经准备好进行中断响应
						output beReadyRestart,//返回1表示已经准备好进行重启
						IO_Interface.slave ioInterface,
						FetchOrder_Interface.slave fetchFace,//只支持以此取一条指令
						SingleChannelIO.master sIoFace
						);
	assign beReadyInter = 1;
	assign beReadyRestart = 1;
	
	logic [1:0]ioTaskStepReg = 0;//io任务的进度
	logic isFetchTaskReg = 0;//是否是取指令
	logic [3:0] rwByteStepReg = 0;//当前读写步数
	logic [3:0] ioAllStepReg = 0; //总的读写步数
	logic isErrorReg = 0; //是否出错

	logic [63:0]tmpReadDataReg = 0;//如果是读内存，临时存储读到的数据

	assign ioInterface.taskReady = isFetchTaskReg ? 0 : ioTaskStepReg==2;
	assign fetchFace.taskReady[0] = isFetchTaskReg ? ioTaskStepReg==2 : 0;
	assign ioInterface.taskError = isFetchTaskReg ? 0 : isErrorReg;
	assign fetchFace.taskError[0] = isFetchTaskReg ? isErrorReg : 0;
	assign ioInterface.readBus = tmpReadDataReg;
	assign fetchFace.readBus[0] = tmpReadDataReg;
	assign sIoFace.taskValid = ioTaskStepReg;

	
	logic[7:0] writeBytes[8];
	logic[63:0] writeBus;
	//解析出当前操作的内存地址
	always_comb begin
		case(ioInterface.widthCtr)
		0: writeBus = ioInterface.writeBus<<56;
		1: writeBus = ioInterface.writeBus<<48;
		2: writeBus = ioInterface.writeBus<<32;
		3: writeBus = ioInterface.writeBus;
		endcase

		writeBytes[0] = writeBus[63:56];
		writeBytes[1] = writeBus[55:48];
		writeBytes[2] = writeBus[47:40];
		writeBytes[3] = writeBus[39:32];
		writeBytes[4] = writeBus[31:24];
		writeBytes[5] = writeBus[23:16];
		writeBytes[6] = writeBus[15:8];
		writeBytes[7] = writeBus[7:0];

		if(isFetchTaskReg)begin
			sIoFace.address = fetchFace.address + rwByteStepReg;
			sIoFace.rwCtrl = 0;
			sIoFace.writeBus = 0;
		end else begin 
			sIoFace.address = ioInterface.address + rwByteStepReg;
			sIoFace.rwCtrl  = ioInterface.rwCtrl;
			sIoFace.writeBus = writeBytes[rwByteStepReg];
		end
	end
	wire ioInterface__taskValid = ioInterface.taskValid;
	wire [1:0] ioInterface__widthCtr = ioInterface.widthCtr;

	always_ff @(posedge clk or negedge rst)begin
		if(!rst || askInterHandle || askRestartHandle)begin
			//硬重启处理
			ioTaskStepReg <= 0;
			rwByteStepReg <= 0;
			isFetchTaskReg <= 0;
			ioAllStepReg <= 0;
			isErrorReg <= 0;
			tmpReadDataReg <= 0;
		end else if(ioTaskStepReg==0 && (ioInterface.taskValid || fetchFace.taskValid[0]))begin
			//开始一个读写任务
			isFetchTaskReg <= fetchFace.taskValid[0] && !ioInterface.taskValid;
			ioTaskStepReg <= 1;
			rwByteStepReg <= 0;
			isErrorReg <= 0;
			tmpReadDataReg <= 0;

			if(ioInterface.taskValid)begin

				if(ioInterface.widthCtr == 0)begin
					ioAllStepReg <= 1;
				end else if(ioInterface.widthCtr == 1)begin
					ioAllStepReg <= 2;
				end else if(ioInterface.widthCtr == 2)begin
					ioAllStepReg <= 4;
				end else if(ioInterface.widthCtr == 3)begin
					ioAllStepReg <= 8;
				end
			end else begin
				ioAllStepReg <= 4;
			end
		end else if(ioTaskStepReg==1 && sIoFace.taskReady)begin
			tmpReadDataReg <= (tmpReadDataReg<<8) | {56'd0,sIoFace.readBus};
			if((rwByteStepReg == ioAllStepReg-1) || sIoFace.taskError)begin
				ioTaskStepReg <= 2;
				rwByteStepReg <= 0;
				isErrorReg <= sIoFace.taskError;
			end else begin
				rwByteStepReg <= rwByteStepReg + 1;
			end
		end else if(ioTaskStepReg==2)begin
			ioTaskStepReg <= 0;
			rwByteStepReg <= 0;
			isFetchTaskReg <= 0;
			ioAllStepReg <= 0;
			isErrorReg <= 0;
			tmpReadDataReg <= 0;
		end
	end
endmodule
