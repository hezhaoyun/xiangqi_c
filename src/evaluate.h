#ifndef EVALUATE_H
#define EVALUATE_H

#include "bitboard.h"

// Evaluates the board position and returns a score from the perspective of the current player.
int evaluate(Board* board);

#endif // EVALUATE_H
