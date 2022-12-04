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
#include "os.h"
#include "pv.h"

#define OPT_KILLER 1
#define OPT_HASH 1

enum {
  TT_MIN_DEPTH = 4,
  INFINITY_SCORE = 10000,
  CHECKMATE_SCORE = -INFINITY_SCORE,
  DRAW_SCORE = 0,
  MIN_ITERATION_DEPTH = 6,
  MAX_ITERATION_DEPTH = 20,
};

struct move mate_move = {.result = CHECK | MATE};

static score_t search_position(struct search_job *job, struct pv *parent_pv,
                               struct position *position, int depth,
                               score_t alpha, score_t beta);

/* Search a single move - call search_position after making the move. Return 1
   for a beta cutoff, and 0 in all other cases including self-check. */
static inline int search_move(struct search_job *job, struct pv *parent_pv,
                              struct pv *pv,
                              const struct position *from_position, int depth,
                              score_t *best_score, /* in/out */
                              score_t *alpha,      /* in/out */
                              score_t beta, struct move *move,
                              struct move **best_move,  /* in/out */
                              enum tt_entry_type *type, /* in/out */
                              int *n_legal_moves /* in/out */) {
  struct position position;

  /* Copy and move */
  copy_position(&position, from_position);
  make_move(&position, move);

  /* Return early if moving into self-check. All other moves are legal. */
  if (in_check(&position)) return 0;
  if (n_legal_moves) (*n_legal_moves)++;

  /* Move history is hashed against the position being moved from */
  history_push(job->history, from_position->hash, move);
  change_player(&position);

  /* Record whether this move puts the opponent in check */
  if (in_check(&position)) move->result |= CHECK;

  /* Recurse into search_position */
  score_t score =
      -search_position(job, pv, &position, depth - 1, -beta, -*alpha);

  history_pop(job->history);

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
      xboard_thought(job, parent_pv, depth, score, clock() - job->start_time,
                     job->result.n_leaf);
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
static inline void update_result(struct search_job *job,
                                 struct position *position, int depth,
                                 struct move *move, score_t score) {
  if (depth == job->depth) {
    job->result.score = score;
    if (move) {
      memcpy(&job->result.move, move, sizeof(job->result.move));
    }
  }
}

/* Search a single position and all possible moves - call search_move for each
   move */
static score_t search_position(struct search_job *job, struct pv *parent_pv,
                               struct position *position, int depth,
                               score_t alpha, score_t beta) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);
  ASSERT(depth <= job->depth);

  /* For statistics, count leaf nodes at horizon only (even if they extend) */
  if (depth == 0) job->result.n_leaf++;

  /* Breaking the 50-move rule or threefold repetition rule forces a draw */
  if (position->halfmove > 50 ||
      is_repeated_position(job->history, position->hash, 3)) {
    return DRAW_SCORE;
  }

  /* First phase - try to exit early */

  /* If there are any moves, best_move and best_score will be updated by the end
     of the function */
  score_t best_score = -INFINITY_SCORE;
  struct move *best_move = 0;

  /* Struct holding the princpal variation of children for this node */
  struct pv pv;
  pv.length = 0;

  /* Standing pat - in a quiescence search, evaluate taking no action - this
     could be better than the consequences of taking a piece. */
  if (depth <= 0 && !in_check(position)) {
    best_score = evaluate(position);
    if (best_score >= beta) return beta;
    if (best_score > alpha) alpha = best_score;
  }

  /* Default node type - this will change to TT_EXACT on alpha update or TT_BETA
     on beta cutoff */
  enum tt_entry_type type = TT_ALPHA;

  /* Try to get a beta cutoff or alpha update from a killer move */
  if (OPT_KILLER && (depth >= 0) &&
      !check_legality(position, &job->killer_moves[depth]) &&
      search_move(job, parent_pv, &pv, position, depth, &best_score, &alpha,
                  beta, &job->killer_moves[depth], &best_move, &type, 0)) {
    update_result(job, position, depth, &job->killer_moves[depth], best_score);
    return beta;
  }

  /* Probe the transposition table at higher levels */
  struct tt_entry *tte = 0;
  if (OPT_HASH && depth > TT_MIN_DEPTH) tte = tt_probe(position->hash);

  /* If the position has already been searched at the same or greater depth, use
     the result from the tt.  Do not use this at the root, because the move that
     will be made needs to be searched. */
  if (tte && (tte->depth >= depth)) {
    if (depth < job->depth) {
      if (tte->type == TT_ALPHA && tte->score > alpha) return alpha;
      if (tte->type == TT_BETA && tte->score > beta) return beta;
      if (tte->type == TT_EXACT) return tte->score;
    }
  }

  /* If there is a valid best move from the transposition table, try to get a
     beta cutoff or alpha update. */
  if (tte && !check_legality(position, &tte->best_move) &&
      search_move(job, parent_pv, &pv, position, depth, &best_score, &alpha,
                  beta, &tte->best_move, &best_move, &type, 0)) {
    update_result(job, position, depth, &tte->best_move, best_score);
    return beta;
  }

  /* Second phase - generate and search all moves */

  /* Generate the list of pseudo-legal moves (including those leading into
     self-check). list_entry will point to the first sorted item. */
  struct move_list move_buf[N_MOVES];
  struct move_list *list_entry = move_buf;
  int n_pseudo_legal_moves;
  if (depth > 0 || position->check[WHITE] || position->check[BLACK]) {
    n_pseudo_legal_moves = generate_search_movelist(position, &list_entry);
  } else {
    n_pseudo_legal_moves = generate_quiescence_movelist(position, &list_entry);
    if (n_pseudo_legal_moves == 0) return best_score;
  }

  /* Search through the list of pseudo-legal moves. search_move will update
     best_score, best_move, alpha, and type, and n_legal_moves. */
  int n_legal_moves = 0;
  if (n_pseudo_legal_moves > 0) {
    while (list_entry) {
      if (!(tte && move_equal(&tte->best_move, &list_entry->move))) {
        if (search_move(job, parent_pv, &pv, position, depth, &best_score,
                        &alpha, beta, &list_entry->move, &best_move, &type,
                        &n_legal_moves)) {
          update_result(job, position, depth, &list_entry->move, beta);
          return beta;
        }
      }
      list_entry = list_entry->next;
    }
  }

  /* Checkmate or stalemate. For checkmate, reduce the score by the distance
     from root to mate. */
  if (n_legal_moves == 0) {
    type = TT_EXACT;
    if (in_check(position)) {
      alpha = CHECKMATE_SCORE + (job->depth - depth);
      best_move = &mate_move;
    } else {
      alpha = DRAW_SCORE;
    }
  }

  /* Update the result if at root */
  update_result(job, position, depth, best_move, alpha);

  /* Update the transposition table at higher levels */
  if (depth > TT_MIN_DEPTH) {
    tt_update(position->hash, type, depth, alpha, best_move);
  }

  return alpha;
}

/* Perform a search */
void search(int target_depth, double time_budget, struct history *history,
            struct position *position, struct search_result *res,
            int show_thoughts) {
  /* Prepare for search */
  struct search_job job;
  memset(&job, 0, sizeof(job));
  job.history = history;
  job.show_thoughts = show_thoughts;
  tt_zero();

  double remaining_time_budget = time_budget;

  int min = (target_depth < 1) ? MIN_ITERATION_DEPTH : target_depth;
  int max = (target_depth < 1) ? MAX_ITERATION_DEPTH : target_depth + 1;
  for (int depth = min; depth < max; depth++) {
    job.start_time = clock();
    job.depth = depth;
    double start_time = time_now();

    /* Enter recursive search with the current position as the root */
    struct pv pv;
    search_position(&job, &pv, position, job.depth, -INFINITY_SCORE,
                    INFINITY_SCORE);

    /* Copy results and calculate stats */
    memcpy(res, &job.result, sizeof(*res));
    double branching_factor = pow((double)res->n_leaf, 1.0 / (double)depth);
    res->branching_factor = branching_factor;
    res->time = clock() - job.start_time;
    res->collisions = tt_collisions();

    double time = time_now() - start_time;
    remaining_time_budget -= time;
    double predicted_next_iteration_time = time * branching_factor;
    if (predicted_next_iteration_time > remaining_time_budget) break;
  }
}
