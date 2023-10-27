//寄存器组模块
module RegGroup(
		//寄存器数据读取通道，所有模块共用
		output [31:0]r1,r2,r3,r4,r5,r6,r7,ds,flag,pc,tpc,ipc,sp,tlb,sys,
		
		//指令加载模块修改cp、tcp寄存器的操作接口
		input [31:0] loadorder_pc,loadorder_tpc,loadorder_ipc,loadorder_sys,//数据写入通道
		input loadorder_tpc_ask,
		input loadorder_ipc_ask,
		input loadorder_sys_ask,
		
		//回写模块修改各寄存器的操作接口
		input [31:0] back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_r7,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		input back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_r7_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		
		//中断模块la寄存器的操作接口[在进入中断后,cpu会自动切换为r0权限,并关闭中断信号进入]
		input interrupt_ask,//切换r0并关闭中断请求,将数据写入ipc
		input [31:0] interrupt_pc,
		input [31:0] interrupt_ipc,
		
		input clk,//时钟信号
		input pc_stop,//将pc寄存器暂停输入
		input all_rst,//各寄存器重启信号[所有寄存器数据全部初始化]
		
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num
	);
	
	//寄存器组
	reg [31:0]r1_reg=0,r2_reg=0,r3_reg=0,r4_reg=0,r5_reg=0,r6_reg=0,r7_reg=0,ds_reg=0,flag_reg=0,pc_reg=32'd1024,tpc_reg=0,ipc_reg=0,sp_reg=0,tlb_reg = 0,sys_reg=0;
	
	//在暂停模式下，输出缓存中的数据，正常模式下输出寄存器中数据。[暂停模式下，寄存器数据可以被修改，而缓存器会保存下暂停前寄存器最后一刻的值]
	assign r1 = r1_reg;
	assign r2 = r2_reg;
	assign r3 = r3_reg;
	assign r4 = r4_reg;
	assign r5 = r5_reg;
	assign r6 = r6_reg;
	assign r7 = r7_reg;
	assign ds = ds_reg;
	assign flag = flag_reg;
	assign pc = pc_reg;//暂停过程中，输出cache
	assign tpc = tpc_reg;
	assign ipc = ipc_reg;
	assign sp = sp_reg;
	assign tlb = tlb_reg;
	assign sys = sys_reg;
	
	
	reg interrupt_reg = 0;
	reg[7:0]interrupt_num_reg= 0;
	assign next_interrupt = interrupt_reg;
	assign next_interrupt_num = interrupt_num_reg;
	
	reg [31:0] nextOrderAddress_reg = 0;
	assign nextOrderAddress = nextOrderAddress_reg;
	reg next_isRunning_reg = 0;
	assign next_isRunning = next_isRunning_reg;
	
	always@(posedge clk)begin
		if(all_rst)begin
			interrupt_reg<= 0;
			interrupt_num_reg<=0;
			nextOrderAddress_reg <= 0;
			next_isRunning_reg<=0;
		end
		else begin
			interrupt_reg<= interrupt;
			interrupt_num_reg<= interrupt_num;
			nextOrderAddress_reg <= thisOrderAddress;
			next_isRunning_reg<=this_isRunning;
		end
		
		if(all_rst)begin
			r1_reg <= 0;
		end
		else if(back_r1_ask && !interrupt_ask)begin
			r1_reg <= back_r1;
		end
		
		
		if(all_rst)begin
			r2_reg <= 0;
		end
		else if(back_r2_ask && !interrupt_ask)begin
			r2_reg <= back_r2;
		end
		
		if(all_rst)begin
			r3_reg <= 0;
		end
		else if(back_r3_ask && !interrupt_ask)begin
			r3_reg <= back_r3;
		end
		
		if(all_rst)begin
			r4_reg <= 0;
		end
		else if(back_r4_ask && !interrupt_ask)begin
			r4_reg <= back_r4;
		end
		
		if(all_rst)begin
			r5_reg <= 0;
		end
		else if(back_r5_ask && !interrupt_ask)begin
			r5_reg <= back_r5;
		end
		
		if(all_rst)begin
			r6_reg <= 0;
		end
		else if(back_r6_ask && !interrupt_ask)begin
			r6_reg <= back_r6;
		end
		
		if(all_rst)begin
			r7_reg <= 0;
		end
		else if(back_r7_ask && !interrupt_ask)begin
			r7_reg <= back_r7;
		end
		
		if(all_rst)begin
			ds_reg <= 0;
		end
		else if(back_ds_ask && !interrupt_ask)begin
			ds_reg <= back_ds;
		end
		
		if(all_rst)begin
			flag_reg <= 0;
		end
		else if(back_flag_ask && !interrupt_ask)begin
			flag_reg <= back_flag;
		end
		
		if(all_rst)begin
			sp_reg <= 0;
		end
		else if(back_sp_ask && !interrupt_ask)begin
			sp_reg <= back_sp;
		end
		
		if(all_rst)begin
			tlb_reg <= 0;
		end
		else if(back_tlb_ask && !interrupt_ask)begin
			tlb_reg <= back_tlb;
		end
		
		
		
		
		//如果写回模块要求操作pc/tpc，那么就把回写的值写入进去。否则将取值模块的值写入
		if(all_rst)begin
			pc_reg<=32'd1024;
		end
		else if(interrupt_ask)begin
			pc_reg <= interrupt_pc;
		end
		else if(!pc_stop)begin
			pc_reg <= loadorder_pc;
		end
		
		
		
		if(all_rst)begin
			tpc_reg <= 0;
		end
		else if(back_tpc_ask && !interrupt_ask)begin
			tpc_reg <= back_tpc;
		end
		else if(loadorder_tpc_ask && !interrupt_ask)begin
			tpc_reg <= loadorder_tpc;
		end
		
		//中断模块
		if(all_rst||interrupt_ask)begin
			sys_reg <= 0;
		end
		else if(loadorder_sys_ask)begin
			sys_reg <= loadorder_sys;
		end
		
		if(all_rst)begin
			ipc_reg <= 0;
		end
		else if(back_ipc_ask && !interrupt_ask)begin
			ipc_reg <= back_ipc;
		end
		else if(interrupt_ask)begin
			ipc_reg <= interrupt_ipc;
		end
		
		
	end
endmodule
