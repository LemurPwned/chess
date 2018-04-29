#include "mov_register.h"

void add_move_to_register(struct chess_register *cr, struct chess_move *cm){
  printf("Adding a move: (%s, %s)\n", cm->white, cm->black);
  cr->reg[++cr->t_count] = *cm;
}

bool validate_move(char *move){
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

char* print_move(struct chess_move *cm, char buffer[30]){
  sprintf(buffer, "%d.%s %s", cm->move_id, cm->white, cm->black);
  return buffer;
}
void print_register(struct chess_register *cr){
  char buffer[30];
  char *res;
  for (int i = 1; i <= cr->t_count; i++){
    res = print_move(&cr->reg[i], buffer);
    printf("%s\n", res);
  }
}

void regex_test(){
  char test_regex[10][6]  = {"q3", "g3", "qg3", "xg3", "3", "faf", "qg"};
  for (int i = 0; i  < 6; i++){
    printf("Current move %s: ", test_regex[i]);
    if (validate_move(test_regex[i])){
      printf("Matched\n");
    }
    else{
      printf("No match\n");
    }
  }
}

void register_test(){
  struct chess_register cr;
  bzero(&cr, sizeof(cr));
  struct chess_move current_move = {0, "g3", "qh3"};
  add_move_to_register(&cr, &current_move);
  struct chess_move current_move2 = {1, "h3", "ka1"};
  add_move_to_register(&cr, &current_move2);
  print_register(&cr);
}
