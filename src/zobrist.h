#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>

// Zobrist keys for each piece type on each square
// [piece_zobrist_idx][row][col]
extern uint64_t zobrist_keys[14][10][9];

// Zobrist key for the player to move (toggled on player change)
extern uint64_t zobrist_player;

// Function to initialize the Zobrist keys with random values
void init_zobrist_keys();

#endif // ZOBRIST_H
