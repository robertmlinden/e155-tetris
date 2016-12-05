#ifndef TETRISLIB_H
#define TETRISLIB_H

// Board Specifications ///////////////////////////////////////////////////

#define BOARD_WIDTH 12
#define BOARD_HEIGHT 22

// Pieces and Piece Types ////////////////////////////////////////////////////////

typedef struct {
	int pieceShape;
	int rotation;
	
	// r and c are the location of the top left tile of the 4x4 piece representation
	int r;
	int c;
} FallingPiece;

#define NONEXISTENT -1

typedef struct {
	int row, col;
	char value;
} LedPixel;

#define NUM_PIECES 7
#define NUM_ROTATIONS 4
#define PIECE_BLOCK_SIZE 4

// Miscellaneous ///////////////////////////////////////////////////

typedef int bool;
#define true 1
#define false 0

#endif