#ifndef MOVE_H
#define MOVE_H

#include "bitboard.h"
#include <stdbool.h>

// --- Pre-computed Attack Tables ---
extern U128 KING_ATTACKS[90];
extern U128 GUARD_ATTACKS[90];
extern U128 BISHOP_ATTACKS[90];
extern int BISHOP_LEGS[90][90];
extern U128 HORSE_ATTACKS[90];
extern int HORSE_LEGS[90][90];
extern U128 PAWN_ATTACKS[2][90];
extern U128 RAYS[4][90];

// --- Bitboard Masks ---
extern U128 RED_SIDE_MASK;
extern U128 BLACK_SIDE_MASK;

// Represents a single move from a source square to a destination square
typedef struct {
    int from_sq;
    int to_sq;
    // We can add captured_piece, promotion, etc. later if needed
} Move;

// A list to store generated moves
#define MAX_MOVES 256
typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

// --- Move Generation ---

// One-time initialization of all pre-computed attack tables
void init_move_generator();

// Generates all pseudo-legal moves for the current player
void generate_pseudo_legal_moves(const Board* board, MoveList* move_list);

// Generates all legal moves for the current player
void generate_legal_moves(Board* board, MoveList* move_list);

// Generates only legal capture moves for the current player
void generate_capture_moves(Board* board, MoveList* move_list);

// --- Attack Info ---

// Checks if a given square is attacked by the specified player
bool is_square_attacked_by(const Board* board, int sq, int attacker_player);

bool is_king_in_check(const Board* board, int player);

// --- Sliding Piece Move Generation ---
U128 get_rook_moves_bb(int sq, U128 occupied);
U128 get_cannon_moves_bb(int sq, U128 occupied);

#endif // MOVE_H
