#include "opening_book.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BookEntry* opening_book = NULL;
static int book_size = 0;

void load_opening_book(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Could not open opening book file: %s\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size > 0) {
        book_size = file_size / sizeof(BookEntry);
        opening_book = (BookEntry*)malloc(file_size);
        if (opening_book) {
            fread(opening_book, sizeof(BookEntry), book_size, file);
            printf("Opening book loaded with %d entries.\n", book_size);
        } else {
            printf("Failed to allocate memory for opening book.\n");
            book_size = 0;
        }
    }

    fclose(file);
}

Move query_opening_book(Board* board) {
    if (!opening_book || book_size == 0) {
        return (Move){0, 0};
    }

    uint64_t current_hash = board->hash_key;
    Move possible_moves[MAX_MOVES];
    int num_possible_moves = 0;

    for (int i = 0; i < book_size; ++i) {
        if (opening_book[i].hash_key == current_hash) {
            if (num_possible_moves < MAX_MOVES) {
                possible_moves[num_possible_moves++] = opening_book[i].move;
            }
        }
    }

    if (num_possible_moves > 0) {
        // Return a random move from the possible moves
        return possible_moves[rand() % num_possible_moves];
    }

    return (Move){0, 0};
}
