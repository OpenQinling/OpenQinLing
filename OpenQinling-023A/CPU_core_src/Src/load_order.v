//取值流程
module LoadOrder(
        //pc寄存器操作接口
		input[31:0]pc_r,
		output[31:0]pc_w,
		
		//tpc寄存器操作接口
		input[31:0]tpc_r,
		output[31:0]tpc_w,
		output tpc_ask,
		
		//ipc寄存器操作接口
		input[31:0]ipc_r,
		output[31:0]ipc_w,
		output ipc_ask,
		
		//系统模式寄存器操作接口
		input[31:0]sys_r,
		output[31:0]sys_w,
		output sys_ask,
		
		input clk,//时钟信号
		input isStop,//当前是否终止运行中
		input rst,//重启信号
		
		output suspend,//请求时钟管理器暂停的信号
		
		output rst_ask,//请求时钟管理器重启cpu
		
		//指令读取总线的接口
		output[31:0] add_bus,
		input [31:0] data_bus,
		input  isCplt,//输出1表示读取成功
		
		//////////数据输出///////////////
		output[31:0]order,//输出读取的地址
		
		output[31:0]nextOrderAddress,
		output next_isRunning,
		
		//软中断发出接口
		output interrupt,
		output[7:0]interrupt_num
	);
	
	assign add_bus = pc_r;//将pc寄存器地址作为取值地址
	
	assign suspend = !isCplt;//在取值完成前，暂停流水线
	
	reg [31:0]order_reg=0;//输出读取的指令
	assign order = order_reg;
	
	//pc寄存器读写接口
	reg [31:0] pc_wt;
	assign pc_w = pc_wt;
	
	//tpc寄存器读写接口
	reg [31:0] tpc_wt;
	assign tpc_w = tpc_wt;
	reg tpc_ask_t;
	assign tpc_ask = isCplt ? tpc_ask_t : 0;
	
	//ipc寄存器读写接口
	reg [31:0] ipc_wt;
	assign ipc_w = ipc_wt;
	reg ipc_ask_t;
	assign ipc_ask = isCplt ? ipc_ask_t : 0;
	
	//sys寄存器操作接口
	reg [31:0] sys_wt;
	assign sys_w = sys_wt;
	reg sys_askt;
	assign sys_ask = isCplt ? sys_askt : 0;
	
	
	//中断信号输出接口
	reg interrupt_t;
	reg[7:0]interrupt_num_t;
	
	//重启信号
	reg rst_ask_reg;
	assign rst_ask = isCplt ? rst_ask_reg : 0;
	
	always@(*)begin//在读取指令后，pc寄存器自增的值
		if(data_bus[31:27]===10)begin//有条件跳转暂停流水线，pc不进行改变。卡死流水线。
			interrupt_t = 0;
			interrupt_num_t = 0;
			pc_wt = pc_r+4;
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===11)begin//对于无条件短跳转，直接将跳转地址附给pc
			interrupt_t = 0;
			interrupt_num_t = 0;
			pc_wt = {pc_r[31:27],data_bus[26:0]};
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===12)begin//函数调用跳转:pc<=函数地址; tpc<=pc;
			interrupt_t = 0;
			interrupt_num_t = 0;
			pc_wt = {pc_r[31:27],data_bus[26:0]};
			tpc_wt = pc_r;
			tpc_ask_t = !isStop ? 1 : 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===13)begin//软中断处理
			if(data_bus[7:0]>15)begin
				interrupt_t = 1;
				interrupt_num_t = data_bus[7:0];
			end
			else begin
				interrupt_t = 1;
				interrupt_num_t = 1;
			end
			pc_wt = pc_r+4;
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===14)begin//功能块返回跳转
			if(data_bus[26:0]===0)begin//函数返回跳转:pc和tpc的值互换
				interrupt_t = 0;
				interrupt_num_t = 0;
				pc_wt = tpc_r;
				tpc_wt = pc_r;
				tpc_ask_t = !isStop ? 1 : 0;
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				sys_wt = 32'bz;
				sys_askt = 0;
				rst_ask_reg = 0;
			end
			else begin//中断返回跳转
				interrupt_t = 0;
				interrupt_num_t = 0;
				pc_wt = ipc_r;
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
				ipc_wt = pc_r;
				ipc_ask_t = !isStop ? 1 : 0;
				sys_wt = 32'bz;
				sys_askt = 0;
				rst_ask_reg = 0;
			end
		end
		else if(data_bus[31:27]===15)begin//设置cpu权限、模式
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			rst_ask_reg = 0;
			case(data_bus[26:0])
			0:begin
				pc_wt = pc_r+4;
				sys_wt = {sys_r[31:1],1'd1};//开启中断
				interrupt_t = 0;
				interrupt_num_t = 0;
				sys_askt = 1;
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
			end
			1:begin
				if(sys_r[1])begin
					sys_wt = 32'bz;//保护模式下不允许关中断
					interrupt_t = 1;
					interrupt_num_t = 8;
					sys_askt = 0;
				end
				else begin
					sys_wt = {sys_r[31:1],1'd0};//关闭中断
					interrupt_t = 0;
					interrupt_num_t = 0;
					sys_askt = 1;
				end
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				pc_wt = pc_r+4;
			end
			2:begin
				sys_wt = {sys_r[31:2],1'd1,sys_r[0]};//切为保护模式
				interrupt_t = 0;
				interrupt_num_t = 0;
				sys_askt = 1;
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				pc_wt = pc_r+4;
			end
			3:begin
				sys_wt = {sys_r[31:3],1'd1,sys_r[1:0]};//开启虚拟内存
				interrupt_t = 0;
				interrupt_num_t = 0;
				sys_askt = 1;
				
				ipc_wt = pc_r;
				ipc_ask_t = 1;
				pc_wt = ipc_r;
			end
			4:begin
				if(sys_r[1])begin
					sys_wt = 32'bz;//保护模式下不允许关闭虚拟内存
					interrupt_t = 1;
					interrupt_num_t = 8;
					sys_askt = 0;
				end
				else begin
					sys_wt = {sys_r[31:3],1'd0,sys_r[1:0]};//关闭虚拟内存
					interrupt_t = 0;
					interrupt_num_t = 0;
					sys_askt = 1;
				end
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				pc_wt = pc_r+4;
			end
			default:begin
				sys_wt = 32'bz;
				interrupt_t = 1;
				interrupt_num_t = 3;
				sys_askt = 0;
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				pc_wt = pc_r+4;
			end
		    endcase
		end
		else if(data_bus[31:27]===16)begin//重启cpu
			pc_wt = pc_r+4;
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			if(sys_r[1])begin
				//保护模式下不允许软重启[只有通过中断进入实模式才能使用该指令]
				interrupt_t = 1;
				interrupt_num_t = 8;
				rst_ask_reg = 0;
			end
			else begin
				interrupt_t = 0;
				interrupt_num_t = 0;
				rst_ask_reg = 1;
			end
		end
		else if(data_bus[31:27]===0)begin//空指令，pc+1
			pc_wt = pc_r+4;
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			interrupt_t = 0;
			interrupt_num_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else begin//普通指令，自增4
			if(data_bus[31:27]>18)begin
				interrupt_t = 1;
				interrupt_num_t = 3;
			end
			else begin
				interrupt_t = 0;
				interrupt_num_t = 0;
			end
			
			
			pc_wt = pc_r+4;
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
	end
	
	
	reg [31:0] nextOrderAddress_reg = 0;
	assign nextOrderAddress = nextOrderAddress_reg;
	reg next_isRunning_reg = 0;
	assign next_isRunning = next_isRunning_reg;
	
	reg interrupt_reg = 0;
	reg[7:0]interrupt_num_reg = 0;
	assign interrupt = interrupt_reg;
	assign interrupt_num = interrupt_num_reg;
	
	always@(posedge clk)begin
		if(rst)begin
			order_reg <= 0;
			next_isRunning_reg<=0;
			
			interrupt_reg<= 0;
			interrupt_num_reg<=0;
		end
		else if(!isStop)begin
			nextOrderAddress_reg <= pc_r;
			order_reg <= data_bus;
			next_isRunning_reg<=1;
			
			interrupt_reg<= interrupt_t;
			interrupt_num_reg<= interrupt_num_t;
		end
	end
	
endmodule