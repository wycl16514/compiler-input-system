#ifndef __L_H
#define __L_H
extern  int ii_newfile(char* name);
extern  unsigned char* ii_text();
extern  int ii_flush(int force);
extern  int ii_length();
extern  int ii_lineno();
extern  unsigned char* ii_ptext();
extern  int ii_plength();
extern  int ii_plineno();
extern  unsigned  char* ii_mark_start();
extern  unsigned  char* ii_mark_end();
extern  unsigned  char* ii_move_start();
extern  unsigned  char* ii_to_mark();
extern  unsigned  char* ii_mark_prev();
extern  int ii_advance();
extern  int ii_flush(int force);
extern  int ii_fillbuf(unsigned  char* starting_at);
extern  int ii_look(int n);
extern  int ii_pushback(int n);
extern  void ii_term();
extern  void ii_unterm();
extern  int ii_input();
extern  void ii_unput(int c);
extern  int ii_lookahead(int n);
extern  int ii_flushbuf();
#endif