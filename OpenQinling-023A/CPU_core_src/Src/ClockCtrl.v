/*
cpu流水线暂停/重启，有4种情况:
	1.指令加载模块等待指令缓存器响应，需暂停cp寄存器/取值流程，后面流程正常运行    
	2.执行模块是执行的内存读写操作，等待数据缓存响应，需暂停cp/取值/译码/取操作数/执行流程,回写流程正常运行
	3.中断处理，立刻刷新流水线。
	4.cpu重启。将所有寄存器数据全部清0，并刷新流水线
*/
//时序控制器模块
module ClockCtrl(
		input loadorder_ask,//指令加载模块发出的暂停流水线请求
		input execute_ask,//执行模块内存读写发出的暂停流水线请求
		input int_ask,//中断模块发出的清除流水线请求
		input rst_ask,//cpu重启请求
		
		output Load_rst,Analysis_rst,Execute_rst,AllGroup_rst,//重启信号发出
		output Load_isStop,Analysis_stop,Execute_stop,AllGroup_stop//暂停信号发出
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
		//cpu重启请求优先级最高，立刻清除所有寄存器数据、刷新流水线
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
		else if(execute_ask)begin//当内存读写时，暂停cp-参数获取模块，并终止执行模块的输出
			tLoad_rst = 0;
			tAnalysis_rst = 0;
			tExecute_rst = 1;
			tAllGroup_rst = 0;
			
			tLoad_isStop = 1;
			tAnalysis_stop =1;
			tExecute_stop = 0;
			tAllGroup_stop = 1;
		end
		else if(loadorder_ask)begin//当在指令加载时，暂停cp，并终止指令加载模块的输出
			tLoad_rst = 1;
			tAnalysis_rst = 0;
			tExecute_rst = 0;
			tAllGroup_rst = 0;
			
			tLoad_isStop = 0;
			tAnalysis_stop =0;
			tExecute_stop = 0;
			tAllGroup_stop = 1;
		end
		else if(int_ask)begin//中断暂停，清空流水线
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

