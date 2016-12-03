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
#define USE_BONUS 0xB

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

#define KEYCHECK_INTERVAL_MICROS 100

#define TICK_LENGTH_SECONDS 0.4

#define BONUS_PIECE_POTENTIAL_NEEDED 1

#define RESET 12
#define LOAD 21

#define N 32

#define NEXT_PIECE_LED_ROW_BEGIN 0x15
#define NEXT_PIECE_LED_ROW_END   0x1A
#define NEXT_PIECE_LED_COL_BEGIN 0x15
#define NEXT_PIECE_LED_COL_END   0x1A

#define BONUS_PIECE_LED_ROW_BEGIN 0x15
#define BONUS_PIECE_LED_ROW_END   0x1A
#define BONUS_PIECE_LED_COL_BEGIN 0x08
#define BONUS_PIECE_LED_COL_END   0x0D

// Pointers that will be memory mapped when pioInit() is called
volatile unsigned int *gpio; //pointer to base of gpio
volatile unsigned int *sys_timer; // pointer to base of system timer

const unsigned int CLK_FREQ = 1200000000;
bool acceptNewKeystroke = true;
bool useBonusPiece = false;

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

void displays(FallingPiece* fallingPiece, FallingPiece* nextPiece, FallingPiece* bonusPiece,
		char board[BOARD_HEIGHT][BOARD_WIDTH], int score) {
	printf("********************************************************\n");
	printf("Game State\n\n");
	displayBoard(fallingPiece, board);
	printf("Score: %d\n\n", score);
	displayPiece(nextPiece);
	if(bonusPiece -> pieceShape != NONEXISTENT) {
		displayPiece(bonusPiece);
	}
	printf("********************************************************\n");
}

void gameBoardToLedBoardCoords(int* row, int* col) {
	int temp = *col;
	*col = 26 - *row;
	*row = temp + 2;
}

void ledBoardToGameBoardCoords(int* row, int* col) {
	// printf("%d, %d, ", *row, *col);
	int temp = *col;
	*col = *row - 2;
	*row = 26 - temp;
	// printf("%d, %d\n", *row, *col);
}

bool isBoardSquare(int row, int col) {
	return row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH;
}

void delayMicros(unsigned int micros) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;
	while(!(sys_timer[0] & 0b0010));
}

void delaySeconds(double seconds) {
	delayMicros((int) (seconds * 1000000));
}

void digitalWrite(int pin, int val) {
	int reg = pin / 32;
	int offset = pin % 32;
	if (val) GPSET[reg] = 1 << offset;
	else GPCLR[reg] = 1 << offset;
}

char spiSendReceive(char byte) {
	SPI0FIFO = byte;
	while(!SPI0CSbits.DONE);
	// printf("%d,", SPI0FIFO);
	return SPI0FIFO;
}

void spiSendUpdatedPixel(char value, int row, int col) {
	SPI0CSbits.TA = 1;
	spiSendReceive((char) (row & 0xFF));
	spiSendReceive((char) (col & 0xFF));
	spiSendReceive(value);
	SPI0CSbits.TA = 0;
}

void sendBoardState(FallingPiece* fallingPiece, FallingPiece* nextPiece,
			FallingPiece* bonusPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	digitalWrite(RESET, 1);
	digitalWrite(LOAD, 1);
	spiSendReceive(JUNK_BYTE);
	digitalWrite(RESET, 0);

	int numSpacesSent = 0;

	// printf("GOT HERE\n");

	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPiece(piece, fallingPiece);

	// printf("GOT HERE 2\n");	

	char nextPieceLED[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPiece(nextPieceLED, nextPiece);

	// printf("GOT HERE 3\n");

	int nextCount = 0;	

	char bonusPieceLED[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	if(bonusPiece -> pieceShape != NONEXISTENT) {
		getPiece(bonusPieceLED, bonusPiece);
	}
	
	int lrow, lcol;
	for(lrow = 0; lrow < N; lrow++) {
		for(lcol = 0; lcol < N; lcol++) {
			// printf("(%d, %d)\n", lrow, lcol);
			int brow = lrow;
			int bcol = lcol;
			ledBoardToGameBoardCoords(&brow, &bcol);
			if(isBoardSquare(brow, bcol)) {
				int fallingPieceRowDisplayBegin = fallingPiece -> r >= 1 ? fallingPiece -> r : 1;
				int fallingPieceRowDisplayEnd = fallingPiece -> r + 3;
				int fallingPieceColDisplayBegin = fallingPiece -> c;
				int fallingPieceColDisplayEnd = fallingPiece -> c + 3;

				if(isInSquare(brow, bcol, fallingPieceRowDisplayBegin, fallingPieceRowDisplayEnd,
								fallingPieceColDisplayBegin, fallingPieceColDisplayEnd)) {
					char sendChar = (board[brow][bcol] == ' ') ? 
							piece[brow - fallingPiece -> r][bcol - fallingPiece -> c] :
							board[brow][bcol];
					spiSendReceive(sendChar);
					printf("%c", sendChar);
					if(sendChar == ' ') numSpacesSent++;
				}
				else {
					spiSendReceive(board[brow][bcol]);
					printf("%c", board[brow][bcol]);
					if(board[brow][bcol] == ' ') numSpacesSent++;
				}
			
				// spiSendReceive(board[brow][bcol]);
				// printf("%c", board[brow][bcol]);
				// printf("%d,", board[brow][bcol]);
			}
			else if(isOnSquare(lrow, lcol, NEXT_PIECE_LED_ROW_BEGIN, NEXT_PIECE_LED_ROW_END,
							NEXT_PIECE_LED_COL_BEGIN, NEXT_PIECE_LED_COL_END)) {
				spiSendReceive('#');
				printf("#");
			}
			else if(isInSquare(lrow, lcol, NEXT_PIECE_LED_ROW_BEGIN + 1, NEXT_PIECE_LED_ROW_END - 1,
							NEXT_PIECE_LED_COL_BEGIN + 1, NEXT_PIECE_LED_COL_END - 1)) {
				char sendChar = nextPieceLED[lrow - (NEXT_PIECE_LED_ROW_BEGIN + 1)][lcol - (NEXT_PIECE_LED_COL_BEGIN + 1)];
				spiSendReceive(sendChar);
				printf("%c", sendChar);
				if(sendChar == ' ') numSpacesSent++;
				else nextCount++;
			}
			else if(isOnSquare(lrow, lcol, BONUS_PIECE_LED_ROW_BEGIN, BONUS_PIECE_LED_ROW_END,
							BONUS_PIECE_LED_COL_BEGIN, BONUS_PIECE_LED_COL_END)) {
				spiSendReceive('#');
				printf("#");
			}
			else if(isInSquare(lrow, lcol, BONUS_PIECE_LED_ROW_BEGIN + 1, BONUS_PIECE_LED_ROW_END - 1,
							BONUS_PIECE_LED_COL_BEGIN + 1, BONUS_PIECE_LED_COL_END - 1) &&
							bonusPiece -> pieceShape != NONEXISTENT) {
				char sendChar = bonusPieceLED[lrow - (BONUS_PIECE_LED_ROW_BEGIN + 1)][lcol - (BONUS_PIECE_LED_COL_BEGIN + 1)];
				spiSendReceive(sendChar);
				printf("%c", sendChar);
				if(sendChar == ' ') numSpacesSent++;
			}
			else {
				spiSendReceive(' ');
				printf(" ");
				numSpacesSent++;
			}
		}
		printf("\n");
	}
	digitalWrite(LOAD, 0);
	printf("Next Count = %d\n", nextCount);
	printf("Number of spaces sent: %d\n", numSpacesSent);
}

// THIS NEEDS TO BE CHANGED TO ALSO WATCH FOR KEY PRESSES
void delayMicrosAndWaitForKeyPress(unsigned int micros, FallingPiece* fallingPiece,
					FallingPiece* nextPiece, FallingPiece* bonusPiece,
					char board[BOARD_HEIGHT][BOARD_WIDTH], int score) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;

	sys_timer[6] = sys_timer[1] + KEYCHECK_INTERVAL_MICROS;
	sys_timer[0] &= 0b1000;

	// printf("Before While!\n");

	while(!(sys_timer[0] & 0b0010)) {
		if(sys_timer[0] & 0b1000) {
			// printf("Before Receive!\n");
			char keyByte = spiSendReceive(JUNK_BYTE);
			// printf("After Receive!\n");
			// printf("Key Byte = \"%d\", ", keyByte);
			if(keyByte >> 7) {
				if(acceptNewKeystroke) {
					acceptNewKeystroke = false;
					char keyCode = (keyByte & 0b1111);
					switch(keyCode) {
						case MOVE_LEFT:
							move(fallingPiece, false, board);
							sendBoardState(fallingPiece, nextPiece, bonusPiece, board);
							displays(fallingPiece, nextPiece, bonusPiece, board, score);
							break;
						case MOVE_RIGHT:
							move(fallingPiece, true, board);
							sendBoardState(fallingPiece, nextPiece, bonusPiece, board);
							displays(fallingPiece, nextPiece, bonusPiece, board, score);
							break;
						case ROTATE_CCW:
							rotate(fallingPiece, false, board);
							sendBoardState(fallingPiece, nextPiece, bonusPiece, board);
							displays(fallingPiece, nextPiece, bonusPiece, board, score);
							break;
						case ROTATE_CW:
							rotate(fallingPiece, true, board);
							sendBoardState(fallingPiece, nextPiece, bonusPiece, board);
							displays(fallingPiece, nextPiece, bonusPiece, board, score);
							break;
						case USE_BONUS:
							useBonusPiece = true;
							break;
					}
				}
				
			}
			else {
				acceptNewKeystroke = true;
			}
			sys_timer[6] = sys_timer[1] + KEYCHECK_INTERVAL_MICROS;
			sys_timer[0] &= 0b1000;
		}
	}
}

void delaySecondsAndWaitForKeyPress(double seconds, FallingPiece* fallingPiece,
					FallingPiece* nextPiece,
					FallingPiece* bonusPiece,
					char board[BOARD_HEIGHT][BOARD_WIDTH],
					int score) {
    delayMicrosAndWaitForKeyPress((int) (seconds * 1000000), fallingPiece, nextPiece, bonusPiece, board, score);
}

void main(void) {
	pioInit();
	spi0Init();
	timerInit();

	digitalWrite(LOAD, 0);
	digitalWrite(RESET, 0);

	// Seed our random number generator
	srand(time(NULL));

	FallingPiece fallingPiece, nextPiece, bonusPiece;
    
	char board[BOARD_HEIGHT][BOARD_WIDTH];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	newFallingPiece(&nextPiece);
	bonusPiece.pieceShape = NONEXISTENT;

	// LedPixel* changedPixels = malloc(32 * 32 * sizeof(LedPixel));
	// initChangedPixels(changedPixels, board);
	
	int score = 0;

	int bonusPiecePotential = 0;

	bool gameOver = false;

	displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
	// printf("GOT HERE\n");
	sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);

	printf("BOARD STATE SENT!!\n");

	while(!gameOver) {
		// printf("AA\n");
 		delaySecondsAndWaitForKeyPress(TICK_LENGTH_SECONDS, &fallingPiece, &nextPiece, &bonusPiece, board, score);
		// printf("BB\n");
 		int rowsEliminatedOnTick = tick(&fallingPiece, &nextPiece, board);

		bonusPiecePotential += rowsEliminatedOnTick >= 0 ? rowsEliminatedOnTick : 0;
		if(bonusPiecePotential >= BONUS_PIECE_POTENTIAL_NEEDED &&
						bonusPiece.pieceShape == NONEXISTENT) {
			newFallingPiece(&bonusPiece);
		}

		displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
		sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);
		printf("BOARD STATE SENT!!\n");

		if(rowsEliminatedOnTick == -1) {
			gameOver = true;
		}
		else if(rowsEliminatedOnTick == -2) {
			// This is not a piece transition, Nothing to do here
		}
		else {
			score += rowsEliminatedOnTick;

			delaySeconds(TICK_LENGTH_SECONDS);

			if(useBonusPiece) {
				fallingPiece = bonusPiece;
				bonusPiece.pieceShape = NONEXISTENT;
				useBonusPiece = false;
				bonusPiecePotential = 0;
			}
			else {
				fallingPiece = nextPiece;
				newFallingPiece(&nextPiece);
			}

			displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
			sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);
			printf("BOARD STATE SENT!!\n");
		}
     	}

	printf("DONE\n");
	// free(changedPixels);
}

void main2(void) {
	pioInit();
	timerInit();
	spi0Init();

	srand(time(NULL));

	FallingPiece fallingPiece, nextPiece, bonusPiece;

	// A flag indicating that there is a available bonus piece
	// bool hasBonus = false;
    
	char board[BOARD_HEIGHT][BOARD_WIDTH];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	newFallingPiece(&nextPiece);
	bonusPiece.pieceShape = NONEXISTENT;
	
	int tickLengthSeconds = 1;
	int score = 0;
	int bonusPiecePotential = 0;

	bool gameOver = false;

	displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);

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
				if(bonusPiecePotential >= BONUS_PIECE_POTENTIAL_NEEDED) {
					useBonusPiece = true;
					printf("ACTIVATED!!\n");
				}
				break;
			case 't':
				rowsEliminatedOnTick = tick(&fallingPiece, &nextPiece, board);
				bonusPiecePotential += rowsEliminatedOnTick >= 0 ? rowsEliminatedOnTick : 0;
				printf("Bonus Piece Potential = %d\n", bonusPiecePotential);
				if(bonusPiecePotential >= BONUS_PIECE_POTENTIAL_NEEDED &&
						bonusPiece.pieceShape == NONEXISTENT) {
					newFallingPiece(&bonusPiece);
				}

				if(rowsEliminatedOnTick == -1) {
					gameOver = true;
				}
				else if(rowsEliminatedOnTick == -2) {
					// This is not a piece transition, Nothing to do here
				}
				else {
					score += rowsEliminatedOnTick;
					if(useBonusPiece) {
						fallingPiece = bonusPiece;
						bonusPiece.pieceShape = NONEXISTENT;
						useBonusPiece = false;
						bonusPiecePotential = 0;
					}
					else {
						fallingPiece = nextPiece;
						newFallingPiece(&nextPiece);
					}
				}
				
				break;
			
		}		

		displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
    	}

	printf("DONE\n");
}


/////////////////////////////////////////////////////

void initChangedPixels(LedPixel* changedPixels, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	int lrow, lcol;
	for(lrow = 0; lrow < N; lrow++) {
		for(lcol = 0; lcol < N; lcol++) {
			int row, col;
			ledBoardToGameBoardCoords(&row, &col);

			int index = (lrow * N) + lcol;

			LedPixel ledPixel;
			ledPixel.row = row;
			ledPixel.col = col;

			if(isBoardSquare(row, col)) {				
				ledPixel.value = board[row][col];
			}
			else {
				 ledPixel.value = ' ';
			}
			changedPixels[index] = ledPixel;
		}
	}
}