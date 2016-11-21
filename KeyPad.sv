/*
	Inputs sclk, mosi, cs
	Output miso
	
	Wait for chip select to go low
	For the next four clock rising clock edges, send four bits representing the key pressed
*/

module Final(input logic 	clk, reset,
				 input logic [3:0] rows,
				 output logic [3:0] cols,
				 output logic [3:0] key,
				 output logic [3:0] columns,
				 output logic 			led);
				 
				 logic 		 slowclk;
				 logic [3:0] d0;
				 
				 clkdiv		clkdiv(clk, reset, slowclk);
				 keypad		keypad(slowclk, reset, rows, cols, d0, key);
//				 display		display(slowclk, reset, d0, data);
//				 ledmod		ledmod(data, slowclk, led);

				 assign columns = cols;
				 
endmodule

module keypad (input logic slowclk, reset,
					input logic [3:0] rows,
					output logic [3:0] cols, d0,
					output logic [3:0] keyPressed);
					
					logic 		state;
					logic[3:0] key;
					
		always_ff@(posedge slowclk or posedge reset)
			if (reset) begin	
				state <= 0;
				cols <= 4'b1000;
				d0 <= 4'b0000;
				keyPressed <= 4'h2;
			end else if(~(|rows)) begin
				state <= 0;
				case(cols)
					4'b1000: cols <= 4'b0100;
					4'b0100: cols <= 4'b0010;
					4'b0010: cols <= 4'b0001;
					4'b0001: cols <= 4'b1000;
					default: cols <= 4'b1000;
				endcase
			end else if(~state) begin
				state <= 1;
				keyPressed <= key;
			end
			
			// assign data = {rows,cols};
			
			always_comb
				case({rows,cols})
					8'b1000_1000: key <= 4'hD;
					8'b0100_1000: key <= 4'hC;
					8'b0010_1000: key <= 4'hB;
					8'b0001_1000: key <= 4'hA;
					8'b1000_0100: key <= 4'hF;
					8'b0100_0100: key <= 4'h9;
					8'b0010_0100: key <= 4'h6;
					8'b0001_0100: key <= 4'h3;
					8'b1000_0010: key <= 4'h0;
					8'b0100_0010: key <= 4'h8;
					8'b0010_0010: key <= 4'h5;
					8'b0001_0010: key <= 4'h2;
					8'b1000_0001: key <= 4'hE;
					8'b0100_0001: key <= 4'h7;
					8'b0010_0001: key <= 4'h4;
					8'b0001_0001: key <= 4'h1;
					default: key <= 4'h2;
				endcase
				
endmodule

//module display(slowclk, reset, d0, data);
//			input 		slowclk;
//			input 		reset;
//			input [3:0]	d0;
//			output [7:0] data; 
//			
//			always_comb
//				case(d0)
//					0:	data = 8'b1111_0000;
//					1:	data = 8'b0000_0000;
//					2:	data = 8'b0000_0000;
//					3:	data = 8'b0000_0000;
//					4:	data = 8'b0000_0000;
//					5:	data = 8'b1000_0000;
//					6:	data = 8'b0000_0000;
//					7:	data = 8'b0100_0000;
//					8:	data = 8'b0010_0000;
//					9:	data = 8'b0001_0000;
//					10:	data = 8'b0000_0000;
//					11:	data = 8'b0000_0000;
//					12:	data = 8'b0000_0000;
//					13:	data = 8'b0000_0000;
//					14:	data = 8'b0000_0000;
//					15:	data = 8'b0000_0000;
//					default: data = 8'b0000_0000;
//				endcase
//endmodule

module clkdiv(input logic clk, reset,
				  output logic slowclk);
				  
		logic [13:0] count;
		
		always_ff@(posedge clk or posedge reset)
			if (reset) count <= 0;
			else count <= count + 1;
		assign slowclk = count[13];
endmodule

//module ledmod (input logic [7:0] data,
//					input logic 		slowclk,
//					output logic 		led);
//					
//		always_ff@(posedge slowclk)
//			if (data == 8'b10000000) led <= 1;
//			else led <= 0;
//endmodule


			
