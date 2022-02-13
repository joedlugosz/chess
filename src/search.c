/*
 *  Searching
 */

#include "search.h"

#include <time.h>

#include "history.h"
#include "io.h"
#include "movegen.h"

int search_depth = 7;
int search_best = 5;
int boundary = 1000000;
int log_search = 1;
int show = 1;
log_s think_log = {.new_every = NE_MOVE};

/* Options */
static const option_s options[] = {
    {"Search depth", INT_OPT, .value.integer = &search_depth, 1, 10, 0},
    {"Boundary score", INT_OPT, .value.integer = &boundary, 0, 2000000, 0},
    {"Show thinking", BOOL_OPT, .value.integer = &show, 0, 0, 0},
    {"New Thinking log every", COMBO_OPT, .value.integer = (int *)&(think_log.new_every), 0, 0,
     &newevery_combo},
};
const options_s search_opts = {sizeof(options) / sizeof(options[0]), options};

static score_t search_ply(search_job_s *job, state_s *state, int depth, score_t alpha,
                          score_t beta) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);
  
  if (depth <= 0) {
    /* Evaluate taking no action - i.e. not making any possible capture
      moves, this could be better than the consequences of taking the
      piece.  If this is better than the opponent's beta, it causes a
      cutoff.  Doing nothing may also be better than the supplied alpha.
      If there are no possible captures, this is equivalent to calling
      evaluate() from search().*/
    score_t standing_pat = evaluate(state);
    if (standing_pat >= beta) return beta;
    if (standing_pat > alpha) alpha = standing_pat;
  }
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  int n_moves;
  if (depth > 0) {
    n_moves = generate_search_movelist(state, &list_entry);
  } else {
    n_moves = generate_quiescence_movelist(state, &list_entry);
    /* Quiet node at depth */
    if (n_moves == 0) return evaluate(state);
  }
  job->result.n_possible += n_moves;
  while (list_entry) {
    move_s *move = &list_entry->move;
    list_entry = list_entry->next;
    state_s next_state;
    copy_state(&next_state, state);
    make_move(&next_state, move);
    /* Can't move into check */
    if (in_check(&next_state)) {
      continue;
    }
    job->result.n_searched++;
    change_player(&next_state);
    score_t score = -search_ply(job, &next_state, depth - 1, -beta, -alpha);
    write_search_history(job, depth, move);
    /* Alpha update - best move found */
    if (score > alpha) {
      alpha = score;
      /* Update the chosen move if found at the top level */
      if (depth == job->depth) {
        job->result.score = score;
        memcpy(&job->result.move, move, sizeof(job->result.move));
        xboard_thought(stdout, job, depth, score, clock() - job->start_time,
                       job->result.n_searched++);
      }
    }
    /* Beta cutoff */
    if (score >= beta) {
      LOG_THOUGHT(job, depth, score, alpha, beta);
      job->result.n_beta++;
      return beta;
    }
    LOG_THOUGHT(job, depth, score, alpha, beta);
  }
  return alpha;
}

void search(int depth, state_s *state, search_result_s *res) {
  search_job_s job;
  memset(&job, 0, sizeof(job));
  job.depth = depth;
  job.start_time = clock();
  search_ply(&job, state, depth, -boundary, boundary);
  memcpy(res, &job.result, sizeof(*res));
  res->cutoff = 100.0 - (double)res->n_searched / (double)res->n_possible * 100.0;
  res->time = clock() - job.start_time;
}
