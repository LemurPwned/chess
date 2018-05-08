#ifndef mov_register_h
#define mov_register_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include <assert.h>

#define MAX_MOVES 100
#define SINGLE_MOVE_LEN 150


struct chess_move{
  int move_id;
  char white[SINGLE_MOVE_LEN];
  char black[SINGLE_MOVE_LEN];
};
struct chess_register{
  struct chess_move reg[MAX_MOVES];
  int t_count;
};

void add_move_to_register(struct chess_register *cr, struct chess_move *cm);
void print_register(struct chess_register *cr);
char* print_move(struct chess_move *cm, char buffer[300]);
char* get_register(struct chess_register *cr, char large_buffer[1000]);
bool validate_move(char *move);

// tests
void register_test();
void regex_test();

#endif
