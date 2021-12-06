
#ifndef SYS_H
#define SYS_H

# include "chess.h"
# include "board.h"
# include "search.h"

typedef struct engine_s_ {
  enum {
    ENGINE_PLAYING_AS_WHITE = WHITE,
    ENGINE_PLAYING_AS_BLACK = BLACK,
    ENGINE_FORCE_MODE,
    ENGINE_ANALYSE_MODE,
    ENGINE_QUIT
  } mode;

  int xboard_mode;
  int waiting;
  int resign_delayed;
  clock_t start_time;
  clock_t elapsed_time;
  
  unsigned long otim;
  int move_n;
  int game_n;
  
  state_s game;

  int depth;

} engine_s;

/* command.c*/
int accept_command(engine_s *e, const char *in);

int no_piece_at_pos(engine_s *, pos_t);
int move_is_illegal(engine_s *, move_s *);

enum { POS_BUF_SIZE = 3, MOVE_BUF_SIZE = 10 };
int parse_pos(const char *, pos_t *);
int parse_move(const char *, move_s *);
int format_pos(char *, pos_t);
int format_move(char *, move_s *, int);

void list_features(void);
void feature_accepted(const char *name);
void list_options(void);
int set_option(engine_s *e, const char *name);

int get_fen(const state_s *state, char *out, size_t outsize);
int load_fen(state_s *state, const char *placement, const char *active,
             const char *castling, const char *en_passant);


#endif /* SYS_H */
