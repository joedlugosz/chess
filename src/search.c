/*
 *  Searching
 */

#include "search.h"

#include <math.h>
#include <time.h>

#include "history.h"
#include "io.h"
#include "movegen.h"
#include "options.h"

const int BOUNDARY = 1000000;

static score_t search_ply(search_job_s *job, state_s *state, int depth, score_t alpha,
                          score_t beta);

/* Search a single move - call search_ply after making the move. In/out args are
   updated on an alpha update: alpha, best_move. Returns 1 for a beta cutoff,
   and 0 in all other cases including impossible moves into check. */
static inline int search_move(search_job_s *job, state_s *state, int depth, score_t *best_score,
                              score_t *alpha, score_t beta, move_s *move, move_s **best_move) {
  state_s next_state;
  copy_state(&next_state, state);
  make_move(&next_state, move);

  /* Can't move into check */
  if (in_check(&next_state)) return 0;

  change_player(&next_state);

  /* Recurse into search_ply */
  score_t score = -search_ply(job, &next_state, depth - 1, -beta, -*alpha);

  if (score > *best_score) {
    *best_score = score;
    *best_move = move;
  }

  /* Alpha update - best move found */
  if (score > *alpha) {
    write_search_history(job, depth, move);
    *alpha = score;

    /* Show the best move if it updates at root level */
    if (depth == job->depth) {
      job->result.score = score;
      memcpy(&job->result.move, move, sizeof(job->result.move));
      xboard_thought(job, depth, score, clock() - job->start_time, job->result.n_leaf);
    }
  }

  DEBUG_THOUGHT(job, depth, score, *alpha, beta);

  /* Beta cutoff */
  if (score >= beta) {
    if (depth >= 0) {
      memcpy(&job->killer_moves[depth], move, sizeof(job->killer_moves[0]));
    }
    return 1;
  }

  return 0;
}

/* Search a single ply and all possible moves - call search_move for each move */
static score_t search_ply(search_job_s *job, state_s *state, int depth, score_t alpha,
                          score_t beta) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);

  /* Count leaf nodes at depth 0 only (even if they extend) */
  if (depth == 0) job->result.n_leaf++;

  score_t best_score = -BOUNDARY;
  move_s *best_move = 0;

  /* Quiescence - evaluate taking no action - this could be better than the
     consequences of taking the piece. */
  if (depth <= 0) {
    best_score = evaluate(state);
    if (best_score >= beta) return beta;
    if (best_score > alpha) alpha = best_score;
  }

  /* Try to get a cutoff from a killer move */
  if ((depth >= 0) && !check_legality(state, &job->killer_moves[depth]) &&
      search_move(job, state, depth, &best_score, &alpha, beta, &job->killer_moves[depth],
                  &best_move))
    return beta;

  /* Generate the move list - list_entry will point to the first sorted item */
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  int n_moves;
  if (depth > 0) {
    n_moves = generate_search_movelist(state, &list_entry);
    if (n_moves == 0) return evaluate(state);
  } else {
    n_moves = generate_quiescence_movelist(state, &list_entry);
    /* Quiet node at depth */
    if (n_moves == 0) return best_score;
  }

  /* Search each move */
  while (list_entry) {
    move_s *move = &list_entry->move;
    list_entry = list_entry->next;
    if (search_move(job, state, depth, &best_score, &alpha, beta, move, &best_move)) return beta;
  }

  /* Update the result if at the top level */
  if (depth == job->depth) {
    job->result.score = alpha;
    memcpy(&job->result.move, best_move, sizeof(job->result.move));
  }

  return alpha;
}

/* Entry point to recursive search */
void search(int depth, state_s *state, search_result_s *res) {
  search_job_s job;
  memset(&job, 0, sizeof(job));
  job.depth = depth;
  job.start_time = clock();

  search_ply(&job, state, depth, -BOUNDARY, BOUNDARY);

  memcpy(res, &job.result, sizeof(*res));
  res->branching_factor = pow((double)res->n_leaf, 1.0 / (double)depth);
  res->time = clock() - job.start_time;
}
