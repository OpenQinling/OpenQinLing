<-工程中构建cpu模块的方法->

该cpu源码全部为verilog语言
只需要将./Src包中的.v文件全部复制入工程。然后在需要使用该cpu核心的地方实例化cpu模块即可

<-023A-CPU实例化示例代码(verilog语言)->
    
    CPU_023A core(
		clk,//连接时钟信号源

        //外部中断接口
		int_ask,//外部中断请求信号输入[1bit]
		int_num,//外部中断的中断号输入[8bits]

        //连接mmu接口[cpu core中不包含mmu/cache等设备，需根据cpu特性自行制作]如果不使用虚拟内存，可不连接
		is_enable_vm,//cpu是否启用了mmu
		tlb_address,//cpu如果启用了mmu,mmuTLB表的内存地址[32bits]

        //连接指令缓存的接口
		order_bus,//指令数据传输的总线[32bits]
		orderAddress_bus,//要读取的指令的地址总线[32bits]
		loadOrder_cplt,//指令缓存器响应的信号线,当为1表示响应了,cpu在读取指令时会阻塞等待响应信号[1bit]

        //连接数据缓存的接口
		data_bus,//数据读写总线,如果单次读1-2字节，数据从低位传输[32bits]
		dataAddress_bus,//数据访问地址输出[32bits]
		dataRW_ctrl,//数据读写请求信号线,0/1为不读不写,2为读,3为写[2bits]
		dataRW_size,//读写的数据量，1-4字节。1位读1字节，2位读2字节，3为读4字节
		dataRW_cplt,

        //cpu内存寄存器数据的查看接口(用于Debug);sys_d为3bits,其余都为32bits
		r1_d,r2_d,r3_d,r4_d,r5_d,r6_d,r7_d,r8_d,flag_d,pc_d,tpc_d,ipc_d,sp_d,tlb_d,sys_d
	);