//ȡֵ����
module LoadOrder(
		//flag�ź�
		input[31:0]flag_r,
		input[31:0]cs_r,
		
        //pc�Ĵ��������ӿ�
		input[31:0]pc_r,
		output[31:0]pc_w,
		
		//tpc�Ĵ��������ӿ�
		input[31:0]tpc_r,
		output[31:0]tpc_w,
		output tpc_ask,
		
		//ipc�Ĵ��������ӿ�
		input[31:0]ipc_r,
		output[31:0]ipc_w,
		output ipc_ask,
		
		//ϵͳģʽ�Ĵ��������ӿ�
		input[31:0]sys_r,
		output[31:0]sys_w,
		output sys_ask,
		
		input clk,//ʱ���ź�
		input isStop,//��ǰ�Ƿ���ֹ������
		input rst,//�����ź�
		
		output suspend,//����ʱ�ӹ�������ͣ���ź�
		
		output rst_ask,//����ʱ�ӹ���������cpu
		
		//ָ���ȡ���ߵĽӿ�
		output[31:0] add_bus,
		input [31:0] data_bus,
		input  isCplt,//���1��ʾ��ȡ�ɹ�
		
		//////////�������///////////////
		output[31:0]order,//�����ȡ�ĵ�ַ
		
		output[31:0]nextOrderAddress,
		output next_isRunning,
		
		//�ڲ��ж��źŷ����ӿ�
		output interrupt,
		output[7:0]interrupt_num,
		
		//����3����ˮ�������е�ָ��������������Ϣ
		input isDepTPC_1,isDepIPC_1,//Ӱ��ļĴ���
		input isEffTPC_1,isEffIPC_1,isEffFlag_1,isEffCS_1,//�����ļĴ���
		input isFourCycle_1,
		
		input isDepTPC_2,isDepIPC_2,
		input isEffTPC_2,isEffIPC_2,isEffFlag_2,isEffCS_2,
		input isFourCycle_2,
		
		input isDepTPC_3,isDepIPC_3,
		input isEffTPC_3,isEffIPC_3,isEffFlag_3,isEffCS_3,
		input isFourCycl_3
	);
	
	assign add_bus = pc_r;//��pc�Ĵ�����ַ��Ϊȡֵ��ַ
	
	assign suspend = !isCplt;//��ȡֵ���ǰ����ͣ��ˮ��
	
	reg [31:0]order_reg=0;//�����ȡ��ָ��
	assign order = order_reg;
	
	//pc�Ĵ�����д�ӿ�
	reg [31:0] pc_wt;
	assign pc_w = pc_wt;
	
	//tpc�Ĵ�����д�ӿ�
	reg [31:0] tpc_wt;
	assign tpc_w = tpc_wt;
	reg tpc_ask_t;
	assign tpc_ask = isCplt ? tpc_ask_t : 0;
	
	//ipc�Ĵ�����д�ӿ�
	reg [31:0] ipc_wt;
	assign ipc_w = ipc_wt;
	reg ipc_ask_t;
	assign ipc_ask = isCplt ? ipc_ask_t : 0;
	
	//sys�Ĵ��������ӿ�
	reg [31:0] sys_wt;
	assign sys_w = sys_wt;
	reg sys_askt;
	assign sys_ask = isCplt ? sys_askt : 0;
	
	
	//�ж��ź�����ӿ�
	reg interrupt_t;
	reg[7:0]interrupt_num_t;
	
	//�����ź�
	reg rst_ask_reg;
	assign rst_ask = isCplt ? rst_ask_reg : 0;
	
	reg insertNop;//��ǰ�����Ƿ��������
	always@(*)begin//�ڶ�ȡָ���pc�Ĵ���������ֵ
		if(data_bus[31:27]==10)begin//��������ת��ͣ��ˮ�ߣ�pc�����иı䡣������ˮ�ߡ�
			if(isEffFlag_1||isEffFlag_2)begin
				pc_wt = pc_r;
				
				insertNop = 1;
			end
			else begin
				if(flag_r===0)begin
					pc_wt = {pc_r[31:27],data_bus[26:0]};
				end
				else begin
					pc_wt = pc_r+4;
				end
				insertNop = 0;
			end
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			interrupt_t = 0;
			interrupt_num_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===11)begin//��������������ת��ֱ�ӽ���ת��ַ����pc
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
			insertNop = 0;
		end
		else if(data_bus[31:27]===12)begin//����������ת:pc<=������ַ; tpc<=pc;
			if(isDepTPC_1 || isEffTPC_1 || isEffTPC_2 || isEffTPC_3 || isEffCS_1 || isEffCS_2)begin
				insertNop = 1;
				pc_wt = pc_r;
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
			end
			else begin
				pc_wt = {cs_r[4:0],data_bus[26:0]};
				tpc_wt = pc_r+4;
				tpc_ask_t = !isStop ? 1 : 0;
				insertNop = 0;
			end
			interrupt_t = 0;
			interrupt_num_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			rst_ask_reg = 0;
		end
		else if(data_bus[31:27]===13)begin//���жϴ���
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
			insertNop = 0;
		end
		else if(data_bus[31:27]===14)begin//���ܿ鷵����ת
			if(data_bus[26:0]===0)begin//����������ת:pc��tpc��ֵ����
				if(isDepTPC_1 || isEffTPC_1 || isEffTPC_2 || isEffTPC_3)begin
					pc_wt = pc_r;
					tpc_ask_t = 0;
					tpc_wt = 32'bz;
					insertNop = 1;
				end
				else begin
					pc_wt = tpc_r;
					tpc_ask_t = !isStop ? 1 : 0;
					tpc_wt = pc_r+4;
					insertNop = 0;
				end
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				
				sys_wt = 32'bz;
				sys_askt = 0;
			end
			else if(data_bus[26:0]===1)begin//�жϷ�����ת
				if(isDepIPC_1 || isEffIPC_1 || isEffIPC_2 || isEffIPC_3)begin
					pc_wt = pc_r;
					ipc_ask_t = 0;
					ipc_wt = 32'bz;
					insertNop = 1;
				end
				else begin
					pc_wt = ipc_r;
					ipc_ask_t = !isStop ? 1 : 0;
					ipc_wt = pc_r+4;
					insertNop = 0;
				end
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
				
				sys_wt = 32'bz;
				sys_askt = 0;
			end
			else if(data_bus[26:0]===2)begin
				if(isFourCycle_1 || isFourCycle_2 || isFourCycl_3)begin
					pc_wt = pc_r;
					sys_wt = 32'bz;
					sys_askt = 0;
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
					insertNop = 1;
				end
				else begin
					pc_wt = ipc_r;
					
					sys_wt = {sys_r[31:1],1'd1};//�����ж�
					sys_askt = !isStop ? 1 : 0;
					
					ipc_wt = pc_r+4;
					ipc_ask_t = 1;
					insertNop = 0;
					
				end
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
			end
			else if(data_bus[26:0]===3)begin
				if(isFourCycle_1 || isFourCycle_2 || isFourCycl_3)begin
					pc_wt = pc_r;
					sys_wt = 32'bz;
					sys_askt = 0;
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
					insertNop = 1;
				end
				else begin//�����ж�/�����ڴ�
					pc_wt = ipc_r;
					
					sys_wt = {sys_r[31:3],1'd1,sys_r[1],1'd1};
					sys_askt = !isStop ? 1 : 0;
					
					ipc_wt = pc_r+4;
					ipc_ask_t = 1;
					insertNop = 0;
				end
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
			end
			else if(data_bus[26:0]===4)begin
				if(isFourCycle_1 || isFourCycle_2 || isFourCycl_3)begin
					pc_wt = pc_r;
					sys_wt = 32'bz;
					sys_askt = 0;
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
					insertNop = 1;
				end
				else begin
					pc_wt = ipc_r;
					
					sys_wt = 32'b111;//�����ж�/�����ڴ�/����ģʽ
					sys_askt = !isStop ? 1 : 0;
					
					ipc_wt = pc_r+4;
					ipc_ask_t = 1;
					insertNop = 0;
				end
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
			end
			else begin
				pc_wt = pc_r+4;
				ipc_ask_t = 0;
				ipc_wt = 32'bz;
				insertNop = 0;
				
				tpc_wt = 32'bz;
				tpc_ask_t = 0;
				
				sys_wt = 32'bz;
				sys_askt = 0;
			end
			rst_ask_reg = 0;
			interrupt_t = 0;
			interrupt_num_t = 0;
		end
		else if(data_bus[31:27]===15)begin//����cpuȨ�ޡ�ģʽ
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			rst_ask_reg = 0;
			if(isFourCycle_1 || isFourCycle_2 || isFourCycl_3)begin
				//���ǰ3��ָ��������һ��4����ָ��Ͳ����ָ��
				pc_wt = pc_r;
				sys_wt = 32'bz;//�����ж�
				interrupt_t = 0;
				interrupt_num_t = 0;
				sys_askt = 0;
				ipc_wt = 32'bz;
				ipc_ask_t = 0;
				insertNop = 1;
			end
			else begin
				insertNop = 0;
				case(data_bus[26:0])
				0:begin
					pc_wt = pc_r+4;
					sys_wt = {sys_r[31:1],1'd1};//�����ж�
					interrupt_t = 0;
					interrupt_num_t = 0;
					sys_askt = !isStop ? 1 : 0;
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
				end
				1:begin
					if(sys_r[1])begin
						sys_wt = 32'bz;//����ģʽ�²�������ж�
						interrupt_t = 1;
						interrupt_num_t = 8;
						sys_askt = 0;
					end
					else begin
						sys_wt = {sys_r[31:1],1'd0};//�ر��ж�
						interrupt_t = 0;
						interrupt_num_t = 0;
						sys_askt = !isStop ? 1 : 0;
					end
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
					pc_wt = pc_r+4;
				end
				2:begin
					sys_wt = {sys_r[31:2],1'd1,sys_r[0]};//��Ϊ����ģʽ
					interrupt_t = 0;
					interrupt_num_t = 0;
					sys_askt = !isStop ? 1 : 0;
					ipc_wt = 32'bz;
					ipc_ask_t = 0;
					pc_wt = pc_r+4;
				end
				3:begin
					sys_wt = {sys_r[31:3],1'd1,sys_r[1:0]};//���������ڴ�
					interrupt_t = 0;
					interrupt_num_t = 0;
					sys_askt = !isStop ? 1 : 0;
					
					ipc_wt = pc_r+4;
					ipc_ask_t = 1;
					pc_wt = ipc_r;
				end
				4:begin
					if(sys_r[1])begin
						sys_wt = 32'bz;//����ģʽ�²�����ر������ڴ�
						interrupt_t = 1;
						interrupt_num_t = 8;
						sys_askt = 0;
					end
					else begin
						sys_wt = {sys_r[31:3],1'd0,sys_r[1:0]};//�ر������ڴ�
						interrupt_t = 0;
						interrupt_num_t = 0;
						sys_askt = !isStop ? 1 : 0;
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
		
		end
		else if(data_bus[31:27]===31)begin//����cpu
			
			tpc_wt = 32'bz;
			tpc_ask_t = 0;
			ipc_wt = 32'bz;
			ipc_ask_t = 0;
			sys_wt = 32'bz;
			sys_askt = 0;
			
			if(isFourCycle_1 || isFourCycle_2 || isFourCycl_3)begin
				pc_wt = pc_r;
				interrupt_t = 0;
				interrupt_num_t = 0;
				rst_ask_reg = 0;
				insertNop = 1;
			end
			else begin
				insertNop = 0;
				pc_wt = pc_r+4;
				if(sys_r[1]==1)begin
					//����ģʽ�²�����������[ֻ��ͨ���жϽ���ʵģʽ����ʹ�ø�ָ��]
					interrupt_t = 1;
					interrupt_num_t = 8;
					rst_ask_reg = 0;
				end
				else begin
					interrupt_t = 0;
					interrupt_num_t = 0;
					rst_ask_reg = !isStop ? 1 : 0;
				end
			end
		end
		else if(data_bus[31:27]===0)begin//��ָ�pc+1
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
			insertNop = 0;
		end
		else begin//��ָͨ�����4
			if(data_bus[31:27]>22 && data_bus[31:27]<31)begin
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
			insertNop = 0;
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
		if(rst || (!isStop && insertNop))begin
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