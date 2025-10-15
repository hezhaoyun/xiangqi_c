#ifndef CONSTANTS_H
#define CONSTANTS_H

// --- Piece Constants ---
// Negative for Black, positive for Red.
typedef enum {
    B_KING = -1, B_GUARD = -2, B_BISHOP = -3, B_HORSE = -4, B_ROOK = -5, B_CANNON = -6, B_PAWN = -7,
    R_KING = 1,  R_GUARD = 2,  R_BISHOP = 3,  R_HORSE = 4,  R_ROOK = 5,  R_CANNON = 6,  R_PAWN = 7,
    EMPTY = 0
} Piece;

// --- Player Constants ---
#define PLAYER_R  1
#define PLAYER_B -1

// --- Search and Evaluation Constants ---
#define MATE_VALUE 10000
#define DRAW_VALUE 0

// --- Piece Base Values ---
// Note: These values might be better placed in evaluate.c/h, 
// but we mirror the Python structure for now.
static const int PIECE_VALUES[] = {
    0,      // EMPTY
    0,      // R_KING (dummy value at index 1)
    100,    // R_GUARD
    100,    // R_BISHOP
    450,    // R_HORSE
    900,    // R_ROOK
    500,    // R_CANNON
    100,    // R_PAWN
    // Negative piece values can be accessed by abs(piece_enum)
};

int get_piece_value(Piece p);

#endif // CONSTANTS_H
