//执行模块
module ExecuteModule(
		//上级流水线的参数传入
		input [4:0]mode,//主模�?
		input rw,//内存读写的方向控�?
		input [1:0]subMode,//子模式�?�内存读写的字节控制
		input [31:0]x1,x2,//参数
		input [4:0]m_num,l_num,//位操作指令的位限�?
		input [3:0]y1_channel_select,//y1的�?�道
		input [1:0]y2_channel_select,//y2的�?�道
		input cpu_isStop,//cpu是否暂停�?
		
		//cpuRAM读写接口
		output[31:0]ram_data_bus_write,
		input [31:0]ram_data_bus_read,
		output[31:0]ram_add_bus,
		output[1:0]ram_size,
		output[1:0]ram_rw,//00为不读不写，10为读�?11为写[ram_rw[0]下降沿写入]
		input isCplt,//内存设备响应信号
		
		input clk,//时钟信号
		input isStop,//当前模块的暂停信�?
		input rst,//重启信号
		
		output suspend,//请求时钟管理器暂停的信号
		
		output[3:0]y1_channel,//y1通道选择输出
		output[1:0]y2_channel,//y2通道是否启用[0为不启用�?1为flag,2为sp]
		output[31:0]y1_data,//y1数据输出
		output[31:0]y2_data,//只有在运算模式下才有效，直�?�flag寄存�?
		
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num,
		
		//指令类型、指令依赖�?�指令影响信�?
		input isDepTPC,isDepIPC,
		input isEffTPC,isEffIPC,isEffFlag,isEffCS,
		input isFourCycle,//是否�?4周期类型的指令�?�是�?1
		output next_isDepTPC,next_isDepIPC,
		output next_isEffTPC,next_isEffIPC,next_isEffFlag,next_isEffCS,
		output next_isFourCycle
	);
	
	//内存操作接口
	reg [31:0] setData_t;
	reg [31:0]ram_add_bus_t;
	reg [1:0]ram_rw_t;
	reg [1:0]ram_size_t;
	assign ram_data_bus_write = setData_t;
	assign ram_add_bus = ram_add_bus_t;
	assign ram_rw = ram_rw_t;
	assign ram_size = ram_size_t;
	
	//暂停请求接口
	reg suspend_reg;
	assign suspend = suspend_reg;
	
	
	//数据输出接口
	reg [31:0]y1_data_t;
	reg [31:0]y2_data_t;
	reg [3:0] y1_channel_t;
	reg [1:0] y2_channel_t;
	
	//中断接口
	reg interrupt_t;
	reg [7:0]interrupt_num_t;
	
	//整数四则运算alu的接�?
	reg[31:0] alu_x1;
	reg[31:0] alu_x2;
	reg[2:0] alu_mode;
	reg alu_enable;
	wire[31:0] alu_y1;
	wire[31:0] alu_flag;
	wire alu_cplt;
	
	//整数四则运算alu
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
	
	//整数转浮点接�?
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
	//浮点转整数模块接�?
	reg [31:0]fti_x;
	wire [31:0]fti_y;
	ALU_floatTranInt fti(
		fti_x,
		fti_y
	);
	//浮点加减法模块接�?
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
	//浮点乘法模块接口
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
	//浮点除法模块接口
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
	
	
	
	reg [63:0] rolr_tmp;//移位运算暂存
	reg [31:0] bit_sopr_tmp1,bit_sopr_tmp2,bit_sopr_tmp3,bit_sopr_tmp4;//bit位操作暂存线�
	
	always@(*)begin
		//内接�?
		if((mode===8)&&subMode!=0)begin//出入�?
			ram_add_bus_t = (mode===8 && rw===1) ? x1-(1<<(subMode-1)) : x1;//如果为入栈操作，地址�?为当前sp+写入字节�?
			setData_t = x2;
			ram_rw_t = rw?3:2;
			ram_size_t = subMode;
		end
		else if((mode===7 || mode===16 || mode===17)&&subMode!=0)begin//读写内存
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
		
		//中断接口
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
		
		//根据模式连接各alu的参数输入接�?
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
		
		//根据模式选定y1/y2/cplt的输出�??
		case(mode)
		1:begin//无符号整数运�?
			y1_data_t = alu_y1;
			y2_data_t = alu_flag;
			suspend_reg = (subMode===3 && x2===0)? 0 : !alu_cplt;
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		2:begin//有符号整数运�?
			y1_data_t = alu_y1;
			y2_data_t = alu_flag;
			suspend_reg = (subMode===3 && x2===0)? 0 : !alu_cplt;
			
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		3:begin//单精度浮点运�?
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
		4:begin//数据格式转换
			case(subMode)
			0:begin//无符号整数转浮点
				y1_data_t = itf_y;
				y2_data_t = 0;
				suspend_reg = !itf_cplt;
			end
			1:begin//有符号整数转浮点
				y1_data_t = itf_y;
				y2_data_t = 0;
				suspend_reg = !itf_cplt;
			end
			2:begin//浮点转有符号整数
				y1_data_t = fti_y;
				y2_data_t = 0;
				suspend_reg = 0;
				
			end
			3:begin//正负数转�?
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
		5:begin//移位运算
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
		6:begin//位运�?
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
		7:begin//读写内存
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		16:begin//读栈内存
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		17:begin//写写内存
			y1_data_t = ram_data_bus_read;
			y2_data_t = 0;
			suspend_reg = !isCplt;
			rolr_tmp = 0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
		end
		8:begin//出入�?
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
		9:begin//数据转移
			y1_data_t = x2;
			y2_data_t = 0;
			suspend_reg =0;
			bit_sopr_tmp1 = 0;bit_sopr_tmp2 = 0;bit_sopr_tmp3 = 0;bit_sopr_tmp4 = 0;
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
		19:begin//位操�?
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
		
		//输出通道
		if((subMode===3 && x2===0)&&(mode===1 || mode===2 || mode===3)  ||  (mode===5 && x2>32))begin
			//如果为除�?/移位运算有问题，输出通道全部�?0
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
	
	//输出到下级流程的寄存�?
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

