parameter N = 6'd32;

module LED(input logic clk, reset,
						output logic clkOut,
						output logic R0, G0, B0, R1, G1, B1,
						output logic [3:0] A,
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
		
		logic [5:0] col;
		logic [30:0] cycle_cnt;
		
		// Send led board to reflect the board state stored on the FPGA
		always_ff @(posedge clk) begin
			if(reset) cycle_cnt <= 0;
			else cycle_cnt <= cycle_cnt + 1;
		end
		
		always_ff @(posedge clk) begin
			if (cycle_cnt == 0 || reset) begin
				A <= 4'b1;
				col <= 6'b0;
				lch <= 0;
				blank <= 0;
				idle <= 0;
			end else if ((&col) & (A < 4'd14)) begin
				A <= A + 1;
				col <= 6'b0;
				lch <= 0;
				blank <= 0;
			end else if ((&col) & (A == 4'd14)) begin
				idle <= 1;
			end else if(~col[5]) begin
				if (board[A][col] == 7'd32) begin
					R0 <= 1;
					G0 <= 0;
					B0 <= 0;
				end else begin
					R0 <= 1;
					G0 <= 0;
					B0 <= 0;
				end
				
				if (board[5'd16 + A][col] == 7'd32) begin
					R1 <= 0;
					G1 <= 0;
					B1 <= 1;
				end else begin
					R1 <= 0;
					G1 <= 0;
					B1 <= 1;
				end
				
				col <= col + 1;
			end else if(col == 6'd32) begin
				blank <= 1;
				lch <= 1;
				col <= col + 1;
			end else if(col == 6'd33) begin
				lch <= 0;
				col <= col + 1;
			end else if(col == 6'd34) begin
				blank <= 0;
				col <= col + 1;
			end else begin
				col <= col + 1;
			end
		end
			
		
		// Wait some time such that we update the board about 100-200 times a second

endmodule
