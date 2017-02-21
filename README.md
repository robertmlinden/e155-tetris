# e155-tetris

## Introduction
Welcome to Paul Slaats's and Robert Linden's E155 Microprocessors Final Project! We have implemented our nuanced version of Tetris on an 32x32 Adafruit LED board using a Raspberry Pi and an FPGA, and while you obviously cannot execute the code yourself (unless you went through the trouble of buying all of the proper equipment and hooking it all up), we will do our best here to make the project come alive to you here on Github.

## Demo
We'll get right to it:
||Insert Demo Here||

## Technical Overview
Our implementation of LED Tetris is split between a Raspberry Pi 3.0 and a MuddPi FPGA. All communication between the two devices is done via SPI with the Pi acting as the master and the FPGA acting as the slave. The FPGA is responsible for registering keystrokes on a 4x4 matrix keypad, encoding them as a byte, and sending this information to the Pi. The FPGA is also responsible for accepting the state of the LED board and sending digital signals to the LED matrix to make the matrix light up appropriately. All code on the FPGA is written in SystemVerilog. 
The Pi encodes and updates the board state and provides the timer that governs the fall rate of the falling piece. The Pi takes the game state, lays out the appropriate 32x32 display that should appear on the LED board, and sends it to the FPGA. It accepts keystroke byte representations from the FPGA and updates the game state accordingly. All code on the Pi is written in C.
While our version of Tetris is very similar to any typical implementation of Tetris, there are a some notable rules and game dynamics:
  * The user can “lead-drop” a piece, meaning that if they press the correct key, the falling piece, will instantly land on the first piece below its current location.
  * In addition to the board state, the user can see the next piece, the bonus piece (if available, see next bullet), and their score.
  * The user’s score increases by the number of rows eliminated on a given tick, squared.
  * If a user eliminates a row and doesn’t already have a bonus piece available, a bonus piece will be generated at random. If the user presses the correct key, the next falling piece will not be the default next piece, but instead the bonus piece. This works to the user’s advantage because the bonus piece may fit better in the current board state than what would have otherwise been the next piece.
  * Piece landings are “sticky” meaning that once there is a tile occupied directly below any tile belonging to the falling piece, the falling piece will be solidified and the next piece will begin falling.

## Microcontroller Design
The Raspberry Pi 3.0 is in charge of two major tasks: one is updating and maintaining the game state and the other is driving the SPI communication with the FPGA. The code is loosely organized in 3 files: Tetris.c, Board.c, and Tetrislib.h. Tetrislib.h contains some constants and structs used in both Tetris.c and Board.c. Board.c contains all of the functions that update the board state such as move, rotate, tick, eliminateRow, and so on. Tetris.c contains everything else. This includes SPI functions, such as sendBoardState(), which takes the game state, converts it to the LED matrix representation and sends it to the FPGA. Tetris.c contains System Timer functions, such as delayMicrosAndWaitForKeyPress(), which runs a while loop for the duration of a game tick, and within the while loop, checks for new key presses and makes calls to update the game state accordingly. Tetris.c also contains the main method, which drives the flow of the game. Every time something interesting happens (move, rotate, game tick), processTick() is called, which delegates calls to functions in board.c to update the game state.

## FPGA Design
The FPGA is comprised of four primary modules: one that registers keystrokes, one that sends them to the Pi, one that reads in the board state from the Pi, and one that controls the LED board. The former two and the later two operate independently from one another.  
The module that registers keystrokes is adapted from Lab 3. The FPGA registers a keystroke from when the key is first pressed until it is released. at which point the keypad module defaults to ‘D’, which is not used for gameplay. Before passing this information to the Pi, the key pressed is encoded as a byte of the format X000####. X is 1 if a key was pressed and 0 otherwise, and #### is the binary representation of the key just pressed.  The spi_slave module is taken from Prof. Harris’ Digital Design and Computer Architecture. Its purpose is to send data just from the FPGA to the Pi; the byte sent from the Pi to the FPGA during the keyByte transfer is junk and is just meant to initiate the SPI transfer.
Another SPI module “spi_board_slave” takes in 1024 bytes in sequence and populates a 32x32 matrix representing the LED board with the incoming values. The module resets its matrow and matcol counters at 0 so long as the LOAD signal is low. Once the LOAD signal goes high, the counters increment appropriately with the serial clock to store the led matrix data in the right array slot. Once the entire board has been read in through SPI, we latch the board and display it on the LED board. This is done in the the led_matrix module, which is described in more detail in the “New Hardware” section above.


## LED Matrix Technical Details
We used a 32x32 RGB LED Matrix, manufactured by Adafruit. What made it particularly difficult to work with this product was that instead of providing a data sheet with timing diagrams, Adafruit instead provided a tutorial on how to drive the LED matrix using an Arduino Uno. Therefore, in order to control it with an FPGA, we had to look up resources online in which others reverse-engineered the component. This got us pretty far, but to get it working, we had to play around a little with the system ourselves.
The 32x32 matrix requires a 5V power supply and takes in 13 digital signals. The board is split into two parts, top and bottom, that are controlled independently but concurrently by two sets of RGB pins. R0, G0, and B0 control the color of the top half of the board, and R1, G1, and B1, control the bottom half of the board. Four pins, [3:0] A, are used to indicate that row A and row A + 16 are currently under control. More specifically, the LED board is comprised of 32 32-bit shift registers, one for each row. On every SCLK positive edge, the RGB bit values in row A and A + 16 are shifted by one register from column 31 towards column 0. That means that the next RGB values will first be shifted into the column 31 shift register.
After 32 clock cycles, the RGB data must be latched, or else it will simply shift out bit by bit on subsequent clock cycles. To latch the data, both the LATCH and BLANK signals should be driven high on the negative edge of the 32nd clock cycle. Then, on the negative edge of the 33rd LATCH should be driven low, and finally, on the negative edge of the 34th, BLANK should be driven low. We didn’t play around with LATCH very much so we don’t know exactly what it does, but resources we have found have described it as an enable signal, which must be on while the board is being latched in order for the row to light up with the RGB values shifted in.
Following the previous steps will light up a single row for a very short amount of time. In order to light up the whole board, row-multiplexing must be used. After waiting a bit of time, the value of A should be incremented to light up the next two rows and the process can be repeated exactly as above until data has been latched for every row. In order to prevent flickering, the resources we found suggested to repeat this entire process 100-200 times. We suggest that in order to ensure that all of the rows light up with the same intensity, that the row-multiplexing counter should be about 16 times slower than the counter that determines the interval between rounds of lighting up the board. That way, all of the rows are given about equal time for illumination.

## HDL Block Diagram
A block diagram showing the logic that the FPGA synthesizes from the SystemVerilog code
||Insert Image Here||

## Hardware Schematic
A schematic showing the wiring connections made between the hardware components
||Insert Image Here||

## Results
