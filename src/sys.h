
#ifndef SYS_H
#define SYS_H

# include "chess.h"
# include "board.h"
# include "search.h"

typedef struct engine_s_ {
  enum {
    ENGINE_PLAYING_AS_WHITE = WHITE,
    ENGINE_PLAYING_AS_BLACK = BLACK,
    FORCE_MODE,
    ANALYSE_MODE,
    QUIT
  } engine_mode;

  player_e side_to_move;
  
  int xboard_mode;
  player_e ai_player;
  int run;
  int force;
  int waiting;
  int resign;
  clock_t ui_clock;
  
  unsigned long otim;
  int move_n;
  int game_n;
  
  state_s game;

  ai_result_s result;

} engine_s;

extern engine_s engine;
//extern state_s ui_game;


/* command.c*/
typedef void (*ui_fn)(engine_s *);
int accept_command(engine_s *e, const char *in);

/* moves.c */
int check_force_move(state_s *state, pos_t from);
int check_move(state_s *state, pos_t from, pos_t to);
//void ai_move(void);
//int process_move(const char *in);

int decode_position(const char *instr, pos_t *pos);
int decode_instruction(const char *instr, pos_t *from, pos_t *to);
int encode_position(char *instr, pos_t pos);
//void encode_instruction(char *instr, pos_t from, pos_t to);
int encode_move(char *instr, pos_t from, pos_t to, int check, int capture);
int encode_move_bare(char *instr, pos_t from, pos_t to);

void list_features(void);
void feature_accepted(const char *name);
void list_options(void);
int set_option(engine_s *e, const char *name);

/* io.c */
//const char *get_line(void);
const char *get_input(void);
const char *get_delim(char delim);

/* ui.c */
void print_board(FILE *f, state_s *state, plane_t hl1, plane_t hl2);
void print_plane(FILE *f, plane_t plane, plane_t indicator);
void print_plane_rank(FILE *f, unsigned char rank, unsigned char indicator);

void print_thought_moves(FILE *f, int depth, notation_s moves[]);

/* build.c */
void print_program_info();

int get_fen(const state_s *state, char *out, size_t outsize);
int load_fen(state_s *state, const char *placement, const char *active,
             const char *castling, const char *en_passant);


#endif /* SYS_H */
