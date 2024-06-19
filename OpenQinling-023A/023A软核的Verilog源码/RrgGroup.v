//�Ĵ�����ģ��
module RegGroup(
		//�Ĵ������ݶ�ȡͨ��������ģ�鹲��
		output [31:0]r1,r2,r3,r4,r5,r6,r7,ds,flag,pc,tpc,ipc,sp,tlb,sys,
		
		//ָ�����ģ���޸�cp��tcp�Ĵ����Ĳ����ӿ�
		input [31:0] loadorder_pc,loadorder_tpc,loadorder_ipc,loadorder_sys,//����д��ͨ��
		input loadorder_tpc_ask,
		input loadorder_ipc_ask,
		input loadorder_sys_ask,
		
		//��дģ���޸ĸ��Ĵ����Ĳ����ӿ�
		input [31:0] back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_r7,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		input back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_r7_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		
		//�ж�ģ��la�Ĵ����Ĳ����ӿ�[�ڽ����жϺ�,cpu���Զ��л�Ϊr0Ȩ��,���ر��ж��źŽ���]
		input interrupt_ask,//�л�r0���ر��ж�����,������д��ipc
		input [31:0] interrupt_pc,
		input [31:0] interrupt_ipc,
		
		input clk,//ʱ���ź�
		input pc_stop,//��pc�Ĵ�����ͣ����
		input all_rst,//���Ĵ��������ź�[���мĴ�������ȫ����ʼ��]
		
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num
	);
	
	//�Ĵ�����
	reg [31:0]r1_reg=0,r2_reg=0,r3_reg=0,r4_reg=0,r5_reg=0,r6_reg=0,r7_reg=0,ds_reg=0,flag_reg=0,pc_reg=32'd1024,tpc_reg=0,ipc_reg=0,sp_reg=0,tlb_reg = 0,sys_reg=0;
	
	//����ͣģʽ�£���������е����ݣ�����ģʽ������Ĵ��������ݡ�[��ͣģʽ�£��Ĵ������ݿ��Ա��޸ģ����������ᱣ������ͣǰ�Ĵ������һ�̵�ֵ]
	assign r1 = r1_reg;
	assign r2 = r2_reg;
	assign r3 = r3_reg;
	assign r4 = r4_reg;
	assign r5 = r5_reg;
	assign r6 = r6_reg;
	assign r7 = r7_reg;
	assign ds = ds_reg;
	assign flag = flag_reg;
	assign pc = pc_reg;//��ͣ�����У����cache
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
		
		
		
		
		//���д��ģ��Ҫ�����pc/tpc����ô�Ͱѻ�д��ֵд���ȥ������ȡֵģ���ֵд��
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
		
		//�ж�ģ��
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
