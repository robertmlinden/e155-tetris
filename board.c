#include <time.h>
#include <stdlib.h>

int BOARD_WIDTH = 12;
int BOARD_HEIGHT = 22;

enum Direction {
	LEFT,
	RIGHT
}

enum PieceShape {
	S,
	Z,
	I,
	O,
	T,
	L,
	J
};

typedef int bool;
#define true 1
#define false 0

// (0, 0) is top-left, (NUM_ROWS, 0) is bottom left, (0, NUM_COLS) is top right
// row number corresponds to first entry corresponds to y-value
// col number corresponds to second entry corresponds to x-value
// Therefore, board is indexed (y, x)
char board[BOARD_WIDTH][BOARD_HEIGHT];

typedef struct {
	PieceShape pieceShape;
	int rotation;
	
	// r and c are the location of the top left tile of the 4x4 piece representation
	int r;
	int c;
} FallingPiece;


// Rotate
// Gravity
// Move
// Level elimintation
// Start at top
// Stop moving when they hit the bottom or another piece
// Acceleration downwards?
// floating piece on the bottom?
// Another button to just lead-drop the piece?

void initBoard(char[][] board) {
	for(int r = 0; r < BOARD_HEIGHT; r++) {
		for(int c = 0; c < BOARD_WIDTH; c++) {
			if(r == 0 || r == BOARD_HEIGHT - 1 || c == 0 || c == BOARD_WIDTH - 1) {
				board[r][c] = '#';
			}
			else {
				board[r][c] = ' ';
			}
		}
	}
}

// Niavely stops a piece when it makes contact with one below
void tick(FallingPiece* fallingPiece, char[][] board) {
	fallingPiece -> r = (FallingPiece -> r) + 1;

	for(int r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(int c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterTick = r + fallingPiece->r;
			int boardCAfterTick = c + fallingPiece->c;
			if(piece[r][c] != ' ' && board[boardRAfterTick + 1][boardCAfterTick] != ' ') {
				// The piece needs to stop faling here, so transition to new falling piece
				solidifyFallingPiece(fallingPiece, board);
				levelElimination(board);
				newFallingPiece(fallingPiece);
			}
		}
	}
}



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

void solidifyFallingPiece(FallingPiece* fallingPiece, char[][] board) {
	for(int r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(int c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardR = r + fallingPiece->r;
			int boardC = c + fallingPiece->c;
			if(piece[r][c] != ' ') {
				board[boardR][boardC] = piece[r][c];
			}
		}
	}
}

bool move(FallingPiece* fallingPiece, bool moveRight, char[][] baord) {
	int h_offset = moveRight ? 1 : -1;
	
	char[][] piece = PIECES[fallingPiece->pieceShape][fallingPiece->rotation];
	for(int r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(int c = 0; c < PIECE_BLOCK_SIZE; c++) {
			int boardRAfterMove = r + fallingPiece->r;
			int boardCAfterMove = c + fallingPiece->c + h_offset;
			if(piece[r][c] != ' ' && board[boardRAfterMove][boardCAfterMove] != ' ') {
				return false;
			}
		}
	}

	// The move was valid!
	fallingPiece -> c += direction ? 1 : -1;
	return true;
}

// STILL NEED TO IMPLEMENT ADJUSTING A PIECE IF SIMPLE ROTATION IS INVALID BUT ADDING A 1 OR 2 SQUARE TRANSLATION WOULD IN FACT MAKE IT VALID
// VERIFY THIS BY PLAYING SOME TETRIS!
bool rotate(FallingPiece* fallingPiece, bool rotateClockwise, char[][] board) {
	// Temporarily rotate the falling piece to see if then new configuration is valid on the board
	if(rotateClockwise) {
		fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
	}
	else {
		fallingPiece -> rotation = (fallingPiece -> rotation + 3) % 4;
	}

	// Check to see if configuration is non-overlapping and in-bounds
	for(int r = 0; r < PIECE_BLOCK_SIZE; r++) {
		for(int c = 0; c < PIECE_BLOCK_SIZE; c++) {
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
bool isInBounds(r, c) {
	return r <= BOARD_HEIGHT - 2  && c >= 1 && c <= BOARD_HEIGHT - 2;
}

void displayBoard(char[][] board) {
	for(int r = 0; r < BOARD_HEIGHT; r++) {
		for(int c = 0; c < BOARD_WIDTH; c++) {
			printf("%c", asdf);
		}
	}
}

