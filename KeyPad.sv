/*
	Inputs sclk, mosi, cs
	Output miso
	
	Wait for chip select to go low
	For the next four clock rising clock edges, send four bits representing the key pressed
*/

module Final(input logic clk, reset,
	     input logic sclk, cs, sdi,
	     input logic  [3:0] rows,
	     output logic [3:0] cols,
	     output logic [3:0] keyPressedS,
	     output logic [3:0] columns,
	     output logic sdo);
				 
				 logic 		 slowclk;
				 logic [3:0] key;
				 logic [3:0] keyPressed;
				 logic [3:0] keyPressedC;
				 
				 clkdiv		clkdiv(clk, reset, slowclk);
				 keypad		keypad(slowclk, reset, rows, cols, keyPressed);
				 key_spi	key_spi(sclk, cs, sdi, keyPressedS, sdo);
				 synchronizer	synchronizer(clk, reset, keyPressed, keyPressedC);
				 synchronizer2  synchronizer2(clk, reset, keyPressed, keyPressedC, keyPressedS);
				 
				 assign columns = cols;
				 
endmodule

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

module keypad (input logic        slowclk, reset,
					input logic  [3:0] rows,
					output logic [3:0] cols,
					output logic [3:0] keyPressed);
					
					logic 		state;
					logic [3:0] key;
					
		always_ff@(posedge slowclk or posedge reset)
			if (reset) begin	
				state <= 0;
				cols <= 4'b1000;
				keyPressed <= 4'hD;
			end else if(~(|rows)) begin
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

/*
module display(input 		 slowclk,
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

			
