#ifndef TETRISLIB_H
#define TETRISLIB_H

// Board Specifications ///////////////////////////////////////////////////

#define BOARD_WIDTH 12
#define BOARD_HEIGHT 22

// Pieces and Piece Types ////////////////////////////////////////////////////////

#define S 0
#define Z 1
#define I 2
#define O 3
#define T 4
#define L 5
#define J 6

typedef struct {
	int pieceShape;
	int rotation;
	
	// r and c are the location of the top left tile of the 4x4 piece representation
	int r;
	int c;
} FallingPiece;

typedef struct {
	int row, col;
	char value;
} LedPixel;

#define NUM_PIECES 7
#define NUM_ROTATIONS 4
#define PIECE_BLOCK_SIZE 4

#define NEEDED_BONUS_PIECE_POTENTIAL 5

// Miscellaneous ///////////////////////////////////////////////////

typedef int bool;
#define true 1
#define false 0

#endif