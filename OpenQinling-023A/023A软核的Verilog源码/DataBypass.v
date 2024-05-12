//数据旁路，将修改的数据前递，降低程序占空率
module DataBypass(
	/////////////////////////旁路A////////////////////////////////
		//寄存器组模块的数据输入
		input [31:0]reg_r1,reg_r2,reg_r3,reg_r4,reg_r5,reg_r6,reg_cs,reg_ds,reg_flag,reg_pc,reg_tpc,reg_ipc,reg_sp,reg_tlb,reg_sys,
		
		//回写模块的数据输入
		input[31:0]back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_cs,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		input back_r1_c,back_r2_c,back_r3_c,back_r4_c,back_r5_c,back_r6_c,back_cs_c,back_ds_c,back_flag_c,back_tpc_c,back_ipc_c,back_sp_c,back_tlb_c,
		
		//数据输出口
		//[正常情况下是将寄存器的值作为数据的输出，但如果执行模块、回写模块对某寄存器的值进行了修改的话，该寄存器的值就会选择执行模块、回写模块的数据]
		output [31:0]r1,r2,r3,r4,r5,r6,cs,ds,flag,pc,tpc,ipc,sp,tlb,sys,
		
	
	//////////////////////////旁路B//////////////////////////////
		input[31:0]sys_info,//cpu状态配置信息。用于判断旁路b是否可被启用
		
		//取操作数模块输入
		input [4:0]mode,//主模式
		input [31:0]reg_x1,reg_x2,//参数
		input [31:0]x2_inum,//x2的立即数
		input [3:0]reg_x1_channel,
		input [3:0]reg_x2_channel,
		
		//执行模块的数据输入
		input[3:0]y1_channel_t,//y1通道选择输出
		input[1:0]y2_channel_t,//y2通道是否启用[0为不启用，1为flag,2为sp]
		input[31:0]y1_data,//y1数据
		input[31:0]y2_data,//只有在运算模式下才有效，直通flag寄存器
		
		//数据输出口
		output[31:0]x1,x2
	);
	
	//sys/pc直接输出寄存器组中的数据
	assign sys = reg_sys;
	assign pc = reg_pc;
	
	//数据输出
	reg [31:0]r1_r,r2_r,r3_r,r4_r,r5_r,r6_r,cs_r,ds_r,flag_r,pc_r,tpc_r,ipc_r,sp_r,tlb_r,sys_r;
	assign r1 = r1_r;
	assign r2 = r2_r;
	assign r3 = r3_r;
	assign r4 = r4_r;
	assign r5 = r5_r;
	assign r6 = r6_r;
	assign cs = cs_r;
	assign ds = ds_r;
	assign flag = flag_r;
	assign tpc = tpc_r;
	assign ipc = ipc_r;
	assign sp = sp_r;
	assign tlb = tlb_r;
	
	//其它寄存器:如果执行模块有修改，则输出执行模块修改的数据；
	//否则判断回写模块是否有修改，如果有修改输出回写模块数据。如果都无修改才输出寄存器中数据
	always@(*)begin
	  if(back_r1_c)begin
		r1_r = back_r1;
	  end
	  else begin
		r1_r = reg_r1;
	  end
	  
	  if(back_r2_c)begin
		r2_r = back_r2;
	  end
	  else begin
		r2_r = reg_r2;
	  end
	  
	  if(back_r3_c)begin
		r3_r = back_r3;
	  end
	  else begin
		r3_r = reg_r3;
	  end
	  
	  if(back_r4_c)begin
		r4_r = back_r4;
	  end
	  else begin
		r4_r = reg_r4;
	  end
	  
	  if(back_r5_c)begin
		r5_r = back_r5;
	  end
	  else begin
		r5_r = reg_r5;
	  end
	  
	  if(back_r6_c)begin
		r6_r = back_r6;
	  end
	  else begin
		r6_r = reg_r6;
	  end
	  
	  if(back_cs_c)begin
		cs_r = back_cs;
	  end
	  else begin
		cs_r = reg_cs;
	  end
	  
	  if(back_ds_c)begin
		ds_r = back_ds;
	  end
	  else begin
		ds_r = reg_ds;
	  end
	  
	  if(back_flag_c)begin
		flag_r = back_flag;
	  end
	  else begin
		flag_r = reg_flag;
	  end
	  
	  if(back_tpc_c)begin
		tpc_r = back_tpc;
	  end
	  else begin
		tpc_r = reg_tpc;
	  end
	  
	  if(back_ipc_c)begin
		ipc_r = back_ipc;
	  end
	  else begin
		ipc_r = reg_ipc;
	  end
	  
	  if(back_sp_c)begin
		sp_r = back_sp;
	  end
	  else begin
		sp_r = reg_sp;
	  end
	  
	  if(back_tlb_c)begin
		tlb_r = back_tlb;
	  end
	  else begin
		tlb_r = reg_tlb;
	  end
	end
	
	
	
	reg [31:0]x1_t,x2_t;
	assign x1 = x1_t;
	assign x2 = x2_t;
	reg [3:0]y1_channel;
	reg [3:0]y2_channel;
	always@(*)begin
		if(y2_channel_t==1)begin
			y2_channel = 9;
		end
		else if(y2_channel_t==2)begin
			y2_channel = 13;
		end
		else begin
			y2_channel = 0;
		end
	end
	
	always@(*)begin
		if(y1_channel_t == 14 && sys_info[2])begin
			y1_channel = 0;
		end
		else begin
			y1_channel = y1_channel_t;
		end
	end
	
	always@(*)begin
		if(y2_channel==reg_x1_channel && y2_channel!=0)begin
			x1_t = y2_data;
		end
		else if(y1_channel==reg_x1_channel && y1_channel!=0)begin
			x1_t = y1_data;
		end
		else begin
			x1_t = reg_x1;
		end
		
		
		if(y2_channel==reg_x2_channel && y2_channel!=0)begin
			x2_t = y2_data;
		end
		else if(y1_channel==reg_x2_channel && y1_channel!=0)begin
			x2_t = y1_data;
		end
		else if(reg_x2_channel==0 && (mode==16 || mode==17) && y1_channel==13)begin
			x2_t = y1_data + x2_inum;
		end
		else if(reg_x2_channel==0 && (mode==16 || mode==17) && y2_channel==13)begin
			x2_t = y2_data + x2_inum;
		end
		else if(reg_x2_channel==0 && mode==7 && y1_channel==8)begin
			x2_t = {y1_data[15:0],reg_x2[15:0]};
		end
		else begin
			x2_t = reg_x2;
		end
	end
endmodule