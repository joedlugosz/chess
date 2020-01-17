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

/* Options */
static const option_s options[] = {
  { "Boundary score", INT_OPT,   &boundary,     0, 2000000,          0 },
  { "Show thinking",  BOOL_OPT,  &show,         0, 0,                0 },
  { "New Thinking log every", COMBO_OPT, &(think_log.new_every), 0, 0, &newevery_combo },
};
const options_s search_opts = { 
  sizeof(options)/sizeof(options[0]), options 
};

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

static score_t search_ply(search_context_s *ctx, state_s *state, int depth, score_t alpha, score_t beta)
{
  if(ctx->halt) return 0;

  ASSERT(depth < SEARCH_DEPTH_MAX);
  if(depth > search_depth) {
    /* Evaluate taking no action - i.e. not making any possible capture
      moves, this could be better than the consequences of taking the
      piece.  If this is better than the opponent's beta, it causes a 
      cutoff.  Doing nothing may also be better than the supplied alpha.
      If there are no possible captures, this is equivalent to calling 
      eval() from search().*/
    score_t standing_pat = evaluate(state);
    if(standing_pat >= beta) return beta;
    if(standing_pat > alpha) alpha = standing_pat;
  }
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  int n_moves;
  if(depth < search_depth) {
    n_moves = generate_search_movelist(state, &list_entry);
  } else {
    n_moves = generate_quiescence_movelist(state, &list_entry);
    /* Quiet node at depth */
    if(n_moves == 0) 
      return evaluate(state);
  }
  ctx->n_possible += n_moves;
  while(list_entry) {
    move_s *move = &list_entry->move;
    list_entry = list_entry->next;
    state_s next_state;
    copy_state(&next_state, state);
    make_move(&next_state, move);
    /* Can't move into check */ 
    if(in_check(&next_state)) {
      continue;
    }
    ctx->n_searched++;
    /* Most of the history must be written at this point so it passes to the next recursion */
    write_move_history(ctx, depth, move, state->to_move, 0);
    change_player(&next_state);
    /* Recurse down to search_depth - 1, then do quiescence search if necessary */
    score_t score = -search_ply(ctx, &next_state, depth + 1, -beta, -alpha);
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
  res->n_searched = search.n_searched;
  res->cutoff = (double)search.n_searched / (double)search.n_possible * 100.0f;
}
