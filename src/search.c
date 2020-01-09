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
const option_s _search_opts[N_SEARCH_OPTS] = {
  { "Search depth",   SPIN_OPT,  &search_depth, 0, SEARCH_DEPTH_MAX, 0 },
  { "Boundary score", INT_OPT,   &boundary,     0, 2000000,          0 },
  { "Show thinking",  BOOL_OPT,  &show,         0, 0,                0 },
  { "New Thinking log every", COMBO_OPT, &(think_log.new_every), 0, 0, &newevery_combo },
};
const options_s search_opts = { N_SEARCH_OPTS, _search_opts };

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
  plane_t attacks;
  score_t score, stand_pat;
  state_s next_state;
  pos_t from, to;
  /* to_move has already changed as a result of previous move */
  player_e attacking = current_state->to_move;
  /* Limit for search and repeat history */
  ASSERT(depth < SEARCH_DEPTH_MAX);
  //ASSERT(piece_player[(int)current_state->piece_at[current_state->to]] != current_state->to_move);
  /* TODO: ctx->running will be used to break out of a search */
  if(ctx->halt) {
    return 0;
  }
  /* Evaluate taking no action - i.e. not making any possible capture
     moves, this could be better than the consequences of taking the
     piece. */
  stand_pat = eval(current_state) * player_factor[attacking];  // ???
  /* If this is better than the opponent's beta, it causes a cutoff. */
  if(stand_pat >= beta) {
    return beta;
  }
  /* Doing nothing may be better than the supplied alpha.  If there
     are no possible captures, this is equivalent to calling eval() from
     search().
     TODO: Can this logic be moved to search() so the call is avoided 
   */
  if(stand_pat > alpha) {
    alpha = stand_pat;
  }

  to = current_state->to;
  
  /* Get all the pieces which can attack the piece that has just been
     moved */
  attacks = get_attacks(current_state, to, attacking);
  /* For each piece */
  while(attacks) {
    /* Get from-position of the attacking piece */
    from = mask2pos(next_bit_from(&attacks));
    ASSERT(from != to);
    /* Copy into next_state and do the move */
    copy_state(&next_state, current_state);
    /* TODO: what about taking and promoting? */
    do_move(&next_state, from, to, 0);    
    /* Can't move into check */
    if(in_check(&next_state))
      continue;
    /* This is a valid move, increment nodes */
    ctx->n_searched++;
    /* Recurse as opponent */
    change_player(&next_state);
    score = -quiesce(ctx, &next_state, depth + 1, -alpha, -beta);
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
  }
  /* alpha holds the score of the best possible outcome of the search */
  return alpha;
}

static score_t search_ply(search_context_s *ctx, state_s *state, int depth, score_t alpha, score_t beta)
{
  /* Limit for search and repeat history */
  ASSERT(depth < SEARCH_DEPTH_MAX);
  /* ctx->halt breaks out of a search */
  if(ctx->halt) return 0;
  state_s next_state;
  /* Buffer to be used by gen_moves */
  move_s move_buf[N_MOVES];
  /* Generate moves */
  //  move_s *move = gen_moves(ctx, state, move_buf, depth);  
  move_s *move = move_buf;

  ctx->n_possible += gen_moves(state, &move);  
  while(move) {
    /* Evaluation score */
    score_t score;
    ctx->n_searched++;
    //    printf("%d %d\n", ctx->n_nodes, depth);
    /* Copy and do the move so the result is in next_state */
    copy_state(&next_state, state);
    do_move(&next_state, move->from, move->to, move->promotion);
    /* Can't move into check */ 
    if(in_check(&next_state)) {
      move = move->next;
      continue;
    }
    /* Most of the history must be written at this point so it passes to the next recursion */
    write_move_history(ctx, depth, move->from, move->to, state->to_move, 0);
    /* Can't repeat a move, relaxed if in check */
    /* TODO: hashing */
    if(is_repeated_move(ctx, depth, move->from, move->to) && !in_check(state)) {
      move = move->next;
      continue;
    }
    change_player(&next_state);
  /* Recurse down to search_depth - 1, then do quiescence search if necessary */
    if(depth < search_depth - 1) {
      score = -search_ply(ctx, &next_state, depth + 1, -beta, -alpha);
    } else {
      score = -quiesce(ctx, &next_state, depth + 1, -beta, -alpha);
    }
    ctx->search_history[depth].score = score;
    /* Beta cutoff */
    if(score >= beta) {
      LOG_THOUGHT(ctx, depth, score, alpha, beta);
      ctx->n_beta++;
      return beta;
    }
    /* Alpha update - best move found */
    if(score > alpha) {
      alpha = score;
      /* Record the move if found at the top level */
      if(depth == 0) {
        ctx->best_move.from = move->from;
        ctx->best_move.to = move->to;
        ctx->best_move.promotion = move->promotion;
      }
    }
    LOG_THOUGHT(ctx, depth, score, alpha, beta);
    /* Next move in the list */
    move = move->next;
  }
  return alpha;
}

void do_search(state_s *state, search_result_s *res)
{
  search_context_s search;
  memset(&search, 0, sizeof(search));
  search.halt = 0;
  search.best_move.from = EMPTY;  
  res->score = search_ply(&search, state, 0, -boundary, boundary);
  memcpy(&res->best_move, &search.best_move, sizeof(res->best_move));
  res->n_searched = search.n_searched;
  res->cutoff = (double)search.n_searched / (double)search.n_possible * 100.0f;
}
