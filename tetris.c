#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "tetrislib.h"

/////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////

// GPIO FSEL Types
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2

#define GPFSEL   ((volatile unsigned int *) (gpio + 0))
#define GPSET    ((volatile unsigned int *) (gpio + 7))
#define GPCLR    ((volatile unsigned int *) (gpio + 10))
#define GPLEV    ((volatile unsigned int *) (gpio + 13))
#define INPUT  0
#define OUTPUT 1

// Physical addresses
#define BCM2836_PERI_BASE        0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define TIMER_BASE		        (BCM2836_PERI_BASE + 0x3000)
#define SPI_BASE 		(GPIO_BASE + 0x4000)
#define BLOCK_SIZE              (4*1024)

#define MOVE_LEFT 0x7
#define MOVE_RIGHT 0x9
#define ROTATE_CCW 0x8
#define ROTATE_CW 0x5

volatile unsigned int *spi0; //pointer to base of spi0

// A struct to readably access relevant bits in the spi0 chip select register
typedef struct
{
	unsigned 			:7;
	unsigned TA			:1;
	unsigned			:8;
	unsigned DONE			:1;
	unsigned 			:15;
} spi0csbits;

#define SPI0CSbits (* (volatile spi0csbits*) (spi0 + 0))
#define SPI0CS (* (volatile unsigned int*) (spi0 + 0))
#define SPI0FIFO (* (volatile unsigned int*) (spi0 + 1))
#define SPI0CLK (* (volatile unsigned int*) (spi0 + 2))

#define SCLK_FREQ 150000

#define JUNK_BYTE	   0b00000000

#define KEYCHECK_INTERVAL_MICROS 10000

#define TICK_LENGTH_SECONDS 1.0

// Pointers that will be memory mapped when pioInit() is called
volatile unsigned int *gpio; //pointer to base of gpio
volatile unsigned int *sys_timer; // pointer to base of system timer

const unsigned int CLK_FREQ = 1200000000;

void pinMode(int pin, int function)
{
	unsigned index, shift;
	
	// make sure the range of the pin to is [0, 53]
	if (pin > 53 || pin < 0) {
	      printf("Pin out of range, must be 0-53 \n");
	      exit(-1);
	 }

	// find the position of the gpio pin
	index = pin / 10; 
	shift = (pin % 10) * 3;

	// put function to the found positions
	gpio[index] &= ~(((~function) & 7) << shift);
	gpio[index] |= function << shift;	
}

void spi0Init() {
	int  mem_fd;
	void *reg_map;

	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}

	reg_map = mmap(
	  NULL,             //Address at which to start local mapping (null means don't-care)
      BLOCK_SIZE,       //Size of mapped memory block
      PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
      MAP_SHARED,       // This program does not have exclusive access to this memory
      mem_fd,           // Map to /dev/mem
      SPI_BASE);       // Offset to GPIO peripheral

	if (reg_map == MAP_FAILED) {
      printf("gpio mmap error %d\n", (int)reg_map);
      close(mem_fd);
      exit(-1);
    }

	spi0 = (volatile unsigned *)reg_map;

	// From easyPIO.h

	// Set GPIO 8 (CE), 9 (MISO), 10 (MOSI), 11 (SCLK) alt fxn 0 (SPI0)
	pinMode(8, ALT0);
	pinMode(9, ALT0);
	pinMode(10, ALT0);
	pinMode(11, ALT0);

	/*
	 * SPI0CLK is a clock divider.
	 * Therefore, the value that SPI0CLK should be assigned is the
	 * max clock frequency divided by the desired clock frequency
	 */
	SPI0CLK = 250000000 / SCLK_FREQ;

	// Reset SPI settings
	SPI0CS = 0;

	// Enable the SPI
	SPI0CSbits.TA = 1;
}

void pioInit() {
	int  mem_fd;
	void *reg_map;

	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}

	reg_map = mmap(
	  NULL,             //Address at which to start local mapping (null means don't-care)
      BLOCK_SIZE,       //Size of mapped memory block
      PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
      MAP_SHARED,       // This program does not have exclusive access to this memory
      mem_fd,           // Map to /dev/mem
      GPIO_BASE);       // Offset to GPIO peripheral

	if (reg_map == MAP_FAILED) {
      printf("gpio mmap error %d\n", (int)reg_map);
      close(mem_fd);
      exit(-1);
    }

	gpio = (volatile unsigned *)reg_map;
}

void timerInit() {
	int  mem_fd;
	void *reg_map;

	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}

	reg_map = mmap(
	  NULL,             //Address at which to start local mapping (null means don't-care)
      BLOCK_SIZE,       //Size of mapped memory block
      PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
      MAP_SHARED,       // This program does not have exclusive access to this memory
      mem_fd,           // Map to /dev/mem
      TIMER_BASE);       // Offset to system timer

	if (reg_map == MAP_FAILED) {
      printf("gpio mmap error %d\n", (int)reg_map);
      close(mem_fd);
      exit(-1);
    }

	sys_timer = (volatile unsigned *)reg_map;
}

void digitalWrite(int pin, int val) {
	int reg = pin / 32;
	int offset = pin % 32;
	if (val) GPSET[reg] = 1 << offset;
	else GPCLR[reg] = 1 << offset;
}

char spiReceive() {
	SPI0FIFO = JUNK_BYTE;
	while(!SPI0CSbits.DONE);
	// printf("%d,", SPI0FIFO);
	return SPI0FIFO;
}

// THIS NEEDS TO BE CHANGED TO ALSO WATCH FOR KEY PRESSES
void delayMicrosAndWaitForKeyPress(unsigned int micros, FallingPiece* fallingPiece, 
					char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;

	sys_timer[6] = sys_timer[1] + KEYCHECK_INTERVAL_MICROS;
	sys_timer[0] &= 0b1000;

	// printf("Before While!\n");

	while(!(sys_timer[0] & 0b0010)) {
		if(sys_timer[0] & 0b1000) {
			// printf("Before Receive!\n");
			char keyByte = spiReceive();
			// printf("After Receive!\n");
			if(keyByte >> 7) {
				char keyCode = (keyByte & 0b1111);
				switch(keyCode) {
					case MOVE_LEFT:
						move(fallingPiece, false, board);
						break;
					case MOVE_RIGHT:
						move(fallingPiece, true, board);
						break;
					case ROTATE_CCW:
						rotate(fallingPiece, false, board);
						break;
					case ROTATE_CW:
						rotate(fallingPiece, true, board);
						break;
				}
			}
			sys_timer[6] = sys_timer[1] + KEYCHECK_INTERVAL_MICROS;
			sys_timer[0] &= 0b1000;
		}
	}
}

void delaySecondsAndWaitForKeyPress(double seconds, FallingPiece* fallingPiece,
					char board[BOARD_HEIGHT][BOARD_WIDTH]) {
    delayMicrosAndWaitForKeyPress((int) (seconds * 1000000), fallingPiece, board);
}

void delayMicros(unsigned int micros) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;
	while(!(sys_timer[0] & 0b0010));
}

void delaySeconds(double seconds) {
	delayMicros((int) (seconds * 1000000));
}

void displays(FallingPiece* fallingPiece, FallingPiece* nextPiece, char board[BOARD_HEIGHT][BOARD_WIDTH], int score) {
	printf("********************************************************\n");
	printf("Game State\n\n");
	displayBoard(fallingPiece, board);
	printf("Score: %d\n\n", score);
	displayPiece(nextPiece);
	printf("********************************************************\n");
}

void main(void) {
	pioInit();
	spi0Init();
	timerInit();

	srand(time(NULL));

	FallingPiece fallingPiece, nextPiece;
    
	char board[BOARD_HEIGHT][BOARD_WIDTH];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	newFallingPiece(&nextPiece);
	
	int score = 0;

	bool gameOver = false;

	displays(&fallingPiece, &nextPiece, board, score);

	while(!gameOver) {
		// printf("AA\n");
 		delaySecondsAndWaitForKeyPress(TICK_LENGTH_SECONDS, &fallingPiece, board);
		// printf("BB\n");
 		int rowsEliminatedOnTick = tick(&fallingPiece, &nextPiece, board);

		displays(&fallingPiece, &nextPiece, board, score);

		if(rowsEliminatedOnTick == -1) {
			gameOver = true;
		}
		else if(rowsEliminatedOnTick == -2) {
			// This is not a piece transition, Nothing to do here
		}
		else {
			score += rowsEliminatedOnTick;
			fallingPiece = nextPiece;
			newFallingPiece(&nextPiece);
			delaySeconds(TICK_LENGTH_SECONDS);
			displays(&fallingPiece, &nextPiece, board, score);
		}
     	}

	printf("DONE\n");
}

/*
	5 = rotateCW 1000
	7 = moveLeft 0100
	8 = rotateCCW 0010
	9 = moveRightRight 0001
	0 = accelerateDownward 1111
*/
void main2(void) {
	pioInit();
	timerInit();
	spi0Init();

	srand(time(NULL));

	FallingPiece fallingPiece, nextPiece/*, bonusPiece*/;

	// A flag indicating that there is a available bonus piece
	// bool hasBonus = false;
    
	char board[BOARD_HEIGHT][BOARD_WIDTH];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	newFallingPiece(&nextPiece);
	
	int tickLengthSeconds = 1;
	int score = 0;
	// int bonusPiecePotential = 0;

	bool gameOver = false;
	// bool useBonus = false;

	displays(&fallingPiece, &nextPiece, board, score);

	while(!gameOver) {
		printf("\nEnter a one-letter command:\n");
		printf("\ta: Move Left\n");
		printf("\td: Move Right\n");
		printf("\tw: Rotate Clockwise\n");
		printf("\ts: Rotate Counterclockwise\n");
		printf("\tb: Use bonus piece (if available) for next piece\n");
		printf("\tt: Tick\n");
		printf("\n");
		
		int selection = getchar();
		int rowsEliminatedOnTick;
	
		switch((char) selection) {
			case 'a':
				move(&fallingPiece, false, board);
				break;
			case 'd':
				move(&fallingPiece, true, board);
				break;
			case 'w':
				rotate(&fallingPiece, true, board);
				break;
			case 's':
				rotate(&fallingPiece, false, board);
				break;
			case 'b':
				// useBonus = true;
				break;
			case 't':
				rowsEliminatedOnTick = tick(&fallingPiece, &nextPiece, board/*, &bonusPiece, &useBonus, &hasBonus*/);
				if(rowsEliminatedOnTick == -1) {
					gameOver = true;
				}
				else if(rowsEliminatedOnTick == -2) {
					// This is not a piece transition, Nothing to do here
				}
				else {
					score += rowsEliminatedOnTick;
					newFallingPiece(&fallingPiece);
					newFallingPiece(&nextPiece);
				}
				/*
				if(!hasBonus) {
					bonusPiecePotential += rowsEliminatedOnTick;
				}
				if(bonusPiecePotential >= NEEDED_BONUS_PIECE_POTENTIAL) {
					newFallingPiece(&bonusPiece);
					bonusPiecePotential = 0;
					hasBonus = true;
				}
				*/
				break;
			
		}		

		displays(&fallingPiece, &nextPiece, board, score);
		// displayPiece(&bonusPiece);
    	}

	printf("DONE\n");
}


// Not Needed //////////////////////////////////////////////////////////////////////////

/*
void playnote(int pitch, int duration) {
	sys_timer[6] = sys_timer[1] + duration * 1000;
	sys_timer[0] &= 0b1000;
	if(pitch) {
		while(!(sys_timer[0] & 0b1000)) {
			digitalWrite(26, 1);
	
			delayMicros(1000000/pitch/2);
	
			digitalWrite(26, 0);
	
			delayMicros(1000000/pitch/2);
		}
	}
	else {
		delayMicros(duration * 1000);
	}
}
*/