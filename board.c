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

char board[BOARD_WIDTH][BOARD_HEIGHT];

typedef struct {
	PieceShape pieceShape;
	int rotation;
	
	// r and c are the location of the top left tile of the 4x4 piece representation
	int r;
	int c;
} FallingPiece;

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

void move(FallingPiece* fallingPiece, int direction, char[][] baord) {
	int h_offset = direction ? 1 : -1;
	
	char[][] piece = PIECES[fallingPiece->pieceShape][fallingPiece->rotation];
	for(int x = 0; x < PIECE_BLOCK_SIZE; x++) {
		for(int y = 0; y < PIECE_BLOCK_SIZE; y++) {
			if(piece[x][y] != ' ') {
				
			}
		}
	}
	fallingPiece -> c += direction ? 1 : -1;
}

void rotate(FallingPiece* fallingPiece, char[][] board) {
	fallingPiece -> rotation = (fallingPiece -> rotation + 1) % 4;
	adjustToBounds(fallingPiece, board);
}

void adjustToBounds(FallingPiece* fallingPiece, char[][] board) {
	// TODO
}

