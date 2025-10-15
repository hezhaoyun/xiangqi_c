#include "textual_ui.h"
#include "bitboard.h"
#include "move.h"
#include "engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Helper to parse a move from algebraic notation (e.g., "h2e2")
static Move parse_move_string(const char* move_str) {
    if (strlen(move_str) != 4) return (Move){0,0};

    int from_c = move_str[0] - 'a';
    int from_r = 9 - (move_str[1] - '0');
    int to_c = move_str[2] - 'a';
    int to_r = 9 - (move_str[3] - '0');

    int from_sq = from_r * 9 + from_c;
    int to_sq = to_r * 9 + to_c;

    return (Move){from_sq, to_sq};
}

// Helper to get algebraic notation from square index
static void get_square_notation(int sq, char* notation) {
    if (sq < 0 || sq >= 90) {
        strcpy(notation, "??");
        return;
    }
    int r = sq / 9;
    int c = sq % 9;
    notation[0] = 'a' + c;
    notation[1] = '0' + (9 - r);
    notation[2] = '\0';
}

void run_textual_ui() {
    Board board;
    init_board(&board, NULL); // Initialize with default position
    init_move_generator();

    char input[10];
    while (1) {
        print_board(&board);

        if (board.player_to_move == PLAYER_R) {
            printf("Enter your move (e.g. h2e2): ");
            scanf("%s", input);

            if (strcmp(input, "exit") == 0) break;

            Move user_move = parse_move_string(input);
            
            // Basic validation
            MoveList legal_moves;
            generate_legal_moves(&board, &legal_moves);
            bool move_is_legal = false;
            for(int i=0; i<legal_moves.count; ++i) {
                if(legal_moves.moves[i].from_sq == user_move.from_sq && legal_moves.moves[i].to_sq == user_move.to_sq) {
                    move_is_legal = true;
                    break;
                }
            }

            if (move_is_legal) {
                move_piece(&board, user_move.from_sq, user_move.to_sq);
            } else {
                printf("Illegal move.\n");
                continue;
            }

        } else {
            printf("Computer is thinking...\n");
            Move best_move = search(&board, 10, 5000); // 5 seconds time limit, depth 6
            if (best_move.from_sq != 0 || best_move.to_sq != 0) {
                char from_notation[3];
                char to_notation[3];
                get_square_notation(best_move.from_sq, from_notation);
                get_square_notation(best_move.to_sq, to_notation);
                printf("Computer moves: %s%s\n", from_notation, to_notation);
                move_piece(&board, best_move.from_sq, best_move.to_sq);
            } else {
                printf("Checkmate or stalemate!\n");
                break;
            }
        }
    }
}