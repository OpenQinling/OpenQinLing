/*
cpu��ˮ����ͣ/��������4�����:
	1.ָ�����ģ��ȴ�ָ�������Ӧ������ͣcp�Ĵ���/ȡֵ���̣�����������������    
	2.ִ��ģ����ִ�е��ڴ��д�������ȴ����ݻ�����Ӧ������ͣcp/ȡֵ/����/ȡ������/ִ������,��д������������
	3.�жϴ�������ˢ����ˮ�ߡ�
	4.cpu�����������мĴ�������ȫ����0����ˢ����ˮ��
*/
//ʱ�������ģ��
module ClockCtrl(
		input loadorder_ask,//ָ�����ģ�鷢������ͣ��ˮ������
		input execute_ask,//ִ��ģ���ڴ��д��������ͣ��ˮ������
		input int_ask,//�ж�ģ�鷢���������ˮ������
		input rst_ask,//cpu��������
		
		output Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,//�����źŷ���
		output Load_isStop,Analysis_stop,Execute_stop,AllGroup_stop//��ͣ�źŷ���
	);
	
	reg tLoad_rst,tAnalysis_rst,tExecute_rst,tAllGroup_rst;
	reg tLoad_isStop,tAnalysis_stop,tExecute_stop,tAllGroup_stop;
	assign Load_rst = tLoad_rst;
	assign Analysis_rst = tAnalysis_rst;
	assign Execute_rst = tExecute_rst;
	assign AllGroup_rst= tAllGroup_rst;
	assign Load_isStop = tLoad_isStop;
	assign Analysis_stop = tAnalysis_stop;
	assign Execute_stop =tExecute_stop;
	assign AllGroup_stop = tAllGroup_stop;
	
	always@(*)begin
		//cpu�����������ȼ���ߣ�����������мĴ������ݡ�ˢ����ˮ��
		if(rst_ask)begin
			tLoad_rst = 1;
			tAnalysis_rst = 1;
			tExecute_rst = 1;
			tAllGroup_rst = 1;
			
			tLoad_isStop = 0;
			tAnalysis_stop =0;
			tExecute_stop = 0;
			tAllGroup_stop = 0;
		end
		else if(execute_ask || loadorder_ask)begin//��ͣcpu���󣬽�cpu��ˮ���и���ģ��ȫ������
			tLoad_rst = 0;
			tAnalysis_rst = 0;
			tExecute_rst = 0;
			tAllGroup_rst = 0;
			
			tLoad_isStop = 1;
			tAnalysis_stop =1;
			tExecute_stop = 1;
			tAllGroup_stop = 1;
		end
		else if(int_ask)begin//�ж���ͣ�������ˮ��
			tLoad_rst = 1;
			tAnalysis_rst = 1;
			tExecute_rst = 1;
			tAllGroup_rst = 0;
			
			tLoad_isStop = 0;
			tAnalysis_stop =0;
			tExecute_stop = 0;
			tAllGroup_stop = 0;
		end
		else begin
			tLoad_rst = 0;
			tAnalysis_rst = 0;
			tExecute_rst = 0;
			tAllGroup_rst = 0;
			
			tLoad_isStop = 0;
			tAnalysis_stop =0;
			tExecute_stop = 0;
			tAllGroup_stop = 0;
		end
	end
endmodule 

