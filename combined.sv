parameter N = 32;

module TetrisFinal(input logic 	     clk, reset,
						 input logic sclk, cs, sdi, load,
						 input logic  [3:0] rows,
						 output logic [3:0] cols,
						 output logic [7:0] keyByte,
						 output logic [3:0] keyPressed,
						 output logic [4:0] sendCount,
						 output logic [15:0] numNotSpaces,
						 output logic clkOut,
						 output logic sdo,
						 output logic R0, G0, B0, R1, G1, B1,
						 output logic [3:0] A,
						 output logic lch,
						 output logic blank,
						 output logic boardOut);
				 
				 logic 		 slowclk;
				 logic [3:0] key;
				 // logic [3:0] keyPressed;
				 logic [3:0] keyPressedC;
				 logic [3:0] keyPressedS;
				 logic [7:0] board [N - 1:0][N - 1:0];
				 logic [7:0] latched_board [N - 1:0][N - 1:0];
				 logic led_idle;
				 
				 logic [7:0] fakeBoard [N - 1:0][N - 1:0];
		
				logic [11:0] cnt;
				
				always_ff @(posedge clk) begin
					fakeBoard[cnt >> 5][cnt & 5'h1F] <= (cnt & 1) ? 8'd32 : 8'd35;
					if(reset) cnt <= 0;
					else cnt <= cnt + 1;
				end
				 
				 logic [11:0] sCount;
				 assign sendCount = sCount >> (11 - 4);
				 
				 clkdiv			clkdiv(clk, reset, slowclk);
				 keypad			keypad(slowclk, reset, rows, cols, keyPressed);
				 
				 synchronizer	synchronizer(clk, reset, keyPressed, keyPressedC);
				 synchronizer2 synchronizer2(clk, reset, keyPressed, keyPressedC, keyPressedS);
				 
				 led_matrix led_matrix(clk, board/*latched_board*/, R0, G0, B0, R1, G1, B1, A, lch, blank, led_idle);
				 
				 spi_slave		key_spi(sclk, cs, sdi, sdo, keyByte);
				 spi_board_slave spi_board_slave(sclk, cs, sdi, load, board, numNotSpaces, sCount, boardOut, matrow, matcol);
				 
				 pad_and_latch_board_state palbs(sclk, ~load, led_idle, board, latched_board);
				 
				 assign keyByte[7] = (keyPressedS == 4'hD) ? 0 : 1;
				 assign keyByte[6:0] = {3'b0, keyPressedS};
				 assign clkOut = clk;
endmodule

module pad_and_latch_board_state(input logic sclk,
											input logic board_spi_done,
											input logic matrix_idle,
											input logic [7:0] board [N-1:0][N-1:0],
											output logic [7:0] latched_board [N-1:0][31:0]);

		always_ff @(posedge sclk)
			if(board_spi_done & matrix_idle) begin
				latched_board <= board;
			end
											
endmodule

//module spi_board_slave(input logic sclk,
//								input logic cs,
//								input logic sdi,
//								input logic load,
//								output logic [7:0] board [N - 1:0][N - 1:0]);
////								output logic [15:0] numNotSpaces,
////								output logic [11:0] sendCount,
////								output logic [7:0] boardOut,
////								output logic [4:0] matrow,
////								output logic [4:0] matcol);
//
//
//		logic [4:0] matcol;
//		logic [4:0] matrow;
//		logic [2:0] cnt;
//		logic [7:0] char;
//
//		always_ff@(posedge sclk)
//			if(load) begin
//				if (cnt < 4'd8) begin
//					board[matrow][matcol] <= {board[matrow][matcol][6:0], sdi};
//					char <= {char[6:0], sdi};
//				end
//			end
//		
////		always_ff@(negedge sclk) begin
////			if(load) begin
////				if (cnt < 4'd7) begin
////					cnt <= cnt + 3'b1;
////				end 
////			end
////		end
//		
//		always_ff@(negedge sclk) begin
//			if (load) begin	
//				if (cnt < 4'd7) begin
//					cnt <= cnt + 3'b1;
//				end else if (cnt == 4'd7) begin
////					sendCount <= sendCount + 1; 
////					if(board[matrow][matcol] == 8'd74) begin
////						numNotSpaces <= numNotSpaces + 1;
////					end
//					if(matcol < N - 1) begin
//						matcol <= matcol + 1;
//					end else if (matcol == N - 1) begin
//						matrow <= matrow + 1;
//					end
//					cnt <= 0;
//				end
////				if(matrow == 5'd0 && matcol == 5'd1) begin
////					boardOut <= board[matrow][matcol];
////				end
//			end else if (~load) begin
//				matrow <= 0;
//				matcol <= 0;
//			end
//		end
//		
//endmodule

module spi_board_slave(input logic sclk,
								input logic cs,
								input logic sdi,
								input logic load,
								output logic [7:0] board [N - 1:0][N - 1:0],
								output logic [15:0] numNotSpaces,
								output logic [11:0] sendCount,
								output logic [7:0] boardOut,
								output logic [4:0] matrow,
								output logic [4:0] matcol);
		
		logic [2:0] cnt;
		logic [7:0] char;

		always_ff@(posedge sclk)
			if(load) begin
				if (cnt < 4'd8) begin
					board[matrow][matcol] <= {board[matrow][matcol][6:0], sdi};
					char <= {char[6:0], sdi};
				end
			end
		
		always_ff@(negedge sclk) begin
			if(load) begin
				if (cnt < 4'd7) begin
					cnt <= cnt + 3'b1;
				end else if (cnt == 4'd7) begin
					sendCount <= sendCount + 1; 
					if(board[matrow][matcol] == 8'd74) begin
						numNotSpaces <= numNotSpaces + 1;
					end
					if(matcol < N - 1) begin
						matcol <= matcol + 1;
					end else if (matcol == N - 1) begin
//						matcol <= 0;
						matrow <= matrow + 1;
					end
					cnt <= 0;
				end
				if(matrow == 5'd0 && matcol == 5'd1) begin
					boardOut <= board[matrow][matcol];
				end
			end begin
				matrow <= 0;
				matcol <= 0;
			end
		end
		
endmodule

module keypad (input logic        slowclk, reset,
					input logic  [3:0] rows,
					output logic [3:0] cols,
					output logic [3:0] keyPressed);
					
					logic 		state;
					logic [3:0] key;
					
		always_ff@(posedge slowclk)
			if(~(|rows)) begin
				state <= 0;
				keyPressed <= 4'hD;
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

module clkdiv(input  logic 	clk, reset,
				  output logic 	slowclk);
				  
		logic [13:0] count;
		
		always_ff@(posedge clk or posedge reset)
			if (reset) count <= 0;
			else count <= count + 1;
		assign slowclk = count[13];
endmodule


module synchronizer(input logic clk, reset,
						  input logic [3:0] keyPressed,
						  output logic [3:0] keyPressedC);

					always@(posedge clk or posedge reset)
						if (reset) begin 
							keyPressedC <= 0;
						end else begin
							keyPressedC <= keyPressed;
						end
						
endmodule


module synchronizer2(input logic clk, reset,
						   input logic [3:0] keyPressed,
							input logic [3:0] keyPressedC,
						   output logic [3:0] keyPressedS);
							
					always@(posedge clk or posedge reset)
						if (reset) begin 
							keyPressedS <= 4'hD;
						end else if (keyPressed == keyPressedC) begin
							keyPressedS <= keyPressedC;
						end else begin
							keyPressedS <= 4'hD;
						end
						
endmodule

module led_matrix(input logic clk,
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
				col <= 13'd0; 
			end else if ((&col) && (A < 4'd15)) begin 
				col <= 13'd0;
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
				end else if(board[A][col] == 8'd35) begin
					R0 <= 1;
					G0 <= 1;
					B0 <= 1;
				end else if(board[A][col] == 8'd70) begin
					R0 <= 1;
					G0 <= 0;
					B0 <= 0;
				end else begin
					R1 <= board[A][col] & 1;
					G1 <= (board[A][col] >> 1) & 1;
					B1 <= |(board[A][col]);
				end
				
				if (board[A + 5'd16][col] == 8'd32) begin
					R1 <= 0;
					G1 <= 1;
					B1 <= 0;
				end else if(board[A + 5'd16][col] == 8'd35) begin
					R1 <= 1;
					G1 <= 1;
					B1 <= 1;
				end else if(board[A + 5'd16][col] == 8'd70) begin
					R1 <= 1;
					G1 <= 0;
					B1 <= 0;
				end else begin
					R1 <= board[A + 5'd16][col] & 1;
					G1 <= (board[A + 5'd16][col] >> 1) & 1;
					B1 <= |(board[A + 5'd16][col]);
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

module spi_slave (input logic sclk,
						input logic cs,
						input logic sdi,
						output logic sdo,
						input logic [7:0] keyByte);
						
	logic [2:0] cnt;
	logic 		qdelayed;
	logic [7:0] q;
	
	always_ff@(negedge sclk)
	if (~cs) cnt = 0;
	else cnt = cnt + 3'b1;
	
	always_ff@(posedge sclk)
		q <= (cnt == 0) ? {keyByte[6:0], sdi} : {q[6:0], sdi};
		
	always_ff@(negedge sclk)
		qdelayed = q[7];
	assign sdo = (cnt == 0) ? keyByte[7] : qdelayed;
	
endmodule
