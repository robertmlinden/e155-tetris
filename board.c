#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tetrislib.h"


char PIECES[NUM_PIECES][NUM_ROTATIONS][PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE] = 
{
	{
	// S
		{{' ', ' ', ' ', ' '},
		 {' ', 'S', 'S', ' '},
		 {'S', 'S', ' ', ' '},
		 {' ', ' ', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', 'S', ' ', ' '},
		 {' ', 'S', 'S', ' '},
		 {' ', ' ', 'S', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {' ', 'S', 'S', ' '},
		 {'S', 'S', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {'S', ' ', ' ', ' '},
		 {'S', 'S', ' ', ' '},
		 {' ', 'S', ' ', ' '}}
	},
	
	{
	// Z
		{{' ', ' ', ' ', ' '},
		 {'Z', 'Z', ' ', ' '},
		 {' ', 'Z', 'Z', ' '},
		 {' ', ' ', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', 'Z', ' '},
		 {' ', 'Z', 'Z', ' '},
		 {' ', 'Z', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'Z', 'Z', ' ', ' '},
		 {' ', 'Z', 'Z', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', 'Z', ' ', ' '},
		 {'Z', 'Z', ' ', ' '},
		 {'Z', ' ', ' ', ' '}}
	},
	
	{
	// I
		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'I', 'I', 'I', 'I'},
		 {' ', ' ', ' ', ' '}},

		{{' ', 'I', ' ', ' '},
		 {' ', 'I', ' ', ' '},
		 {' ', 'I', ' ', ' '},
		 {' ', 'I', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {'I', 'I', 'I', 'I'},
		 {' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '}},

		 {{' ', ' ', 'I', ' '},
		 {' ', ' ', 'I', ' '},
		 {' ', ' ', 'I', ' '},
		 {' ', ' ', 'I', ' '}}
	},
	
	{
	// O
		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'O', 'O', ' ', ' '},
		 {'O', 'O', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'O', 'O', ' ', ' '},
		 {'O', 'O', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'O', 'O', ' ', ' '},
		 {'O', 'O', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'O', 'O', ' ', ' '},
		 {'O', 'O', ' ', ' '}}
	},
	
	{
	// T
		{{' ', ' ', ' ', ' '},
		 {' ', 'T', ' ', ' '},
		 {'T', 'T', 'T', ' '},
		 {' ', ' ', ' ', ' '}},	

		{{' ', ' ', ' ', ' '},
		 {' ', 'T', ' ', ' '},
		 {' ', 'T', 'T', ' '},
		 {' ', 'T', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'T', 'T', 'T', ' '},
		 {' ', 'T', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', 'T', ' ', ' '},
		 {'T', 'T', ' ', ' '},
		 {' ', 'T', ' ', ' '}}
	},
	
	{
	// L
		{{' ', ' ', ' ', ' '},
		 {' ', ' ', 'L', ' '},
		 {'L', 'L', 'L', ' '},
		 {' ', ' ', ' ', ' '}},	
	
		{{' ', ' ', ' ', ' '},
		 {' ', 'L', ' ', ' '},
		 {' ', 'L', ' ', ' '},
		 {' ', 'L', 'L', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'L', 'L', 'L', ' '},
		 {'L', ' ', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {'L', 'L', ' ', ' '},
		 {' ', 'L', ' ', ' '},
		 {' ', 'L', ' ', ' '}}
	},
	
	{
	// J
		{{' ', ' ', ' ', ' '},
		 {'J', ' ', ' ', ' '},
		 {'J', 'J', 'J', ' '},
		 {' ', ' ', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', 'J', 'J', ' '},
		 {' ', 'J', ' ', ' '},
		 {' ', 'J', ' ', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', ' ', ' ', ' '},
		 {'J', 'J', 'J', ' '},
		 {' ', ' ', 'J', ' '}},

		{{' ', ' ', ' ', ' '},
		 {' ', 'J', ' ', ' '},
		 {' ', 'J', ' ', ' '},
		 {'J', 'J', ' ', ' '}}
	}
};

// Rotate
// Gravity
// Move
// Level elimintation
// Start at top
// Stop moving when they hit the bottom or another piece
// Acceleration downwards?
// floating piece on the bottom?
// Another button to just lead-drop the piece?

void initBoard(char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	int r, c;
	for(r = 0; r < BOARD_HEIGHT; r++) {
		for(c = 0; c < BOARD_WIDTH; c++) {
			if(r == 0 || r == BOARD_HEIGHT - 1 || c == 0 || c == BOARD_WIDTH - 1) {
				board[r][c] = '#';
			}
			else {
				board[r][c] = ' ';
			}
		}
	}
}

// NEED TO ADJUST TO ACCOUNT FOR THE FACT THAT THERE MAY BE PIECES IN THE STARTING SPOT
void newFallingPiece(FallingPiece* fallingPiece) {
	// srand SHOULD ONLY BE CALLED ONCE!! MOVE THIS TO A ONE-TIME PLACE!!!
	srand(time(NULL));
	fallingPiece->pieceShape = rand() % NUM_PIECES;    //returns a pseudo-random integer between 0 and RAND_MAX
	fallingPiece->rotation = 0;

	// DETERMINE EXACT COORDINATES FOR EACH STARTING PIECE
	// THIS IS JUST A SIMPLE WORKAROUND FOR NOW
	fallingPiece->r = 0;
	fallingPiece->c = (BOARD_WIDTH / 2) - 2;
}

bool solidifyFallingPiece(FallingPiece* fallingPiece, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	memcpy(piece, PIECES[fallingPiece->pieceShape][fallingPiece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardR = r + fallingPiece->r;
			int boardC = c + fallingPiece->c;
			if(piece[r][c] != ' ') {
				if(boardR <= 1) {
					return false;
				}
				board[boardR][boardC] = piece[r][c];
			}
		}
	}
	return true;
}

// Niavely stops a piece when it makes contact with one below
// Returns true if the user can keep PLAYING
// Returns false if the user lost
bool tick(FallingPiece* fallingPiece, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	fallingPiece -> r = (fallingPiece -> r) + 1;
	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	memcpy(piece, PIECES[fallingPiece->pieceShape][fallingPiece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterTick = r + fallingPiece->r;
			int boardCAfterTick = c + fallingPiece->c;
			if(piece[r][c] != ' ' && board[boardRAfterTick + 1][boardCAfterTick] != ' ') {
				// The piece needs to stop falling here, so transition to new falling piece
				bool continueGame = solidifyFallingPiece(fallingPiece, board);
				if(!continueGame) {
					return false;
				}
				lineCheck(board);
				newFallingPiece(fallingPiece);
				return true;
			}
		}
	}
}

bool move(FallingPiece* fallingPiece, bool moveRight, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	int h_offset = moveRight ? 1 : -1;
	
	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	memcpy(piece, PIECES[fallingPiece->pieceShape][fallingPiece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterMove = r + fallingPiece->r;
			int boardCAfterMove = c + fallingPiece->c + h_offset;
			if(piece[r][c] != ' ' && board[boardRAfterMove][boardCAfterMove] != ' ') {
				return false;
			}
		}
	}

	// The move was valid!
	fallingPiece -> c += h_offset;
	return true;
}

// STILL NEED TO IMPLEMENT ADJUSTING A PIECE IF SIMPLE ROTATION IS INVALID BUT ADDING A 1 OR 2 SQUARE TRANSLATION WOULD IN FACT MAKE IT VALID
// VERIFY THIS BY PLAYING SOME TETRIS!
bool rotate(FallingPiece* fallingPiece, bool rotateClockwise, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	// Temporarily rotate the falling piece to see if then new configuration is valid on the board
	if(rotateClockwise) {
		fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
	}
	else {
		fallingPiece -> rotation = (fallingPiece -> rotation + 3) % 4;
	}

	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	memcpy(piece, PIECES[fallingPiece->pieceShape][fallingPiece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);

	// Check to see if configuration is non-overlapping and in-bounds
	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterRotate = r + fallingPiece->r;
			int boardCAfterRotate = c + fallingPiece->c;
			if(!isInBounds(boardRAfterRotate, boardCAfterRotate) || (piece[r][c] != ' ' && board[boardRAfterRotate][boardCAfterRotate] != ' ')) {
				// Rotate the piece back because it was invalid
				if(rotateClockwise) {
					fallingPiece -> rotation = (fallingPiece -> rotation + 3) % 4;
				}
				else {
					fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
				}
				return false;
			}
		}
	}

	// The rotation was valid!
	return true;
}

// Allow r <= 0 for start placement
// Can utilize isInSquare() if appropriate
bool isInBounds(r, c) {
	return r <= BOARD_HEIGHT - 2  && c >= 1 && c <= BOARD_HEIGHT - 2;
}

void displayBoard(FallingPiece* fallingPiece, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	int fallingPieceRowDisplayBegin = fallingPiece -> r >= 1 ? fallingPiece -> r : 1;
	int fallingPieceRowDisplayEnd = fallingPiece -> r + 3;
	int fallingPieceColDisplayBegin = fallingPiece -> c;
	int fallingPieceColDisplayEnd = fallingPiece -> c + 3;
	
	char piece[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	memcpy(piece, PIECES[fallingPiece->pieceShape][fallingPiece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);

	int r, c;
	for(r = 0; r < BOARD_HEIGHT; r++) {
		for(c = 0; c < BOARD_WIDTH; c++) {
			//if(isInSquare(r, c, fallingPieceRowDisplayBegin, fallingPieceRowDisplayEnd,
			//					fallingPieceColDisplayBegin, fallingPieceColDisplayEnd)) {
			//	char printChar = (board[r][c] == ' ') ? piece[r - fallingPieceRowDisplayBegin][c - fallingPieceColDisplayBegin] :
			//											board[r][c];
			//	printf("%c");
			//}
			//else {
				printf("%c", board[r][c]);
			if(r == 12 && c == 1) {
				printf("%d!", board[r][c] == '#');
			}
		//	}
		}
		printf("\n");
	}
}

bool isInSquare(r, c, rbegin, rend, cbegin, cend) {
	return r >= rbegin && r <= rend && c >= cbegin && c <= cend;
}

void deleteRow(int rDeleted, char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	int r, c;
 	for (r = rDeleted; r > 1; r--)
	{
	        for (c = 1; c < BOARD_WIDTH - 1; c++)
	        {
	            board[r][c] = board[r-1][c];
	        }
    	}   
}

int lineCheck(char board[BOARD_WIDTH][BOARD_HEIGHT]) {
	int r;
	int deleteCount = 0;
	for(r = 1; r < BOARD_HEIGHT-1; r++){
		int c = 1;
		while(c < BOARD_WIDTH - 1){
			if (board[r][c] == ' '){
				break;
			} else {
				c++;
			}
			if (c == BOARD_WIDTH - 1){
				deleteRow(r, board);
				deleteCount++;
				r--;
			}
		}
	}
	return deleteCount;
}
