#ifndef ENGINE_H
#define ENGINE_H

#include "bitboard.h"
#include "move.h"
#include <stdint.h>

// Struct to hold a move and its score for move ordering
typedef struct {
    Move move;
    int score;
} ScoredMove;

// Searches for the best move from the given board position.
// Returns the best move found.
Move search(Board* board, int max_depth, long time_limit_ms);


// History table for move ordering
extern int history_table[14][90];
void clear_history_table();

#endif // ENGINE_H
