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
#include "pv.h"

#define OPT_KILLER 1
#define OPT_HASH 1

enum { TT_MIN_DEPTH = 4, BOUNDARY = 10000, CHECKMATE_SCORE = -BOUNDARY, DRAW_SCORE = 0 };

move_s mate_move = {.result = CHECK | MATE};

static score_t search_ply(search_job_s *job, struct pv *parent_pv, state_s *state, int depth,
                          score_t alpha, score_t beta);

/* Search a single move - call search_ply after making the move. In/out args are
   updated on an alpha update: alpha, best_move. Returns 1 for a beta cutoff,
   and 0 in all other cases including impossible moves into check. */
static inline int search_move(search_job_s *job, struct pv *parent_pv, struct pv *pv,
                              state_s *state, int depth, score_t *best_score, score_t *alpha,
                              score_t beta, move_s *move, move_s **best_move, tt_type_e *type,
                              int *n_legal) {
  state_s next_state;
  copy_state(&next_state, state);
  make_move(&next_state, move);

  /* Can't move into check */
  if (in_check(&next_state)) return 0;

  /* All other moves are legal options */
  if (n_legal) (*n_legal)++;

  score_t score;
  /* Breaking the threefold repetition rule or 50 move rule forces a draw. */
  if (next_state.halfmove > 50 || is_repeated_position(job->history, next_state.hash, 3)) {
    score = DRAW_SCORE;
  } else {
    /* Normal search - recurse into search_ply */
    history_push(job->history, state->hash, move);
    change_player(&next_state);
    /* Move gives check */
    if (in_check(&next_state)) move->result |= CHECK;
    score = -search_ply(job, pv, &next_state, depth - 1, -beta, -*alpha);
    history_pop(job->history);
  }

  if (score > *best_score) {
    *best_score = score;
    *best_move = move;
  }

  /* Alpha update */
  if (score > *alpha) {
    *alpha = score;
    *type = TT_EXACT;

    /* Update the PV and show it if it updates at root level */
    pv_add(parent_pv, pv, move);
    if (job->show_thoughts && depth == job->depth) {
      xboard_thought(job, parent_pv, depth, score, clock() - job->start_time, job->result.n_leaf);
    }
  }

  DEBUG_THOUGHT(job, parent_pv, depth, score, *alpha, beta);

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

/* Update the result if at the top level */
/* This only happens when score = beta = BOUNDARY...? */
static inline void update_result(search_job_s *job, state_s *state, int depth, move_s *move,
                                 score_t score) {
  if (depth == job->depth) {
    job->result.score = score;
    if (move) {
      memcpy(&job->result.move, move, sizeof(job->result.move));
    }
  }
}

/* Search a single ply and all possible moves - call search_move for each move
 */
static score_t search_ply(search_job_s *job, struct pv *parent_pv, state_s *state, int depth,
                          score_t alpha, score_t beta) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);
  ASSERT(depth <= job->depth);

  /* Count leaf nodes at depth 0 only (even if they extend) */
  if (depth == 0) job->result.n_leaf++;

  /* The first phase of searching a node involves trying to find ways to return
     early, before the work of generating and searching all the possible moves
     for the node. */

  /* If there are any moves, this will point to the best move by the end of the
     function */
  score_t best_score = -BOUNDARY;
  move_s *best_move = 0;

  /* Struct holding the princpal variation of children for this node */
  struct pv pv;
  pv.length = 0;

  /* Quiescence - evaluate taking no action - this could be better than the
     consequences of taking the piece. */
  if (depth <= 0 && !in_check(state)) {
    best_score = evaluate(state);
    if (best_score >= beta) return beta;
    if (best_score > alpha) alpha = best_score;
  }

  /* Default node type - this will change to TT_EXACT on alpha update or TT_BETA
     on beta cutoff */
  tt_type_e type = TT_ALPHA;

  /* Try to get a cutoff from a killer move */
  if (OPT_KILLER && (depth >= 0) && !check_legality(state, &job->killer_moves[depth]) &&
      search_move(job, parent_pv, &pv, state, depth, &best_score, &alpha, beta,
                  &job->killer_moves[depth], &best_move, &type, 0)) {
    update_result(job, state, depth, &job->killer_moves[depth], best_score);
    return beta;
  }

  /* Probe the transposition table, but only at higher levels */
  ttentry_s *tte = 0;
  if (OPT_HASH && depth > TT_MIN_DEPTH) tte = tt_probe(state->hash);

  /* If the position has already been searched at the same or greater depth, use
     the result from the tt. At root level, accapt only exact and copy out the
     move */
  if (tte && (tte->depth >= depth)) {
    if (depth < job->depth) {
      if (tte->type == TT_ALPHA && tte->score > alpha) return alpha;
      if (tte->type == TT_BETA && tte->score > beta) return beta;
      if (tte->type == TT_EXACT) return tte->score;
    } else {
      if (tte->type == TT_EXACT) {
        update_result(job, state, depth, &tte->best_move, tte->score);
        return tte->score;
      }
    }
  }

  /* If there is a best move from the transposition table, try searching it
     first. A beta cutoff will avoid move generation, otherwise alpha will get a
     good starting value. */
  if (tte && !check_legality(state, &tte->best_move) &&
      search_move(job, parent_pv, &pv, state, depth, &best_score, &alpha, beta, &tte->best_move,
                  &best_move, &type, 0)) {
    update_result(job, state, depth, &tte->best_move, best_score);
    return beta;
  }

  /* The second phase of searching a node occurs if an early exit was not
     achieved. This involves a search through all possible moves for this
     position, with an earlier exit if a beta cutoff is found. */

  /* Generate the list of pseudo-legal moves. These are moves which appear to be
     legal without considering check. It is easier to test for check once the
     move has been made, then abandon the move if it turns out to be illegal. To
     assist with sorting, moves are are stored as a linked list within the array
     move_buf. list_entry will point to the first sorted item. In front of the
     search horizon, a list is generated for a normal search, with all moves
     that can be made for all pieces. At and beyond the horizon, a separate
     function is used to generate a list of only moves which are interesting
     enough to extend the search, such as a capture of the piece just moved.
     If no moves are generated, recursion stops. */
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry = move_buf;
  int n_pseudo_legal_moves;
  if (depth > 0 || state->check[WHITE] || state->check[BLACK]) {
    n_pseudo_legal_moves = generate_search_movelist(state, &list_entry);
  } else {
    n_pseudo_legal_moves = generate_quiescence_movelist(state, &list_entry);
    if (n_pseudo_legal_moves == 0) return best_score;
  }

  /* Search through the list of pseudo-legal moves. search_move will update
     best_score, best_move, alpha, and type, and will increment n_legal_moves
     whenever a genuine legal move is found which does not lead into check */
  int n_legal_moves = 0;
  if (n_pseudo_legal_moves > 0) {
    while (list_entry) {
      if (!(tte && move_equal(&tte->best_move, &list_entry->move))) {
        if (search_move(job, parent_pv, &pv, state, depth, &best_score, &alpha, beta,
                        &list_entry->move, &best_move, &type, &n_legal_moves)) {
          update_result(job, state, depth, &list_entry->move, beta);
          return beta;
        }
      }
      list_entry = list_entry->next;
    }
  }

  /* If no genuine legal moves were found in the search, it means a checkmate or
     stalemate. For checkmate, reduce the checkmate score by the distance from
     root to mate, so that the shortest path to mate has the strongest score.
     Stalemate causes a draw score of zero, which is a goal for the losing side */
  if (n_legal_moves == 0) {
    if (in_check(state)) {
      alpha = CHECKMATE_SCORE + (job->depth - depth);
      best_move = &mate_move;
    } else {
      alpha = DRAW_SCORE;
    }
  }

  /* Update the result if at the top level */
  update_result(job, state, depth, best_move, alpha);

  /* Update the transposition table at higher levels */
  if (depth > TT_MIN_DEPTH) {
    tt_update(state->hash, type, depth, alpha, best_move);
  }

  return alpha;
}

/* Entry point to recursive search */
void search(int depth, struct history *history, state_s *state, search_result_s *res,
            int show_thoughts) {
  search_job_s job;
  memset(&job, 0, sizeof(job));
  job.depth = depth;
  job.start_time = clock();
  job.history = history;
  job.show_thoughts = show_thoughts;

  tt_zero();

  struct pv pv;
  search_ply(&job, &pv, state, job.depth, -BOUNDARY, BOUNDARY);

  memcpy(res, &job.result, sizeof(*res));
  res->branching_factor = pow((double)res->n_leaf, 1.0 / (double)depth);
  res->time = clock() - job.start_time;
  res->collisions = tt_collisions();
}
