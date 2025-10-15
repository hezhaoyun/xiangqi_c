#ifndef OPENING_BOOK_H
#define OPENING_BOOK_H

#include "bitboard.h"
#include "move.h"
#include <stdint.h>

// Represents a single entry in the opening book
typedef struct {
    uint64_t hash_key;
    Move move;
} BookEntry;

// Loads the opening book from a binary file
void load_opening_book(const char* filename);

// Queries the opening book for the current board position
// Returns a random valid move if the position is found, otherwise a null move {0,0}
Move query_opening_book(Board* board);

#endif // OPENING_BOOK_H
