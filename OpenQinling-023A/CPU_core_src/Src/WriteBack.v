//回写通道解析模块/系统控制模块
module WriteBack(
		input[3:0]y1_channel,//y1通道选择输出
		input[1:0]y2_channel,//y2通道是否启用[0为不启用，1为flag,2为sp]
		input[31:0]y1_data,//y1数据
		input[31:0]y2_data,//只有在运算模式下才有效，直通flag寄存器
		
		//寄存器数据输出通道
		output[31:0]r1,r2,r3,r4,r5,r6,r7,ds,flag,tpc,ipc,sp,tlb,
		
		//寄存器是否允许输出
		output r1_c,r2_c,r3_c,r4_c,r5_c,r6_c,r7_c,ds_c,flag_c,tpc_c,ipc_c,sp_c,tlb_c,
		
		//cpu系统模式信息
		input [31:0]sys_info,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num
	);
	
	//寄存器操作接口
	reg [31:0]r1t,r2t,r3t,r4t,r5t,r6t,r7t,dst,flagt,tpct,ipct,spt,tlbt;
	assign r1 = r1t;
	assign r2 = r2t;
	assign r3 = r3t;
	assign r4 = r4t;
	assign r5 = r5t;
	assign r6 = r6t;
	assign r7 = r7t;
	assign ds = dst;
	assign flag = y2_channel===1 ? y2_data : flagt;
	assign tpc = tpct;
	assign ipc = ipct;
	assign sp =  y2_channel===2 ? y2_data : spt;
	assign tlb = tlbt;
	
	assign r1_c = y1_channel===1;
	assign r2_c = y1_channel===2;
	assign r3_c = y1_channel===3;
	assign r4_c = y1_channel===4;
	assign r5_c = y1_channel===5;
	assign r6_c = y1_channel===6;
	assign r7_c = y1_channel===7;
	assign ds_c = y1_channel===8;
	assign flag_c = y1_channel===9 || y2_channel===1;
	assign tpc_c = y1_channel===11;
	assign ipc_c = y1_channel===12;
	assign sp_c = y1_channel===13|| y2_channel===2;
	assign tlb_c = y1_channel===14 && !sys_info[2];
	
	
	reg interrupt_t;
	assign next_interrupt = interrupt_t || interrupt;
	reg [7:0]interrupt_num_t;
	assign next_interrupt_num = interrupt ? interrupt_num : interrupt_num_t;
	
	always@(*)begin
		//y1输出通道选择
		case(y1_channel)
		1:begin
			r1t =y1_data;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		2:begin
			r1t =32'bz;
			r2t = y1_data;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		3:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =y1_data;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		4:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = y1_data;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		5:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =y1_data;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		6:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = y1_data;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		7:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =y1_data;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		8:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t = 32'bz;
			dst = y1_data;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		9:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = y1_data;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		11:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = y1_data;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		12:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = y1_data;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		13:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = y1_data;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		14:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = (sys_info[2]) ? 32'bz :y1_data;
			interrupt_t = (sys_info[2]) ? 1 :0;
			interrupt_num_t= (sys_info[2]) ? 8 :0;
		end
		default:begin
			r1t =32'bz;
			r2t = 32'bz;
			r3t =32'bz;
			r4t = 32'bz;
			r5t =32'bz;
			r6t = 32'bz;
			r7t =32'bz;
			dst = 32'bz;
			flagt = 32'bz;
			tpct = 32'bz;
			ipct = 32'bz;
			spt = 32'bz;
			tlbt = 32'bz;
			interrupt_t = 0;
			interrupt_num_t=0;
		end
		endcase
	end
	
	
	
	
	
endmodule
