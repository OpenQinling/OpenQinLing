///////////////////////////////////////////////////
//TB测试文件(可以指定编译生成的.bin文件进行仿真运行)
//////////////////////////////////////////////////
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
			file = $fopen(binPath,"rb");
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
	logic askInterHandle;
	logic askRestartHandle;
	wire allDeviceBeReadyInter;
	wire allDeviceBeReadyRestart;
	
	wire interAsk = 0;
	logic [7:0]interCode = 16;
	IO_Interface ioInterface();
	FetchOrder_Interface fetchFace();

	SingleChannelIO sIoFace();

	SingleChannelIO_TransMod transMod(clk,//时钟
						rst,
						askInterHandle,
						askRestartHandle,
						allDeviceBeReadyInter,
						allDeviceBeReadyRestart,
						ioInterface,
						fetchFace,
						sIoFace
						);


	VritualMemory#(
		.memLen(1024*1024),
		.binPath("D:\\Files\\Test024A_Project\\main.bin") //指定的.bin文件路径(仿真测试时,请改成字节的.bin测试文件路径)
	) vritual(
		sIoFace,
		clk,
		rst
	);
	OpenQinling_024ACore core(
		clk,rst,
		allRegValue,
		askInterHandle,
		askRestartHandle,
		allDeviceBeReadyInter,
		allDeviceBeReadyRestart,
		interAsk,
		interCode,
		ioInterface,
		fetchFace
	);
endmodule
