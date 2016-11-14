module keypad(input  logic		clk, reset,
			  input  logic [3:0]	rows,
			  output logic [3:0]	cols,
			  output logic [3:0]	dataout);

	logic		slowclk;
	logic [3:0]	d0, d1;

	clkdiv		clkdiv(clk, reset, slowclk);
	keypad		keypad(slowclk, reset, rows, cols, d0, d1);
	display		display(slowclk, reset, d0, dataout);
	
endmodule

module keypad(input	logic			slowclk, reset,
			  input logic [3:0]		rows,
			  output logic [3:0]	cols, d0);

	logic		state;
	logic [3:0] key;

	always_ff@(posedge slowclk or posedge reset)
		if (reset) begin
			state <= 0;
			cols <= 4b'0111;
			d0 <= 4'b0000;
		end else if (&rows) begin
			state <= 0;
			cols <= (cols[0], cols[3:1]);
		end else if (~state) begin
			state <= 1;
			d0 <= key;
		end

	always_comb
		case({rows, cols})
			8'b0111_0111: key <= 'hC;
			8'b1011_0111: key <= 'hD;
			8'b1101_0111: key <= 'hE;
			8'b1110_0111: key <= 'hF;
			8'b0111_1011: key <= 'h3;
			8'b1011_1011: key <= 'h6;
			8'b1101_1011: key <= 'h9;
			8'b1110_1011: key <= 'hB;
			8'b0111_1101: key <= 'h2;
			8'b1011_1101: key <= 'h5;
			8'b1101_1101: key <= 'h8;
			8'b1110_1101: key <= 'h0;
			8'b0111_0111: key <= 'h1;
			8'b1011_1011: key <= 'h4;
			8'b1101_1101: key <= 'h7;
			8'b1110_1110: key <= 'hA;
			default: key <= 'h0;
		endcase

endmodule

module clkdiv(input  logic clk, reset,
			  output logic slowclk);

		logic [16:0] count;

		always_ff@(posedge clk or posedge reset)
			if (reset) count <= 0;
			else count <= count + 1;
		assign slowclk = count[16];

endmodule

module decoder (input	logic [3:0] a,
				output	logic [3:0] y);

	always_comb
		case(a)
			0:	y = 4'b0000;
			1:	y = 4'b0000;
			2:	y = 4'b0000;
			3:	y = 4'b0000;
			4:	y = 4'b0000;
			5:	y = 4'b1000;
			6:	y = 4'b0000;
			7:	y = 4'b0100;
			8:	y = 4'b0010;
			9:	y = 4'b0001;
			10:	y = 4'b0000;
			11:	y = 4'b0000;
			12:	y = 4'b0000;
			13:	y = 4'b0000;
			14:	y = 4'b0000;
			15:	y = 4'b0000;

		endcase
endmodule

module display(slowclk, reset, d0, dataout);
	input			slowclk;
	input			reset;
	input [3:0]		d0;
	output [3:0]	dataout;

	decoder decoder(d0, dataout);

endmodule
