#include "tt.h"
#include <string.h>

// Transposition table size (e.g., 2^20 entries)
#define TT_SIZE (1 << 20)

// The transposition table itself
static TTEntry transposition_table[TT_SIZE];

void init_tt() {
    // Initialize all entries to zero/empty state
    memset(transposition_table, 0, sizeof(transposition_table));
}

TTEntry* probe_tt(uint64_t hash_key) {
    // Use modulo for simple hashing
    unsigned int index = hash_key % TT_SIZE;
    if (transposition_table[index].hash_key == hash_key) {
        return &transposition_table[index];
    }
    return NULL;
}

void store_tt_entry(uint64_t hash_key, int depth, int score, int flag, Move best_move) {
    unsigned int index = hash_key % TT_SIZE;
    // Always replace scheme (simplest, can be improved with depth/age replacement)
    transposition_table[index].hash_key = hash_key;
    transposition_table[index].depth = depth;
    transposition_table[index].score = score;
    transposition_table[index].flag = flag;
    transposition_table[index].best_move = best_move;
}
