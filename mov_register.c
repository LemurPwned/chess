#include "mov_register.h"

void add_move_to_register(struct chess_register *cr, struct chess_move *cm){
  printf("sizeof reg %d, sizeof cm %d\n", sizeof(cr->reg[0]), sizeof(cm));
  printf("Adding a move: (%s, %s)\n", cm->white, cm->black);
  cr->reg[cr->t_count++] = *cm;
}

bool validate_move(char *move){
  if ((strlen(move) > 3) || (strlen(move) < 2)){
    return false;
  }
  // allow all figures and no letter if pawn is to be moved
  char *regex_string = "^([kqrbn]{1})?([a-h])([1-8])";
  regex_t regex_compiled;
  if (regcomp(&regex_compiled, regex_string, REG_EXTENDED)){
    printf("Could not compile regex\n");
    return false;
  }

  int match = regexec(&regex_compiled, move, 0, NULL, 0);
  if (match) {
    return false;
  }
  else{
    return true;
  }
  return false;
}

char* print_move(struct chess_move *cm, char buffer[300]){
  sprintf(buffer, "%d.%s %s\n", cm->move_id, cm->white, cm->black);
  return buffer;
}

void print_register(struct chess_register *cr){
  char buffer[300];
  char *res;
  for (int i = 0; i <= cr->t_count; i++){
    bzero(&buffer, sizeof(buffer));
    res = print_move(&cr->reg[i], buffer);
    printf("%s\n", res);
  }
}

char* get_register(struct chess_register *cr, char large_buffer[1000]){
  char buffer[300];
  char *res;
  strcpy(large_buffer, "\n");
  for (int i = 0; i <= cr->t_count; i++){
    bzero(&buffer, sizeof(buffer));
    res = print_move(&cr->reg[i], buffer);
    strcat(large_buffer, res);
    strcat(large_buffer, "\n");
  }
}


void regex_test(){
  char test_regex[10][6]  = {"q3", "g3", "qg3", "xg3", "3", "faf", "qg"};
  int test_result[10] = {0, 1, 1, 0, 0, 0, 0};
  for (int i = 0; i  < 6; i++){
    assert(validate_move(test_regex[i]) == test_result[i]);
  }
}

void register_test(){
  struct chess_register cr;
  char test_buffer[300];
  char test_buffer2[300] = {"0.g3 qh3"};
  bzero(&cr, sizeof(cr));
  struct chess_move current_move = {0, "g3", "qh3"};
  print_move(&current_move, test_buffer);
  assert(strcmp(test_buffer, test_buffer2) == 0);
  add_move_to_register(&cr, &current_move);
  struct chess_move current_move2 = {1, "h3", "ka1"};
  add_move_to_register(&cr, &current_move2);
  print_register(&cr);
}
