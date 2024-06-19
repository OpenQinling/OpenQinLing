module OpenQinling_023A(
		input clk,//ʱ�ӽӿ�
		input int_ask,//�ж�����ӿ�
		input[7:0]int_num,//�жϺ�����ӿ�[0-255]
		 
		output is_enable_vm,//�����ڴ��Ƿ�����
		output[31:0]tlb_address,//�����ڴ�tlb���ַ���[0-4GB]
		
		input [31:0]order_bus,//ָ���������[4bytes]
		output [31:0]order_address,//ָ���ַ���[0-4GB]
		input order_read_cplt,//ָ�����Ӧ�ź�����
		
		input [31:0]data_bus_r,//�ڴ������
		output[31:0]data_bus_w,//�ڴ�д����
		output [31:0]data_address,//�ڴ��ַ���[0-4GB]
		output [1:0]data_rw,//�ڴ��дģʽ����[0/1������д,2��,3д]
		output [1:0]data_size,//�����ڴ��д�ֽ�������[1-4bytes]
		input data_rw_cplt,//���ݻ�����Ӧ�ź�����
		
		output stoping,//cpu��ͣ��
		output intering,//cpu��Ӧ�ж���
		output rsting,//cpu������
		
		//cpu�ڲ��Ĵ������ݲ鿴�ӿ�(����Debug)
		output[31:0]r1_d,r2_d,r3_d,r4_d,r5_d,r6_d,cs_d,ds_d,flag_d,pc_d,tpc_d,ipc_d,sp_d,tlb_d,sys_d
	);
	
	//�Ĵ������ݶ�ȡ�ӿ�
	wire [31:0]r1,r2,r3,r4,r5,r6,cs,ds,flag,pc,tpc,ipc,sp,tlb,sys;
	wire [31:0]reg_r1,reg_r2,reg_r3,reg_r4,reg_r5,reg_r6,reg_cs,reg_ds,reg_flag,reg_pc,reg_tpc,reg_ipc,reg_sp,reg_tlb,reg_sys;
	assign r1_d = reg_r1;
	assign r2_d = reg_r2;
	assign r3_d = reg_r3;
	assign r4_d = reg_r4;
	assign r5_d = reg_r5;
	assign r6_d = reg_r6;
	assign cs_d = reg_cs;
	assign ds_d = reg_ds;
	assign flag_d = reg_flag;
	assign pc_d = reg_pc;
	assign tpc_d = reg_tpc;
	assign ipc_d = reg_ipc;
	assign sp_d = reg_sp;
	assign tlb_d = reg_tlb;
	assign sys_d = reg_sys;
	assign tlb_address = reg_tlb;
	assign is_enable_vm = reg_sys[2];
	
	//ָ�����ģ������cp��tcp�Ĵ���ͨ��
	wire [31:0]loadorder_pc,loadorder_tpc,loadorder_ipc;
	wire loadorder_tpc_ask;
	wire loadorder_ipc_ask;
	wire [31:0]loadorder_sys;
	wire loadorder_sys_ask;
	
	//��дģ������Ĵ����ӿ�
	wire [31:0] back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_cs,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb;
	wire back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_cs_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask;
	
	//�ж��ź�
	wire interrupt1,interrupt2,interrupt3,interrupt4,interrupt_tmp;
	wire[7:0] interrupt_num1,interrupt_num2,interrupt_num3,interrupt_num4,interrupt_num_tmp;
	//��ˮ�߸����̵�ǰִ��ָ��������ַ
	wire[31:0]pro1_add,pro2_add,pro3_add,pro4_add;
	wire pro1_isRun,pro2_isRun,pro3_isRun,pro4_isRun;
	
	//�ж�ģ������ӿ�
	wire interrupt_ask;
	wire [31:0]interrupt_ipc;
	wire [31:0]interrupt_pc;
	
	wire loadOrder_s;
	wire rwRAM_s;
	
	wire int_clear_sign;
	
	wire cpu_rst_ask;//cpu��������
	
	wire Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,Load_stop,Analysis_stop,Execute_stop,AllGroup_stop;
	
	//cpu������ָ�����͡�������Ϣ
	wire isDepTPC_1,isDepIPC_1;
	wire isEffTPC_1,isEffIPC_1,isEffFlag_1,isEffCS_1;
	wire isFourCycle_1;
		
	wire isDepTPC_2,isDepIPC_2;
	wire isEffTPC_2,isEffIPC_2,isEffFlag_2,isEffCS_2;
	wire isFourCycle_2;
		
	wire isDepTPC_3,isDepIPC_3;
	wire isEffTPC_3,isEffIPC_3,isEffFlag_3,isEffCS_3;
	wire isFourCycl_3;
	
	assign stoping = Load_stop | Analysis_stop | Execute_stop | AllGroup_stop;
	assign rsting = cpu_rst_ask;
	
	
	
	
	//ʱ�ӹ���ģ��
	ClockCtrl clockctrl(
		loadOrder_s,
		rwRAM_s,
		int_clear_sign,
		cpu_rst_ask,
		Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,
		Load_stop,Analysis_stop,Execute_stop,AllGroup_stop
	);
	
	
	//�Ĵ�����ģ��
	RegGroup reg_group(
		reg_r1,reg_r2,reg_r3,reg_r4,reg_r5,reg_r6,reg_cs,reg_ds,reg_flag,reg_pc,reg_tpc,reg_ipc,reg_sp,reg_tlb,reg_sys,
		loadorder_pc,loadorder_tpc,loadorder_ipc,loadorder_sys,
		loadorder_tpc_ask,loadorder_ipc_ask,loadorder_sys_ask,
		back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_cs,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_cs_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
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
	
	wire suspend;//ָ�����ģ��������ͣ��ˮ���ź�
	wire [31:0]order;//ָ�����ģ���ȡ����ָ��
	
	//ָ�����ģ��
	LoadOrder loadorder_core(
		flag,
		cs,
		pc,//ȡֵ��ַ����
		loadorder_pc,//����ȡֵ��ַ
		
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
		Load_stop,//��ǰ�Ƿ���ֹ������
		Load_rst,//�����ź�
		
		loadOrder_s,//����ʱ�ӹ�������ͣ���ź�
		cpu_rst_ask,//��������cpu�ź�
		
		//ָ���ȡ���ߵĽӿ�
		order_address,
		order_bus,
		order_read_cplt,//���1��ʾ��ȡ�ɹ�
		
		//////////�������///////////////
		order,//�����ȡ�ĵ�ַ
		pro1_add,
		pro1_isRun,
		interrupt1,interrupt_num1,
		
		isDepTPC_1,isDepIPC_1,
		isEffTPC_1,isEffIPC_1,isEffFlag_1,isEffCS_1,
		isFourCycle_1,
		
		isDepTPC_2,isDepIPC_2,
		isEffTPC_2,isEffIPC_2,isEffFlag_2,isEffCS_2,
		isFourCycle_2,
		
		isDepTPC_3,isDepIPC_3,
		isEffTPC_3,isEffIPC_3,isEffFlag_3,isEffCS_3,
		isFourCycl_3
	);
	
	wire [4:0]mode;//��ģʽ
	wire rw;//�ڴ��д�ķ������
	wire [1:0]subMode;//��ģʽ���ڴ��д���ֽڿ���
	wire [31:0]x1,x2;
	wire [31:0]reg_x1,reg_x2;
	wire [31:0]x2_inum;
	wire [4:0]m_num,l_num;
	wire [3:0]x1_channel_select,x2_channel_select;
	wire [3:0]y1_channel_select;
	wire [1:0]y2_channel_select;
	
	//ָ�����ģ��
	OrderAnalysis analy_core(
		order,//ָ���ȡ��������ָ��
		clk,
		Analysis_rst,//��ǰ�Ƿ���ֹ������
		Analysis_stop,//�����ź�
		
		r1,r2,r3,r4,r5,r6,cs,ds,flag,pc,tpc,ipc,sp,tlb,sys,//�Ĵ������ݶ�ȡ�ӿ�
		
		mode,
		rw,
		subMode,
		reg_x1,reg_x2,
		x2_inum,
		m_num,l_num,
		x1_channel_select,
		x2_channel_select,
		y1_channel_select,
		y2_channel_select,
		
		pro1_add,pro2_add,
		pro1_isRun,pro2_isRun,
		interrupt1,interrupt_num1,
		interrupt2,interrupt_num2,
		
		isDepTPC_1,isDepIPC_1,
		isEffTPC_1,isEffIPC_1,isEffFlag_1,isEffCS_1,
		isFourCycle_1,
		isDepTPC_2,isDepIPC_2,
		isEffTPC_2,isEffIPC_2,isEffFlag_2,isEffCS_2,
		isFourCycle_2
	);
	
	wire [31:0]data_ram_data_bus_write,data_ram_data_bus_read,data_ram_add_bus;
	wire [31:0]data_ram_bus_tmp;
	wire [1:0]data_ram_size,data_ram_rw;
	
	wire [3:0]y1_channel;
	wire [1:0]y2_channel;
	wire [31:0]y1_data,y2_data;
	
	wire [1:0] int_arm_rw;
	wire [1:0]int_arm_size;
	wire [31:0]int_arm_data_read;
	wire [31:0]int_arm_add;
	wire int_arm_ask;
	assign data_rw = int_arm_ask? int_arm_rw:data_ram_rw;
	assign data_size = int_arm_ask? int_arm_size:data_ram_size;
	assign data_address = int_arm_ask? int_arm_add:data_ram_add_bus;
	assign data_bus_w = !int_arm_ask&&data_ram_rw==3 ? data_ram_data_bus_write : 32'bz;
	assign int_arm_data_read = int_arm_ask ? data_bus_r : 32'bz;
	assign data_ram_data_bus_read = int_arm_ask ? 32'bz : data_bus_r;
	
	
	ExecuteModule execute(
		mode,
		rw,
		subMode,
		x1,x2,
		m_num,l_num,
		y1_channel_select,
		y2_channel_select,
		stoping,
		data_ram_data_bus_write,
		data_ram_data_bus_read,
		data_ram_add_bus,
		data_ram_size,data_ram_rw,
		data_rw_cplt,
		clk,
		Execute_stop,//��ǰ�Ƿ���ֹ������
		Execute_rst,//�����ź�
		rwRAM_s,
		y1_channel,
		y2_channel,
		y1_data,
		y2_data,
		pro2_add,pro3_add,
		pro2_isRun,pro3_isRun,
		interrupt2,interrupt_num2,
		interrupt3,interrupt_num3,
		
		isDepTPC_2,isDepIPC_2,
		isEffTPC_2,isEffIPC_2,isEffFlag_2,isEffCS_2,
		isFourCycle_2,
		isDepTPC_3,isDepIPC_3,
		isEffTPC_3,isEffIPC_3,isEffFlag_3,isEffCS_3,
		isFourCycl_3
	);
	
	WriteBack writeback(
		y1_channel,
		y2_channel,
		y1_data,
		y2_data,
		back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_cs,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_cs_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		reg_sys,
		interrupt3,interrupt_num3,
		interrupt_tmp,interrupt_num_tmp
	);
	
	//�жϺ���
	InterruptCore inter(
		int_ask,
		int_num,
		interrupt4,
		interrupt_num4,
		reg_sys,
		interrupt_ask,
		interrupt_ipc,
		int_clear_sign,
		intering,
		pro1_add,pro2_add,pro3_add,pro4_add,pc,
		pro1_isRun,pro2_isRun,pro3_isRun,pro4_isRun,
		interrupt_pc,
		int_arm_data_read,
		int_arm_add,
		int_arm_size,
		int_arm_rw,
		data_rw_cplt,
		int_arm_ask,
		clk
	);
	
	DataBypass bypass(
		reg_r1,reg_r2,reg_r3,reg_r4,reg_r5,reg_r6,reg_cs,reg_ds,reg_flag,reg_pc,reg_tpc,reg_ipc,reg_sp,reg_tlb,reg_sys,
		back_r1,back_r2,back_r3,back_r4,back_r5,back_r6,back_cs,back_ds,back_flag,back_tpc,back_ipc,back_sp,back_tlb,
		back_r1_ask,back_r2_ask,back_r3_ask,back_r4_ask,back_r5_ask,back_r6_ask,back_cs_ask,back_ds_ask,back_flag_ask,back_tpc_ask,back_ipc_ask,back_sp_ask,back_tlb_ask,
		r1,r2,r3,r4,r5,r6,cs,ds,flag,pc,tpc,ipc,sp,tlb,sys,
		
		reg_sys,
		mode,
		reg_x1,
		reg_x2,
		x2_inum,
		x1_channel_select,
		x2_channel_select,
		y1_channel,
		y2_channel,
		y1_data,
		y2_data,
		
		x1,x2
		
	);
	

endmodule


