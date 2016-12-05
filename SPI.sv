parameter N = 32;

module SPI(input logic sclk,
								input logic reset,
								input logic cs,
								input logic sdi,
								output logic [15:0] numNotSpaces,
								output logic isReset);
								
		logic [7:0] board [N - 1:0][N - 1:0];
		
		spi_board_slave spi_board_slave(sclk, reset, cs, sdi, board, numNotSpaces);

endmodule

module spi_board_slave(input logic sclk,
								input logic reset,
								input logic cs,
								input logic sdi,
								output logic [7:0] board [N - 1:0][N - 1:0],
								output logic [15:0] numNotSpaces,
								output logic isReset);
		
		logic [4:0] matrow, matcol;
		
		logic [3:0] cnt;
		logic 		qdelayed;
		logic [7:0] q;
		logic [7:0] char;

		always_ff@(posedge sclk)
			if (cnt < 4'd8) begin
				board[matrow][matcol] <= {board[matrow][matcol][6:0], sdi};
				char <= {char[6:0], sdi};
			end 
		
		always_ff@(negedge sclk)
			/*if(reset) begin
				numNotSpaces <= 0;
				matrow <= 0;
				matcol <= 0;
				cnt <= 0;
				isReset <= ~isReset;
			end else*/ if (cnt == 4'd7) begin
				if(char == 8'd74) begin
					numNotSpaces <= numNotSpaces + 1;
				end
				if(matcol < N - 1) begin
					matcol <= matcol + 1;
				end else begin
					matcol <= 0;
					matrow <= matrow + 1;
				end
				if(matrow == N - 1 && matcol == N - 1) begin
					// numNotSpaces <= 0;
					matrow <= 0;
					matcol <= 0;
					cnt <= 0;
				end
				cnt <= 0;
			end else begin
				cnt <= cnt + 4'b1;
			end
endmodule
