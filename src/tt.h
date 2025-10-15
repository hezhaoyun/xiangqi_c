#ifndef TT_H
#define TT_H

#include "bitboard.h"
#include "move.h"
#include <stdint.h>

// Transposition Table Entry Flags
#define TT_EXACT 0
#define TT_LOWER 1 // alpha
#define TT_UPPER 2 // beta

// A single entry in the transposition table
typedef struct {
    uint64_t hash_key; // Zobrist key
    int depth;
    int score;
    int flag;
    Move best_move;
} TTEntry;

// Initializes the transposition table
void init_tt();

// Probes the transposition table for a given hash key.
// Returns a pointer to the entry if found, otherwise NULL.
TTEntry* probe_tt(uint64_t hash_key);

// Stores an entry in the transposition table.
void store_tt_entry(uint64_t hash_key, int depth, int score, int flag, Move best_move);

#endif // TT_H
