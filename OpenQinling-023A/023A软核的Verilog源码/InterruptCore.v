//�жϺ���
module InterruptCore(
		//�ж�����ӿ�
		input int_sign_external,//�ⲿ�ж��ź�
		input [7:0]int_num_external,//�ⲿ�жϺ������
		input int_sign_internal,//�ڲ��ж��ź�
		input [7:0]int_num_internal,//�ڲ��жϺ������
		
		input [31:0]sys,//�ж��Ƿ����Ĵ���
		/*
			��ta==1������£�ʱ�����������sign==1���жϾͻᱻ������
			���жϴ����󣬻��Զ���ta��0
		*/
		output la_ta_ask,//cpuȨ����Ϊr0,���ر��жϵ�����,����ipc��pc��ַ�������źš����жϺ�������������ȫ����1
		//����ipc�Ĵ������ݵĽӿ�
		output [31:0]ipc_w,
		output clean_ask,//�����ˮ������
		output intering,//cpu�ж���Ӧ���źŷ���
		
		//cpu��ˮ�߸����̵�ִ��ָ���ַ��ִ��״̬
		input [31:0]p1_add,p2_add,p3_add,p4_add,pc,
		input p1_run,p2_run,p3_run,p4_run,
		
		//����cp�Ĵ�����ֵ�Ľӿ�
		output [31:0]pc_w,
		
		//����RAM��д�ӿڡ����ж�δ����ʱ,ִ��ģ��ram�ӿ����ӵ�cpu�����ram�ӿ��ϡ��ж�Ԥ��������У���ģ���ram�ӿ����ӹ�ȥ
		input [31:0]ram_data_bus_read,
		output[31:0]ram_add_bus,
		output[1:0]ram_size,
		output[1:0]ram_rw,//00Ϊ������д��10Ϊ����11Ϊд[ram_rw[0]�½���д��]
		input isCplt,//�ڴ��豸��Ӧ�ź�
		output get_ram_ask,//�����ȡram�����ӿ�
		
		input clk
	);
	
	
	
	reg[2:0]flow_index = 0;//��ǰ�����жϴ���Ĳ���
	
	//д��ipc�Ĵ������ݵĽӿ�
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
	
	//�ڴ�����ӿ�[���ڶ�ȡ�ж�����������]
	reg [31:0]ram_add_bus_reg = 0;
	reg [1:0]ram_size_reg = 0;
	reg [1:0]ram_rw_reg = 0;
	reg get_ram_ask_reg = 0;
	assign ram_add_bus = ram_add_bus_reg;
	assign ram_size = ram_size_reg;
	assign ram_rw = ram_rw_reg;
	assign get_ram_ask = get_ram_ask_reg;
	
	//����pc�Ĵ����Ľӿ�
	reg [31:0]pc_w_reg = 0;
	assign pc_w = pc_w_reg;
	
	
	
	assign intering = flow_index===2;
	always@(posedge clk)begin
		if(flow_index===0 && sys[0] && (int_sign_external||int_sign_internal))begin
			//�жϳ����������1:������ˮ���м���ִ����ɵ��Ǹ�ָ��ĵ�ַ��ipc�У���ˢ����ˮ�ߡ�
			//(������ⲿ�жϺ��ڲ��ж�ͬʱ���֣�ִ���ⲿ�ж�,���������ڲ��жϵ�ָ��ĵ�ַ����ipc�У��Ӷ����ⲿ�жϴ�����ɺ�����ִ�������жϵ�ָ��ٶԸ�ָ��������ж�������)
			flow_index <= 1;
			save_ipc_reg <= save_ipc_t;
			
			//�����жϺ������д�ж��������ȡ�жϺ�����ַ��
			
			//�����жϺŻ�ȡ�жϴ�������ַ������ڵ��ڴ��ַ��
			ram_add_bus_reg <= (int_sign_external ? int_num_external : int_num_internal)<<2;
			ram_size_reg <= 3;
			ram_rw_reg <= 2;
			get_ram_ask_reg <= 1;
		end
		else if(flow_index===1 && isCplt)begin
			//��ȡ�жϺ�����ַ��ɺ󣬽�������ַ����cp��
			flow_index <= 2;
			
			//ֹͣ�ڴ��д
			ram_add_bus_reg <= 0;
			ram_size_reg <= 0;
			ram_rw_reg <= 0;
			get_ram_ask_reg <= 0;
			
			
			pc_w_reg <= ram_data_bus_read;
		end
		else if(flow_index===2)begin
			flow_index <= 0;
		end
	end
endmodule