parameter N = 6'd32;

module LED(input logic clk, reset,
						output logic clkOut,
						output logic R0, G0, B0, R1, G1, B1,
						output logic [3:0] A,
						output logic [3:0] Aled,
						output logic lch,
						output logic blank,
						output logic idle);

		logic [7:0] board [N - 1:0][N - 1:0];
		
		logic [11:0] cnt;
		
		always_ff @(posedge clk) begin
			board[cnt >> 5][cnt & 5'h1F] <= (cnt & 1) ? 8'd32 : 8'd35;
			if(reset) cnt <= 0;
			else cnt <= cnt + 1;
		end
						
		led_matrix led_matrix(clk, reset, board, R0, G0, B0, R1, G1, B1, A, lch, blank, idle);
		
		led_for_As	led_for_As(reset, clk, Aled);
		
		assign clkOut = clk;
						
endmodule

module led_matrix(input logic clk,
						input logic reset,
						input logic [7:0] board [N - 1:0][N - 1:0],
						output logic R0, G0, B0, R1, G1, B1,
						output logic [3:0] A,
						output logic lch,
						output logic blank,
						output logic idle);
		
		logic [13:0] col;
		logic [17:0] cycle_cnt;
		
		// Send led board to reflect the board state stored on the FPGA
		always_ff @(posedge clk) begin
			cycle_cnt <= cycle_cnt + 1;
		end
		
		always_ff@(posedge clk) begin
			if (cycle_cnt == 0) begin 
				col <= 6'b000000; 
			end else if ((&col) && (A < 4'd15)) begin 
				col <= 6'b000000;
			end else begin 
				col <= col + 1;
			end
		end
		
		always_ff @(posedge clk) begin
			if (cycle_cnt == 0) begin
				A <= 4'b0000;
				idle <= 0;
			end else if ((&col) && (A < 4'd15)) begin
				A <= A + 1;
			end else if ((&col) && (A == 4'd15)) begin
				idle <= 1;
			end 
		end
		
		always_ff@(posedge clk) begin
			if(~col[5]) begin
				if (board[A][col] == 8'd32) begin
					R0 <= 0;
					G0 <= 1;
					B0 <= 0;
				end else begin
					R0 <= 0;
					G0 <= 0;
					B0 <= 1;
				end				
				
				if (board[5'd16 + A][col] == 8'd32) begin
					R1 <= 1;
					G1 <= 0;
					B1 <= 0;
				end else begin
					R1 <= 1;
					G1 <= 1;
					B1 <= 0;
				end
			end else begin
				R0 <= 0;
				G0 <= 0;
				B0 <= 0;
				
				R1 <= 0;
				G1 <= 0;
				B1 <= 0;
			end
		end
			
		always_ff@(negedge clk) begin
			if(col == 6'd31) begin
				blank <= 1;
				lch <= 1;
			end else if(col == 6'd32) begin
				lch <= 0;
			end else if(col == 6'd33) begin
				blank <= 0;
			end 
		end
		
endmodule

module led_for_As (input logic reset, clk,
						 output logic [3:0] Aled);
						 
	logic [27:0] cycle_cnt2;
	logic [23:0] cycle_cnt3;
		
		// Send led board to reflect the board state stored on the FPGA
		always_ff @(posedge clk) begin
			cycle_cnt2 <= cycle_cnt2 + 1;
		end
		
		always_ff @(posedge clk) begin
			cycle_cnt3 <= cycle_cnt3 + 1;
		end
						 
	always_ff @(posedge clk) begin
			if (cycle_cnt2 == 0) begin
				Aled <= 4'b0000;
			end else if ((&cycle_cnt3) && (Aled < 4'd15)) begin
				Aled <= Aled + 1;
			end
		end

	
endmodule

//parameter N = 6'd32;
//
//module LED(input logic clk, reset,
//						output logic clkOut,
//						output logic R0, G0, B0, R1, G1, B1,
//						output logic [3:0] A,
//						output logic [3:0] Aled,
//						output logic lch,
//						output logic blank,
//						output logic idle);
//
//		logic [7:0] board [N - 1:0][N - 1:0];
//		
//		logic [11:0] cnt;
//		
//		always_ff @(posedge clk) begin
//			board[cnt >> 5][cnt & 5'h1F] <= (cnt & 1) ? 8'd32 : 8'd35;
//			if(reset) cnt <= 0;
//			else cnt <= cnt + 1;
//		end
//						
//		led_matrix led_matrix(clk, reset, board, R0, G0, B0, R1, G1, B1, A, lch, blank, idle);
//		
//		led_for_As	led_for_As(A, Aled);
//		
//		assign clkOut = clk;
//						
//endmodule
//
//module led_matrix(input logic clk,
//						input logic reset,
//						input logic [7:0] board [N - 1:0][N - 1:0],
//						output logic R0, G0, B0, R1, G1, B1,
//						output logic [3:0] A,
//						output logic lch,
//						output logic blank,
//						output logic idle);
//		
//		logic [5:0] col;
//		logic [10:0] cycle_cnt;
//		
//		// Send led board to reflect the board state stored on the FPGA
//		always_ff @(posedge clk) begin
//			if(reset) cycle_cnt <= 0;
//			else cycle_cnt <= cycle_cnt + 1;
//		end
//		
//		always_ff@(posedge clk) begin
//			if ((cycle_cnt == 0) | (reset)) begin 
//				col <= 6'b000000; 
//			end else if ((&col) & (A < 4'd15)) begin 
//				col <= 6'b000000;
//			end else begin 
//				col <= col + 1;
//			end
//		end
//		
//		always_ff @(posedge clk) begin
//			if ((cycle_cnt == 0) | (reset)) begin
//				A <= 4'b0000;
//	//			col <= 6'b000000;
//				lch <= 0;
//				blank <= 0;
//				idle <= 0;
//			end else if ((&col) & (A < 4'd15)) begin
//				A <= A + 1;
//	//			col <= 6'b0;
//				lch <= 0;
//				blank <= 0;
//			end else if ((&col) & (A == 4'd15)) begin
//				idle <= 1;
//			end else if(~col[5]) begin
//				if (board[A][col] == 8'd32) begin
//					R0 <= 1;
//					G0 <= 0;
//					B0 <= 0;
//				end else begin
//					R0 <= 1;
//					G0 <= 0;
//					B0 <= 0;
//				end
//				
//				if (board[5'd16 + A][col] == 8'd32) begin
//					R1 <= 0;
//					G1 <= 0;
//					B1 <= 1;
//				end else begin
//					R1 <= 0;
//					G1 <= 0;
//					B1 <= 1;
//				end
//				
//	//			col <= col + 1;
//			end else if(col == 6'd32) begin
//				blank <= 1;
//				lch <= 1;
//	//			col <= col + 1;
//			end else if(col == 6'd33) begin
//				lch <= 0;
//	//			col <= col + 1;
//			end else if(col == 6'd34) begin
//				blank <= 0;
//	//			col <= col + 1;
//			end 
//			
//			
//	//			else begin
//	//			col <= col + 1;
//	//		end
//			
//		end
//		
//
//			
//endmodule
//
//module led_for_As (input logic [3:0] A,
//						 output logic [3:0] Aled);
//						 
//	assign Aled = A;
//	
//endmodule

 
