#include "bitboard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// --- Global Masks ---
U128 SQUARE_MASKS[90];
U128 CLEAR_MASKS[90];

// --- FEN Mapping ---
static const char FEN_MAP[] = "kabnr cpKABNR CP";
static const Piece FEN_PIECE_MAP[] = {
    B_KING, B_GUARD, B_BISHOP, B_HORSE, B_ROOK, 0, B_CANNON, B_PAWN,
    R_KING, R_GUARD, R_BISHOP, R_HORSE, R_ROOK, 0, R_CANNON, R_PAWN};

static const char PIECE_TO_FEN_CHAR[] = "pcrnbak.KABNRCP";

// --- LSB/MSB Helpers (using GCC/Clang builtins) ---
int get_lsb_index(U128 bb)
{
    if (bb == 0)
        return -1;
    unsigned long long low = (unsigned long long)bb;
    if (low != 0)
    {
        return __builtin_ctzll(low);
    }
    unsigned long long high = (unsigned long long)(bb >> 64);
    return __builtin_ctzll(high) + 64;
}

int get_msb_index(U128 bb)
{
    if (bb == 0)
        return -1;
    unsigned long long high = (unsigned long long)(bb >> 64);
    if (high != 0)
    {
        return 63 - __builtin_clzll(high) + 64;
    }
    return 63 - __builtin_clzll((unsigned long long)bb);
}

int popcount(U128 bb)
{
    return __builtin_popcountll((unsigned long long)bb) + __builtin_popcountll((unsigned long long)(bb >> 64));
}

int get_piece_to_bb_index(Piece p)
{
    if (p > 0)
        return p - 1;
    if (p < 0)
        return abs(p) - 1 + 7;
    return -1;
}

int get_player_bb_idx(int player)
{
    return (player == PLAYER_R) ? 0 : 1;
}

int get_piece_to_zobrist_idx(Piece p)
{
    if (p < 0)
        return abs(p) - 1;
    if (p > 0)
        return p + 6;
    return -1;
}

int get_piece_value(Piece p)
{
    return PIECE_VALUES[abs(p)];
}

void init_bitboard_masks()
{
    for (int i = 0; i < 90; ++i)
    {
        SQUARE_MASKS[i] = (U128)1 << i;
        CLEAR_MASKS[i] = ~SQUARE_MASKS[i];
    }
}

static void set_piece(Board *board, Piece piece_type, int sq)
{
    U128 mask = SQUARE_MASKS[sq];
    int player = (piece_type > 0) ? PLAYER_R : PLAYER_B;
    int r = sq / 9;
    int c = sq % 9;

    board->board[sq] = piece_type;
    board->piece_bitboards[get_piece_to_bb_index(piece_type)] |= mask;
    board->color_bitboards[get_player_bb_idx(player)] |= mask;
    board->hash_key ^= zobrist_keys[get_piece_to_zobrist_idx(piece_type)][r][c];
}

void parse_fen(Board *board, const char *fen)
{
    // 1. Clear board state
    memset(board, 0, sizeof(Board));

    // 2. Parse board layout
    const char *p = fen;
    int rank = 0, file = 0;
    while (*p != ' ')
    {
        if (*p == '/')
        {
            rank++;
            file = 0;
        }
        else if (*p >= '1' && *p <= '9')
        {
            file += *p - '0';
        }
        else
        {
            for (int i = 0; i < 16; ++i)
            {
                if (FEN_MAP[i] == *p)
                {
                    Piece piece = FEN_PIECE_MAP[i];
                    if (piece != EMPTY)
                    {
                        set_piece(board, piece, rank * 9 + file);
                    }
                    break;
                }
            }
            file++;
        }
        p++;
    }

    // 3. Parse player to move
    p++; // skip space
    board->player_to_move = (*p == 'w') ? PLAYER_R : PLAYER_B;
    if (board->player_to_move == PLAYER_B)
    {
        board->hash_key ^= zobrist_player;
    }

    // 4. Store initial hash
    board->history[board->history_ply] = board->hash_key;
}

void init_board(Board *board, const char *fen)
{
    init_bitboard_masks();
    init_zobrist_keys();
    if (fen)
    {
        parse_fen(board, fen);
    }
    else
    {
        parse_fen(board, "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1");
    }
}

void print_board(const Board *board)
{
    printf("\n(Player: %c, Hash: %016llx)\n  +-------------------+\n",
           (board->player_to_move == PLAYER_R) ? 'R' : 'B', board->hash_key);
    for (int r = 0; r < 10; ++r)
    {
        printf("%d | ", 9 - r);
        for (int c = 0; c < 9; ++c)
        {
            Piece p = board->board[r * 9 + c];
            printf("%c ", PIECE_TO_FEN_CHAR[p + 7]);
        }
        printf("|\n");
    }
    printf("  +-------------------+\n    a b c d e f g h i\n\n");
}

Piece move_piece(Board *board, int from_sq, int to_sq)
{
    Piece moving_piece = board->board[from_sq];

    if (moving_piece == EMPTY)
        return EMPTY;

    Piece captured_piece = board->board[to_sq]; // Capture MUST be read before overwriting

    // 1. Update mailbox board

    board->board[from_sq] = EMPTY;
    board->board[to_sq] = moving_piece;

    int r_from = from_sq / 9, c_from = from_sq % 9;
    int r_to = to_sq / 9, c_to = to_sq % 9;

    // 2. Update Zobrist hash for the moving piece
    int moving_z_idx = get_piece_to_zobrist_idx(moving_piece);
    board->hash_key ^= zobrist_keys[moving_z_idx][r_from][c_from];
    board->hash_key ^= zobrist_keys[moving_z_idx][r_to][c_to];

    // 3. Update bitboards for the moving piece
    U128 move_mask = SQUARE_MASKS[from_sq] | SQUARE_MASKS[to_sq];
    board->piece_bitboards[get_piece_to_bb_index(moving_piece)] ^= move_mask;
    board->color_bitboards[get_player_bb_idx(board->player_to_move)] ^= move_mask;

    // 4. Handle capture
    if (captured_piece != EMPTY)
    {
        int captured_z_idx = get_piece_to_zobrist_idx(captured_piece);
        board->hash_key ^= zobrist_keys[captured_z_idx][r_to][c_to];

        int captured_player = (captured_piece > 0) ? PLAYER_R : PLAYER_B;
        board->piece_bitboards[get_piece_to_bb_index(captured_piece)] &= CLEAR_MASKS[to_sq];
        board->color_bitboards[get_player_bb_idx(captured_player)] &= CLEAR_MASKS[to_sq];
    }

    // 5. Switch player and update hash
    board->player_to_move *= -1;
    board->hash_key ^= zobrist_player;

    board->history_ply++;
    board->history[board->history_ply] = board->hash_key;

    return captured_piece;
}

void unmove_piece(Board *board, int from_sq, int to_sq, Piece captured_piece)
{
    board->history_ply--;
    Piece moving_piece = board->board[to_sq];
    int r_from = from_sq / 9, c_from = from_sq % 9;
    int r_to = to_sq / 9, c_to = to_sq % 9;

    // 1. Revert player and hash
    board->player_to_move *= -1;
    board->hash_key ^= zobrist_player;

    // 2. Restore mailbox board
    board->board[from_sq] = moving_piece;
    board->board[to_sq] = captured_piece;

    // 3. Move piece back from to_sq to from_sq
    U128 move_mask = SQUARE_MASKS[from_sq] | SQUARE_MASKS[to_sq];
    board->piece_bitboards[get_piece_to_bb_index(moving_piece)] ^= move_mask;
    board->color_bitboards[get_player_bb_idx(board->player_to_move)] ^= move_mask;

    int moving_z_idx = get_piece_to_zobrist_idx(moving_piece);
    board->hash_key ^= zobrist_keys[moving_z_idx][r_from][c_from];
    board->hash_key ^= zobrist_keys[moving_z_idx][r_to][c_to];

    // 4. Restore captured piece if any
    if (captured_piece != EMPTY)
    {
        int captured_player = (captured_piece > 0) ? PLAYER_R : PLAYER_B;
        board->piece_bitboards[get_piece_to_bb_index(captured_piece)] |= SQUARE_MASKS[to_sq];
        board->color_bitboards[get_player_bb_idx(captured_player)] |= SQUARE_MASKS[to_sq];

        int captured_z_idx = get_piece_to_zobrist_idx(captured_piece);
        board->hash_key ^= zobrist_keys[captured_z_idx][r_to][c_to];
    }
}

void to_fen(const Board *board, char *fen_string)
{
    int char_idx = 0;
    for (int r = 0; r < 10; ++r)
    {
        int empty_count = 0;
        for (int c = 0; c < 9; ++c)
        {
            Piece p = board->board[r * 9 + c];
            if (p == EMPTY)
            {
                empty_count++;
            }
            else
            {
                if (empty_count > 0)
                {
                    fen_string[char_idx++] = empty_count + '0';
                    empty_count = 0;
                }
                fen_string[char_idx++] = PIECE_TO_FEN_CHAR[p + 7];
            }
        }
        if (empty_count > 0)
        {
            fen_string[char_idx++] = empty_count + '0';
        }
        if (r < 9)
        {
            fen_string[char_idx++] = '/';
        }
    }

    fen_string[char_idx++] = ' ';
    fen_string[char_idx++] = (board->player_to_move == PLAYER_R) ? 'w' : 'b';
    strcpy(fen_string + char_idx, " - - 0 1");
}

void copy_board(const Board *src, Board *dest)
{
    memcpy(dest, src, sizeof(Board));
}
