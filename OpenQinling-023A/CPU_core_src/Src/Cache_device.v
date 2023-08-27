//256字节缓存器
module Cache_256Byte(
		input rw_ctrl,//0为读，1为写
		input clk,//如果为读模式，上升沿数据写入
		input[1:0]size_ctrl,
		input[15:0]address,
		inout[31:0]data_bus
	);
	reg [7:0]data[65535:0];
	reg [31:0]read_data;
	assign data_bus = !rw_ctrl?read_data:32'bz;
	
		 initial data[0]=8'h48;
        initial data[1]=8'h20;
        initial data[2]=8'h00;
        initial data[3]=8'h0a;
        initial data[4]=8'h48;
        initial data[5]=8'h30;
        initial data[6]=8'h00;
        initial data[7]=8'h14;
        initial data[8]=8'h32;
        initial data[9]=8'h13;
        initial data[10]=8'h10;
        initial data[11]=8'h00;
        initial data[12]=8'h58;
        initial data[13]=8'h00;
        initial data[14]=8'h00;
        initial data[15]=8'h0c;
	
	always@(*)begin
		case(size_ctrl)
		1:begin
		  read_data[7:0] = data[address];
		  read_data[31:8] = 0;
		end
		2:begin
		  read_data[7:0] = data[address+1];
		  read_data[15:8] = data[address];
		  read_data[31:16] = 0;
		end
		3:begin
		  read_data[7:0] = data[address+3];
		  read_data[15:8] = data[address+2];
		  read_data[23:16] = data[address+1];
		  read_data[31:24] = data[address];
		end
		default:read_data=0;
		endcase
	end
	
	always@(posedge clk)begin
	   if(rw_ctrl)begin
           case(size_ctrl)
            1:begin
              data[address] <= data_bus[7:0];
            end
            2:begin
              data[address+1] <= data_bus[7:0];
              data[address] <= data_bus[15:8];
            end
            3:begin
              data[address+3] <= data_bus[7:0];
              data[address+2] <= data_bus[15:8];
              data[address+1] <= data_bus[23:16];
              data[address] <= data_bus[31:24];
            end
            endcase
	   end
	end
	
endmodule




//6k字节缓存器
module Cache_6KByte(
		input rw_ctrl,//0为读，1为写
		input clk,//如果为写模式，上升沿数据写入
		input[1:0]size_ctrl,
		input[15:0]address,
		inout[31:0]data_bus
	);
	reg [7:0]data[65535:0];
	reg [31:0]read_data;
	assign data_bus = !rw_ctrl?read_data:32'bz;
	
		initial data[0]=8'h00;
        initial data[1]=8'h00;
        initial data[2]=8'h00;
        initial data[3]=8'h84;
        initial data[4]=8'h00;
        initial data[5]=8'h00;
        initial data[6]=8'h0f;
        initial data[7]=8'hff;
        initial data[8]=8'h00;
        initial data[9]=8'h00;
        initial data[10]=8'h00;
        initial data[11]=8'h0c;
        initial data[12]=8'h00;
        initial data[13]=8'h00;
        initial data[14]=8'h01;
        initial data[15]=8'hb0;
        initial data[16]=8'h00;
        initial data[17]=8'h00;
        initial data[18]=8'h02;
        initial data[19]=8'h8d;
        initial data[20]=8'h00;
        initial data[21]=8'h00;
        initial data[22]=8'h00;
        initial data[23]=8'h4e;
        initial data[24]=8'h00;
        initial data[25]=8'h00;
        initial data[26]=8'h02;
        initial data[27]=8'h00;
	
	always@(*)begin
		case(size_ctrl)
		1:begin
		  read_data[7:0] = data[address];
		  read_data[31:8] = 0;
		end
		2:begin
		  read_data[7:0] = data[address+1];
		  read_data[15:8] = data[address];
		  read_data[31:16] = 0;
		end
		3:begin
		  read_data[7:0] = data[address+3];
		  read_data[15:8] = data[address+2];
		  read_data[23:16] = data[address+1];
		  read_data[31:24] = data[address];
		end
		default:read_data=0;
		endcase
	end
	
	
	always@(posedge clk)begin
	   if(rw_ctrl)begin
		   
           case(size_ctrl)
            1:begin
              data[address] <= data_bus[7:0];
            end
            2:begin
              data[address+1] <= data_bus[7:0];
              data[address] <= data_bus[15:8];
            end
            3:begin
			  
              data[address+3] <= data_bus[7:0];
              data[address+2] <= data_bus[15:8];
              data[address+1] <= data_bus[23:16];
              data[address] <= data_bus[31:24];
            end
            endcase
	   end
	end
	
endmodule