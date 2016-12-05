#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tetrislib.h"

/*
 * The 4x4 matrix representation for each of the 7 pieces and their rotations.
 * The matrix must be 4x4 so that all pieces and pieces rotations have a consistent
 * representation size.
 * 
 * Pieces are indexed as PIECES[PIECE_NUMBER][ROTATION_NUMBER]
 * Piece numbers are indexed using the constants below
 * Piece rotations are indexed in the following way:
 *     0 is the default starting rotation.
 *     Rotation N + 1 is N rotated clockwise
 */

#define S 0
#define Z 1
#define I 2
#define O 3
#define T 4
#define L 5
#define J 6

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


/*
 * Initializes the board with its bounding box
 */
void initBoard(char board[BOARD_HEIGHT][BOARD_WIDTH]) {
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


/*
 * A helper function that takes a FallingPiece (information about the piece)
 * And returns (via the "pieceChars" matrix) a 4x4 character representation of the piece
 */
void getPieceChars(char pieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE], FallingPiece* piece) {
	memcpy(pieceChars, PIECES[piece->pieceShape][piece->rotation], 
		sizeof(char) * PIECE_BLOCK_SIZE * PIECE_BLOCK_SIZE);
}


/*
 * Resets the falling piece to a new random piece starting on the top of the board
 */
void newFallingPiece(FallingPiece* fallingPiece) {
	// Get a random piece shape for falling piece
	fallingPiece->pieceShape = rand() % NUM_PIECES;

	// Initialize rotation of falling piece to default orientation
	fallingPiece->rotation = 0;

	// Determine exact coordinates for each starting piece based on piece shape
	if (fallingPiece->pieceShape == S) {
		fallingPiece->r = -2;
		fallingPiece->c = 5;
	} else if (fallingPiece->pieceShape == Z){
		fallingPiece->r = -2;
		fallingPiece->c = 4;
	} else if (fallingPiece->pieceShape == I){
		fallingPiece->r = -2;
		fallingPiece->c = 4;
	} else if (fallingPiece->pieceShape == O){
		fallingPiece->r = -3;
		fallingPiece->c = 5;
	} else if (fallingPiece->pieceShape == T){
		fallingPiece->r = -2;
		fallingPiece->c = 4;
	} else if (fallingPiece->pieceShape == L){
		fallingPiece->r = -2;
		fallingPiece->c = 5;
	} else if (fallingPiece->pieceShape == J){
		fallingPiece->r = -2;
		fallingPiece->c = 4;
	} else {
		fallingPiece->r = 0;
		fallingPiece->c = 6;
	}

}


/*
 * Takes a falling piece which has just landed and transfers representation of the
 * piece from fallingPiece to board. This is in preparation for a new falling piece
 * to be generated.
 */
bool solidifyFallingPiece(FallingPiece* fallingPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardR = r + fallingPiece->r;
			int boardC = c + fallingPiece->c;
			if(fallingPieceChars[r][c] != ' ') {
				printf("(%d, %d)\n", boardR, boardC); 
				if(boardR <= 1) {
					return false;
				}
				board[boardR][boardC] = fallingPieceChars[r][c];
			}
		}
	}
	return true;
}


/*
 * Niavely stops a piece when it makes contact with one below. This is commonly knows as "sticky landings"
 * Returns -1 if the user loses on this tick (if any part of the currently falling piece lands
 *      on the top row of the board)
 * Returns the number of rows eliminated due to the tick if the piece has just landed
 * Returns -2 if the piece is still floating after the gravity tick
 */
int tick(FallingPiece* fallingPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	fallingPiece -> r = (fallingPiece -> r) + 1;

	return checkForSolidification(fallingPiece, board);
}

/*
 * Checks to see if the falling piece is currently just above another
 * piece or the floor, which would trigger a sticky landing
 */
int checkForSolidification(FallingPiece* fallingPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterTick = r + fallingPiece->r;
			int boardCAfterTick = c + fallingPiece->c;
			if(fallingPieceChars[r][c] != ' ' && board[boardRAfterTick + 1][boardCAfterTick] != ' ' && boardRAfterTick > 0) {
				// The piece needs to stop falling here
				bool continueGame = solidifyFallingPiece(fallingPiece, board);
				if(!continueGame) {
					printf("DON'T CONTINUE\n");
					return -1;
				}
				int scoreAddition = lineCheck(board);				

				return scoreAddition;
			}
		}
	}
	return -2;
}


/*
 * Attempts to move a piece over by 1.
 * Will not allow if this would cause the falling piece to overlap with an established piece
 */
bool move(FallingPiece* fallingPiece, bool moveRight, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	int h_offset = moveRight ? 1 : -1;
	
	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterMove = r + fallingPiece->r;
			int boardCAfterMove = c + fallingPiece->c + h_offset;
			if(boardRAfterMove > 0 && fallingPieceChars[r][c] != ' ' && board[boardRAfterMove][boardCAfterMove] != ' ') {
				return -4;
			}
		}
	}

	// The move was valid!
	fallingPiece -> c += h_offset;
	return checkForSolidification(fallingPiece, board);
}


/*
 * Rotates a piece by 90 degrees
 * Will not allow  if this would cause the falling piece to overlap with an established piece
 */
bool rotate(FallingPiece* fallingPiece, bool rotateClockwise, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	// Temporarily rotate the falling piece to see if then new configuration is valid on the board
	if(rotateClockwise) {
		fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
	}
	else {
		fallingPiece -> rotation = (fallingPiece -> rotation + 3) % 4;
	}

	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	// Check to see if configuration is non-overlapping and in-bounds
	int r, c;
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterRotate = r + fallingPiece->r;
			int boardCAfterRotate = c + fallingPiece->c;
			if(!isInBounds(boardRAfterRotate, boardCAfterRotate) || (boardRAfterRotate > 0 && (fallingPieceChars[r][c] != ' ' && board[boardRAfterRotate][boardCAfterRotate] != ' '))) {
				// Rotate the fallingPieceChars back because it was invalid
				if(rotateClockwise) {
					fallingPiece -> rotation = (fallingPiece -> rotation + 3) % 4;
				}
				else {
					fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
				}
				return -4;
			}
		}
	}

	// The rotation was valid!
	return checkForSolidification(fallingPiece, board);
}


/*
 * Checks to see if all parts of the piece are within bounds.
 * This is currently only used after a piece has been rotated
 * This doesn't check for minimum row because a piece can be rotated
 * right when it's generated, in which case part of it can go above
 * the top row.
 */
bool isInBounds(r, c) {
	return r <= BOARD_HEIGHT - 2  && c >= 1 && c <= BOARD_HEIGHT - 2;
}


/*
 * Prints the board state to terminal (including the falling piece)
 */
void displayBoard(FallingPiece* fallingPiece, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	int fallingPieceRowDisplayBegin = fallingPiece -> r >= 1 ? fallingPiece -> r : 1;
	int fallingPieceRowDisplayEnd = fallingPiece -> r + 3;
	int fallingPieceColDisplayBegin = fallingPiece -> c;
	int fallingPieceColDisplayEnd = fallingPiece -> c + 3;
	
	char fallingPieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(fallingPieceChars, fallingPiece);	

	int r, c;
	for(r = 0; r < BOARD_HEIGHT; r++) {
		for(c = 0; c < BOARD_WIDTH; c++) {
			if(isInSquare(r, c, fallingPieceRowDisplayBegin, fallingPieceRowDisplayEnd,
								fallingPieceColDisplayBegin, fallingPieceColDisplayEnd)) {
				char printChar = (board[r][c] == ' ') ? 
						fallingPieceChars[r - fallingPiece -> r][c - fallingPiece -> c] :
						board[r][c];
				printf("%c", printChar);
			}
			else {
				printf("%c", board[r][c]);
			}
		}
		printf("\n");
	}
}


/*
 * Prints a piece to terminal in its bounding box.
 * Used for next piece and bonus piece.
 */
void displayPiece(FallingPiece* piece) {
	printf("\nPiece:\n");
		
	char pieceChars[PIECE_BLOCK_SIZE][PIECE_BLOCK_SIZE];
	getPieceChars(pieceChars, piece);	

	// Print piece
	int r, c;
	printf("******\n");
	for(r = 0; r < PIECE_BLOCK_SIZE; r++) {
		printf("*");
		for(c =0; c < PIECE_BLOCK_SIZE; c++) {
			printf("%c", pieceChars[r][c]);
		}
		printf("*\n");
	}
	printf("******\n");
}

/*
 * Displays the game state in terminal for simulations purposes.
 */
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


/*
 * Determines whether the row and column in question (r, c) are in or on the box
 * bounded by rbegin, rend, cbegin, and cend
 */
bool isInSquare(r, c, rbegin, rend, cbegin, cend) {
	return r >= rbegin && r <= rend && c >= cbegin && c <= cend;
}


/*
 * Determines whether the row and column in question (r, c) are on (and not in) the box
 * bounded by rbegin, rend, cbegin, and cend
 */
bool isOnSquare(r, c, rbegin, rend, cbegin, cend) {
	return ((r == rbegin || r == rend) && (c >= cbegin && c <= cend) || 
		(c == cbegin || c == cend) && (r >= rbegin && r <= rend));
}


/*
 * Eliminates a row and shifts down all above rows
 */
void deleteRow(int rDeleted, char board[BOARD_HEIGHT][BOARD_WIDTH]) {
	int r, c;
 	for (r = rDeleted; r > 1; r--)
	{
	        for (c = 1; c < BOARD_WIDTH - 1; c++)
	        {
	            board[r][c] = board[r-1][c];
	        }
    	}   
}


/*
 * Checks to see whether a row needs to be eliminated
 */
int lineCheck(char board[BOARD_HEIGHT][BOARD_WIDTH]) {
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