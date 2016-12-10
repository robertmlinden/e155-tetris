module LEDTest();

	logic clk, clkOut;
	logic reset;
	logic R0, G0, B0, R1, G1, B1;
	logic [3:0] A;
	logic lch;
	logic blank;
	logic idle;
	
	LED led(clk, reset, clkOut, R0, G0, B0, R1, G1, B1, A, lch, blank, idle);
	
	always begin
		clk <= 1; #5; clk <= 0; #5;
	end
	
	initial
	begin
		reset = 1; #13;
		reset = 0;
	end

endmodule
