#include "evaluate.h"
#include "bitboard.h"
#include "move.h"
#include <stdlib.h>
#include <stdio.h>

// --- Piece Values ---
// Using the values from constants.h is an option, but having them here
// makes the evaluation function more self-contained.
static const int MATERIAL_VALUES[] = { 0, 10000, 200, 200, 450, 900, 500, 100 }; // Indexed by Piece type (abs value)

// --- Piece-Square Tables (Midgame & Endgame) ---
// From Red's perspective (bottom of the board)

static const int KING_PST_MG[10][9] = {
    {  0,   0,   0,   8,   8,   8,   0,   0,   0},
    {  0,   0,   0,   8,   8,   8,   0,   0,   0},
    {  0,   0,   0,   6,   6,   6,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   6,   6,   6,   0,   0,   0},
    {  0,   0,   0,   8,   8,   8,   0,   0,   0},
    {  0,   0,   0,   8,   8,   8,   0,   0,   0},
};

static const int GUARD_PST_MG[10][9] = {
    {  0,   0,   0,  20,   0,  20,   0,   0,   0},
    {  0,   0,   0,   0,  23,   0,   0,   0,   0},
    {  0,   0,   0,  20,   0,  20,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,  20,   0,  20,   0,   0,   0},
    {  0,   0,   0,   0,  23,   0,   0,   0,   0},
    {  0,   0,   0,  20,   0,  20,   0,   0,   0},
};

static const int BISHOP_PST_MG[10][9] = {
    {  0,   0,  20,   0,   0,   0,  20,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,  23,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,  20,   0,   0,   0,  20,   0,   0},
    {  0,   0,  20,   0,   0,   0,  20,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,  23,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,  20,   0,   0,   0,  20,   0,   0},
};

static const int HORSE_PST_MG[10][9] = {
    { 90,  90,  90,  96,  90,  96,  90,  90,  90},
    { 90,  96, 103,  97,  94,  97, 103,  96,  90},
    { 92,  98,  99, 103,  99, 103,  99,  98,  92},
    { 93, 108, 100, 107, 100, 107, 100, 108,  93},
    { 90, 100,  99, 103, 104, 103,  99, 100,  90},
    { 90,  98, 101, 102, 103, 102, 101,  98,  90},
    { 92,  94,  98,  95,  98,  95,  98,  94,  92},
    { 93,  92,  94,  95,  92,  95,  94,  92,  93},
    { 85,  90,  92,  93,  78,  93,  92,  90,  85},
    { 88,  85,  90,  88,  90,  88,  90,  85,  88},
};

static const int ROOK_PST_MG[10][9] = {
    {206, 208, 207, 213, 214, 213, 207, 208, 206},
    {206, 212, 209, 216, 233, 216, 209, 212, 206},
    {206, 208, 207, 214, 216, 214, 207, 208, 206},
    {206, 213, 213, 216, 216, 216, 213, 213, 206},
    {208, 211, 211, 214, 215, 214, 211, 211, 208},
    {208, 212, 212, 214, 215, 214, 212, 212, 208},
    {204, 209, 204, 212, 214, 212, 204, 209, 204},
    {198, 208, 204, 212, 212, 212, 204, 208, 198},
    {200, 208, 206, 212, 200, 212, 206, 208, 200},
    {194, 206, 204, 212, 200, 212, 204, 206, 194},
};

static const int CANNON_PST_MG[10][9] = {
    {100, 100,  96,  91,  90,  91,  96, 100, 100},
    { 98,  98,  96,  92,  89,  92,  96,  98,  98},
    { 97,  97,  96,  91,  92,  91,  96,  97,  97},
    { 96,  99,  99,  98, 100,  98,  99,  99,  96},
    { 96,  96,  96,  96, 100,  96,  96,  96,  96},
    { 95,  96,  99,  96, 100,  96,  99,  96,  95},
    { 96,  96,  96,  96,  96,  96,  96,  96,  96},
    { 97,  96, 100,  99, 101,  99, 100,  96,  97},
    { 96,  97,  98,  98,  98,  98,  98,  97,  96},
    { 96,  96,  97,  99,  99,  99,  97,  96,  96},
};

static const int PAWN_PST_MG[10][9] = {
    {  9,   9,   9,  11,  13,  11,   9,   9,   9},
    { 19,  24,  34,  42,  44,  42,  34,  24,  19},
    { 19,  24,  32,  37,  37,  37,  32,  24,  19},
    { 19,  23,  27,  29,  30,  29,  27,  23,  19},
    { 14,  18,  20,  27,  29,  27,  20,  18,  14},
    {  7,   0,  13,   0,  16,   0,  13,   0,   7},
    {  7,   0,   7,   0,  15,   0,   7,   0,   7},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
};

static const int PAWN_PST_EG[10][9] = {
    { 20,  20,  20,  25,  30,  25,  20,  20,  20},
    { 40,  50,  60,  70,  75,  70,  60,  50,  40},
    { 40,  50,  60,  65,  70,  65,  60,  50,  40},
    { 40,  50,  55,  60,  60,  60,  55,  50,  40},
    { 30,  40,  45,  50,  50,  50,  45,  40,  30},
    { 15,  20,  25,  30,  30,  30,  25,  20,  15},
    { 10,  15,  20,  20,  20,  20,  20,  15,  10},
    {  5,   5,   5,   5,   5,   5,   5,   5,   5},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
    {  0,   0,   0,   0,   0,   0,   0,   0,   0},
};

// Helper to get the correct PST for a given piece type
static const int (*PST_MG(Piece p)) [9] {
    switch(abs(p)) {
        case R_KING: return KING_PST_MG;
        case R_GUARD: return GUARD_PST_MG;
        case R_BISHOP: return BISHOP_PST_MG;
        case R_HORSE: return HORSE_PST_MG;
        case R_ROOK: return ROOK_PST_MG;
        case R_CANNON: return CANNON_PST_MG;
        case R_PAWN: return PAWN_PST_MG;
        default: return NULL;
    }
}

static const int (*PST_EG(Piece p)) [9] {
    if (abs(p) == R_PAWN) return PAWN_PST_EG;
    return PST_MG(p); // Endgame PSTs are same as midgame for non-pawns
}

// --- Pattern Bonuses ---
#define BONUS_BOTTOM_CANNON 80
#define BONUS_PALACE_HEART_HORSE 70

static int calculate_pattern_score(const Board* board) {
    int pattern_score = 0;

    // --- Red Player Patterns ---
    // Bottom Cannon
    U128 red_cannons = board->piece_bitboards[get_piece_to_bb_index(R_CANNON)];
    if (red_cannons & (U128)0x1FF) { // Check if any red cannon is on the black's bottom rank (rank 0)
        pattern_score += BONUS_BOTTOM_CANNON;
    }
    // Palace Heart Horse
    U128 red_horses = board->piece_bitboards[get_piece_to_bb_index(R_HORSE)];
    if (red_horses & SQUARE_MASKS[4]) { // d1 square for black palace center
        pattern_score += BONUS_PALACE_HEART_HORSE;
    }

    // Black Player Patterns ---
    // Bottom Cannon
    U128 black_cannons = board->piece_bitboards[get_piece_to_bb_index(B_CANNON)];
    if (black_cannons & ((U128)0x1FF << 81)) { // Check if any black cannon is on red's bottom rank (rank 9)
        pattern_score -= BONUS_BOTTOM_CANNON;
    }
    // Palace Heart Horse
    U128 black_horses = board->piece_bitboards[get_piece_to_bb_index(B_HORSE)];
    if (black_horses & SQUARE_MASKS[85]) { // d10 square for red palace center
        pattern_score -= BONUS_PALACE_HEART_HORSE;
    }

    return pattern_score;
}

#define KING_SAFETY_PENALTY_PER_GUARD 50

static int calculate_king_safety_score(const Board* board) {
    int king_safety_score = 0;

    // Red player's king safety
    int red_guard_count = popcount(board->piece_bitboards[get_piece_to_bb_index(R_GUARD)]);
    if (red_guard_count < 2) {
        king_safety_score -= (2 - red_guard_count) * KING_SAFETY_PENALTY_PER_GUARD;
    }

    // Black player's king safety
    int black_guard_count = popcount(board->piece_bitboards[get_piece_to_bb_index(B_GUARD)]);
    if (black_guard_count < 2) {
        king_safety_score += (2 - black_guard_count) * KING_SAFETY_PENALTY_PER_GUARD;
    }

    return king_safety_score;
}

#define DYNAMIC_BONUS_ATTACK_PER_MISSING_DEFENDER 15

static int calculate_dynamic_bonus_score(const Board* board) {
    int dynamic_score = 0;

    // --- Red attacking Black's Palace ---
    int black_defenders = popcount(board->piece_bitboards[get_piece_to_bb_index(B_GUARD)]);
    int missing_black_defenders = 2 - black_defenders;
    if (missing_black_defenders > 0) {
        int red_attackers = 0;
        // Define black palace zone
        for (int r = 0; r <= 2; ++r) {
            for (int c = 3; c <= 5; ++c) {
                if (is_square_attacked_by(board, r * 9 + c, PLAYER_R)) {
                    red_attackers++;
                }
            }
        }
        dynamic_score += red_attackers * missing_black_defenders * DYNAMIC_BONUS_ATTACK_PER_MISSING_DEFENDER;
    }

    // --- Black attacking Red's Palace ---
    int red_defenders = popcount(board->piece_bitboards[get_piece_to_bb_index(R_GUARD)]);
    int missing_red_defenders = 2 - red_defenders;
    if (missing_red_defenders > 0) {
        int black_attackers = 0;
        // Define red palace zone
        for (int r = 7; r <= 9; ++r) {
            for (int c = 3; c <= 5; ++c) {
                if (is_square_attacked_by(board, r * 9 + c, PLAYER_B)) {
                    black_attackers++;
                }
            }
        }
        dynamic_score -= black_attackers * missing_red_defenders * DYNAMIC_BONUS_ATTACK_PER_MISSING_DEFENDER;
    }

    return dynamic_score;
}


static int calculate_mobility_score(const Board* board) {
    int mobility_score = 0;
    U128 occupied = board->color_bitboards[0] | board->color_bitboards[1];

    const int MOBILITY_BONUS_ROOK = 1;
    const int MOBILITY_BONUS_HORSE = 3;
    const int MOBILITY_BONUS_CANNON = 1;

    for (int player_idx = 0; player_idx < 2; ++player_idx) {
        int player = (player_idx == 0) ? PLAYER_R : PLAYER_B;
        U128 own_pieces_bb = board->color_bitboards[player_idx];

        // Rook mobility
        Piece rook_type = (player == PLAYER_R) ? R_ROOK : B_ROOK;
        U128 rooks_bb = board->piece_bitboards[get_piece_to_bb_index(rook_type)];
        while (rooks_bb) {
            int sq = get_lsb_index(rooks_bb);
            U128 moves_bb = get_rook_moves_bb(sq, occupied) & ~own_pieces_bb;
            mobility_score += popcount(moves_bb) * MOBILITY_BONUS_ROOK * player;
            rooks_bb &= CLEAR_MASKS[sq];
        }

        // Horse mobility
        Piece horse_type = (player == PLAYER_R) ? R_HORSE : B_HORSE;
        U128 horses_bb = board->piece_bitboards[get_piece_to_bb_index(horse_type)];
        while (horses_bb) {
            int sq = get_lsb_index(horses_bb);
            U128 potential_moves = HORSE_ATTACKS[sq] & ~own_pieces_bb;
            while (potential_moves) {
                int to_sq = get_lsb_index(potential_moves);
                int leg_sq = HORSE_LEGS[sq][to_sq];
                if (!(occupied & SQUARE_MASKS[leg_sq])) {
                    mobility_score += MOBILITY_BONUS_HORSE * player;
                }
                potential_moves &= CLEAR_MASKS[to_sq];
            }
            horses_bb &= CLEAR_MASKS[sq];
        }

        // Cannon mobility
        Piece cannon_type = (player == PLAYER_R) ? R_CANNON : B_CANNON;
        U128 cannons_bb = board->piece_bitboards[get_piece_to_bb_index(cannon_type)];
        while (cannons_bb) {
            int sq = get_lsb_index(cannons_bb);
            U128 moves_bb = get_cannon_moves_bb(sq, occupied) & ~own_pieces_bb;
            mobility_score += popcount(moves_bb) * MOBILITY_BONUS_CANNON * player;
            cannons_bb &= CLEAR_MASKS[sq];
        }
    }
    return mobility_score;
}


int evaluate(Board* board) {
    int material_score = 0;
    int pst_score = 0;

    // 1. Material Score
    for (int i = 1; i <= 7; ++i) {
        material_score += popcount(board->piece_bitboards[get_piece_to_bb_index(i)]) * MATERIAL_VALUES[i];
        material_score -= popcount(board->piece_bitboards[get_piece_to_bb_index(-i)]) * MATERIAL_VALUES[i];
    }

    // 2. Tapered Eval Phase Weight
    const int OPENING_PHASE_MATERIAL = (900 + 450 + 500) * 2 + (200 + 200) * 2; // Rooks, Horses, Cannons, Guards, Bishops
    int current_phase_material = 0;
    for (int i = 2; i <= 6; ++i) { // Major pieces
        current_phase_material += popcount(board->piece_bitboards[get_piece_to_bb_index(i)]) * MATERIAL_VALUES[i];
        current_phase_material += popcount(board->piece_bitboards[get_piece_to_bb_index(-i)]) * MATERIAL_VALUES[i];
    }
    double phase_weight = (double)current_phase_material / OPENING_PHASE_MATERIAL;
    if (phase_weight > 1.0) phase_weight = 1.0;

    // 3. PST Score
    for (int i = 0; i < 14; ++i) {
        U128 piece_bb = board->piece_bitboards[i];
        Piece piece_type = (i < 7) ? (Piece)(i + 1) : (Piece)(-1 - (i - 7));
        int player = (piece_type > 0) ? PLAYER_R : PLAYER_B;

        const int (*mg_table)[9] = PST_MG(piece_type);
        const int (*eg_table)[9] = PST_EG(piece_type);

        while (piece_bb) {
            int sq = get_lsb_index(piece_bb);
            int r = sq / 9, c = sq % 9;

            // Lookup from Red's perspective
            int pst_r = (player == PLAYER_R) ? 9 - r : r;
            int pst_c = (player == PLAYER_R) ? 8 - c : c;

            int mg_pst = mg_table[pst_r][pst_c];
            int eg_pst = eg_table[pst_r][pst_c];

            int pst = (int)(mg_pst * phase_weight + eg_pst * (1.0 - phase_weight));

            if (player == PLAYER_R) {
                pst_score += pst;
            } else {
                pst_score -= pst;
            }
            piece_bb &= CLEAR_MASKS[sq];
        }
    }

    int mobility_score = calculate_mobility_score(board);
    int pattern_score = calculate_pattern_score(board);
    int king_safety_score = calculate_king_safety_score(board);
    int dynamic_bonus_score = calculate_dynamic_bonus_score(board);

    int final_score = material_score + pst_score + mobility_score + pattern_score + king_safety_score + dynamic_bonus_score;
    return final_score * board->player_to_move;
}
