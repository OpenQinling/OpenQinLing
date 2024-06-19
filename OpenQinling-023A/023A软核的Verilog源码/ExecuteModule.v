//ִ��ģ��
module ExecuteModule(
		//�ϼ���ˮ�ߵĲ�������
		input [4:0]mode,//��ģʽ��
		input rw,//�ڴ��д�ķ������
		input [1:0]subMode,//��ģʽ���ڴ��д���ֽڿ���
		input [31:0]x1,x2,//����
		input [4:0]m_num,l_num,//λ����ָ���λ���ƺ�
		input [3:0]y1_channel_select,//y1��ͨ��
		input [1:0]y2_channel_select,//y2��ͨ��
		input cpu_isStop,//cpu�Ƿ���ͣ��
		
		//cpuRAM��д�ӿ�
		output[31:0]ram_data_bus_write,
		input [31:0]ram_data_bus_read,
		output[31:0]ram_add_bus,
		output[1:0]ram_size,
		output[1:0]ram_rw,//00Ϊ������д��10Ϊ��?11Ϊд[ram_rw[0]�½���д��]
		input isCplt,//�ڴ��豸��Ӧ�ź�
		
		input clk,//ʱ���ź�
		input isStop,//��ǰģ�����ͣ�ź�
		input rst,//�����ź�
		
		output suspend,//����ʱ�ӹ�������ͣ���ź�
		
		output[3:0]y1_channel,//y1ͨ��ѡ�����
		output[1:0]y2_channel,//y2ͨ���Ƿ�����[0Ϊ������,1Ϊflag,2Ϊsp]
		output[31:0]y1_data,//y1�������
		output[31:0]y2_data,//ֻ��������ģʽ�²���Ч��ֱ��flag�Ĵ���
		
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num,
		
		//ָ�����͡�ָ�������š�ָ��Ӱ���ź�
		input isDepTPC,isDepIPC,
		input isEffTPC,isEffIPC,isEffFlag,isEffCS,
		input isFourCycle,//�Ƿ���4�������͵�ָ��
		output next_isDepTPC,next_isDepIPC,
		output next_isEffTPC,next_isEffIPC,next_isEffFlag,next_isEffCS,
		output next_isFourCycle
	);
	
	//�ڴ�����ӿ�
	reg [31:0] setData_t;
	reg [31:0]ram_add_bus_t;
	reg [1:0]ram_rw_t;
	reg [1:0]ram_size_t;
	assign ram_data_bus_write = setData_t;
	assign ram_add_bus = ram_add_bus_t;
	assign ram_rw = ram_rw_t;
	assign ram_size = ram_size_t;
	
	//��ͣ����ӿ�
	reg suspend_reg;
	assign suspend = suspend_reg;
	
	
	//��������ӿ�
	reg [31:0]y1_data_t;
	reg [31:0]y2_data_t;
	reg [3:0] y1_channel_t;
	reg [1:0] y2_channel_t;
	
	//�жϽӿ�
	reg interrupt_t;
	reg [7:0]interrupt_num_t;
	
	//������������alu�Ľӿ�
	reg[31:0] alu_x1;
	reg[31:0] alu_x2;
	reg[2:0] alu_mode;
	reg alu_enable;
	wire[31:0] alu_y1;
	wire[31:0] alu_flag;
	wire alu_cplt;
	
	//������������alu
	ALU_int int_alu(
		clk,
		alu_enable,
		cpu_isStop,
		alu_mode,
		alu_x1,
		alu_x2,
		{alu_flag,alu_y1},
		alu_cplt
	);
	
	//����ת����ӿ�
	reg[31:0] itf_x;
	wire[31:0] itf_y;
	reg itf_mode;
	reg itf_enable;
	wire itf_cplt;
	ALU_intTranFloat itf(
		clk,
		itf_enable,
		cpu_isStop,
		itf_mode,
		itf_x,
		itf_y,
		itf_cplt
	);
	//����ת����ģ��ӿ�
	reg [31:0]fti_x;
	wire [31:0]fti_y;
	ALU_floatTranInt fti(
		fti_x,
		fti_y
	);
	//����Ӽ���ģ��ӿ�
	reg fas_mode;
	reg fas_enable;
	wire fas_cplt;
	reg[31:0] fas_x1;
	reg[31:0] fas_x2;
	wire[31:0]fas_y;
	FPU_AddSub fpu_as(
		clk,
		fas_enable,
		cpu_isStop,
		fas_mode,
		fas_x1,
		fas_x2,
		fas_y,
		fas_cplt
	);
	//����˷�ģ��ӿ�
	reg fmul_enable;
	wire fmul_cplt;
	reg[31:0] fmul_x1;
	reg[31:0] fmul_x2;
	wire[31:0]fmul_y;
	FPU_Mul fpu_mul(
		clk,
		fmul_enable,
		cpu_isStop,
		fmul_x1,
		fmul_x2,
		fmul_y,
		fmul_cplt
	);
	//�������ģ��ӿ�
	reg fdiv_enable;
	wire fdiv_cplt;
	reg[31:0] fdiv_x1;
	reg[31:0] fdiv_x2;
	wire[31:0]fdiv_y;
	FPU_Div fpu_div(
		clk,
		fdiv_enable,
		cpu_isStop,
		fdiv_x1,
		fdiv_x2,
		fdiv_y,
		fdiv_cplt
	);
	
	
	
	reg [63:0] rolr_tmp;//��λ�����ݴ�
	reg [31:0] bit_sopr_tmp1,bit_sopr_tmp2,bit_sopr_tmp3,bit_sopr_tmp4;//bitλ�����ݴ���·
	
	always@(*)begin
		//�ڽӿ�
		if((mode===8)&&subMode!=0)begin//����ģʽ
			ram_add_bus_t = (mode===8 && rw===1) ? x1-(1<<(subMode-1)) : x1;//���Ϊ��ջ��������ַΪ��ǰsp+д���ֽ�
			setData_t = x2;
			ram_rw_t = rw?3:2;
			ram_size_t = subMode;
		end
		else if((mode===7 || mode===16 || mode===17)&&subMode!=0)begin//��д�ڴ�
			ram_add_bus_t = x2;
			setData_t = x1;
			ram_rw_t = rw?3:2;
			ram_size_t = subMode;
		end
		else begin
			ram_add_bus_t = 0;
			setData_t = 0;
			ram_rw_t = 0;
			ram_size_t = 0;
		end
		
		//�жϽӿ�
		if((subMode===3 && x2===0)&&(mode===1 || mode===2 || mode===3))begin
			interrupt_t = 1;
			interrupt_num_t = 5;
		end
		else if(mode===5 &&(subMode===0 || subMode===2)& x2>32)begin
			interrupt_t = 1;
			interrupt_num_t = 6;
		end
		else if(mode===5 &&(subMode===1 || subMode===3)& x2>32)begin
			interrupt_t = 1;
			interrupt_num_t = 7;
		end
		else begin
			interrupt_t = 0;
			interrupt_num_t = 0;
		end
		
		//����ģʽ���Ӹ�alu�Ĳ�������ӿ�
		if(mode===1)begin
			alu_x1 = x1;
			alu_x2 = x2;
			alu_enable = (subMode===3 && x2===0)? 0 : 1;
			alu_mode = {1'b0,subMode};
		end
		else if(mode===2)begin
			alu_x1 = x1;
			alu_x2 = x2;
			alu_enable = (subMode===3 && x2===0)? 0 : 1;
			alu_mode = {1'b1,subMode};
		end
		else begin
			alu_x1 = 0;
			alu_x2 = 0;
			alu_enable = 0;
			alu_mode = 0;
		end
		
		if(mode===3&&(subMode===0||subMode===1))begin
			fas_mode = subMode;
			fas_enable = 1;
			fas_x1 = x1;
			fas_x2 = x2;
		end
		else begin
			fas_mode = 0;
			fas_enable = 0;
			fas_x1 = 0;
			fas_x2 = 0;
		end
		
		if(mode===3&&subMode===2)begin
			fmul_enable = 1;
			fmul_x1 = x1;
			fmul_x2 = x2;
		end
		else begin
			fmul_enable = 0;
			fmul_x1 = 0;
			fmul_x2 = 0;
		end
		
		if(mode===3&&subMode===3)begin
			fdiv_enable = 1;
			fdiv_x1 = x1;
			fdiv_x2 = x2;
		end
		else begin
			fdiv_enable = 0;
			fdiv_x1 = 0;
			fdiv_x2 = 0;
		end
		
		if(mode===4 && subMode===0)begin
			itf_x = x2;
			itf_mode = 0;
			itf_enable = 1;
		end
		else if(mode===4 && subMode===1)begin
			itf_x = x2;
			itf_mode = 1;
			itf_enable = 1;
		end
		else begin
			itf_x = 0;
			itf_mode = 0;
			itf_enable = 0;
		end
		
		if(mode===4 && subMode===2)begin
			fti_x = x2;
		end
		else begin
			fti_x = 0;
		end
		
		//����ģʽѡ��y1/y2/cplt�����
		case(mode)
		1:begin//�޷�����������
			y1_data_t = alu_y1;
			y2_data_t = alu_flag;
			suspend_reg = (subMode===3 && x2===0)? 0 : !alu_cplt;
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		2:begin//�з�����������
			y1_data_t = alu_y1;
			y2_data_t = alu_flag;
			suspend_reg = (subMode===3 && x2===0)? 0 : !alu_cplt;
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		3:begin//�����ȸ�������
			case(subMode)
			0:begin
				y1_data_t = fas_y;
				y2_data_t = 0;
				suspend_reg = !fas_cplt;
			end
			1:begin
				y1_data_t = fas_y;
				y2_data_t = 0;
				suspend_reg = !fas_cplt;
			end
			2:begin
				y1_data_t = fmul_y;
				y2_data_t = 0;
				suspend_reg = !fmul_cplt;
			end
			3:begin
				y1_data_t = fdiv_y;
				y2_data_t = 0;
				suspend_reg = !fdiv_cplt;
			end
			default:begin
				y1_data_t = 0;
				y2_data_t = 0;
				suspend_reg = 0;
			end
			endcase
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		4:begin//���ݸ�ʽת��
			case(subMode)
			0:begin//�޷�������ת����
				y1_data_t = itf_y;
				y2_data_t = 0;
				suspend_reg = !itf_cplt;
			end
			1:begin//�з�������ת����
				y1_data_t = itf_y;
				y2_data_t = 0;
				suspend_reg = !itf_cplt;
			end
			2:begin//����ת�з�������
				y1_data_t = fti_y;
				y2_data_t = 0;
				suspend_reg = 0;
				
			end
			3:begin//������ת����
				y1_data_t = ~(x2-1);
				y2_data_t = 0;
				suspend_reg = 0;
				
			end
			default:begin
				y1_data_t = 0;
				y2_data_t = 0;
				suspend_reg = 0;
			end
			endcase
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		5:begin//��λ����
			case(subMode)
			0:begin
				{y2_data_t,y1_data_t} = x1<<x2;
				
				rolr_tmp = 0;
			end
			1:begin
				y1_data_t = x1>>x2;
				y2_data_t = x1<<32-x2;
				
				rolr_tmp = 0;
			end
			2:begin
				//{y2_data_t,y1_data_t} = x1<<x2;
				rolr_tmp = x1<<x2;
				y2_data_t = rolr_tmp[63:32];
				y1_data_t = x1<<x2 | rolr_tmp>>32;
			end
			3:begin
				y2_data_t = x1<<32-x2;
				y1_data_t = x1>>x2 | y2_data_t;
				
				rolr_tmp = 0;
			end
			endcase
			suspend_reg = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		6:begin//λ����
			case(subMode)
			0:begin
				y1_data_t = x1 & x2;
				y2_data_t[0] = x1 && x2;
				y2_data_t[31:1] = 0;
			end
			1:begin
				
				y1_data_t = x1 | x2;
				y2_data_t[0] = x1 || x2;
				y2_data_t[31:1] = 0;
			end
			2:begin
				y1_data_t = ~x2;
				y2_data_t[0] = !x2;
				y2_data_t[31:1] = 0;
			end
			3:begin
				y1_data_t = x1^x2;
				y2_data_t[0] = (x1==0?0:1)^(x2==0?0:1);
				y2_data_t[31:1] = 0;
			end
			endcase
			suspend_reg = 0;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		7:begin//��д�ڴ�
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		16:begin//��ջ�ڴ�
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		17:begin//дջ�ڴ�
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		8:begin//�����ջ
			y1_data_t = ram_data_bus_read;
			case(rw)
			0:begin
				y2_data_t = x1+(1<<(subMode-1));
			end
			1:begin
				y2_data_t = x1-(1<<(subMode-1));
			end
			endcase
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		9:begin//����ת��
		    case(subMode)
			0:begin
			     y1_data_t = x2;
			end
			1:begin
			     y1_data_t = (x1<<16)|x2;
			end
			endcase
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
			
			y2_data_t = 0;
			suspend_reg =0;
		end
		18:begin
			case(subMode)
			0:begin
				if(x2[7])begin
					y1_data_t = {24'hffffff,x2[7:0]};
				end
				else begin
					y1_data_t = x2;
				end
				
			end
			1:begin
				if(x2[15])begin
					y1_data_t = {16'hffff,x2[15:0]};
				end
				else begin
					y1_data_t = x2;
				end
			end
			endcase
			
			y2_data_t = 0;
			suspend_reg = 0;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		19:begin//λ����
			case(subMode)
			0:begin//set
				bit_sopr_tmp1 = x1>>(m_num+1);
				bit_sopr_tmp2 = x1<<(32-l_num);
				bit_sopr_tmp3 = bit_sopr_tmp1<<(m_num+1) | bit_sopr_tmp2>>(32-l_num);
				bit_sopr_tmp4 = x2<<(31-m_num+l_num);
				
				y1_data_t = bit_sopr_tmp3 | bit_sopr_tmp4>>(31-m_num);
				
			end
			1:begin//get
				bit_sopr_tmp1 = x2<<(31-m_num);
				y1_data_t = bit_sopr_tmp1>>(31-m_num+l_num);
				
				bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
			end
			default:begin
				y1_data_t = 0;
				bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
			end
			endcase
			
			y2_data_t = 0;
			suspend_reg =0;
			rolr_tmp = 0;
		end
		20:begin
			y1_data_t = 0;
			suspend_reg = 0;
			case({rw,subMode})
			0:begin
				y2_data_t = x1===x2;
			end
			1:begin
				y2_data_t = x1!==x2;
			end
			2:begin
				y2_data_t = x1>x2;
			end
			3:begin
				y2_data_t = x1<x2;
			end
			4:begin
				y2_data_t = x1>=x2;
			end
			5:begin
				y2_data_t = x1<=x2;
			end
			default:begin
				y2_data_t = 0;
			end
			endcase
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		21:begin
			case({rw,subMode})
			0:begin
				y2_data_t = x1===x2;
			end
			1:begin
				y2_data_t = x1!==x2;
			end
			2:begin
				if(x1[31]&&x2[31])begin
					y2_data_t = x1<x2;
				end
				else if(x1[31])begin
					y2_data_t = 0;
				end
				else if(x2[31])begin
					y2_data_t = 1;
				end
				else begin
					y2_data_t = x1>x2;
				end
				
			end
			3:begin
				if(x1[31]&&x2[31])begin
					y2_data_t = x1>x2;
				end
				else if(x1[31])begin
					y2_data_t = 1;
				end
				else if(x2[31])begin
					y2_data_t = 0;
				end
				else begin
					y2_data_t = x1<x2;
				end
			end
			4:begin
				if(x1[31]&&x2[31])begin
					y2_data_t = x1<x2 || x1===x2;
				end
				else if(x1[31])begin
					y2_data_t = 0;
				end
				else if(x2[31])begin
					y2_data_t = 1;
				end
				else begin
					y2_data_t = x1>x2 || x1===x2;
				end
			end
			5:begin
				if(x1[31]&&x2[31])begin
					y2_data_t = x1<x2 || x1===x2;
				end
				else if(x1[31])begin
					y2_data_t = 1;
				end
				else if(x2[31])begin
					y2_data_t = 0;
				end
				else begin
					y2_data_t = x1>x2 || x1===x2;
				end
			end
			default:begin
				y2_data_t = 0;
			end
			endcase
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		22:begin
			case({rw,subMode})
			00:begin
				y2_data_t = x1===x2;
			end
			1:begin
				y2_data_t = x1!==x2;
			end
			2:begin
				if(x1===0 && x2===0)begin
					y2_data_t = 0;
				end
				else if(x1[31]&&x2[31])begin
					y2_data_t = x1[30:0]<x2[30:0];
				end
				else if(x1[31])begin
					y2_data_t = 0;
				end
				else if(x2[31])begin
					y2_data_t = 1;
				end
				else begin
					y2_data_t = x1[30:0]>x2[30:0];
				end
			end
			3:begin
				if(x1===0 && x2===0)begin
					y2_data_t = 0;
				end
				else if(x1[31]&&x2[31])begin
					y2_data_t = x1[30:0]>x2[30:0];
				end
				else if(x1[31])begin
					y2_data_t = 1;
				end
				else if(x2[31])begin
					y2_data_t = 0;
				end
				else begin
					y2_data_t = x1[30:0]<x2[30:0];
				end
			end
			4:begin
				if(x1===0 && x2===0)begin
					y2_data_t = 1;
				end
				else if(x1[31]&&x2[31])begin
					y2_data_t = x1[30:0]<x2[30:0] || x1[30:0]===x2[30:0];
				end
				else if(x1[31])begin
					y2_data_t = 0;
				end
				else if(x2[31])begin
					y2_data_t = 1;
				end
				else begin
					y2_data_t = x1[30:0]>x2[30:0] || x1[30:0]===x2[30:0];
				end
			end
			5:begin
				if(x1===0 && x2===0)begin
					y2_data_t = 1;
				end
				else if(x1[31]&&x2[31])begin
					y2_data_t = x1[30:0]>x2[30:0] || x1[30:0]===x2[30:0];
				end
				else if(x1[31])begin
					y2_data_t = 1;
				end
				else if(x2[31])begin
					y2_data_t = 0;
				end
				else begin
					y2_data_t = x1[30:0]<x2[30:0] || x1[30:0]===x2[30:0];
				end
			end
			default:begin
				y2_data_t = 0;
			end
			endcase
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		default:begin
			y1_data_t = 0;
			y2_data_t = 0;
			suspend_reg = 0;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		endcase
		
		//���ͨ��
		if((subMode===3 && x2===0)&&(mode===1 || mode===2 || mode===3)  ||  (mode===5 && x2>32))begin
			//���Ϊ����/��λ���������⣬���ͨ��ȫ��0
			y1_channel_t = 0;
			y2_channel_t = 0;
		end
		else if((mode>=1 && mode<=9)||mode===16||mode===17||mode===18||mode===19||mode===20||mode===21||mode===22)begin
			y1_channel_t = y1_channel_select;
			y2_channel_t = y2_channel_select;
		end
		else begin
			y1_channel_t = 0;
			y2_channel_t = 0;
		end
	end
	
	//������¼����̵ļĴ���
	reg [3:0]y1_channel_reg=0;
	reg [1:0]y2_channel_reg = 0;
	reg [31:0]y1_data_reg = 0;
	reg [31:0]y2_data_reg = 0;
	assign y1_channel = y1_channel_reg;
	assign y2_channel = y2_channel_reg;
	assign y1_data = y1_data_reg;
	assign y2_data = y2_data_reg;
	
	reg [31:0] nextOrderAddress_reg = 0;
	assign nextOrderAddress = nextOrderAddress_reg;
	reg next_isRunning_reg = 0;
	assign next_isRunning = next_isRunning_reg;
	
	reg interrupt_reg = 0;
	reg[7:0]interrupt_num_reg=0;
	assign next_interrupt = interrupt_reg;
	assign next_interrupt_num = interrupt_num_reg;
	
	reg next_isDepTPC_t = 0,next_isDepIPC_t = 0;
	reg next_isEffTPC_t = 0,next_isEffIPC_t = 0,next_isEffFlag_t = 0,next_isEffCS_t = 0;
	reg next_isFourCycle_t = 0;
	assign next_isDepTPC = next_isDepTPC_t;
	assign next_isDepIPC = next_isDepIPC_t;
	assign next_isEffTPC = next_isEffTPC_t;
	assign next_isEffIPC = next_isEffIPC_t;
	assign next_isEffFlag = next_isEffFlag_t;
	assign next_isEffCS = next_isEffCS_t;
	assign next_isFourCycle = next_isFourCycle_t;
	always@(posedge clk)begin
		if(rst)begin
			y1_data_reg <=0;
			y2_data_reg <=0;
			y1_channel_reg <= 0;
			y2_channel_reg <= 0;
			
			next_isRunning_reg<=0;
			interrupt_reg<=0;
			interrupt_num_reg<=0;
			
			next_isDepTPC_t<=0;
			next_isDepIPC_t<=0;
			next_isEffTPC_t<=0;
			next_isEffIPC_t<=0;
			next_isEffFlag_t<=0;
			next_isEffCS_t <= 0;
			next_isFourCycle_t<=0;
		end
		else if(!isStop)begin
			y1_data_reg <=y1_data_t;
			y2_data_reg <=y2_data_t;
			y1_channel_reg <= y1_channel_t;
			y2_channel_reg <= y2_channel_t;
			nextOrderAddress_reg <= thisOrderAddress;
			next_isRunning_reg<=this_isRunning;
			if(interrupt)begin
				interrupt_reg<=1;
				interrupt_num_reg<=interrupt_num;
			end
			else begin
				interrupt_reg<=interrupt_t;
				interrupt_num_reg<=interrupt_num_t;
			end
			
			
			
			next_isDepTPC_t<=isDepTPC;
			next_isDepIPC_t<=isDepIPC;
			next_isEffTPC_t<=isEffTPC;
			next_isEffIPC_t<=isEffIPC;
			next_isEffFlag_t<=isEffFlag;
			next_isEffCS_t <= isEffCS;
			next_isFourCycle_t<=isFourCycle;
		end
	end
endmodule

