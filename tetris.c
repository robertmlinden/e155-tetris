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
#define BLOCK_SIZE              (4*1024)

// Pointers that will be memory mapped when pioInit() is called
volatile unsigned int *gpio; //pointer to base of gpio
volatile unsigned int *sys_timer; // pointer to base of system timer

const unsigned int CLK_FREQ = 1200000000;

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


void pinmode(int pin_num, int mode) {
	GPFSEL[pin_num / 10] |= (mode << ((pin_num % 10) * 3));
}

void digitalWrite(int pin, int val) {
	int reg = pin / 32;
	int offset = pin % 32;
	if (val) GPSET[reg] = 1 << offset;
	else GPCLR[reg] = 1 << offset;
}

// THIS NEEDS TO BE CHANGED TO ALSO WATCH FOR KEY PRESSES
void delayMicros(unsigned int micros) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;
	while(!(sys_timer[0] & 0b0010));
}

void delaySeconds(double seconds) {
    delayMicros((int) seconds * 1000000);
}

void main(void) {
	pioInit();
	timerInit();

	// Set pin 26 to output
	// pinmode(26, OUTPUT);

	srand(time(NULL));

	FallingPiece fallingPiece/*, bonusPiece*/;

	// A flag indicating that there is a available bonus piece
	// bool hasBonus = false;
    
	char board[BOARD_WIDTH][BOARD_HEIGHT];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	
	int tickLengthSeconds = 1;
	int score = 0;
	// int bonusPiecePotential = 0;

	bool gameOver = false;
	// bool useBonus = false;

	while(!gameOver) {
		printf("Enter a one-letter command:\n");
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
				rowsEliminatedOnTick = tick(&fallingPiece, board/*, &bonusPiece, &useBonus, &hasBonus*/);
				if(rowsEliminatedOnTick == -1) {
					gameOver = true;
				}
				score += rowsEliminatedOnTick;
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

		printf("AAAA\n");
		// delaySeconds(tickLengthSeconds);
	        displayBoard(&fallingPiece, board);
		// displayBonusPiece(&bonusPiece);

		printf("Score: %d\n\n", score);
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