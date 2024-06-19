//ָ�����+������ȡ
module OrderAnalysis(
		input [31:0]order,//ָ���ȡ��������ָ��
		input clk,//ʱ������
		input rst,//����
		input isStop,//�Ƿ�ֹͣ
		
		//�Ĵ��������ӿ�
		input[31:0]r1,r2,r3,r4,r5,r6,cs,ds,flag,pc,tpc,ipc,sp,tlb,sys,
		
		output [4:0]mode,//��ģʽ
		output rw,//�ڴ��д�ķ������
		output [1:0]subMode,//��ģʽ���ڴ��д���ֽڿ���
		output [31:0]x1,x2,//����
		output [31:0]x2_inum,//x2��������
		output [4:0]m_num,l_num,
		output [3:0]x1_channel_select,
		output [3:0]x2_channel_select,
		output [3:0]y1_channel_select,
		output [1:0]y2_channel_select,
		
		//��ǰ������ִ�е�ָ���ַ
		input [31:0] thisOrderAddress,
		output[31:0]nextOrderAddress,
		input this_isRunning,
		output next_isRunning,
		
		//���жϷ����ӿ�
		input interrupt,
		input[7:0]interrupt_num,
		output next_interrupt,
		output[7:0]next_interrupt_num,
		
		//ָ�����͡�ָ��������ָ��Ӱ����Ϣ
		output isDepTPC,isDepIPC,
		output isEffTPC,isEffIPC,isEffFlag,isEffCS,
		output isFourCycle,//�Ƿ�Ϊ4�������͵�ָ���Ϊ1
		output next_isDepTPC,next_isDepIPC,
		output next_isEffTPC,next_isEffIPC,next_isEffFlag,next_isEffCS,
		output next_isFourCycle
		
	);
	//����Ĵ���
	wire [4:0]mode_reg = (order[31:27]>=1 && order[31:27]<=9)||order[31:27]===16||order[31:27]===17||order[31:27]===18||order[31:27]===19||order[31:27]==20||order[31:27]===21||order[31:27]===22 ? order[31:27] : 0;
	reg rw_reg;
	reg [1:0]subMode_reg;
	reg [3:0]x1_channel_reg;
	reg [3:0]x2_channel_reg;
	reg [20:0]num_reg;
	reg [3:0]y1_channel_reg;
	reg [1:0]y2_channel_reg;
	reg [4:0]m_num_reg,l_num_reg;
	//��ǰָ����������������
	assign isDepTPC = x1_channel_reg==11 || x2_channel_reg==11;
	assign isDepIPC = x1_channel_reg==12 || x2_channel_reg==12;
	assign isEffTPC = y1_channel_reg==11;
	assign isEffIPC = y1_channel_reg==12;
	assign isEffFlag = y1_channel_reg==9 || y2_channel_reg== 1;
	assign isEffCS = y1_channel_reg==7;
	assign isFourCycle = (order[31:27]>=1 && order[31:27]<=9)||(order[31:27]>=16 && order[31:27]<=22);
	
	
	always@(*)begin
		///////////////x1�Ĵ���ͨ������///////////////////////
		if((mode_reg>=1 && mode_reg<=7)||mode_reg===9||mode_reg===18||mode_reg==19||mode_reg==20||mode_reg===21||mode_reg===22)begin
			x1_channel_reg = order[23:20];
		end
		else if(mode_reg===17)begin//ջд��ģʽ
			x1_channel_reg = order[24:21];
		end
		else if(mode_reg===8)begin//����ջģʽ
			x1_channel_reg = 13;
		end
		else begin
			x1_channel_reg = 0;
		end
		
		
		//////////////x2�Ĵ���ͨ������///////////////////////
		if((mode_reg>=1 && mode_reg<=9)||mode_reg===18||mode_reg==19||mode_reg==20||mode_reg===21||mode_reg===22)begin
			x2_channel_reg = order[19:16];
		end
		else begin
			x2_channel_reg = 0;
		end
		
		
		//////////////��ģʽ����////////////////////////
		if((mode_reg>=1 && mode_reg<=9)||order[31:27]===18||mode_reg==19||mode_reg==20||mode_reg===21||mode_reg===22)begin
			subMode_reg = order[25:24];
		end
		else if(mode_reg===16||mode_reg===17)begin
			subMode_reg = order[26:25];
		end
		else begin
			subMode_reg = 0;
		end
		
		
		/////////////�ڴ��дģʽ����///////////////////////
		if((mode_reg>=1 && mode_reg<=9)||mode_reg==20||mode_reg===21||mode_reg===22)begin
			rw_reg = order[26];
		end
		else if(mode_reg===17)begin
			rw_reg = 1;
		end
		else begin
			rw_reg = 0;
		end
	
	
		///////////////y1�Ĵ���ͨ������///////////////////////
		if(mode_reg==4 || mode_reg===9 || mode_reg===18)begin
			//�����ת����ת������,y1=x1;
			y1_channel_reg = x1_channel_reg;
		end
		else if(mode_reg==19)begin
			y1_channel_reg = order[23:20];
		end
		else if(order[31:27]===16)begin//�����ջ��ȡ��y1��ַΪ[24:21]
			y1_channel_reg = order[24:21];
		end
		else if(mode_reg==1 || mode_reg==2 || mode_reg==3 || mode_reg==5 || mode_reg==6)begin
			y1_channel_reg = order[15:12];
		end
		else if(mode_reg===7 && rw_reg===0)begin
			//���ڴ棬y1Ϊѡ����x1ͨ��
			y1_channel_reg = x1_channel_reg;
		end
		else if(mode_reg===8 && rw_reg===0)begin
			//����ǳ�ջ��y1=x2;
			y1_channel_reg = x2_channel_reg;
		end
		else begin//y1=0;
			y1_channel_reg = 0;
		end
		
		////////////////y2�Ĵ���ͨ������/////////////////////
		if((mode_reg>=1 && mode_reg<= 6)||mode_reg===17||mode_reg==20||mode_reg===21||mode_reg===22)begin
			//�������/�Ƚϲ�����y2={flag};
			y2_channel_reg = 1;
		end
		else if(mode_reg===8)begin
			//��ջ������y2={sp};
			y2_channel_reg = 2;
		end
		else begin//������������Ҫy2ͨ����y2=0
			y2_channel_reg = 0;
		end
		
		///////////////x2����������/////////////////////////
		if(mode_reg==4 || mode_reg==7 || mode_reg==8 || mode_reg==9 || mode_reg==18 ||mode_reg==20||mode_reg===21||mode_reg===22)begin
			num_reg = {5'b0,order[15:0]};
		end
		else if(mode_reg==19)begin
			num_reg = {15'b0,order[15:10]};
		end
		else if(mode_reg==16 || mode_reg==17)begin
			num_reg = order[20:0];
		end
		else if(mode_reg==1 || mode_reg==2 || mode_reg==3 || mode_reg==5 || mode_reg==6)begin
			num_reg = {9'b0,order[11:0]};
		end
		else begin
			num_reg = 0;
		end
		
		//////////////λ����ָ���λ���ƽ���///////////////////
		if(mode_reg==19)begin
			m_num_reg = order[9:5];
			l_num_reg = order[4:0];
		end
		else begin
			m_num_reg = 0;
			l_num_reg = 0;
		end
	end
	
	
	
	//x1��x2������ȡ
	reg [31:0]x1_reg,x2_reg;
	always@(*)begin
		//x2����
		case(x2_channel_reg)
		0:begin
			case(mode_reg)
			16:x2_reg=sp+num_reg;
			17:x2_reg=sp+num_reg;
			7:x2_reg={ds[15:0],num_reg[15:0]};
			default:x2_reg={16'd0,num_reg[15:0]};
			endcase
		end
		1:x2_reg=r1;
		2:x2_reg=r2;
		3:x2_reg=r3;
		4:x2_reg=r4;
		5:x2_reg=r5;
		6:x2_reg=r6;
		7:x2_reg=cs;
		8:x2_reg=ds;
		9:x2_reg=flag;
		10:x2_reg=pc;
		11:x2_reg=tpc;
		12:x2_reg=ipc;
		13:x2_reg=sp;
		14:x2_reg=tlb;
		15:x2_reg=sys;
		endcase
		
		//x1ѡ��
		case(x1_channel_reg)
		1:x1_reg=r1;
		2:x1_reg=r2;
		3:x1_reg=r3;
		4:x1_reg=r4;
		5:x1_reg=r5;
		6:x1_reg=r6;
		7:x1_reg=cs;
		8:x1_reg=ds;
		9:x1_reg=flag;
		10:x1_reg=pc;
		11:x1_reg=tpc;
		12:x1_reg=ipc;
		13:x1_reg=sp;
		14:x1_reg=tlb;
		15:x1_reg=sys;
		default:x1_reg=0;
		endcase
	end
	
	
	reg [31:0]x1_t = 0,x2_t = 0;
	reg [3:0] y1_channel_t = 0;
	reg [1:0] y2_channel_t = 0;
	reg [3:0] x1_channel_t = 0;
	reg [3:0] x2_channel_t = 0;
	reg [31:0]x2_inum_t = 0;
	reg [4:0]m_num_t = 0,l_num_t = 0;
	reg rw_t = 0;
	assign rw = rw_t;
	reg [1:0]sub_mode_t = 0;
	reg [4:0]mode_t = 0;
	assign x1 = x1_t;
	assign x2 = x2_t;
	assign mode = mode_t;
	assign subMode = sub_mode_t;
	assign y1_channel_select = y1_channel_t;
	assign y2_channel_select = y2_channel_t;
	assign x1_channel_select = x1_channel_t;
	assign x2_channel_select = x2_channel_t;
	assign x2_inum = x2_inum_t;
	assign m_num = m_num_t;
	assign l_num = l_num_t;
	
	
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
	assign next_isFourCycle = next_isFourCycle_t;
	assign next_isEffCS = next_isEffCS_t;
	
	always@(posedge clk)begin
		if(rst)begin
			x1_t <=0;
			x2_t <=0;
			y1_channel_t <= 0;
			y2_channel_t <= 0;
			x1_channel_t <= 0;
			x2_channel_t <= 0;
			sub_mode_t<=0;
			mode_t <=0;
			rw_t <= 0;
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
			m_num_t <= 0;
			l_num_t <= 0;
		end
		else if(!isStop)begin
			x1_t <=x1_reg;
			x2_t <=x2_reg;
			y1_channel_t <= y1_channel_reg;
			y2_channel_t <= y2_channel_reg;
			x1_channel_t <= x1_channel_reg;
			x2_channel_t <= x2_channel_reg;
			x2_inum_t <= num_reg;
			rw_t <= rw_reg;
			sub_mode_t<=subMode_reg;
			mode_t <=mode_reg;
			m_num_t <= m_num_reg;
			l_num_t <= l_num_reg;
			
			nextOrderAddress_reg <= thisOrderAddress;
			next_isRunning_reg<=this_isRunning;
			
			interrupt_reg<=interrupt;
			interrupt_num_reg<=interrupt_num;
			
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
