/*
 *
 *  Searching - Negamax Alpha-Beta
 *  Quiescence search
 *
 *  3.x           - Originals
 *  4.2           - Search entry point no longer changes to_move, handled by ui.c
 *  4.3           - Rationalised search functions
 *  4.4           - Common functions moved to search.c
 *  5.0  02/2019  - Moved back into one file, other search techniqes superseded
 *                  Cutoff stats
 *                  Changed default search depth
 */

#include "search.h"
#include "movegen.h"
#include "history.h"
#include "sys.h"

int search_depth = 6;
int search_best = 5;
int boundary = 1000000;
int log_search = 1;
int show = 1;
log_s think_log = {
  .new_every = NE_MOVE
};

enum {
  N_SEARCH_OPTS = 4
};

const score_t player_factor[N_PLAYERS] = { 1, -1 };

/* Options */
static const option_s options[] = {
  { "Boundary score", INT_OPT,   &boundary,     0, 2000000,          0 },
  { "Show thinking",  BOOL_OPT,  &show,         0, 0,                0 },
  { "New Thinking log every", COMBO_OPT, &(think_log.new_every), 0, 0, &newevery_combo },
};
const options_s search_opts = { 
  sizeof(options)/sizeof(options[0]), options 
};

//clock_t search_start_time;
//int search_nodes;
//int session_extreme;
//player_e ai_player;
//int n_ai_moves;
//static volatile int running = 0;


/* implemented in log.c */
//void log_thought(log_s *log, search_s *ctx, int depth, score_t score, score_t alpha, score_t beta);
/* in this file */
//static score_t search(search_s *ctx, state_s *state, int depth, score_t alpha, score_t beta);


void set_depth(int d)
{
  search_depth = d;
}

/*
 *  Thought Printing
 */
/* Thoughts shown by XBoard */
void xboard_thought(FILE *f, search_context_s *ctx, int depth, score_t score, clock_t time, int nodes)
{
  fprintf(f, "\n%2d %7d %7lu %7d ", depth, score, time / (CLOCKS_PER_SEC / 100), nodes); 
  print_thought_moves(f, depth, ctx->search_history);
}
/* Thought logging stuff */
#ifndef LOGGING
# define LOG_THOUGHT(c, d, s, a, b)
#else
# define LOG_THOUGHT(c, d, s, a, b) log_thought(&think_log, c, d, s, a, b)
void debug_thought(FILE *f, search_context_s *ctx, 
  int depth, score_t score, score_t alpha, score_t beta)
{
  fprintf(f, "\n%2d %10d ", depth, ctx->n_searched);
  if(alpha > -100000) fprintf(f, "%7d ", alpha); 
  else                fprintf(f, "     -B "); 
  if(beta  <  100000) fprintf(f, "%7d ", beta); 
  else                fprintf(f, "     +B "); 
  print_thought_moves(f, depth, ctx->search_history);
}

void log_thought(log_s *log, search_context_s *ctx, 
  int depth, score_t score, score_t alpha, score_t beta)
{
  if(log->logging) {
    open_log(log);
    debug_thought(log->file, ctx, depth, score/10, alpha/10, beta/10);
    close_log(log);
  }
}
#endif


/* Quiescence search */
score_t quiesce(search_context_s *ctx, state_s *current_state, 
    int depth, score_t alpha, score_t beta)
{
  if(ctx->halt) {
    return 0;
  }

  ASSERT(depth < SEARCH_DEPTH_MAX);
  /*ASSERT(piece_player[(int)current_state->piece_at[current_state->to]]
   != current_state->to_move);*/

  /* to_move has already changed as a result of previous move */
  player_e attacking = current_state->to_move;
  /* Evaluate taking no action - i.e. not making any possible capture
     moves, this could be better than the consequences of taking the
     piece. */
  score_t stand_pat = eval(current_state) * player_factor[attacking];
  /* If this is better than the opponent's beta, it causes a cutoff. */
  if(stand_pat >= beta) {
    return beta;
  }
  /* Doing nothing may also be better than the supplied alpha.  If there
     are no possible captures, this is equivalent to calling eval() from
     search(). */
  if(stand_pat > alpha) {
    alpha = stand_pat;
  }

  pos_t to = current_state->to;
  
  /* Get all the pieces which can attack the piece that has just been
     moved */
  plane_t attacks = get_attacks(current_state, to, attacking);
  while(attacks) {
    pos_t from = mask2pos(next_bit_from(&attacks));
    ASSERT(from != to);
    move_s move = { from, to, is_promotion_move(current_state, from, pos2mask[to])
      ? QUEEN : PAWN };
    do {
      state_s next_state;
      copy_state(&next_state, current_state);
      make_move(&next_state, &move);    
      /* Can't move into check */
      if(in_check(&next_state))
        break;
      /* This is a valid move, increment nodes */
      ctx->n_searched++;
      /* Recurse as opponent */
      change_player(&next_state);
      score_t score = -quiesce(ctx, &next_state, depth + 1, -beta, -alpha);
      /* Beta cutoff */
      if(score >= beta) {
        LOG_THOUGHT(ctx, depth, score, alpha, beta);
        return beta;
      }
      /* Alpha update */
      if(score > alpha) {
        alpha = score;
      }
      LOG_THOUGHT(ctx, depth, score, alpha, beta);
    } while(--move.promotion > PAWN);
  }
  /* alpha holds the score of the best possible outcome of the search */
  return alpha;
}

static score_t search_ply(search_context_s *ctx, state_s *state, int depth, score_t alpha, score_t beta)
{
  if(ctx->halt) return 0;

  ASSERT(depth < SEARCH_DEPTH_MAX);

  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  ctx->n_possible += gen_moves(state, &list_entry);

  while(list_entry) {
    ctx->n_searched++;
    move_s *move = &list_entry->move;
    state_s next_state;
    copy_state(&next_state, state);
    make_move(&next_state, move);
    /* Can't move into check */ 
    if(in_check(&next_state)) {
      list_entry = list_entry->next;
      continue;
    }
    /* Most of the history must be written at this point so it passes to the next recursion */
    write_move_history(ctx, depth, move, state->to_move, 0);
    /* Can't repeat a move, relaxed if in check */
    /* TODO: hashing */
    if(is_repeated_move(ctx, depth, move) && !in_check(state)) {
      list_entry = list_entry->next;
      continue;
    }
    change_player(&next_state);
    /* Recurse down to search_depth - 1, then do quiescence search if necessary */
    score_t score;
    if(depth < search_depth - 1) {
      score = -search_ply(ctx, &next_state, depth + 1, -beta, -alpha);
    } else {
      score = -quiesce(ctx, &next_state, depth + 1, -beta, -alpha);
    }
    ctx->search_history[depth].score = score;
    /* Alpha update - best move found */
    if(score > alpha) {
      alpha = score;
      /* Record the move if found at the top level */
      if(depth == 0) {
        memcpy(ctx->best_move, move, sizeof(*ctx->best_move));
      }
    }
    /* Beta cutoff */
    if(score >= beta) {
      LOG_THOUGHT(ctx, depth, score, alpha, beta);
      ctx->n_beta++;
      return beta;
    }
    LOG_THOUGHT(ctx, depth, score, alpha, beta);
    /* Next move in the list */
    list_entry = list_entry->next;
  }
  return alpha;
}

void do_search(int depth, state_s *state, search_result_s *res)
{
  search_context_s search;
  memset(&search, 0, sizeof(search));
  search.horizon_depth = depth;
  search.best_move = &res->best_move;
  res->score = search_ply(&search, state, 0, -boundary, boundary);
  //memcpy(&res->best_move, &search.best_move, sizeof(res->best_move));
  res->n_searched = search.n_searched;
  res->cutoff = (double)search.n_searched / (double)search.n_possible * 100.0f;
}
