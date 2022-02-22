/*
 *  Searching
 */

#include "search.h"

#include <math.h>
#include <time.h>

#include "hash.h"
#include "history.h"
#include "io.h"
#include "movegen.h"
#include "options.h"

const int BOUNDARY = 1000000;
enum { TT_MIN_DEPTH = 4 };

static score_t search_ply(search_job_s *job, state_s *state, int depth, score_t alpha,
                          score_t beta);

/* Search a single move - call search_ply after making the move. In/out args are
   updated on an alpha update: alpha, best_move. Returns 1 for a beta cutoff,
   and 0 in all other cases including impossible moves into check. */
static inline int search_move(search_job_s *job, state_s *state, int depth, score_t *best_score,
                              score_t *alpha, score_t beta, move_s *move, move_s **best_move,
                              tt_type_e *type) {
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
    *type = TT_EXACT;

    /* Show the best move if it updates at root level */
    if (depth == job->depth) {
      xboard_thought(job, depth, score, clock() - job->start_time, job->result.n_leaf);
    }
  }

  DEBUG_THOUGHT(job, depth, score, *alpha, beta);

  /* Beta cutoff */
  if (score >= beta) {
    if (depth >= 0) {
      memcpy(&job->killer_moves[depth], move, sizeof(job->killer_moves[0]));
    }
    *type = TT_BETA;
    return 1;
  }

  return 0;
}

/* Search a single ply and all possible moves - call search_move for each move */
static score_t search_ply(search_job_s *job, state_s *state, int depth, score_t alpha,
                          score_t beta) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);
  ASSERT(depth <= job->depth);

  /* Count leaf nodes at depth 0 only (even if they extend) */
  if (depth == 0) job->result.n_leaf++;

  /* If there are any moves, this will point to the best move by the end of the
     function */
  score_t best_score = -BOUNDARY;
  move_s *best_move = 0;

  /* Quiescence - evaluate taking no action - this could be better than the
     consequences of taking the piece. */
  if (depth <= 0) {
    best_score = evaluate(state);
    if (best_score >= beta) return beta;
    if (best_score > alpha) alpha = best_score;
  }

  /* Default node type - this will change to TT_EXACT on alpha update or
     TT_BETA on beta cutoff */
  tt_type_e type = TT_ALPHA;

  /* Try to get a cutoff from a killer move */
  if ((depth >= 0) && !check_legality(state, &job->killer_moves[depth]) &&
      search_move(job, state, depth, &best_score, &alpha, beta, &job->killer_moves[depth],
                  &best_move, &type))
    return beta;

  /* Probe the transposition table, but only at higher levels */
  ttentry_s *tte = 0;
  if (depth > TT_MIN_DEPTH) tte = tt_probe(state->hash);

  /* If the position has already been searched at the same or greater depth,
     use the result from the tt. At root level, accapt only exact and copy
     out the move */
  if (tte && (tte->depth >= depth)) {
    if (depth < job->depth) {
      if (tte->type == TT_ALPHA && tte->score > alpha) return alpha;
      if (tte->type == TT_BETA && tte->score > beta) return beta;
      if (tte->type == TT_EXACT) return tte->score;
    } else {
      if (tte->type == TT_EXACT) {
        job->result.score = alpha;
        memcpy(&job->result.move, &tte->best_move, sizeof(job->result.move));
        return tte->score;
      }
    }
  }

  /* If there is a best move from the transposition table, try searching it
     first. A beta cutoff will avoid move generation, otherwise alpha will
     get a good starting value. */
  if (tte && !check_legality(state, &tte->best_move)) {
    if (search_move(job, state, depth, &best_score, &alpha, beta, &tte->best_move, &best_move,
                    &type))
      return beta;
  }

  /* Generate the move list - list_entry will point to the first sorted item */
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  int n_moves;
  if (depth > 0) {
    n_moves = generate_search_movelist(state, &list_entry);
    /* Checkmate or stalemate */
    if (n_moves == 0) return evaluate(state);
  } else {
    n_moves = generate_quiescence_movelist(state, &list_entry);
    /* A quiet node has been found at depth */
    if (n_moves == 0) return best_score;
  }

  /* Search each move, skipping the best move from the tt which has already been searched */
  while (list_entry) {
    if (!(tte && move_equal(&tte->best_move, &list_entry->move))) {
      if (search_move(job, state, depth, &best_score, &alpha, beta, &list_entry->move, &best_move,
                      &type))
        return beta;
    }
    list_entry = list_entry->next;
  }

  /* Update the result if at the top level */
  if (depth == job->depth && best_move) {
    job->result.score = alpha;
    memcpy(&job->result.move, best_move, sizeof(job->result.move));
  }

  /* Update the transposition table at higher levels */
  if (depth > TT_MIN_DEPTH) {
    tt_update(state->hash, type, depth, alpha, best_move);
  }

  return alpha;
}

/* Entry point to recursive search */
void search(int depth, state_s *state, search_result_s *res) {
  search_job_s job;
  memset(&job, 0, sizeof(job));
  job.depth = depth;
  job.start_time = clock();

  tt_zero();

  search_ply(&job, state, job.depth, -BOUNDARY, BOUNDARY);

  memcpy(res, &job.result, sizeof(*res));
  res->branching_factor = pow((double)res->n_leaf, 1.0 / (double)depth);
  res->time = clock() - job.start_time;
  res->collisions = tt_collisions();
}
