#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "tetrislib.h"
#include <time.h>

/////////////////////////////////////////////////////////////////////
// GPIO, SPI0, and SYS_TIMER CONSTANTS (taken from easyPIO.h)
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

// Pointers that will be memory mapped when pioInit() is called
volatile unsigned int *gpio; //pointer to base of gpio
volatile unsigned int *sys_timer; // pointer to base of system timer

#define SPI0CSbits (* (volatile spi0csbits*) (spi0 + 0))
#define SPI0CS (* (volatile unsigned int*) (spi0 + 0))
#define SPI0FIFO (* (volatile unsigned int*) (spi0 + 1))
#define SPI0CLK (* (volatile unsigned int*) (spi0 + 2))

#define SCLK_FREQ 150000

/////////////////////////////////////////////////////////////////////
// Game-Related Constants and Variables
/////////////////////////////////////////////////////////////////////

// The game tick length in seconds
#define TICK_LENGTH_SECONDS 0.05

// The amount of rows that need to be eliminated since the last
// use of a bonus piece (or the beginning of the game) before the user
// gets another (their first) bonus piece
#define BONUS_PIECE_POTENTIAL_NEEDED 1

// The length in pixels of the LED matrix (width and height)
#define N 32

/////////////////////////////////////////////////////////////////////
// SPI Constants
/////////////////////////////////////////////////////////////////////

// The pixel coordinates of where the next piece bounding box will appear
#define NEXT_PIECE_LED_ROW_BEGIN 0x15
#define NEXT_PIECE_LED_ROW_END   0x1A
#define NEXT_PIECE_LED_COL_BEGIN 0x15
#define NEXT_PIECE_LED_COL_END   0x1A

// The pixel coordinates of where the bonus piece bounding box will appear
#define BONUS_PIECE_LED_ROW_BEGIN 0x15
#define BONUS_PIECE_LED_ROW_END   0x1A
#define BONUS_PIECE_LED_COL_BEGIN 0x08
#define BONUS_PIECE_LED_COL_END   0x0D

#define JUNK_BYTE	   0b00000000

// The interval with which the Pi checks for new key presses from the FPGA
#define KEYCHECK_INTERVAL_MICROS 100

// High while the SPI bytes representing the board are
// in the process of being sent to the FPGA
#define LOAD_PIN 16

const unsigned int CLK_FREQ = 1200000000;

/////////////////////////////////////////////////////////////////////
// Global Game Variables
/////////////////////////////////////////////////////////////////////
bool acceptNewKeystroke = true;
bool useBonusPiece = false;
bool gameOver = false;

/////////////////////////////////////////////////////////////////////
// GPIO, SPI0, and SYS_TIMER FUNCTIONS (taken from easyPIO.h)
/////////////////////////////////////////////////////////////////////

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

/*
 * Simply stalls for "micros" microseconds
 */
void delayMicros(unsigned int micros) {
	sys_timer[3] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0001;
	while(!(sys_timer[0] & 0b0001));
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


int digitalRead(int pin) {
    int reg = pin / 32;
    int offset = pin % 32;

    return (GPLEV[reg] >> offset) & 0x00000001;
}


char spiSendReceive(char byte) {
	SPI0FIFO = byte;
	while(!SPI0CSbits.DONE);
	return SPI0FIFO;
}


/*
 * Takes in the game board row (as "int* col") and game board col (as "int* row)
 * and returns the corresponding LED board row (as "int* row") 
 * and LED board col (as "int* col).
 *
 * This is currently unused but could at some point become useful
 */

void gameBoardToLedBoardCoords(int* row, int* col) {
	int temp = *col;
	*col = 26 - *row;
	*row = temp + 2;
}


/*
 * Takes in the LED board row (as "int* col") and LED board col (as "int* row)
 * and returns the corresponding game board row (as "int* row") 
 * and game board col (as "int* col).
 *
 * This is used in sending the game state via SPI from the Pi to the FPGA
 * found in the function sendBoardState()
 */
void ledBoardToGameBoardCoords(int* row, int* col) {
	int temp = *col;
	*col = *row - 2;
	*row = 26 - temp;
}


/*
 * Determines whether the coordinates (row, col) fall within the bounds of the
 * tetris board.
 */
bool isBoardSquare(int row, int col) {
	return row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH;
}


/*
 * Sends the game state via SPI to the FPGA
 * Also prints the LED board representation to terminal if the debugging variable
 * just below PRINT_LED_BOARD_REPRESENTATION is true
 */

#define PRINT_LED_BOARD_REPRESENTATION true

void sendBoardState(FallingPiece* fallingPiece, FallingPiece* nextPiece,
			FallingPiece* bonusPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {

	// Send a junk byte to the FPGA for reset purposes
	// This is the only byte sent over while LOAD_PIN is low
	spiSendReceive(JUNK_BYTE);
	digitalWrite(LOAD_PIN, 1);

	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	char nextPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(nextPieceChars, nextPiece);

	char bonusPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	if(bonusPiece -> pieceShape != NONEXISTENT) {
		getPieceChars(bonusPieceChars, bonusPiece);
	}
	
	// LED board row and column
	int lrow, lcol;

	for(lrow = 0; lrow < N; lrow++) {
		for(lcol = 0; lcol < N; lcol++) {
			// Game board row and col corresponding to the LED row and col
			int brow = lrow;
			int bcol = lcol;
			ledBoardToGameBoardCoords(&brow, &bcol);

			char sendChar;

			// If the LED pixel corresponds to one that displays part
			// of the game board, look at the game board to determine
			// the SPI to be sent over
			if(isBoardSquare(brow, bcol)) {
				int fallingPieceRowDisplayBegin = fallingPiece -> r >= 1 ? fallingPiece -> r : 1;
				int fallingPieceRowDisplayEnd = fallingPiece -> r + 3;
				int fallingPieceColDisplayBegin = fallingPiece -> c;
				int fallingPieceColDisplayEnd = fallingPiece -> c + 3;

				if(isInSquare(brow, bcol, fallingPieceRowDisplayBegin, fallingPieceRowDisplayEnd,
								fallingPieceColDisplayBegin, fallingPieceColDisplayEnd)) {
					sendChar = (board[brow][bcol] == ' ') ? 
							fallingPieceChars[brow - fallingPiece -> r][bcol - fallingPiece -> c] :
							board[brow][bcol];
					if(sendChar != ' ') sendChar = 'F';
					spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
					if(PRINT_LED_BOARD_REPRESENTATION) {
						printf("%c", sendChar);
					}
				}
				else {
					sendChar = board[brow][bcol];
					spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
					if(PRINT_LED_BOARD_REPRESENTATION) {
						printf("%c", sendChar);
					}
				}
			}
			// If on the border of the next piece bounding box, send SPI over
			// corresponding to a border
			else if(isOnSquare(lrow, lcol, NEXT_PIECE_LED_ROW_BEGIN, NEXT_PIECE_LED_ROW_END,
							NEXT_PIECE_LED_COL_BEGIN, NEXT_PIECE_LED_COL_END)) {
				sendChar = '#';
				spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
				if(PRINT_LED_BOARD_REPRESENTATION) {
					printf("%c", sendChar);
				}
			}
			// If within the next piece bounding box, send SPI over
			// corresponding to the next piece
			else if(isInSquare(lrow, lcol, NEXT_PIECE_LED_ROW_BEGIN + 1, NEXT_PIECE_LED_ROW_END - 1,
							NEXT_PIECE_LED_COL_BEGIN + 1, NEXT_PIECE_LED_COL_END - 1)) {
				sendChar = nextPieceChars[lrow - (NEXT_PIECE_LED_ROW_BEGIN + 1)][lcol - (NEXT_PIECE_LED_COL_BEGIN + 1)];
				spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
				if(PRINT_LED_BOARD_REPRESENTATION) {
					printf("%c", sendChar);
				}
			}
			// If on the border of the bonus piece bounding box, send SPI over
			// corresponding to a border
			else if(isOnSquare(lrow, lcol, BONUS_PIECE_LED_ROW_BEGIN, BONUS_PIECE_LED_ROW_END,
							BONUS_PIECE_LED_COL_BEGIN, BONUS_PIECE_LED_COL_END)) {
				sendChar = '#';
				spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
				if(PRINT_LED_BOARD_REPRESENTATION) {
					printf("%c", sendChar);
				}
			}
			// If within the bonus piece bounding box, send SPI over
			// corresponding to the bonus piece (if the player has one)
			else if(isInSquare(lrow, lcol, BONUS_PIECE_LED_ROW_BEGIN + 1, BONUS_PIECE_LED_ROW_END - 1,
							BONUS_PIECE_LED_COL_BEGIN + 1, BONUS_PIECE_LED_COL_END - 1) &&
							bonusPiece -> pieceShape != NONEXISTENT) {
				sendChar = bonusPieceChars[lrow - (BONUS_PIECE_LED_ROW_BEGIN + 1)][lcol - (BONUS_PIECE_LED_COL_BEGIN + 1)];
				spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
				if(PRINT_LED_BOARD_REPRESENTATION) {
					printf("%c", sendChar);
				}
			}
			// If none of the above cases are met,
			// the corresponding LED pixel should not be lit
			else {
				sendChar = ' ';
				spiSendReceive((char) (sendChar == ' ' ? 0 : 1));
				if(PRINT_LED_BOARD_REPRESENTATION) {
					printf("%c", sendChar);
				}
			}
		}
		if(PRINT_LED_BOARD_REPRESENTATION) {
			printf("\n");
		}
	}

	// The board SPI transfer is done, so drop LOAD low
	digitalWrite(LOAD_PIN, 0);
}

/*
 * This function drives game ticks by running a while loop for TICK_LENGTH_SECONDS seconds.
 * In order for key presses to be registered, there must be code within the while loop that
 * checks to see if KEYCHECK_INTERVAL_MICROS time has passed. If so, the Pi (SPI Master)
 * sends a junk byte of data to the FPGA via SPI. The purpose of this is to get
 * a meaningful byte of data back from the FPGA representing the key pressed.
 * The Pi then reads the key and updates the game state accordingly (see switch statement).
 */
void delayMicrosAndWaitForKeyPress(unsigned int micros, FallingPiece* fallingPiece,
					FallingPiece* nextPiece, FallingPiece* bonusPiece,
					char board[BOARD_HEIGHT][BOARD_WIDTH], int score) {
	sys_timer[4] = sys_timer[1] + micros;
	sys_timer[0] &= 0b0010;

	sys_timer[6] = sys_timer[1] + KEYCHECK_INTERVAL_MICROS;
	sys_timer[0] &= 0b1000;

	while(!(sys_timer[0] & 0b0010)) {
		if(sys_timer[0] & 0b1000) {
			char keyByte = spiSendReceive(JUNK_BYTE);
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
							if(bonusPiece -> pieceShape != NONEXISTENT) {
								useBonusPiece = true;
							}
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

/*
 * Delegates work to delayMicrosAndWaitForKeyPress()
 */
void delaySecondsAndWaitForKeyPress(double seconds, FallingPiece* fallingPiece,
					FallingPiece* nextPiece,
					FallingPiece* bonusPiece,
					char board[BOARD_HEIGHT][BOARD_WIDTH],
					int score) {
    delayMicrosAndWaitForKeyPress((int) (seconds * 1000000), fallingPiece, nextPiece, bonusPiece, board, score);
}

void main(void) {
	// Set up and initialize Pi memory pointers
	pioInit();
	spi0Init();
	timerInit();

	pinMode(LOAD_PIN, OUTPUT);
	digitalWrite(LOAD_PIN, 0);

	// Seed our random number generator
	srand(time(NULL));

	FallingPiece fallingPiece, nextPiece, bonusPiece;

	// The tetris game board representation
	// Does not represent the falling piece, next piece, bonus piece, or score.
	// Those pieces of information are stored separately.
	char board[BOARD_HEIGHT][BOARD_WIDTH];
	
	initBoard(board);
	newFallingPiece(&fallingPiece);
	newFallingPiece(&nextPiece);

	// Setting pieceShape to NONEXISTENT signifies that the user does not
	// currently have a bonus piece
	bonusPiece.pieceShape = NONEXISTENT;
	
	// Game score
	int score = 0;

	// The amount of rows the user has eliminated since the last bonus
	// piece used (or since the beginning of the game). If this becomes at
	// least as great as BONUS_PIECE_POTENTIAL_NEEDED, then the user
	// is granted a bonus piece to use at will for their next turn
	int bonusPiecePotential = 0;

	displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
	sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);

	while(!gameOver) {
 		delaySecondsAndWaitForKeyPress(TICK_LENGTH_SECONDS, &fallingPiece, &nextPiece, &bonusPiece, board, score);

		// Let gravity tick and check for the number of rows eliminated
 		int rowsEliminatedOnTick = tick(&fallingPiece, board);

		bonusPiecePotential += rowsEliminatedOnTick >= 0 ? rowsEliminatedOnTick : 0;
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

		displays(&fallingPiece, &nextPiece, &bonusPiece, board, score);
		sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);
     	}
	sendBoardState(&fallingPiece, &nextPiece, &bonusPiece, board);
}

/////////////////////////////////////////////////////////////////////
// Future Work
/////////////////////////////////////////////////////////////////////
// Acceleration downwards?
// soft, non-sticky landings?
// Another button to just lead-drop the piece?
// Rotation glitches