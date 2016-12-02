parameter N = 32;

module SPI(input logic sclk,
								input logic reset,
								input logic cs,
								input logic sdi,
								output logic [7:0] numNotSpaces,
								output logic done);
								
		logic [7:0] board [N - 1:0][N - 1:0];
		
		spi_board_slave spi_board_slave(sclk, reset, cs, sdi, board, done, numNotSpaces);

endmodule

module spi_board_slave(input logic sclk,
								input logic reset,
								input logic cs,
								input logic sdi,
								output logic [7:0] board [N - 1:0][N - 1:0],
								output logic done,
								output logic [7:0] numNotSpaces);
		
		logic [4:0] matrow, matcol;
		logic cs_sync, cs_prev;
		
		always_ff @(posedge sclk) begin
			cs_sync <= cs;
			cs_prev <= cs_sync;
		end
		
		always_ff @(posedge sclk) begin
			if(reset) begin
				matrow <= 0;
				matcol <= 0;
			end
			// negedge cs
			else if(~cs_sync & cs_prev) begin
				if(matcol < N - 1) begin
					matcol <= matcol + 1;
				end else begin
					matcol <= 0;
					matrow <= matrow + 1;
				end
			end
		end
		
		logic [2:0] cnt;
		logic 		qdelayed;
		logic [7:0] q;
		logic [7:0] char;

		always_ff@(negedge sclk)
			if (~cs) cnt <= 0;
			else if(cnt < 5'd31) cnt = cnt + 5'b1;

		always_ff@(posedge sclk)
			if (cnt < 8) begin
				board[matrow][matcol] <= {board[matrow][matcol][6:0], sdi};
				char <= {char[6:0], sdi};
			end 
		
		always_ff@(negedge sclk)
			if(reset) begin
				numNotSpaces <= 0;
			end else if (cnt == 7) begin
				if(char == 8'd35) begin
					numNotSpaces <= numNotSpaces + 1;
				end
			end
endmodule
