#include "move.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// --- Pre-computed Attack Tables ---
U128 KING_ATTACKS[90];
U128 GUARD_ATTACKS[90];
U128 BISHOP_ATTACKS[90];
int BISHOP_LEGS[90][90]; // Maps from_sq, to_sq -> leg_sq
U128 HORSE_ATTACKS[90];
int HORSE_LEGS[90][90];  // Maps from_sq, to_sq -> leg_sq
U128 PAWN_ATTACKS[2][90]; // [player_idx][square]

// Rays for sliding pieces [direction][square]
// Directions: 0:N, 1:E, 2:S, 3:W
U128 RAYS[4][90];

// --- Bitboard Masks ---
U128 RED_SIDE_MASK;
U128 BLACK_SIDE_MASK;

// --- Helper Functions ---
static inline int sq_to_idx(int r, int c) { return r * 9 + c; }
static inline bool is_valid(int r, int c) { return r >= 0 && r < 10 && c >= 0 && c < 9; }




void _precompute_king_guard_attacks() {
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 9; ++c) {
            int sq = sq_to_idx(r, c);
            KING_ATTACKS[sq] = 0;
            GUARD_ATTACKS[sq] = 0;

            // King moves
            int king_moves[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
            for (int i = 0; i < 4; ++i) {
                int nr = r + king_moves[i][0];
                int nc = c + king_moves[i][1];
                if (nc >= 3 && nc <= 5 && ((nr >= 0 && nr <= 2) || (nr >= 7 && nr <= 9))) {
                    KING_ATTACKS[sq] |= SQUARE_MASKS[sq_to_idx(nr, nc)];
                }
            }

            // Guard moves
            int guard_moves[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
            for (int i = 0; i < 4; ++i) {
                int nr = r + guard_moves[i][0];
                int nc = c + guard_moves[i][1];
                if (nc >= 3 && nc <= 5 && ((nr >= 0 && nr <= 2) || (nr >= 7 && nr <= 9))) {
                    GUARD_ATTACKS[sq] |= SQUARE_MASKS[sq_to_idx(nr, nc)];
                }
            }
        }
    }
}

void _precompute_bishop_horse_attacks() {
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 9; ++c) {
            int from_sq = sq_to_idx(r, c);
            BISHOP_ATTACKS[from_sq] = 0;
            HORSE_ATTACKS[from_sq] = 0;

            // Bishop moves
            int bishop_moves[4][2] = {{2, 2}, {2, -2}, {-2, 2}, {-2, -2}};
            for (int i = 0; i < 4; ++i) {
                int nr = r + bishop_moves[i][0];
                int nc = c + bishop_moves[i][1];
                if (is_valid(nr, nc)) {
                    int to_sq = sq_to_idx(nr, nc);
                    BISHOP_ATTACKS[from_sq] |= SQUARE_MASKS[to_sq];
                    BISHOP_LEGS[from_sq][to_sq] = sq_to_idx(r + bishop_moves[i][0]/2, c + bishop_moves[i][1]/2);
                }
            }

            // Horse moves
            int horse_moves[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
            for (int i = 0; i < 8; ++i) {
                int nr = r + horse_moves[i][0];
                int nc = c + horse_moves[i][1];
                if (is_valid(nr, nc)) {
                    int to_sq = sq_to_idx(nr, nc);
                    HORSE_ATTACKS[from_sq] |= SQUARE_MASKS[to_sq];
                    int leg_r = r, leg_c = c;
                    if (abs(horse_moves[i][0]) == 2) leg_r += horse_moves[i][0]/2;
                    else leg_c += horse_moves[i][1]/2;
                    HORSE_LEGS[from_sq][to_sq] = sq_to_idx(leg_r, leg_c);
                }
            }
        }
    }
}

void _precompute_pawn_attacks() {
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 9; ++c) {
            int sq = sq_to_idx(r, c);
            PAWN_ATTACKS[0][sq] = 0; // Red
            PAWN_ATTACKS[1][sq] = 0; // Black

            // Red pawn
            if (is_valid(r - 1, c)) PAWN_ATTACKS[0][sq] |= SQUARE_MASKS[sq_to_idx(r - 1, c)];
            if (r < 5) { // Crossed river
                if (is_valid(r, c - 1)) PAWN_ATTACKS[0][sq] |= SQUARE_MASKS[sq_to_idx(r, c - 1)];
                if (is_valid(r, c + 1)) PAWN_ATTACKS[0][sq] |= SQUARE_MASKS[sq_to_idx(r, c + 1)];
            }

            // Black pawn
            if (is_valid(r + 1, c)) PAWN_ATTACKS[1][sq] |= SQUARE_MASKS[sq_to_idx(r + 1, c)];
            if (r > 4) { // Crossed river
                if (is_valid(r, c - 1)) PAWN_ATTACKS[1][sq] |= SQUARE_MASKS[sq_to_idx(r, c - 1)];
                if (is_valid(r, c + 1)) PAWN_ATTACKS[1][sq] |= SQUARE_MASKS[sq_to_idx(r, c + 1)];
            }
        }
    }
}

void _precompute_rays() {
    for (int sq = 0; sq < 90; ++sq) {
        int r = sq / 9, c = sq % 9;
        RAYS[0][sq] = RAYS[1][sq] = RAYS[2][sq] = RAYS[3][sq] = 0;
        for (int i = r - 1; i >= 0; --i) RAYS[0][sq] |= SQUARE_MASKS[sq_to_idx(i, c)]; // North
        for (int i = c + 1; i < 9; ++i)  RAYS[1][sq] |= SQUARE_MASKS[sq_to_idx(r, i)]; // East
        for (int i = r + 1; i < 10; ++i) RAYS[2][sq] |= SQUARE_MASKS[sq_to_idx(i, c)]; // South
        for (int i = c - 1; i >= 0; --i) RAYS[3][sq] |= SQUARE_MASKS[sq_to_idx(r, i)]; // West
    }
}

void init_move_generator() {
    _precompute_king_guard_attacks();
    _precompute_bishop_horse_attacks();
    _precompute_pawn_attacks();
    _precompute_rays();

    RED_SIDE_MASK = 0;
    for(int i=0; i<45; ++i) RED_SIDE_MASK |= SQUARE_MASKS[i];
    BLACK_SIDE_MASK = ~RED_SIDE_MASK;
}

// --- Actual Move Generation ---

U128 get_rook_moves_bb(int sq, U128 occupied) {
    U128 final_attacks = 0;
    U128 ray;
    int first_blocker;

    // North
    ray = RAYS[0][sq];
    U128 blockers_n = occupied & ray;
    if (blockers_n) {
        first_blocker = get_msb_index(blockers_n);
        final_attacks |= (ray ^ RAYS[0][first_blocker]) | SQUARE_MASKS[first_blocker];
    } else {
        final_attacks |= ray;
    }

    // East
    ray = RAYS[1][sq];
    U128 blockers_e = occupied & ray;
    if (blockers_e) {
        first_blocker = get_lsb_index(blockers_e);
        final_attacks |= (ray ^ RAYS[1][first_blocker]) | SQUARE_MASKS[first_blocker];
    } else {
        final_attacks |= ray;
    }

    // South
    ray = RAYS[2][sq];
    U128 blockers_s = occupied & ray;
    if (blockers_s) {
        first_blocker = get_lsb_index(blockers_s);
        final_attacks |= (ray ^ RAYS[2][first_blocker]) | SQUARE_MASKS[first_blocker];
    } else {
        final_attacks |= ray;
    }

    // West
    ray = RAYS[3][sq];
    U128 blockers_w = occupied & ray;
    if (blockers_w) {
        first_blocker = get_msb_index(blockers_w);
        final_attacks |= (ray ^ RAYS[3][first_blocker]) | SQUARE_MASKS[first_blocker];
    } else {
        final_attacks |= ray;
    }

    return final_attacks;
}

U128 get_cannon_moves_bb(int sq, U128 occupied) {
    U128 attacks = 0;
    U128 ray;
    int screen, target;

    // North
    ray = RAYS[0][sq];
    U128 blockers_n = occupied & ray;
    if (blockers_n) {
        screen = get_msb_index(blockers_n);
        attacks |= (ray ^ RAYS[0][screen]) ^ SQUARE_MASKS[screen]; // Non-captures
        U128 remaining_blockers = blockers_n ^ SQUARE_MASKS[screen];
        if (remaining_blockers) {
            target = get_msb_index(remaining_blockers);
            attacks |= SQUARE_MASKS[target]; // Capture
        }
    } else {
        attacks |= ray;
    }

    // East
    ray = RAYS[1][sq];
    U128 blockers_e = occupied & ray;
    if (blockers_e) {
        screen = get_lsb_index(blockers_e);
        attacks |= (ray ^ RAYS[1][screen]) ^ SQUARE_MASKS[screen];
        U128 remaining_blockers = blockers_e ^ SQUARE_MASKS[screen];
        if (remaining_blockers) {
            target = get_lsb_index(remaining_blockers);
            attacks |= SQUARE_MASKS[target];
        }
    } else {
        attacks |= ray;
    }

    // South
    ray = RAYS[2][sq];
    U128 blockers_s = occupied & ray;
    if (blockers_s) {
        screen = get_lsb_index(blockers_s);
        attacks |= (ray ^ RAYS[2][screen]) ^ SQUARE_MASKS[screen];
        U128 remaining_blockers = blockers_s ^ SQUARE_MASKS[screen];
        if (remaining_blockers) {
            target = get_lsb_index(remaining_blockers);
            attacks |= SQUARE_MASKS[target];
        }
    } else {
        attacks |= ray;
    }

    // West
    ray = RAYS[3][sq];
    U128 blockers_w = occupied & ray;
    if (blockers_w) {
        screen = get_msb_index(blockers_w);
        attacks |= (ray ^ RAYS[3][screen]) ^ SQUARE_MASKS[screen];
        U128 remaining_blockers = blockers_w ^ SQUARE_MASKS[screen];
        if (remaining_blockers) {
            target = get_lsb_index(remaining_blockers);
            attacks |= SQUARE_MASKS[target];
        }
    } else {
        attacks |= ray;
    }

    return attacks;
}

void generate_pseudo_legal_moves(const Board* board, MoveList* move_list) {
    move_list->count = 0;
    int player = board->player_to_move;
    int player_idx = get_player_bb_idx(player);
    U128 own_pieces_bb = board->color_bitboards[player_idx];
    U128 occupied = board->color_bitboards[0] | board->color_bitboards[1];

    Piece piece_type;
    int piece_start_idx = (player == PLAYER_R) ? 0 : 7;
    int piece_end_idx = (player == PLAYER_R) ? 7 : 14;

    for (int i = piece_start_idx; i < piece_end_idx; ++i) {
        U128 piece_bb = board->piece_bitboards[i];
        
        piece_type = (player == PLAYER_R) ? (Piece)(i + 1) : (Piece)(-1 - (i - 7));

        while (piece_bb) {
            int from_sq = get_lsb_index(piece_bb);
            U128 moves_bb = 0;

            switch (piece_type) {
                case R_KING:
                case B_KING:
                    moves_bb = KING_ATTACKS[from_sq];
                    break;
                case R_GUARD:
                case B_GUARD:
                    moves_bb = GUARD_ATTACKS[from_sq];
                    break;
                case R_BISHOP:
                case B_BISHOP: {
                    U128 potential_moves = BISHOP_ATTACKS[from_sq];
                    if (piece_type == R_BISHOP) potential_moves &= BLACK_SIDE_MASK;
                    else potential_moves &= RED_SIDE_MASK;
                    
                    while(potential_moves) {
                        int to_sq = get_lsb_index(potential_moves);
                        int leg_sq = BISHOP_LEGS[from_sq][to_sq];
                        if (!(occupied & SQUARE_MASKS[leg_sq])) {
                            moves_bb |= SQUARE_MASKS[to_sq];
                        }
                        potential_moves &= CLEAR_MASKS[to_sq];
                    }
                    break;
                }
                case R_HORSE:
                case B_HORSE: {
                    U128 potential_moves = HORSE_ATTACKS[from_sq];
                    while(potential_moves) {
                        int to_sq = get_lsb_index(potential_moves);
                        int leg_sq = HORSE_LEGS[from_sq][to_sq];
                        if (!(occupied & SQUARE_MASKS[leg_sq])) {
                            moves_bb |= SQUARE_MASKS[to_sq];
                        }
                        potential_moves &= CLEAR_MASKS[to_sq];
                    }
                    break;
                }
                case R_PAWN:
                case B_PAWN:
                    moves_bb = PAWN_ATTACKS[player_idx][from_sq];
                    break;
                case R_ROOK:
                case B_ROOK:
                    moves_bb = get_rook_moves_bb(from_sq, occupied);
                    break;
                case R_CANNON:
                case B_CANNON:
                    moves_bb = get_cannon_moves_bb(from_sq, occupied);
                    break;
                default: break;
            }

            U128 valid_moves_bb = moves_bb & ~own_pieces_bb;

            while (valid_moves_bb) {
                int to_sq = get_lsb_index(valid_moves_bb);
                if (move_list->count < MAX_MOVES) {
                    move_list->moves[move_list->count].from_sq = from_sq;
                    move_list->moves[move_list->count].to_sq = to_sq;
                    move_list->count++;
                }
                valid_moves_bb &= CLEAR_MASKS[to_sq];
            }
            piece_bb &= CLEAR_MASKS[from_sq];
        }
    }
}

bool is_square_attacked_by(const Board* board, int sq, int attacker_player) {
    U128 occupied = board->color_bitboards[0] | board->color_bitboards[1];
    int attacker_idx = get_player_bb_idx(attacker_player);
    int defender_idx = 1 - attacker_idx;

    // Attacked by Pawns (using reverse lookup)
    Piece pawn_type = (attacker_player == PLAYER_R) ? R_PAWN : B_PAWN;
    if (PAWN_ATTACKS[defender_idx][sq] & board->piece_bitboards[get_piece_to_bb_index(pawn_type)]) {
        return true;
    }

    // Attacked by King
    Piece king_type = (attacker_player == PLAYER_R) ? R_KING : B_KING;
    if (KING_ATTACKS[sq] & board->piece_bitboards[get_piece_to_bb_index(king_type)]) {
        return true;
    }

    // Attacked by Horse
    Piece horse_type = (attacker_player == PLAYER_R) ? R_HORSE : B_HORSE;
    U128 potential_horses = HORSE_ATTACKS[sq] & board->piece_bitboards[get_piece_to_bb_index(horse_type)];
    while (potential_horses) {
        int from_sq = get_lsb_index(potential_horses);
        int leg_sq = HORSE_LEGS[from_sq][sq];
        if (!(occupied & SQUARE_MASKS[leg_sq])) {
            return true;
        }
        potential_horses &= CLEAR_MASKS[from_sq];
    }

    // Attacked by Bishop (cannot cross river)
    Piece bishop_type = (attacker_player == PLAYER_R) ? R_BISHOP : B_BISHOP;
    U128 potential_bishops = BISHOP_ATTACKS[sq] & board->piece_bitboards[get_piece_to_bb_index(bishop_type)];
    if (potential_bishops) {
        U128 side_mask = (attacker_player == PLAYER_R) ? RED_SIDE_MASK : BLACK_SIDE_MASK;
        if (side_mask & SQUARE_MASKS[sq]) { // Bishops can only attack on their own side
            while (potential_bishops) {
                int from_sq = get_lsb_index(potential_bishops);
                int leg_sq = BISHOP_LEGS[from_sq][sq];
                if (!(occupied & SQUARE_MASKS[leg_sq])) {
                    return true;
                }
                potential_bishops &= CLEAR_MASKS[from_sq];
            }
        }
    }

    // Attacked by Rook or Cannon (sliding pieces)
    Piece rook_type = (attacker_player == PLAYER_R) ? R_ROOK : B_ROOK;
    if (get_rook_moves_bb(sq, occupied) & board->piece_bitboards[get_piece_to_bb_index(rook_type)]) {
        return true;
    }

    Piece cannon_type = (attacker_player == PLAYER_R) ? R_CANNON : B_CANNON;
    if (get_cannon_moves_bb(sq, occupied) & board->piece_bitboards[get_piece_to_bb_index(cannon_type)]) {
        return true;
    }

    return false;
}

bool is_king_in_check(const Board* board, int player) {
    // Find the king's square
    Piece king_piece = (player == PLAYER_R) ? R_KING : B_KING;
    U128 king_bb = board->piece_bitboards[get_piece_to_bb_index(king_piece)];
    if (king_bb == 0) return true; // Should not happen
    int king_sq = get_lsb_index(king_bb);

    // 1. Check if attacked by opponent's pieces
    if (is_square_attacked_by(board, king_sq, -player)) {
        return true;
    }

    // 2. Check for "flying general" (kings facing each other)
    Piece opponent_king_piece = (player == PLAYER_R) ? B_KING : R_KING;
    U128 opponent_king_bb = board->piece_bitboards[get_piece_to_bb_index(opponent_king_piece)];
    if (opponent_king_bb == 0) return false; // No opponent king, no check
    int opponent_king_sq = get_lsb_index(opponent_king_bb);

    // a. Must be on the same file
    if ((king_sq % 9) != (opponent_king_sq % 9)) {
        return false;
    }

    // b. No pieces in between
    U128 occupied = board->color_bitboards[0] | board->color_bitboards[1];
    int min_sq = (king_sq < opponent_king_sq) ? king_sq : opponent_king_sq;
    int max_sq = (king_sq > opponent_king_sq) ? king_sq : opponent_king_sq;
    
    U128 between_mask = 0;
    for (int s = min_sq + 9; s < max_sq; s += 9) {
        between_mask |= SQUARE_MASKS[s];
    }

    if ((occupied & between_mask) == 0) {
        return true; // Flying general check
    }

    return false;
}

void generate_legal_moves(Board* board, MoveList* move_list) {
    MoveList pseudo_legal_moves;
    generate_pseudo_legal_moves(board, &pseudo_legal_moves);

    move_list->count = 0;
    int player = board->player_to_move;

    for (int i = 0; i < pseudo_legal_moves.count; ++i) {
        Move move = pseudo_legal_moves.moves[i];
        
        Piece captured = move_piece(board, move.from_sq, move.to_sq);
        
        if (!is_king_in_check(board, player)) {
            if (move_list->count < MAX_MOVES) {
                move_list->moves[move_list->count++] = move;
            }
        }
        
        unmove_piece(board, move.from_sq, move.to_sq, captured);
    }
}

void generate_capture_moves(Board* board, MoveList* move_list) {
    move_list->count = 0;
    int player = board->player_to_move;
    int player_idx = get_player_bb_idx(player);
    U128 occupied = board->color_bitboards[0] | board->color_bitboards[1];
    U128 opponent_pieces_bb = (player == PLAYER_R) ? board->color_bitboards[get_player_bb_idx(PLAYER_B)] : board->color_bitboards[get_player_bb_idx(PLAYER_R)];

    Piece piece_type;
    int piece_start_idx = (player == PLAYER_R) ? 0 : 7;
    int piece_end_idx = (player == PLAYER_R) ? 7 : 14;

    for (int i = piece_start_idx; i < piece_end_idx; ++i) {
        U128 piece_bb = board->piece_bitboards[i];
        piece_type = (player == PLAYER_R) ? (Piece)(i + 1) : (Piece)(-1 - i + 7);

        while (piece_bb) {
            int from_sq = get_lsb_index(piece_bb);
            U128 moves_bb = 0;

            switch (piece_type) {
                case R_KING:
                case B_KING:
                    moves_bb = KING_ATTACKS[from_sq];
                    break;
                case R_GUARD:
                case B_GUARD:
                    moves_bb = GUARD_ATTACKS[from_sq];
                    break;
                case R_BISHOP:
                case B_BISHOP: {
                    U128 potential_moves = BISHOP_ATTACKS[from_sq];
                    if (piece_type == R_BISHOP) potential_moves &= BLACK_SIDE_MASK;
                    else potential_moves &= RED_SIDE_MASK;
                    
                    while(potential_moves) {
                        int to_sq = get_lsb_index(potential_moves);
                        int leg_sq = BISHOP_LEGS[from_sq][to_sq];
                        if (!(occupied & SQUARE_MASKS[leg_sq])) {
                            moves_bb |= SQUARE_MASKS[to_sq];
                        }
                        potential_moves &= CLEAR_MASKS[to_sq];
                    }
                    break;
                }
                case R_HORSE:
                case B_HORSE: {
                    U128 potential_moves = HORSE_ATTACKS[from_sq];
                    while(potential_moves) {
                        int to_sq = get_lsb_index(potential_moves);
                        int leg_sq = HORSE_LEGS[from_sq][to_sq];
                        if (!(occupied & SQUARE_MASKS[leg_sq])) {
                            moves_bb |= SQUARE_MASKS[to_sq];
                        }
                        potential_moves &= CLEAR_MASKS[to_sq];
                    }
                    break;
                }
                case R_PAWN:
                case B_PAWN:
                    moves_bb = PAWN_ATTACKS[player_idx][from_sq];
                    break;
                case R_ROOK:
                case B_ROOK:
                    moves_bb = get_rook_moves_bb(from_sq, occupied);
                    break;
                case R_CANNON:
                case B_CANNON:
                    moves_bb = get_cannon_moves_bb(from_sq, occupied);
                    break;
                default: break;
            }

            // Only consider moves that capture an opponent's piece
            U128 capture_moves_bb = moves_bb & opponent_pieces_bb;

            while (capture_moves_bb) {
                int to_sq = get_lsb_index(capture_moves_bb);
                if (move_list->count < MAX_MOVES) {
                    move_list->moves[move_list->count].from_sq = from_sq;
                    move_list->moves[move_list->count].to_sq = to_sq;
                    move_list->count++;
                }
                capture_moves_bb &= CLEAR_MASKS[to_sq];
            }
            piece_bb &= CLEAR_MASKS[from_sq];
        }
    }
}
