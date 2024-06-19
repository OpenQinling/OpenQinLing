//����˷�����[7��ʱ��]
module FPU_Mul(
		input clk,//ʱ���ź�
		input enable,//���ø���˷�
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	
	wire [7:0]e = x1[30:23]+x2[30:23]-127;//�������ָ��λ���
	wire s = x1[31]^x2[31];//������ķ���λ
	
	//m�����������
	reg [23:0]x1_tmp;
	reg [23:0]x2_tmp;
	reg [47:0]y_tmp = 0;//m����������
	
	reg cplt_reg = 0;//����ź����
	assign cplt = cplt_reg;
	
	reg [31:0]out_data = 0;
	assign y = out_data;
	
	reg[3:0]index = 0;//�˷����㲽��
	wire[4:0]mul_index = (index-1)<<2;
	always@(posedge clk)begin
		if((index!=0 && !enable) || (index==8 && !cpu_isStop))begin
			index <= 0;
			x1_tmp<=0;
			x2_tmp<=0;
			y_tmp <= 0;
			out_data <= 0;
			cplt_reg <= 0;
		end
		else if(index===0 && enable)begin
			if(x1===0 || x2===0)begin//����һ������Ϊ0��ֱ�����0
				index<=0;
				cplt_reg <= 1;
				out_data <= 0;
			end
			else begin
				x1_tmp <= {1'b1,x1[22:0]};
				x2_tmp <= {1'b1,x2[22:0]};
				index <= index+1;
				cplt_reg <= 0;
			end
		end
		else if(index>0 && index<=6)begin
			y_tmp <= y_tmp + (x2_tmp[mul_index] ? x1_tmp<<mul_index :0) + (x2_tmp[mul_index+1] ? x1_tmp<<(mul_index+1) :0) + (x2_tmp[mul_index+2] ? x1_tmp<<(mul_index+2) :0) + (x2_tmp[mul_index+3] ? x1_tmp<<(mul_index+3) :0);
			index <= index+1;
		end
		else if(index===7)begin
			out_data[31] <= s;
			out_data[30:23] <= y_tmp[47] ? e+1 : e;
			out_data[22:0] <= y_tmp[47] ? y_tmp[46:24] : y_tmp[45:23];
			index <= 8;
			cplt_reg <= 1;
		end
	end
	
endmodule

//�����������[26��ʱ��]
module FPU_Div(
		input clk,//ʱ���ź�
		input enable,//���ø���˷�
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	wire [7:0]e = x1[30:23]-x2[30:23]+127;//�������ָ��λ���
	wire s = x1[31]^x2[31];//������ķ���λ
	
	//m�����������
	reg [47:0]x1_tmp = 0;
	reg [47:0]x2_tmp = 0;
	reg [47:0]y_tmp = 0;//m����������
	
	reg cplt_reg = 0;//����ź����
	assign cplt = cplt_reg;
	
	reg [31:0]out_data = 0;
	assign y = out_data;
	reg[4:0]index = 0;//�������㲽��
	
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==27 && !cpu_isStop))begin
			x1_tmp <= 0;
			x2_tmp <= 0;
			y_tmp <= 0;
			cplt_reg <= 0;
			out_data <= 0;
			index <= 0;
		end
		else if(index===0 && enable)begin
			if(x1===0 || x2===0)begin//����һ������Ϊ0��ֱ�����0
				index<=0;
				cplt_reg <= 1;
				out_data <= 0;
			end
			else begin
				x1_tmp <= {1'b1,x1[22:0],24'd0};
				x2_tmp <= {1'b1,x2[22:0],24'd0};
				index <= index+1;
				y_tmp <= 0;
				cplt_reg <= 0;
			end
		end
		else if(index>0 && index<=25)begin
			//���������߼�
			x2_tmp <= x2_tmp>>1;
			if(x2_tmp<=x1_tmp)begin
				x1_tmp <= x1_tmp-x2_tmp;
				y_tmp[25-index] <= 1;
			end
			index <= index+1;
		end
		else if(index===26)begin
			out_data[31] <= s;
			out_data[30:23] <= y_tmp[24] ? e : e-1;
			out_data[22:0] <= y_tmp[24] ? y_tmp[23:1] : y_tmp[22:0];
			index <= 27;
			cplt_reg <= 1;
		end
		
	end
endmodule

//����Ӽ�����[��5��ʱ������]
module FPU_AddSub(
		input clk,//ʱ���ź�
		input enable,//���ø���ӷ�
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input mode,//0Ϊ�ӷ�,1Ϊ����
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	wire [7:0] z_t = (x1[30:23] - x2[30:23]);//x1/x2��ָ����ֵ
	wire [7:0] z = z_t[7] ? ~(z_t-1) : z_t;//x1/x2ָ����ľ���ֵ
	//��ȡ�����������������
	wire [25:0]x1_reg = z_t[7] ? {1'b1,x1[22:0]}>>(z) : {1'b1,x1[22:0]};
	wire [25:0]x2_reg = z_t[7] ? {1'b1,x2[22:0]} : {1'b1,x2[22:0]}>>(z);
	
	
	reg [2:0]index = 0;//�������������
	reg cplt_reg = 0;//����ź����
	assign cplt = cplt_reg;
	
	reg [25:0] arg_x1 = 0,arg_x2 = 0;//�������ֵ
	reg [7:0] arg_z = 0;//�����ָ��
	reg [25:0] arg_y = 0;//�������m���
	
	reg [31:0]y_reg = 0;
	assign y = y_reg;
	
	reg [7:0]shifting = 0;//����������ƫ��ֵ
	
	wire[25:0]arg_y_t = (x1[31] ? ~(arg_x1-1) : arg_x1) + ( (mode^x2[31]) ? ~(arg_x2-1) : arg_x2);
	wire[25:0]arg_y_t2 = (shifting===8'b11111111 || shifting===8'b11111110) ? arg_y>>1 : arg_y<<shifting;
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==6 && !cpu_isStop))begin
			index <= 0;
			cplt_reg <= 0;
			arg_x1 <= 0;
			arg_x2 <= 0;
			arg_z <= 0;
			arg_y <= 0;
			y_reg <= 0;
		end
		else if(index===0 && enable)begin//��ȡ�������
			index <= index+1;
			arg_x1 <= x1_reg;
			arg_x2 <= x2_reg;
			arg_z <= z;
			
			if(x1===0 && x2===0)begin
				y_reg <= 0;
				cplt_reg <= 1;
			end
			else if(x2===0)begin
				y_reg <= x1;
				cplt_reg <= 1;
			end
			else if(x1===0)begin
				y_reg <= mode ? {!x2[31],x2[30:0]} : x2;
				cplt_reg <= 1;
			end
		end
		else if(index===1)begin//��������
			index <= index+1;
			arg_y <= {arg_y_t[25],arg_y_t[25] ? ~(arg_y_t[24:0]-1) : arg_y_t[24:0]};
		end
		else if(index===2)begin
			if(arg_y[24])begin
				shifting<=8'b11111111;//��1������ƫ��
				index<=5;
			end
			else if(arg_y[23])begin
				shifting<=0;//������ƫ��
				index<=5;
			end
			else if(arg_y[22])begin
				shifting<=1;//����ƫ��
				index<=5;
			end
			else if(arg_y[21])begin
				shifting<=2;
				index<=5;
			end
			else if(arg_y[20])begin
				shifting<=3;
				index<=5;
			end
			else if(arg_y[19])begin
				shifting<=4;
				index<=5;
			end
			else if(arg_y[18])begin
				shifting<=5;
				index<=5;
			end
			else if(arg_y[17])begin
				shifting<=6;
				index<=5;
			end
			else if(arg_y[16])begin
				shifting<=7;
				index<=5;
			end
			else begin
				index<=index+1;
			end
		end
		else if(index===3)begin
			if(arg_y[15])begin
				shifting<=8;//������ƫ��
				index<=5;
			end
			else if(arg_y[14])begin
				shifting<=9;//����ƫ��
				index<=5;
			end
			else if(arg_y[13])begin
				shifting<=10;
				index<=5;
			end
			else if(arg_y[12])begin
				shifting<=11;
				index<=5;
			end
			else if(arg_y[11])begin
				shifting<=12;
				index<=5;
			end
			else if(arg_y[10])begin
				shifting<=13;
				index<=5;
			end
			else if(arg_y[9])begin
				shifting<=14;
				index<=5;
			end
			else if(arg_y[8])begin
				shifting<=15;
				index<=5;
			end
			else begin
				index<=index+1;
			end
		end
		else if(index===4)begin
			if(arg_y[7])begin
				shifting<=16;
			end
			else if(arg_y[6])begin
				shifting<=17;
			end
			else if(arg_y[5])begin
				shifting<=18;
			end
			else if(arg_y[4])begin
				shifting<=19;
			end
			else if(arg_y[3])begin
				shifting<=20;
			end
			else if(arg_y[2])begin
				shifting<=21;
			end
			else if(arg_y[1])begin
				shifting<=22;
			end
			else if(arg_y[0])begin
				shifting<=23;
			end
			else begin
				shifting<=8'b11111110;
			end
			index<=index+1;
		end
		else if(index===5)begin
			index<=6;
			cplt_reg <= 1;
			if(shifting===8'b11111110)begin
				y_reg <= 0;
			end
			else begin
				y_reg[31] <= arg_y_t[25];
				y_reg[30:23] <= (z_t[7]? x2[30:23] : x1[30:23])- shifting;
				y_reg[22:0] <= arg_y_t2[22:0];
			end
		end
	end
	
endmodule




//����ת����
module ALU_floatTranInt(
		input[31:0]x,
		output[31:0]y
	);
	wire [4:0]index = x[30:23]-127;//��ȡ����ָ��ֵ
	
	wire [23:0] o_tmp1 = {1'b1,x[22:0]};
	
	//��ȡ����ת�����ľ���ֵ
	wire [31:0] o_tmp2 = index>23 ? o_tmp1<<(index-23) : o_tmp1>>(23-index);
	
	assign y = x[30:23]>127 ? (x[31] ? ~(o_tmp2-1) : o_tmp2) : 0;//�����־λΪ1������ֵȡ��Ϊ-
	
endmodule

//����ת����
module ALU_intTranFloat(
		input clk,
		input enable,
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input mode,//0Ϊ�޷��ţ�1Ϊ�з���
		input[31:0]x1,
		output[31:0]y,
		output cplt//����ź�
	);
	reg [31:0]num;//Ҫת�������ľ���ֵ
	reg sign;//Ҫת������������[0Ϊ��]
	reg [4:0] inum;//num���λ������ַ
	//������
	reg [31:0]y_reg = 0;
	assign y = y_reg;
	//�Ƿ����
	reg isCplt = 0;
	assign cplt = isCplt;
	
	reg[2:0]index = 0;//ת������
	/*
		0:������ת��Ϊ��������ȡsignֵ��numֵ
		1:�Ų�num�ĸ�16λ�Ƿ�������λ(���ڵĻ�ֱ��������3��)
		2:��ȡnum�ĵ�16λ�Ƿ�������λ
		3:��ȡnum����Ч����/ָ�����ָ��������ʽ��Ӧλ
	*/
	
	
	reg [8:0]p_getIndex;//��16λ���Ų����
	reg [3:0]p_index;//��ѯѭ��������
	reg [2:0]p_inum;
	reg[7:0]p_num = 0;
	always@(*)begin
		p_getIndex[8] = 0;
		for(p_index=8;p_index>0;p_index=p_index-1)begin
			p_getIndex[p_index-1] = p_getIndex[p_index] ? 1 : p_num[p_index-1];
			//�����һ��p1_getIndexΪ1��˵���Ѿ��鵽����p1_getIndex��1��
			//���򣬽�num��Ӧλ����p1_getIndex�����num��ǰλΪ1��˵���Ͳ鵽��num�����λ
			
			if(!p_getIndex[p_index] && p_num[p_index-1])begin
				//����鵽��num�����λ���ͽ�ֵ����p1_inum
				p_inum = p_index-1;
			end
		end
	end
	
	wire[31:0]getNum = (mode&&x1[31]) ? (~x1)+1 : x1;
	
	reg[3:0]sa_index;
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==6 && !cpu_isStop))begin//����
			num <= 0;
			sign <= 0;
			p_num <= 0;
			index <= 0;
			isCplt <= 0;
		end
		else if(index===0 && enable)begin
			num <= getNum;
			sign <= mode ? x1[31] : 0;
			p_num <= getNum[31:24];
			index <= index+1;
			isCplt <= 0;
		end
		else if(index>=1 && index<=4)begin
			if(p_getIndex==0)begin
				//���δ�ܻ�ȡ��num���λ����,��ʼ����num��16λ
				index <= index+1;
				if(index===1)begin
					p_num <= getNum[23:16];
				end
				else if(index===2)begin
					p_num <= getNum[15:8];
				end
				else if(index===3)begin
					p_num <= getNum[7:0];
				end
			end
			else begin
				index <= 5;
				if(index===1)begin
					inum <= p_inum+24;
				end
				else if(index===2)begin
					inum <= p_inum+16;
				end
				else if(index===3)begin
					inum <= p_inum+8;
				end
				else if(index===4)begin
					inum <= p_inum;
				end
			end
		end
		else if(index===5)begin
			index <= 6;
			isCplt <= 1;
			if(num===0)begin
				y_reg <= 0;
			end
			else begin
				y_reg[31] <= sign;
				y_reg[30:23] <= inum+127;
				y_reg[22:0] <= inum>23 ? num>>(inum-23) : num<<(23-inum);
			end
		end
	
	end
endmodule





//������������alu
module ALU_int(
		input clk,
		input enable,//����alu
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input[2:0]mode,
		//0�޷��żӷ�,1�޷��ż���,2�޷��ų˷�,3�޷��ų���
		//4�з��żӷ�,5�з��ż���,6�з��ų˷�,7�з��ų���
		input[31:0]x1,x2,
		output[63:0]y,
		output cplt//����źŷ���
	);
	
	
	reg [63:0] y_reg;
	assign y = y_reg;
	reg cplt_reg;
	assign cplt = cplt_reg;
	
	
	reg mul_enable;
	reg mul_mode;
	wire [63:0]mul_y;
	wire mul_cplt;
	ALU_mul mul_dev(
		clk,
		mul_enable,
		cpu_isStop,
		mul_mode,
		x1,x2,
		mul_y,
		mul_cplt
	);
	
	reg div_enable;
	reg div_mode;
	wire [31:0]div_y,div_r;
	wire div_cplt;
	ALU_div div_dev(
		clk,
		div_enable,
		cpu_isStop,
		div_mode,
		x1,x2,
		div_y,div_r,
		div_cplt
	);
	
	always@(*)begin//ѡ�������mul����ģ����ź�
		
		if(mode===2 && enable)begin
			
			mul_enable = 1;
			mul_mode = 0;
		end
		else if(mode===6 && enable)begin
			mul_enable = 1;
			mul_mode = 1;
		end
		else begin
			mul_enable = 0;
			mul_mode = 0;
		end
	end
	
	always@(*)begin//ѡ�������div����ģ����ź�
		
		if(mode===3 && enable)begin
			div_enable = 1;
			div_mode = 0;
		end
		else if(mode===7 && enable)begin
			div_enable = 1;
			div_mode = 1;
		end
		else begin
			div_enable = 0;
			div_mode = 0;
		end
	end
	
	always@(*)begin//ѡ��������/����ź����ͨ��
		if(enable)begin
			case(mode)
			0:begin//�޷��żӷ�
				y_reg = x1+x2;
				cplt_reg = 1;
			end
			4:begin//�з��żӷ�
				y_reg = {(x1[31] ? 32'hffffffff : 0),x1} + {(x2[31] ? 32'hffffffff : 0),x2};
				cplt_reg = 1;
			end
			1:begin//�޷��ż���
				y_reg = x1-x2;
				cplt_reg = 1;
			end
			5:begin//�з��ż���
				y_reg = {(x1[31] ? 32'hffffffff : 0),x1} - {(x2[31] ? 32'hffffffff : 0),x2};
				cplt_reg = 1;
			end
			2:begin//�޷��ų˷�
				y_reg = mul_y;
				cplt_reg =mul_cplt;
			end
			6:begin//�з��ų˷�
				y_reg = mul_y;
				cplt_reg =mul_cplt;
			end
			3:begin//�޷��ų���
				y_reg = {div_r,div_y};
				cplt_reg =div_cplt;
			end
			7:begin//�з��ų���
				y_reg = {div_r,div_y};
				cplt_reg =div_cplt;
			end
			endcase
		end
		else begin
			y_reg = 0;
			cplt_reg = 1;
		end
		
	end
endmodule


//��ʱ�����ڳ˷���[�޷�������һ��10��ʱ�ӣ��з�����18��]
module ALU_mul(
		input clk,
		input enable,
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input mode,
		input[31:0]x1,x2,
		output[63:0]y,
		output cplt
	);
	
	reg [6:0]index =0;//��ǰ�˷�����Ĳ���
	
	//�������[���Ϊ�з���ģʽ����Ϊ���������32λ��1]
	wire [63:0]x1_data_t = {mode? (x1[31]?32'hffffffff:0) :0,x1};
	wire [63:0]x2_data_t = {mode? (x2[31]?32'hffffffff:0) :0,x2};
	
	reg [63:0]out_data_cache = 0;//�������
	assign y = out_data_cache;
	
	
	reg cplt_reg = 0;//����ź����
	assign cplt = cplt_reg;
	
	reg [63:0]x1_data = 0;
	reg [63:0]x2_data = 0;
	
	
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==68 && !cpu_isStop))begin
			index <= 0;
			out_data_cache <= 0;
			cplt_reg <= 0;
			x2_data<=0;
			x1_data<=0;
		end
		else if(index===0 && enable)begin
			out_data_cache <= 0;
			index <= index+4;
			cplt_reg <= 0;
			x2_data<=x2_data_t;
			x1_data<=x1_data_t;
		end
		else if(index>0 && index<(mode?68:36))begin
			out_data_cache <= out_data_cache + (x2_data[index-4] ? x1_data<<(index-4) :0) + (x2_data[index-3] ? x1_data<<(index-3) :0) + (x2_data[index-2] ? x1_data<<(index-2) :0) + (x2_data[index-1] ? x1_data<<(index-1) :0);
			
			if(index===(mode?64:32))begin
				cplt_reg <= 1;
				index <= 68;
			end
			else begin
				index <= index+4;
			end
		end
	end
endmodule

//��ʱ�����ڳ�����[����һ��32��ʱ��]
module ALU_div(
		input clk,
		input enable,
		input cpu_isStop,//cpu�Ƿ���ͣ��
		input mode,//0Ϊ�޷��ų���,1Ϊ�з���
		input[31:0]x1,x2,
		output[31:0]y,
		output[31:0]r,//yΪ����,rΪ����
		output cplt
	);
	
	
	reg [32:0] y_reg = 0;//����Ĵ���
	reg [31:0] x1_reg = 0;//�������Ĵ���[������Ϻ�üĴ����洢����]
	reg [62:0] x2_reg = 0;//�����Ĵ���
	reg y_sign = 0;//���Ϊ�з��ų��������������Ӧ�����������Ǹ���(1Ϊ����)
	reg r_sign = 0;//���Ϊ�з��ų���������������ģӦ�����������Ǹ���
	reg [5:0]index =0;//��ǰ��������Ĳ���
	
	reg cplt_reg = 0;//����źżĴ���
	
	
	assign y = y_sign ? (~y_reg)+1:y_reg;
	assign r = r_sign ? (~x1_reg)+1:x1_reg;
	assign cplt = cplt_reg;
	
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==33 && !cpu_isStop))begin
			y_reg<=0;
			x1_reg<=0;
			x2_reg<=0;
			index<=0;
			cplt_reg <= 0;
		end
		else if(index===0 && enable)begin  
			if(mode)begin
				y_sign <= x1[31] ^ x2[31];//���x1/x2������ͬ�������Ϊ����
				r_sign <= x1[31];
				x1_reg <= x1[31]!=1 ? x1 : ~(x1-1);//x1���Ϊ1��תΪ����
				x2_reg <= x2[31]!=1 ? x2<<31 : (~(x2-1))<<31;//x2��Ҫ������31λ��ʼ��������
			end
			else begin
				y_sign <= 0;
				r_sign <= 0;
				x1_reg <= x1;
				x2_reg <= x2<<31;
			end
			y_reg <= 0;
			cplt_reg<= 0;
			index<=index+1;
		end
		else if(index>0 && index<33)begin
			x2_reg <= x2_reg>>1;
			if(x2_reg<=x1_reg)begin
				x1_reg <= x1_reg-x2_reg;
				y_reg[32-index] <= 1;
			end
			
			if(index===32)begin
				cplt_reg <= 1;
				index <= 33;
			end
			else begin
				index<= index+1;
			end
		end
	end
	
endmodule
