#include "engine.h"
#include "evaluate.h"
#include "tt.h"
#include "move.h"
#include "opening_book.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// History table for move ordering
int history_table[14][90];

void clear_history_table() {
    memset(history_table, 0, sizeof(history_table));
}

// Helper to score a move for move ordering
static int score_move(Board* board, Move move) {
    // Prioritize TT best move if available (not yet implemented here)

    // MVV-LVA (Most Valuable Victim - Least Valuable Aggressor)
    Piece captured_piece = board->board[move.to_sq];
    if (captured_piece != EMPTY) {
        Piece moving_piece = board->board[move.from_sq];
        // Score = 1000 * abs(captured_value) - abs(moving_value)
        return 1000 * get_piece_value(captured_piece) - get_piece_value(moving_piece);
    }

    // History heuristic
    Piece moving_piece = board->board[move.from_sq];
    return history_table[get_piece_to_bb_index(moving_piece)][move.to_sq];
}

// Comparison function for qsort
static int compare_moves(const void* a, const void* b) {
    const ScoredMove* sm_a = (const ScoredMove*)a;
    const ScoredMove* sm_b = (const ScoredMove*)b;
    return sm_b->score - sm_a->score; // Descending order
}

// Quiescence search to evaluate noisy positions
static int quiescence_search(Board* board, int alpha, int beta) {
    // Evaluate the current position statically
    int stand_pat = evaluate(board);

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    MoveList capture_moves;
    generate_capture_moves(board, &capture_moves);

    // Sort capture moves (MVV-LVA)
    ScoredMove scored_capture_moves[MAX_MOVES];
    for (int i = 0; i < capture_moves.count; ++i) {
        scored_capture_moves[i].move = capture_moves.moves[i];
        scored_capture_moves[i].score = score_move(board, capture_moves.moves[i]); // Depth 0 for captures
    }
    qsort(scored_capture_moves, capture_moves.count, sizeof(ScoredMove), compare_moves);

    for (int i = 0; i < capture_moves.count; ++i) {
        Move move = scored_capture_moves[i].move;
        Piece captured = move_piece(board, move.from_sq, move.to_sq);

        int score = -quiescence_search(board, -beta, -alpha);

        unmove_piece(board, move.from_sq, move.to_sq, captured);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

// Helper to count major pieces for null move pruning
static int get_major_piece_count(Board* board, int player) {
    int count = 0;
    count += popcount(board->piece_bitboards[get_piece_to_bb_index((player == PLAYER_R) ? R_ROOK : B_ROOK)]);
    count += popcount(board->piece_bitboards[get_piece_to_bb_index((player == PLAYER_R) ? R_HORSE : B_HORSE)]);
    count += popcount(board->piece_bitboards[get_piece_to_bb_index((player == PLAYER_R) ? R_CANNON : B_CANNON)]);
    return count;
}

// Negamax implementation with alpha-beta pruning
static int negamax(Board* board, int depth, int alpha, int beta) {
    // --- Repetition Detection ---
    // Check for 3-fold repetition (current position is the 3rd occurrence)
    if (board->history_ply >= 4) { // Need at least 4 plies to have 3 occurrences
        int repetitions = 0;
        for (int i = board->history_ply - 2; i >= 0; i -= 2) {
            if (board->history[i] == board->hash_key) {
                repetitions++;
            }
        }
        if (repetitions >= 2) { // Current position is the 3rd occurrence
            return 0; // Draw by repetition
        }
    }

    // --- Transposition Table Probe ---
    TTEntry* tt_entry = probe_tt(board->hash_key);
    Move tt_best_move = {0, 0};
    int original_alpha = alpha;

    if (tt_entry != NULL && tt_entry->depth >= depth) {
        if (tt_entry->flag == TT_EXACT) {
            return tt_entry->score;
        } else if (tt_entry->flag == TT_LOWER) {
            alpha = (alpha > tt_entry->score) ? alpha : tt_entry->score;
        } else if (tt_entry->flag == TT_UPPER) {
            beta = (beta < tt_entry->score) ? beta : tt_entry->score;
        }
        if (alpha >= beta) {
            return tt_entry->score;
        }
        tt_best_move = tt_entry->best_move; // Store TT best move
    }

    if (depth == 0) {
        return quiescence_search(board, alpha, beta);
    }

    // --- Null Move Pruning ---
    // If we can make a null move and still get a high score, we can prune this branch.
    // Conditions: not in check, depth is sufficient, and enough major pieces on board.
    bool is_in_check_val = is_king_in_check(board, board->player_to_move);
    if (!is_in_check_val && depth >= 3 && get_major_piece_count(board, board->player_to_move) > 1) {
        board->player_to_move *= -1;
        board->hash_key ^= zobrist_player;
        board->history_ply++;
        board->history[board->history_ply] = board->hash_key;

        int null_move_score = -negamax(board, depth - 1 - 2, -beta, -beta + 1); // R = 2

        board->history_ply--;
        board->hash_key ^= zobrist_player;
        board->player_to_move *= -1;

        if (null_move_score >= beta) {
            // Store in TT (optional, but good for consistency)
            store_tt_entry(board->hash_key, depth, beta, TT_LOWER, (Move){0,0});
            return beta;
        }
    }

    MoveList move_list;
    generate_legal_moves(board, &move_list);

    if (move_list.count == 0) {
        // Check for mate or stalemate
        if (is_in_check_val) {
            return -MATE_VALUE + depth; // Checkmate
        } else {
            return 0; // Stalemate
        }
    }

    // --- Move Ordering ---
    ScoredMove scored_moves[MAX_MOVES];
    for (int i = 0; i < move_list.count; ++i) {
        scored_moves[i].move = move_list.moves[i];
        // Give a huge bonus to the TT move
        if (move_list.moves[i].from_sq == tt_best_move.from_sq && move_list.moves[i].to_sq == tt_best_move.to_sq) {
            scored_moves[i].score = 1000000;
        } else {
            scored_moves[i].score = score_move(board, move_list.moves[i]);
        }
    }
    qsort(scored_moves, move_list.count, sizeof(ScoredMove), compare_moves);

    int best_score = -MATE_VALUE;
    Move best_move_for_tt = {0,0};

    for (int i = 0; i < move_list.count; ++i) {
        Move move = scored_moves[i].move;
        bool is_quiet = (board->board[move.to_sq] == EMPTY);

        // --- Late Move Reduction (LMR) ---
        int reduction = 0;
        if (depth >= 3 && i > 3 && is_quiet && !is_in_check_val) { // i > 3 means we are on the 5th move or later
            reduction = 1;
        }

        Piece captured = move_piece(board, move.from_sq, move.to_sq);
        
        // Search with reduced depth first
        int score = -negamax(board, depth - 1 - reduction, -beta, -alpha);

        // If LMR was used and the score was better than alpha, re-search with full depth
        if (reduction > 0 && score > alpha) {
            score = -negamax(board, depth - 1, -beta, -alpha);
        }
        
        unmove_piece(board, move.from_sq, move.to_sq, captured);

        if (score > best_score) {
            best_score = score;
            best_move_for_tt = move;
        }
        if (best_score > alpha) {
            alpha = best_score;
        }
        if (alpha >= beta) {
            // Beta cutoff, update history table
            Piece moving_piece = board->board[move.from_sq];
            history_table[get_piece_to_bb_index(moving_piece)][move.to_sq] += depth * depth;
            break; 
        }
    }

    // --- Transposition Table Store ---
    int flag = TT_EXACT;
    if (best_score <= original_alpha) {
        flag = TT_UPPER;
    } else if (best_score >= beta) {
        flag = TT_LOWER;
    }
    store_tt_entry(board->hash_key, depth, best_score, flag, best_move_for_tt);

    return best_score;
}

Move search(Board* board, int max_depth, long time_limit_ms) {
    clock_t start_time = clock();

    // Load the opening book (should ideally be done only once)
    static bool book_loaded = false;
    if (!book_loaded) {
        load_opening_book("opening_book.bin");
        book_loaded = true;
    }

    // Query the opening book
    Move book_move = query_opening_book(board);
    if (book_move.from_sq != 0 || book_move.to_sq != 0) {
        printf("Move from opening book: %d -> %d\n", book_move.from_sq, book_move.to_sq);
        return book_move;
    }

    init_tt(); // Initialize TT at the start of each top-level search
    clear_history_table(); // Clear history table at the start of each top-level search

    Move best_move_overall = {0, 0};
    int best_score_overall = -MATE_VALUE;

    printf("Starting iterative deepening search up to depth %d or %ldms...\n", max_depth, time_limit_ms);

    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        Move best_move_this_depth = {0, 0};
        int best_score_this_depth = -MATE_VALUE;
        int alpha = -MATE_VALUE;
        int beta = MATE_VALUE;

        MoveList move_list;
        generate_legal_moves(board, &move_list);

        if (move_list.count == 0) {
            if (is_king_in_check(board, board->player_to_move)) {
                best_score_overall = -MATE_VALUE; // Checkmate
            } else {
                best_score_overall = 0; // Stalemate
            }
            break;
        }

        ScoredMove scored_moves[MAX_MOVES];
        for (int i = 0; i < move_list.count; ++i) {
            scored_moves[i].move = move_list.moves[i];
            scored_moves[i].score = score_move(board, move_list.moves[i]);
        }
        qsort(scored_moves, move_list.count, sizeof(ScoredMove), compare_moves);

        for (int i = 0; i < move_list.count; ++i) {
            // Check time before processing each root move
            if (time_limit_ms > 0) {
                clock_t current_time = clock();
                if ((long)((current_time - start_time) * 1000 / CLOCKS_PER_SEC) >= time_limit_ms) {
                    goto time_up;
                }
            }

            Move move = scored_moves[i].move;
            Piece captured = move_piece(board, move.from_sq, move.to_sq);
            
            int score = -negamax(board, current_depth - 1, -beta, -alpha);
            
            unmove_piece(board, move.from_sq, move.to_sq, captured);

            if (score > best_score_this_depth) {
                best_score_this_depth = score;
                best_move_this_depth = move;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
        
        if (best_move_this_depth.from_sq != 0 || best_move_this_depth.to_sq != 0) {
            best_move_overall = best_move_this_depth;
            best_score_overall = best_score_this_depth;
        }

        printf("  Depth %d: Best score = %d, Best move = %d -> %d\n", 
               current_depth, best_score_overall, best_move_overall.from_sq, best_move_overall.to_sq);

        // If mate is found, no need to search deeper
        if (abs(best_score_overall) > MATE_VALUE - 100) {
            break;
        }
    }

time_up:
    printf("Final Best score: %d\n", best_score_overall);
    return best_move_overall;
}
