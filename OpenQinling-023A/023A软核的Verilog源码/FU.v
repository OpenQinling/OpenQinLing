//浮点乘法运算[7个时钟]
module FPU_Mul(
		input clk,//时钟信号
		input enable,//启用浮点乘法
		input cpu_isStop,//cpu是否暂停中
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	
	wire [7:0]e = x1[30:23]+x2[30:23]-127;//计算出的指数位结果
	wire s = x1[31]^x2[31];//计算出的符号位
	
	//m运算参数缓存
	reg [23:0]x1_tmp;
	reg [23:0]x2_tmp;
	reg [47:0]y_tmp = 0;//m运算结果缓存
	
	reg cplt_reg = 0;//完成信号输出
	assign cplt = cplt_reg;
	
	reg [31:0]out_data = 0;
	assign y = out_data;
	
	reg[3:0]index = 0;//乘法运算步骤
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
			if(x1===0 || x2===0)begin//任意一个参数为0，直接输出0
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

//浮点除法运算[26个时钟]
module FPU_Div(
		input clk,//时钟信号
		input enable,//启用浮点乘法
		input cpu_isStop,//cpu是否暂停中
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	wire [7:0]e = x1[30:23]-x2[30:23]+127;//计算出的指数位结果
	wire s = x1[31]^x2[31];//计算出的符号位
	
	//m运算参数缓存
	reg [47:0]x1_tmp = 0;
	reg [47:0]x2_tmp = 0;
	reg [47:0]y_tmp = 0;//m运算结果缓存
	
	reg cplt_reg = 0;//完成信号输出
	assign cplt = cplt_reg;
	
	reg [31:0]out_data = 0;
	assign y = out_data;
	reg[4:0]index = 0;//除法运算步骤
	
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
			if(x1===0 || x2===0)begin//任意一个参数为0，直接输出0
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
			//除法运算逻辑
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

//浮点加减运算[需5个时钟周期]
module FPU_AddSub(
		input clk,//时钟信号
		input enable,//启用浮点加法
		input cpu_isStop,//cpu是否暂停中
		input mode,//0为加法,1为减法
		input[31:0]x1,x2,
		output[31:0]y,
		output cplt
	);
	wire [7:0] z_t = (x1[30:23] - x2[30:23]);//x1/x2的指数差值
	wire [7:0] z = z_t[7] ? ~(z_t-1) : z_t;//x1/x2指数差的绝对值
	//获取到的两个待计算参数
	wire [25:0]x1_reg = z_t[7] ? {1'b1,x1[22:0]}>>(z) : {1'b1,x1[22:0]};
	wire [25:0]x2_reg = z_t[7] ? {1'b1,x2[22:0]} : {1'b1,x2[22:0]}>>(z);
	
	
	reg [2:0]index = 0;//运算进度索引号
	reg cplt_reg = 0;//完成信号输出
	assign cplt = cplt_reg;
	
	reg [25:0] arg_x1 = 0,arg_x2 = 0;//计算参数值
	reg [7:0] arg_z = 0;//计算的指数
	reg [25:0] arg_y = 0;//计算出的m结果
	
	reg [31:0]y_reg = 0;
	assign y = y_reg;
	
	reg [7:0]shifting = 0;//运算后的索引偏移值
	
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
		else if(index===0 && enable)begin//获取计算参数
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
		else if(index===1)begin//计算数据
			index <= index+1;
			arg_y <= {arg_y_t[25],arg_y_t[25] ? ~(arg_y_t[24:0]-1) : arg_y_t[24:0]};
		end
		else if(index===2)begin
			if(arg_y[24])begin
				shifting<=8'b11111111;//负1，往左偏移
				index<=5;
			end
			else if(arg_y[23])begin
				shifting<=0;//不进行偏移
				index<=5;
			end
			else if(arg_y[22])begin
				shifting<=1;//往右偏移
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
				shifting<=8;//不进行偏移
				index<=5;
			end
			else if(arg_y[14])begin
				shifting<=9;//往右偏移
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




//浮点转整数
module ALU_floatTranInt(
		input[31:0]x,
		output[31:0]y
	);
	wire [4:0]index = x[30:23]-127;//获取浮点指数值
	
	wire [23:0] o_tmp1 = {1'b1,x[22:0]};
	
	//获取浮点转换出的绝对值
	wire [31:0] o_tmp2 = index>23 ? o_tmp1<<(index-23) : o_tmp1>>(23-index);
	
	assign y = x[30:23]>127 ? (x[31] ? ~(o_tmp2-1) : o_tmp2) : 0;//如果标志位为1，绝对值取反为-
	
endmodule

//整数转浮点
module ALU_intTranFloat(
		input clk,
		input enable,
		input cpu_isStop,//cpu是否暂停中
		input mode,//0为无符号，1为有符号
		input[31:0]x1,
		output[31:0]y,
		output cplt//完成信号
	);
	reg [31:0]num;//要转换的数的绝对值
	reg sign;//要转换的数的正负[0为正]
	reg [4:0] inum;//num最高位索引地址
	//结果输出
	reg [31:0]y_reg = 0;
	assign y = y_reg;
	//是否完成
	reg isCplt = 0;
	assign cplt = isCplt;
	
	reg[2:0]index = 0;//转换进度
	/*
		0:将负数转换为整数，获取sign值和num值
		1:排查num的高16位是否存在最高位(存在的话直接跳到第3步)
		2:获取num的低16位是否存在最高位
		3:截取num的有效部分/指数部分给到浮点格式对应位
	*/
	
	
	reg [8:0]p_getIndex;//高16位的排查进度
	reg [3:0]p_index;//查询循环索引号
	reg [2:0]p_inum;
	reg[7:0]p_num = 0;
	always@(*)begin
		p_getIndex[8] = 0;
		for(p_index=8;p_index>0;p_index=p_index-1)begin
			p_getIndex[p_index-1] = p_getIndex[p_index] ? 1 : p_num[p_index-1];
			//如果上一个p1_getIndex为1，说明已经查到。该p1_getIndex置1。
			//否则，将num对应位给到p1_getIndex，如果num当前位为1，说明就查到了num的最高位
			
			if(!p_getIndex[p_index] && p_num[p_index-1])begin
				//假如查到了num的最高位。就将值给到p1_inum
				p_inum = p_index-1;
			end
		end
	end
	
	wire[31:0]getNum = (mode&&x1[31]) ? (~x1)+1 : x1;
	
	reg[3:0]sa_index;
	always@(posedge clk)begin
		if((index!=0 && !enable)||(index==6 && !cpu_isStop))begin//重置
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
				//如果未能获取到num最高位索引,开始遍历num低16位
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





//整数四则运算alu
module ALU_int(
		input clk,
		input enable,//启用alu
		input cpu_isStop,//cpu是否暂停中
		input[2:0]mode,
		//0无符号加法,1无符号减法,2无符号乘法,3无符号除法
		//4有符号加法,5有符号减法,6有符号乘法,7有符号除法
		input[31:0]x1,x2,
		output[63:0]y,
		output cplt//完成信号反馈
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
	
	always@(*)begin//选择输入进mul运算模块的信号
		
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
	
	always@(*)begin//选择输入进div运算模块的信号
		
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
	
	always@(*)begin//选择运算结果/完成信号输出通道
		if(enable)begin
			case(mode)
			0:begin//无符号加法
				y_reg = x1+x2;
				cplt_reg = 1;
			end
			4:begin//有符号加法
				y_reg = {(x1[31] ? 32'hffffffff : 0),x1} + {(x2[31] ? 32'hffffffff : 0),x2};
				cplt_reg = 1;
			end
			1:begin//无符号减法
				y_reg = x1-x2;
				cplt_reg = 1;
			end
			5:begin//有符号减法
				y_reg = {(x1[31] ? 32'hffffffff : 0),x1} - {(x2[31] ? 32'hffffffff : 0),x2};
				cplt_reg = 1;
			end
			2:begin//无符号乘法
				y_reg = mul_y;
				cplt_reg =mul_cplt;
			end
			6:begin//有符号乘法
				y_reg = mul_y;
				cplt_reg =mul_cplt;
			end
			3:begin//无符号除法
				y_reg = {div_r,div_y};
				cplt_reg =div_cplt;
			end
			7:begin//有符号除法
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


//多时钟周期乘法器[无符号运算一次10个时钟，有符号需18个]
module ALU_mul(
		input clk,
		input enable,
		input cpu_isStop,//cpu是否暂停中
		input mode,
		input[31:0]x1,x2,
		output[63:0]y,
		output cplt
	);
	
	reg [6:0]index =0;//当前乘法运算的步骤
	
	//运算参数[如果为有符号模式，且为负数，则高32位补1]
	wire [63:0]x1_data_t = {mode? (x1[31]?32'hffffffff:0) :0,x1};
	wire [63:0]x2_data_t = {mode? (x2[31]?32'hffffffff:0) :0,x2};
	
	reg [63:0]out_data_cache = 0;//结果缓存
	assign y = out_data_cache;
	
	
	reg cplt_reg = 0;//完成信号输出
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

//多时钟周期除法器[运算一次32个时钟]
module ALU_div(
		input clk,
		input enable,
		input cpu_isStop,//cpu是否暂停中
		input mode,//0为无符号除法,1为有符号
		input[31:0]x1,x2,
		output[31:0]y,
		output[31:0]r,//y为得数,r为余数
		output cplt
	);
	
	
	reg [32:0] y_reg = 0;//结果寄存器
	reg [31:0] x1_reg = 0;//被除数寄存器[运算完毕后该寄存器存储余数]
	reg [62:0] x2_reg = 0;//除数寄存器
	reg y_sign = 0;//如果为有符号除法，保存计算结果应当是正数还是负数(1为负数)
	reg r_sign = 0;//如果为有符号除法，保存计算出的模应当是正数还是负数
	reg [5:0]index =0;//当前除法运算的步骤
	
	reg cplt_reg = 0;//完成信号寄存器
	
	
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
				y_sign <= x1[31] ^ x2[31];//如果x1/x2正负不同，输出才为负数
				r_sign <= x1[31];
				x1_reg <= x1[31]!=1 ? x1 : ~(x1-1);//x1如果为1，转为正数
				x2_reg <= x2[31]!=1 ? x2<<31 : (~(x2-1))<<31;//x2需要先左移31位后开始除法运算
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
