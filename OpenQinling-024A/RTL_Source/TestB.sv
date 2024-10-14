
module VritualMemory#(
	parameter integer memLen = 1024*1024,
	parameter string binPath = ""

)(SingleChannelIO.slave ioInterface,
					input logic clk,
					input logic rst
);
	byte memoryData[memLen];

	wire isError = (ioInterface.address >= memLen);
	wire isWrite = ioInterface.rwCtrl && !isError;
	always_comb begin
		if(ioInterface.taskValid)begin
			ioInterface.taskReady = 1;
			ioInterface.taskError = isError;
			if(!ioInterface.rwCtrl && !ioInterface.taskError)begin
				ioInterface.readBus = memoryData[ioInterface.address];
			end else begin
				ioInterface.readBus = 0;
			end
		end else begin
			ioInterface.readBus = 0;
			ioInterface.taskReady = 0;
			ioInterface.taskError = 0;
		end
	end

	always_ff@(posedge clk or negedge rst)begin
		if(!rst)begin
			integer file;
			file = $fopen(binPath,"rb");// "rb"表示以二进制读模式打�?
			if (file == 0) begin
				$display("Error opening file!!!");
			end
			$fread(memoryData, file);
		end else if(isWrite)begin
			memoryData[ioInterface.address] <= ioInterface.writeBus;
		end
	end
endmodule
module TestBench__TopModule();
	logic clk;
	logic rst;
	byte memoryData[];

	integer i;
	initial begin
		rst <= 0;
		clk <= 0;
		#10
		rst <= 1;
		#10
		while(1)begin
			#10 
			clk <= ~clk;
		end
	end

	initial begin
		i <=0;
		while(1)begin
			#40
			i <= i + 1;
		end
		
	end


	logic [63:0]allRegValue[31];
	logic askInterHandle;//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
	logic askRestartHandle;//连接�?有内设和外设,当为1时表示CPU已经在重启处理流程中
	wire allDeviceBeReadyInter;//�?1表示�?有内设都已经准备好进行中断响�?
	wire allDeviceBeReadyRestart;//�?1表示�?有内设和外设都已经准备好进行重启
	
	wire interAsk = 0;//外部中断请求
	logic [7:0]interCode = 16;//外部中断�?
	IO_Interface ioInterface();
	FetchOrder_Interface fetchFace();

	SingleChannelIO sIoFace();

	SingleChannelIO_TransMod transMod(clk,//时钟
						rst,//硬重�?
						//对于askInterHandle请求信号，如果当前正在执行写内存任务执行到了�?半，必须要将内存恢复到开始执行此次写任务前的状�?��??
						askInterHandle,//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
						askRestartHandle,//连接�?有内设和外设,当为1时表示CPU已经在重启处理流程中
						allDeviceBeReadyInter,//返回1表示已经准备好进行中断响�?
						allDeviceBeReadyRestart,//返回1表示已经准备好进行重�?
						ioInterface,
						fetchFace,//只支持以此取�?条指�?
						sIoFace
						);


	VritualMemory#(
		.memLen(1024*1024),
		.binPath("D:\\Files\\Test024A_Project\\main.bin")
	) vritual(
		sIoFace,
		clk,
		rst
	);
	OpenQinling_024ACore core(
		//时钟/硬重启信�?
		clk,rst,
		//�?有程序员可见寄存器的信息
		allRegValue,
		//CPU内核连接外设/内设的接�?
		askInterHandle,//连接内设就行,当为1时表示CPU已经在中断响应处理流程中
		askRestartHandle,//连接�?有内设和外设,当为1时表示CPU已经在重启处理流程中
		allDeviceBeReadyInter,//�?1表示�?有内设都已经准备好进行中断响�?
		allDeviceBeReadyRestart,//�?1表示�?有内设和外设都已经准备好进行重启
		//中断模块的中断链接请�?
		interAsk,//外部中断请求
		interCode,//外部中断�?
		//连接cpu数据缓存/MMU的接�?
		ioInterface,
		//连接ICache的取指接�?
		fetchFace
	);
endmodule