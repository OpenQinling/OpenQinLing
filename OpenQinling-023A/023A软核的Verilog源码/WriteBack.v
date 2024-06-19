//��дͨ������ģ��/ϵͳ����ģ��
module WriteBack(
		input[3:0]y1_channel,//y1ͨ��ѡ�����
		input[1:0]y2_channel,//y2ͨ���Ƿ�����[0Ϊ�����ã�1Ϊflag,2Ϊsp]
		input[31:0]y1_data,//y1����
		input[31:0]y2_data,//ֻ��������ģʽ�²���Ч��ֱͨflag�Ĵ���
		
		//�Ĵ����������ͨ��
		output[31:0]r1,r2,r3,r4,r5,r6,cs,ds,flag,tpc,ipc,sp,tlb,
		
		//�Ĵ����Ƿ��������
		output r1_c,r2_c,r3_c,r4_c,r5_c,r6_c,cs_c,ds_c,flag_c,tpc_c,ipc_c,sp_c,tlb_c,
		
		//cpuϵͳģʽ��Ϣ
		input [31:0]sys_info,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num
	);
	
	//�Ĵ��������ӿ�
	reg [31:0]r1t,r2t,r3t,r4t,r5t,r6t,cst,dst,flagt,tpct,ipct,spt,tlbt;
	assign r1 = y1_data;
	assign r2 = y1_data;
	assign r3 = y1_data;
	assign r4 = y1_data;
	assign r5 = y1_data;
	assign r6 = y1_data;
	assign cs = y1_data;
	assign ds = y1_data;
	assign flag = y2_channel===1 ? y2_data : y1_data;
	assign tpc = y1_data;
	assign ipc = y1_data;
	assign sp =  y2_channel===2 ? y2_data : y1_data;
	assign tlb = y1_data;
	
	assign r1_c = y1_channel===1;
	assign r2_c = y1_channel===2;
	assign r3_c = y1_channel===3;
	assign r4_c = y1_channel===4;
	assign r5_c = y1_channel===5;
	assign r6_c = y1_channel===6;
	assign cs_c = y1_channel===7;
	assign ds_c = y1_channel===8;
	assign flag_c = y1_channel===9 || y2_channel===1;
	assign tpc_c = y1_channel===11;
	assign ipc_c = y1_channel===12;
	assign sp_c = y1_channel===13|| y2_channel===2;
	assign tlb_c = y1_channel===14 && !sys_info[2];
	
	
	wire interrupt_t = (sys_info[2]&&y1_channel===14) ? 1 :0;
	assign next_interrupt = interrupt_t || interrupt;
	wire [7:0]interrupt_num_t = (sys_info[2]&&y1_channel===14) ? 8 :0;
	assign next_interrupt_num = interrupt ? interrupt_num : interrupt_num_t;
endmodule
