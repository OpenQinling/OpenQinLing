//指令解析+参数获取
module Order_Analysis(
		input [31:0]order,//指令读取器读出的指令
		input clk,//时钟输入
		input rst,//重启
		input isStop,//是否停止
		
		//寄存器参数接口
		input[31:0]r1,r2,r3,r4,r5,r6,r7,r8,flag,pc,tpc,ipc,sp,tlb,sys,
		
		output [4:0]mode,//主模式
		output rw,//内存读写的方向控制
		output [1:0]subMode,//子模式、内存读写的字节控制
		output [31:0]x1,x2,//参数
		output [3:0]y1_channel_select,
		output [1:0]y2_channel_select,
		
		//当前流程所执行的指令地址
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		//软中断发出接口
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num
	);
	//输出寄存器
	wire [4:0]mode_reg = (order[31:27]>=1 && order[31:27]<=10)||order[31:27]===17||order[31:27]===18 ? order[31:27] : 0;
	wire rw_reg = (order[31:27]>=1 && order[31:27]<=10) ? order[26] : 0;
	wire [1:0]subMode_reg = (order[31:27]>=1 && order[31:27]<=10)||order[31:27]===17||order[31:27]===18 ? order[25:24] : 0;
	wire [3:0]x1_channel_reg = (order[31:27]>=1 && order[31:27]<=10)||order[31:27]===17||order[31:27]===18 ? (order[31:27]===8 ? 13 : order[23:20]) : 0;//如果为出入栈，x1通道固定为sp寄存器，作为内存读写地址
	wire [3:0]x2_channel_reg = (order[31:27]>=1 && order[31:27]<=10)||order[31:27]===17||order[31:27]===18 ? order[19:16] : 0;
	wire [15:0]num_reg = (order[31:27]>=1 && order[31:27]<=10)||order[31:27]===17||order[31:27]===18 ? order[15:0] : 0;
	reg [3:0]y1_channel_reg;
	reg [1:0]y2_channel_reg;
	
	always@(*)begin
		if((mode_reg>=1 && mode_reg<= 6) || mode_reg===9 || mode_reg===18)begin
			//如果是普通运算、转移,y1=x1;
			y1_channel_reg = x1_channel_reg;
		end
		else if((mode_reg===7 || mode_reg===8) && rw_reg===2)begin
			//如果是内存读取、y1=x2;
			y1_channel_reg = x2_channel_reg;
		end
		else if(mode_reg===10)begin
			//如果是有条件跳转，y1={tpc};
			y1_channel_reg = 11;//tpc编号为11
		end
		else begin//y1=0;
			y1_channel_reg = 0;
		end
		
		if((mode_reg>=1 && mode_reg<= 6)||mode_reg===17)begin
			//运算操作/比较操作，y2={flag};
			y2_channel_reg = 1;
		end
		else if(mode_reg===8)begin
			//堆栈操作，y2={sp};
			y2_channel_reg = 2;
		end
		else begin//其它操作不需要y2通道，y2=0
			y2_channel_reg = 0;
		end
	end
	
	
	
	//x1、x2参数获取
	reg [31:0]x1_reg,x2_reg;
	always@(*)begin
		//x2参数
		case(x2_channel_reg)
		0:x2_reg={16'd0,num_reg};
		1:x2_reg=r1;
		2:x2_reg=r2;
		3:x2_reg=r3;
		4:x2_reg=r4;
		5:x2_reg=r5;
		6:x2_reg=r6;
		7:x2_reg=r7;
		8:x2_reg=r8;
		9:x2_reg=flag;
		10:x2_reg=pc;
		11:x2_reg=tpc;
		12:x2_reg=ipc;
		13:x2_reg=sp;
		14:x2_reg=tlb;
		15:x2_reg=sys;
		endcase
		
		//x1选择
		case(x1_channel_reg)
		1:x1_reg=r1;
		2:x1_reg=r2;
		3:x1_reg=r3;
		4:x1_reg=r4;
		5:x1_reg=r5;
		6:x1_reg=r6;
		7:x1_reg=r7;
		8:x1_reg=r8;
		9:x1_reg=flag;
		10:x1_reg=pc;
		11:x1_reg=tpc;
		12:x1_reg=ipc;
		13:x1_reg=sp;
		14:x1_reg=tlb;
		15:x1_reg=sys;
		default:x1_reg=0;
		endcase
	end
	
	
	reg [31:0]x1_t = 0,x2_t = 0;
	reg [3:0] y1_channel_t = 0;
	reg [1:0] y2_channel_t = 0;
	reg rw_t = 0;
	reg [1:0]sub_mode_t = 0;
	reg [4:0]mode_t = 0;
	assign x1 = x1_t;
	assign x2 = x2_t;
	assign mode = mode_t;
	assign subMode = sub_mode_t;
	assign y1_channel_select = y1_channel_t;
	assign y2_channel_select = y2_channel_t;
	
	reg [31:0] nextOrderAddress_reg = 0;
	assign nextOrderAddress = nextOrderAddress_reg;
	reg next_isRunning_reg = 0;
	assign next_isRunning = next_isRunning_reg;
	
	reg interrupt_reg = 0;
	reg[7:0]interrupt_num_reg=0;
	assign next_interrupt = interrupt_reg;
	assign next_interrupt_num = interrupt_num_reg;
	
	
	always@(posedge clk)begin
		if(rst)begin
			x1_t <=0;
			x2_t <=0;
			y1_channel_t <= 0;
			y2_channel_t <= 0;
			sub_mode_t<=0;
			mode_t <=0;
			rw_t <= 0;
			next_isRunning_reg<=0;
			interrupt_reg<=0;
			interrupt_num_reg<=0;
			
		end
		else if(!isStop)begin
			x1_t <=x1_reg;
			x2_t <=x2_reg;
			y1_channel_t <= y1_channel_reg;
			y2_channel_t <= y2_channel_reg;
			rw_t <= rw_reg;
			sub_mode_t<=subMode_reg;
			mode_t <=mode_reg;
			nextOrderAddress_reg <= thisOrderAddress;
			next_isRunning_reg<=this_isRunning;
			
			interrupt_reg<=interrupt;
			interrupt_num_reg<=interrupt_num;
		end
	end
	
endmodule
