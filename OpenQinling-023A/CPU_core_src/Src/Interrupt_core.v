//中断核心
module InterruptCore(
		//中断请求接口
		input int_sign_external,//外部中断信号
		input [7:0]int_num_external,//外部中断号输入口
		input int_sign_internal,//内部中断信号
		input [7:0]int_num_internal,//内部中断号输入口
		
		input [31:0]sys,//中断是否开启寄存器
		/*
			在ta==1的情况下，时钟上升沿如果sign==1，中断就会被触发。
			在中断触发后，会自动将ta置0
		*/
		output la_ta_ask,//cpu权限置为r0,并关闭中断的请求,设置ipc和pc地址的请求信号。在中断函数启动过程中全程置1
		//设置ipc寄存器数据的接口
		output [31:0]ipc_w,
		output clean_ask,//清空流水线请求
		
		//cpu流水线各流程的执行指令地址和执行状态
		input [31:0]p1_add,p2_add,p3_add,p4_add,pc,
		input p1_run,p2_run,p3_run,p4_run,
		
		//设置cp寄存器数值的接口
		output [31:0]pc_w,
		
		//数据RAM读写接口。在中断未触发时,执行模块ram接口连接到cpu对外的ram接口上。中断预处理过程中，该模块的ram接口连接过去
		input [31:0]ram_data_bus,
		output[31:0]ram_add_bus,
		output[1:0]ram_size,
		output[1:0]ram_rw,//00为不读不写，10为读，11为写[ram_rw[0]下降沿写入]
		input isCplt,//内存设备响应信号
		output get_ram_ask,//请求获取ram操作接口
		
		input clk
	);
	
	
	
	reg[2:0]flow_index = 0;//当前进入中断处理的步骤
	
	//写入ipc寄存器数据的接口
	reg [31:0]save_ipc_t;
	reg [31:0]save_ipc_reg = 0;
	assign ipc_w = save_ipc_reg;
	
	reg la_ta_ask_reg = 0;
	reg clean_ask_reg = 0;
	assign la_ta_ask = la_ta_ask_reg;
	assign clean_ask = clean_ask_reg;
	
	always@(*)begin
		if(flow_index===0 && sys[0] && (int_sign_external||int_sign_internal))begin
			la_ta_ask_reg = 1;
			clean_ask_reg = 1;
		end
		else if(flow_index===1 || flow_index===2)begin
			la_ta_ask_reg = 1;
			clean_ask_reg = 1;
		end
		else begin
			la_ta_ask_reg = 0;
			clean_ask_reg = 0;
		end
		if(int_sign_external&&int_sign_internal)begin
			save_ipc_t = p4_add;
		end
		else if(int_sign_external||int_sign_internal)begin
			if(p3_run)begin
				save_ipc_t = p3_add;
			end
			else if(p2_run)begin
				save_ipc_t = p2_add;
			end
			else if(p1_run)begin
				save_ipc_t = p1_add;
			end
			else begin
				save_ipc_t = pc;
			end
		end
		else begin
			save_ipc_t = 0;
		end
	end
	
	//内存操作接口[用于读取中断向量表数据]
	reg [31:0]ram_add_bus_reg = 0;
	reg [1:0]ram_size_reg = 0;
	reg [1:0]ram_rw_reg = 0;
	reg get_ram_ask_reg = 0;
	assign ram_add_bus = ram_add_bus_reg;
	assign ram_size = ram_size_reg;
	assign ram_rw = ram_rw_reg;
	assign get_ram_ask = get_ram_ask_reg;
	
	//设置pc寄存器的接口
	reg [31:0]pc_w_reg = 0;
	assign pc_w = pc_w_reg;
	
	always@(posedge clk)begin
		if(flow_index===0 && sys[0] && (int_sign_external||int_sign_internal))begin
			//中断程序加载流程1:保存流水线中即将执行完成的那个指令的地址入ipc中，并刷新流水线。
			//(如果是外部中断和内部中断同时出现，执行外部中断,并将引起内部中断的指令的地址放入ipc中，从而让外部中断处理完成后重新执行引起中断的指令，再对该指令产生的中断做处理)
			flow_index <= 1;
			save_ipc_reg <= save_ipc_t;
			
			//根据中断号请求读写中断向量表获取中断函数地址。
			
			//根据中断号获取中断处理函数地址所存放在的内存地址。
			ram_add_bus_reg <= (int_sign_external ? int_num_external : int_num_internal)<<2;
			ram_size_reg <= 3;
			ram_rw_reg <= 2;
			get_ram_ask_reg <= 1;
		end
		else if(flow_index===1 && isCplt)begin
			//读取中断函数地址完成后，将读到地址放入cp。
			flow_index <= 2;
			
			//停止内存读写
			ram_add_bus_reg <= 0;
			ram_size_reg <= 0;
			ram_rw_reg <= 0;
			get_ram_ask_reg <= 0;
			
			
			pc_w_reg <= ram_data_bus;
		end
		else if(flow_index===2)begin
			flow_index <= 0;
		end
	end
endmodule