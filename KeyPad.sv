/*
	Inputs sclk, mosi, cs
	Output miso
	
	Wait for chip select to go low
	For the next four clock rising clock edges, send four bits representing the key pressed
*/

parameter ROWS = 5'd12;
parameter COLS = 5'd22;
parameter N = 6'd32;

module Final(input logic  clk,
				 input logic  sclk, cs, metacs, sdi,
				 input logic  [3:0] rows,
				 output logic [3:0] cols,
				 output logic [3:0] keyPressedS,
				 output logic [7:0] keyByte,
				 output logic sdo,
				 output logic R0, G0, B0, R1, G1, B1,
				 output logic [3:0] A,
				 output logic lch,
				 output logic blank);
				 
				 logic 		 muxclk;
				 logic [3:0] key;
				 logic [3:0] keyPressed;
				 logic [3:0] keyPressedC;
				 logic [7:0] board [N - 1:0][N - 1:0];
				 logic [7:0] latched_board [N - 1:0][N - 1:0];
				 logic 		 board_spi_done, matrix_idle;
				 
				 clkdiv			clkdiv(clk, reset, muxclk);
				 
				 keypad			keypad(muxclk, reset, rows, cols, keyPressed);
				 
				 spi_key_slave	key_spi(sclk, cs, sdi, sdo, keyByte);
				 spi_board_slave board_spi(sclk, metacs, cs, sdi, board, board_spi_done);
				 
				 led_matrix led_matrix(sclk, latched_board, R0, G0, B0, R1, G1, B1, A, lch, blank, matrix_idle);
				 
				 pad_and_latch_board_state palbs(sclk, board_spi_done, matrix_idle, board, latched_board);				 
				 
				 synchronizer	synchronizer(clk, reset, keyPressed, keyPressedC);
				 synchronizer2 synchronizer2(clk, reset, keyPressed, keyPressedC, keyPressedS);
				 
				 assign keyByte[7] = (keyPressedS == 4'hD) ? 0 : 1;
				 assign keyByte[6:0] = {3'b0, keyPressedS};
endmodule

module pad_and_latch_board_state(input logic sclk,
											input logic board_spi_done,
											input logic matrix_idle,
											input logic [7:0] board [N - 1:0][N - 1:0],
											output logic [7:0] latched_board [N - 1:0][N - 1:0]);

		always_ff @(posedge sclk)
			if(board_spi_done & matrix_idle) begin
				latched_board <= board;
			end
											
endmodule

module led_matrix(input logic sclk,
						input logic [7:0] board [N - 1:0][N - 1:0],
						output logic R0, G0, B0, R1, G1, B1,
						output logic [3:0] A,
						output logic lch,
						output logic blank,
						output logic idle);
		
		logic [5:0] col;
		logic [17:0] cycle_cnt;
		
		// Send led board to reflect the board state stored on the FPGA
		always_ff @(posedge sclk)
			cycle_cnt <= cycle_cnt + 1;
		
		always_ff @(posedge sclk) begin
			if (cycle_cnt == 0) begin
				A <= 4'b0;
				col <= 6'b0;
				lch <= 0;
				blank <= 0;
				idle <= 0;
			end else if ((&col) & ~(&A)) begin
				A <= A + 1;
				col <= 6'b0;
				lch <= 0;
				blank <= 0;
			end else if ((&col) & (&A)) begin
				idle <= 1;
			end else if(~col[5]) begin
				if (|(board[A][col])) begin
					R0 <= 1;
					G0 <= 1;
					B0 <= 1;
				end else begin
					R0 <= 0;
					G0 <= 0;
					B0 <= 0;
				end
				
				if (|(board[5'd16 + A][col])) begin
					R1 <= 1;
					G1 <= 1;
					B1 <= 1;
				end else begin
					R1 <= 0;
					G1 <= 0;
					B1 <= 0;
				end
				
				col <= col + 1;
			end else if(col == 6'd32) begin
				lch <= 1;
				blank <= 1;
				col <= col + 1;
			end else begin
				blank <= 0;
				lch <= 0;
				col <= col + 1;
			end
		end
			
		
		// Wait some time such that we update the board about 100-200 times a second

endmodule

module decoder_using_case (input  logic [7:0]  ascii_in,
						   output logic [2:0]  rgb_out);
  
    always_comb
        case (ascii_in)
          8'b01001111 : rgb_out = 3'b101; 	// O
          8'b01001100 : rgb_out = 3'b011; 	// L
          8'b01001010 : rgb_out = 3'b011; 	// J
          8'b01001001 : rgb_out = 3'b110; 	// I
          8'b01010100 : rgb_out = 3'b100; 	// T
          8'b01010011 : rgb_out = 3'b010; 	// S
          8'b01011010 : rgb_out = 3'b001; 	// Z
          8'b00100000 : rgb_out = 3'b000; 	// ' '
          8'b00100011 : rgb_out = 3'b111; 	// #
          default: rgb_out = 8'b000 	  	// ' '
        endcase

endmodule

module spi_board_slave(input logic sclk,
								input logic metacs,
								input logic cs,
								input logic sdi,
								output logic [7:0] board [N - 1:0][N - 1:0],
								output logic done);
		
		logic [4:0] matrow, matcol;
		logic cs_sync;
		
		always_ff @(posedge sclk) begin
			cs_sync <= cs;
			if(~metacs) begin
				matrow <= 0;
				matcol <= 0;
			end
		end
		
		always_ff @(negedge cs_sync)
			if(matcol < N - 1) begin
				matcol <= matcol + 1;
			end else begin
				matcol <= 0;
				matrow <= matrow + 1;
			end
		
		logic [2:0] cnt;
		logic 		qdelayed;
		logic [7:0] q;

		always_ff@(negedge sclk)
			if (~cs) cnt = 0;
			else if(cnt < 5'd31) cnt = cnt + 5'b1;

		always_ff@(posedge sclk)
			if (cnt < 8) begin
				board[matrow][matcol] <= {board[matrow][matcol][6:0], sdi};
			end			
endmodule

// Appears to be working in simulation
// Issue with real CS
module spi_key_slave (input logic sclk,
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

// Works
module keypad (input logic        muxclk, reset,
					input logic  [3:0] rows,
					output logic [3:0] cols,
					output logic [3:0] keyPressed);
					
					logic 		state;
					logic [3:0] key;
					
		always_ff@(posedge muxclk)
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
				  output logic 	muxclk);
				  
		logic [13:0] count;
		
		always_ff@(posedge clk or posedge reset)
			if (reset) count <= 0;
			else count <= count + 1;
		assign muxclk = count[13];
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


/*
module key_spi(input  logic sclk,
					input  logic cs,
               input  logic sdi,
					input  logic[3:0] key,
               output logic sdo);
	
	logic sdoDelayed;
    // assert load
    // apply 256 sclks to shift in key and plaintext, starting with plaintext[0]
    // then deassert load, wait until done
    // then apply 128 sclks to shift out cyphertext, starting with cyphertext[0]
	 
	 logic [3:0] count;
	 logic [7:0] keyByte;
	 
    always_ff @(posedge sclk)
        if (~cs) begin
				//count <= 0;
				keyByte[7] <= (key == 4'hD) ? 0 : 1;
				keyByte[6:0] <= {3'b0, key};
		  end
        else begin
				keyByte = {keyByte[6:0], sdi};
				//count <= count + 1;
		  end
    
    // sdo should change on the negative edge of sck
    always_ff @(negedge sclk) begin
        sdoDelayed <= keyByte[7];
		  sdo <= sdoDelayed;
    end
endmodule
*/

/*
module aes_spi(input  logic sck, 
               input  logic sdi,
               output logic sdo,
               input  logic done,
               output logic [127:0] key, plaintext,
               input  logic [127:0] cyphertext);

    logic         sdodelayed, wasdone;
    logic [127:0] cyphertextcaptured;
               
    // assert load
    // apply 256 sclks to shift in key and plaintext, starting with plaintext[0]
    // then deassert load, wait until done
    // then apply 128 sclks to shift out cyphertext, starting with cyphertext[0]
    always_ff @(posedge sck)
        if (!wasdone)  {cyphertextcaptured, plaintext, key} = {cyphertext, plaintext[126:0], key, sdi};
        else           {cyphertextcaptured, plaintext, key} = {cyphertextcaptured[126:0], plaintext, key, sdi}; 
    
    // sdo should change on the negative edge of sck
    always_ff @(negedge sck) begin
        wasdone = done;
        sdodelayed = cyphertextcaptured[126];
    end
    
    // when done is first asserted, shift out msb before clock edge
    assign sdo = (done & !wasdone) ? cyphertext[127] : sdodelayed;
endmodule
*/

/*
module spi (input logic sclk, cs, mosi,
				input logic[3:0] keyPressed,
				output logic miso);
		
		logic [7:0] fifo;
		
		always_ff @(posedge sclk)
		begin
			if(cs) begin
			
			end else begin
				fifo <= 0
			end
		
endmodule
*/

/*
module display(input 		 muxclk,
				   input 		 reset,
				   input  [3:0] keyPressed,
				   output [7:0] data); 
			
			always_comb
				case(keyPressed)
					4'h0:	 	data = 8'b1111_0000;
					4'h1:		data = 8'b0000_0000;
					4'h2:		data = 8'b0000_0000;
					4'h3:		data = 8'b0000_0000;
					4'h4:		data = 8'b0000_0000;
					4'h5:		data = 8'b1000_0000;
					4'h6:		data = 8'b0000_0000;
					4'h7:		data = 8'b0100_0000;
					4'h8:		data = 8'b0010_0000;
					4'h9:		data = 8'b0001_0000;
					4'hA:		data = 8'b0000_0000;
					4'hB:		data = 8'b0000_0000;
					4'hC:		data = 8'b0000_0000;
					4'hD:		data = 8'b0000_0000;
					4'hE:		data = 8'b0000_0000;
					4'hF:		data = 8'b0000_0000;
					default: data = 8'b0000_0000;
				endcase
endmodule
*/

			
