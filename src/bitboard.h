#ifndef BITBOARD_H
#define BITBOARD_H

#include "constants.h"
#include "zobrist.h"
#include <stdint.h>

// Use GCC/Clang's 128-bit integer type for bitboards
typedef __int128_t U128;

// Maximum history size for repetition detection
#define MAX_HISTORY 256

// --- Pre-computed Masks ---
extern U128 SQUARE_MASKS[90];
extern U128 CLEAR_MASKS[90];

void init_bitboard_masks();

// --- Helper Functions ---
int get_player_bb_idx(int player);
int get_piece_to_zobrist_idx(Piece p);
int get_piece_to_bb_index(Piece p);
int get_lsb_index(U128 bb);
int get_msb_index(U128 bb);
int popcount(U128 bb);

// --- Board Structure ---
typedef struct {
    // Bitboards for each piece type (e.g., R_PAWN, B_HORSE)
    U128 piece_bitboards[14];

    // Bitboards for each color
    U128 color_bitboards[2]; // 0 for Red, 1 for Black

    // Mailbox representation for quick piece lookup
    Piece board[90];

    int player_to_move;
    uint64_t hash_key;

    // History of hash keys for repetition detection
    uint64_t history[MAX_HISTORY];
    int history_ply;

} Board;

// --- FEN and Initialization ---
void init_board(Board* board, const char* fen);
void parse_fen(Board* board, const char* fen);

// --- Board Operations ---
void print_board(const Board* board);
Piece move_piece(Board* board, int from_sq, int to_sq);
void unmove_piece(Board* board, int from_sq, int to_sq, Piece captured_piece);


// --- New functions to match Python implementation ---

// Generates the FEN string for the current board state.
void to_fen(const Board* board, char* fen_string);

// Creates a copy of the board state.
void copy_board(const Board* src, Board* dest);

// Returns a bitboard of all occupied squares.
static inline U128 get_occupied_bitboard(const Board* board) {
    return board->color_bitboards[0] | board->color_bitboards[1];
}

// Gets the piece on a given square.
static inline Piece get_piece_on_square(const Board* board, int sq) {
    return board->board[sq];
}

// Gets the player for a given piece.
static inline int get_player(Piece p) {
    return (p > 0) ? PLAYER_R : PLAYER_B;
}


#endif // BITBOARD_H

