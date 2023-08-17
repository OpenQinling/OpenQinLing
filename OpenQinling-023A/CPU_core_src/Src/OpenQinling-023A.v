//Open-Enlightenment-023A内核
module CPU_023A(
		input clk,//时钟接口
		input int_ask,//中断请求接口
		input[7:0]int_num,//中断号输入接口[0-255]
		 
		output is_enable_vm,//虚拟内存是否启用
		output[31:0]tlb_address,//虚拟内存tlb表地址输出[0-4GB]
		
		input [31:0]order_bus,//指令加载总线[4bytes]
		output [31:0]order_address,//指令地址输出[0-4GB]
		input order_read_cplt,//指令缓存响应信号输入
		
		inout [31:0]data_bus,//内存数据读写总线[1-4bytes]
		output [31:0]data_address,//内存地址输出[0-4GB]
		output [1:0]data_rw,//内存读写模式控制[0/1不读不写,2读,3写]
		output [1:0]data_size,//单次内存读写字节数控制[1-4bytes]
		input data_rw_cplt,//数据缓存响应信号输入
		
		//cpu内部寄存器数据查看接口(用于Debug)
		output[31:0]r1_d,r2_d,r3_d,r4_d,r5_d,r6_d,r7_d,r8_d,flag_d,pc_d,tpc_d,ipc_d,sp_d,tlb_d,sys_d
	);
	
	//寄存器数据读取接口
	wire [31:0]r1,r2,r3,r4,r5,r6,r7,r8,flag,pc,tpc,ipc,sp,tlb,sys;
	assign r1_d = r1;
	assign r2_d = r2;
	assign r3_d = r3;
	assign r4_d = r4;
	assign r5_d = r5;
	assign r6_d = r6;
	assign r7_d = r7;
	assign r8_d = r8;
	assign flag_d = flag;
	assign pc_d = pc;
	assign tpc_d = tpc;
	assign ipc_d = ipc;
	assign sp_d = sp;
	assign tlb_d = tlb;
	assign sys_d = sys;
	assign tlb_address = tlb;
	assign is_enable_vm = sys[2];
	
	//指令加载模块设置cp、tcp寄存器通道
	wire [31:0]loadorder_pc,loadorder_tpc,loadorder_ipc;
	wire loadorder_tpc_ask;
	wire loadorder_ipc_ask;
	wire [31:0]loadorder_sys;
	wire loadorder_sys_ask;
	
	//回写模块操作寄存器接口
	wire [31:0] back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_r7,back_r8,back_flag,back_tpc,back_ipc,back_sp,back_tlb;
	wire back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_r7_ask,back_r8_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask;
	
	//中断信号
	wire interrupt1,interrupt2,interrupt3,interrupt4,interrupt_tmp;
	wire[7:0] interrupt_num1,interrupt_num2,interrupt_num3,interrupt_num4,interrupt_num_tmp;
	//流水线各流程当前执行指令所属地址
	wire[31:0]pro1_add,pro2_add,pro3_add,pro4_add;
	wire pro1_isRun,pro2_isRun,pro3_isRun,pro4_isRun;
	
	//中断模块操作接口
	wire interrupt_ask;
	wire [31:0]interrupt_ipc;
	wire [31:0]interrupt_pc;
	
	wire loadOrder_s;
	wire rwRAM_s;
	
	wire int_clear_sign;
	
	wire cpu_rst_ask;//cpu重启请求
	
	wire Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,Load_stop,Analysis_stop,Execute_stop,AllGroup_stop;
	
	//时钟管理模块
	ClockCtrl clockctrl(
		loadOrder_s,
		rwRAM_s,
		int_clear_sign,
		cpu_rst_ask,
		Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,
		Load_stop,Analysis_stop,Execute_stop,AllGroup_stop
	);
	
	//寄存器组模块
	REG_Group reg_group(
		r1,r2,r3,r4,r5,r6,r7,r8,flag,pc,tpc,ipc,sp,tlb,sys,
		loadorder_pc,loadorder_tpc,loadorder_ipc,loadorder_sys,
		loadorder_tpc_ask,loadorder_ipc_ask,loadorder_sys_ask,
		back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_r7,back_r8,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_r7_ask,back_r8_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		interrupt_ask,
		interrupt_pc,
		interrupt_ipc,
		clk,
		AllGroup_stop,
		AllGroup_rst,
		
		pro3_add,pro4_add,
		pro3_isRun,pro4_isRun,
		
		interrupt_tmp,interrupt_num_tmp,
		interrupt4,interrupt_num4
	);
	
	wire suspend;//指令加载模块请求暂停流水线信号
	wire [31:0]order;//指令加载模块读取到的指令
	
	
	
	//指令加载模块
	LoadOrder loadorder_core(
		pc,//取值地址输入
		loadorder_pc,//设置取值地址
		
		tpc,
		loadorder_tpc,
		loadorder_tpc_ask,
		ipc,
		loadorder_ipc,
		loadorder_ipc_ask,
		sys,
		loadorder_sys,
		loadorder_sys_ask,
		
		clk,
		Load_stop,//当前是否终止运行中
		Load_rst,//重启信号
		
		loadOrder_s,//请求时钟管理器暂停的信号
		cpu_rst_ask,//请求重启cpu信号
		
		//指令读取总线的接口
		order_address,
		order_bus,
		order_read_cplt,//输出1表示读取成功
		
		//////////数据输出///////////////
		order,//输出读取的地址
		pro1_add,
		pro1_isRun,
		interrupt1,interrupt_num1
	);
	
	wire [4:0]mode;//主模式
	wire rw;//内存读写的方向控制
	wire [1:0]subMode;//子模式、内存读写的字节控制
	wire [31:0]x1,x2;//参数
	wire [3:0]y1_channel_select;
	wire [1:0]y2_channel_select;
	
	
	//指令解析模块
	Order_Analysis analy_core(
		order,//指令读取器读出的指令
		clk,
		Analysis_rst,//当前是否终止运行中
		Analysis_stop,//重启信号
		
		r1,r2,r3,r4,r5,r6,r7,r8,flag,pc,tpc,ipc,sp,tlb,sys,//寄存器数据读取接口
		
		mode,
		rw,
		subMode,
		x1,x2,
		y1_channel_select,
		y2_channel_select,
		
		pro1_add,pro2_add,
		pro1_isRun,pro2_isRun,
		interrupt1,interrupt_num1,
		interrupt2,interrupt_num2
	);
	
	wire [31:0]data_ram_data_bus_r,data_ram_data_bus_w,data_ram_data_bus,data_ram_add_bus;
	wire [1:0]data_ram_size,data_ram_rw;
	
	wire [3:0]y1_channel;
	wire [1:0]y2_channel;
	wire [31:0]y1_data,y2_data;
	
	wire [1:0] int_arm_rw;
	wire [1:0]int_arm_size;
	wire [31:0]int_arm_data;
	wire [31:0]int_arm_add;
	wire int_arm_ask;
	
	assign data_rw = int_arm_ask? int_arm_rw:data_ram_rw;
	assign data_size = int_arm_ask? int_arm_size:data_ram_size;
	assign data_address = int_arm_ask? int_arm_add:data_ram_add_bus;
	
	assign data_bus = int_arm_ask?32'bz:data_ram_data_bus_w;
	assign data_ram_data_bus_r = data_bus;
	assign int_arm_data = data_bus;
	assign data_ram_data_bus_w = (!int_arm_ask && data_ram_rw==3)?data_ram_data_bus:32'bz;
	assign data_ram_data_bus = (!int_arm_ask && data_ram_rw==2)?data_ram_data_bus_r:32'bz;
	
	
	ExecuteModule execute(
		mode,
		rw,
		subMode,
		x1,x2,
		y1_channel_select,
		y2_channel_select,
		data_ram_data_bus,data_ram_add_bus,
		data_ram_size,data_ram_rw,
		data_rw_cplt,
		clk,
		Execute_stop,//当前是否终止运行中
		Execute_rst,//重启信号
		rwRAM_s,
		y1_channel,
		y2_channel,
		y1_data,
		y2_data,
		pro2_add,pro3_add,
		pro2_isRun,pro3_isRun,
		interrupt2,interrupt_num2,
		interrupt3,interrupt_num3
	);
	
	WriteBack writeback(
		y1_channel,
		y2_channel,
		y1_data,
		y2_data,
		back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_r7,back_r8,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_r7_ask,back_r8_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		sys,
		interrupt3,interrupt_num3,
		interrupt_tmp,interrupt_num_tmp
	);
	
	//中断核心
	InterruptCore inter(
		int_ask,
		int_num,
		interrupt4,
		interrupt_num4,
		sys,
		interrupt_ask,
		interrupt_ipc,
		int_clear_sign,
		pro1_add,pro2_add,pro3_add,pro4_add,pc,
		pro1_isRun,pro2_isRun,pro3_isRun,pro4_isRun,
		interrupt_pc,
		int_arm_data,
		int_arm_add,
		int_arm_size,
		int_arm_rw,
		data_rw_cplt,
		int_arm_ask,
		clk
	);
endmodule