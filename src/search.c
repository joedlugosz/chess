/*
 *  Searching
 */

#include "search.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "evaluate.h"
#include "hash.h"
#include "history.h"
#include "io.h"
#include "movegen.h"
#include "options.h"
#include "os.h"
#include "pv.h"

/*
 * Build options
 */

#define OPT_CONTEMPT 1
#define OPT_KILLER 1
#define OPT_HASH 1
#define OPT_STAND_PAT 1
/* OPT_LMR + OPT_NULL = +20 elo over 10 games */
#define OPT_LMR 0
#define OPT_PAWN_EXTENSION 0 /* This is probably a bad idea */
#define OPT_NULL 0

enum {
  TT_MIN_DEPTH = 4,
  QUIESCENCE_MAX_DEPTH = 50,
  MIN_ITERATION_DEPTH = 2,
  MAX_ITERATION_DEPTH = 20,
  INFINITY_SCORE = 10000,
  INVALID_SCORE = 10100,
  CHECKMATE_SCORE = -INFINITY_SCORE,
  DRAW_SCORE = 0,
  CONTEMPT_SCORE = -500,
  R_NULL = 2, /* Depth reduction for null move search */
  R_LATE = 1, /* Depth reduction for late move reduction */
  NODES_PER_CHECK = 2000,
};

struct move mate_move = {.result = CHECK | MATE};

static score_t search_position(struct search_job *job, struct pv *parent_pv,
                               struct position *position, int depth,
                               score_t alpha, score_t beta, int do_nullmove);

/* Null-move reduction search - evaluate at depth the consequences of
   hypothetically passing on a turn without making a move. */
static inline int search_null(struct search_job *job, struct pv *pv,
                              struct position *from_position, int depth,
                              score_t alpha, score_t beta) {
  /* Can't nullmove if already in check */
  if (in_check(from_position)) return 0;

  struct position position;
  copy_position(&position, from_position);

  /* The move would be made here */

  change_player(&position);

  /* Recurse into search_position.  `do_nullmove` = 0 so the next ply can't also
     test a null move. */
  score_t score =
      -search_position(job, pv, &position, depth - R_NULL, -beta, -beta + 1, 0);

  /* Beta cutoff */
  return (score >= beta);
}

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
                              int *n_legal_moves,       /* in/out */
                              int is_late_move) {
  struct position position;

  /* Copy and move */
  copy_position(&position, from_position);
  make_move(&position, move);

  /* Return early if moving into self-check. All other moves are legal. */
  if (in_check(&position)) {
    job->result.n_check_moves++;
    return 0;
  }
  //  ASSERT(!in_check(&position));
  if (n_legal_moves) (*n_legal_moves)++;

  score_t score;
  /* Move history is hashed against the position being moved from */
  history_push(job->history, from_position->hash, move);
  change_player(&position);

  /* Record whether this move puts the opponent in check */
  if (in_check(&position)) move->result |= CHECK;

  /* Late move reduction and extensions
     Reduce the search depth for late moves unless they are tactical. Extend
     the depth for pawn moves to try to find a promotion. */
  int extend_reduce;
  if (OPT_PAWN_EXTENSION && from_position->piece_at[move->from] == PAWN &&
      depth < job->depth - 1)
    extend_reduce = 1;
  else if (OPT_LMR && is_late_move && !in_check(from_position) &&
           !in_check(&position) && from_position->piece_at[move->to] == EMPTY &&
           move->promotion == PAWN)
    extend_reduce = -R_LATE;
  else
    extend_reduce = 0;

  /* Recurse into search_position */
  score = -search_position(job, pv, &position, depth + extend_reduce - 1, -beta,
                           -*alpha, 1);

  /* If a reduced search produces a score which will cause an update,
     re-search at full depth in case it turns out to be not so good */
  if (extend_reduce < 0 && score > *alpha) {
    score = -search_position(job, pv, &position, depth - 1, -beta, -*alpha, 1);
  }

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
    double elapsed_time = time_now() - job->start_time;
    if (job->show_thoughts && depth == job->depth)
      xboard_thought(job, parent_pv, depth, score, elapsed_time,
                     job->result.n_leaf,
                     (double)job->result.n_leaf / 1000.0 * elapsed_time,
                     job->result.seldep);
  }

  DEBUG_THOUGHT(job, pv, move, depth, score, *alpha, beta, position.hash);

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
static inline void update_result(struct search_job *job, int depth,
                                 struct move *move, score_t score) {
  if (depth == job->depth) {
    job->result.score = score;
    if (move) {
      memcpy(&job->result.move, move, sizeof(job->result.move));
    }
  }
}

/* Draw score is calclated on a basic contempt assumption, having no real
 * contempt factor for the opponent.  Early and midgame places a penalty of
 * CONTEMPT_SCORE on seeking a draw, otherwise DRAW_SCORE (zero) */
static score_t get_draw_score(const struct position *position) {
  return (OPT_CONTEMPT && !is_endgame(position)) ? CONTEMPT_SCORE : DRAW_SCORE;
}

/* Search a single position and all possible moves - call search_move for each
   move */
static score_t search_position(struct search_job *job, struct pv *parent_pv,
                               struct position *position, int depth,
                               score_t alpha, score_t beta, int do_nullmove) {
  if (job->halt) return 0;

  ASSERT((job->depth - depth) < SEARCH_DEPTH_MAX);
  ASSERT(depth <= job->depth);

  /* For statistics, count leaf nodes at horizon only (even if they extend) */
  if (depth == 0) job->result.n_leaf++;
  job->result.n_node++;
  if (job->next_time_check-- == 0) {
    job->next_time_check = NODES_PER_CHECK;
    if (job->stop_time > job->start_time && time_now() > job->stop_time) {
      job->result.type = SEARCH_RESULT_INVALID;
      job->halt = 1;
      printf("Time check\n");
      return 0;
    }
  }

  if (depth == job->depth) job->result.type = SEARCH_RESULT_PLAY;
  if (job->depth - depth > job->result.seldep)
    job->result.seldep = job->depth - depth;

  /* Breaking the 50-move rule or threefold repetition rule forces a draw */
  if (position->halfmove > 51 ||
      is_repeated_position(job->history, position->hash, 3)) {
    if (depth == job->depth)
      job->result.type = SEARCH_RESULT_DRAW_BY_REPETITION;
    parent_pv->length = 0;
    return get_draw_score(position);
  }

  /* First phase - try to exit early */

  /* Principal variation for this node and its children */
  struct pv pv;
  pv.length = 0;

  /* Null move pruning */
  if (OPT_NULL && do_nullmove && depth <= job->depth - 2 && depth >= 2 &&
      search_null(job, &pv, position, depth, alpha, beta))
    return beta;

  /* If there are any moves, best_move and best_score will be updated by the end
     of the function */
  score_t best_score = -INVALID_SCORE;
  struct move *best_move = 0;

  /* Early exits in quiescence */
  if (OPT_STAND_PAT && depth <= 0 && !in_check(position)) {
    best_score = evaluate(position);
    /* Standing pat - evaluate taking no action - this
       could be better than the consequences of taking a piece. */
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
                  beta, &job->killer_moves[depth], &best_move, &type, 0, 0)) {
    update_result(job, depth, &job->killer_moves[depth], best_score);
    return beta;
  }

  /* Probe the transposition table at higher levels */
  struct tt_entry *tte = 0;
  if (OPT_HASH && depth > TT_MIN_DEPTH)
    tte = tt_probe(position->hash, position->total_a);

  /* If the position has already been searched at the same or greater depth, use
     the result from the tt.  Do not use this at the root, because the move that
     will be made needs to be searched. */
  if (tte && (tte->depth >= depth)) {
    if (depth < job->depth) {
      if (tte->type == TT_ALPHA && tte->score > alpha) return tte->score;
      if (tte->type == TT_BETA && tte->score > beta) return beta;
      if (tte->type == TT_EXACT) return tte->score;
    }
  }

  /* If there is a valid best move from the transposition table, try to get a
     beta cutoff or alpha update. */
  int n_legal_moves = 0;
  if (tte && !check_legality(position, &tte->best_move) &&
      search_move(job, parent_pv, &pv, position, depth, &best_score, &alpha,
                  beta, &tte->best_move, &best_move, &type, &n_legal_moves,
                  0)) {
    update_result(job, depth, &tte->best_move, best_score);
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
    /* No quiescence moves found - this is the bottom of the search.  Return
     * evaluation. */
    n_pseudo_legal_moves = generate_quiescence_movelist(position, &list_entry);
    if (n_pseudo_legal_moves == 0)
      return (OPT_STAND_PAT) ? best_score : evaluate(position);
  }

  /* Search through the list of pseudo-legal moves. search_move will update
     best_score, best_move, alpha, and type, and n_legal_moves. */
  if (n_pseudo_legal_moves > 0) {
    int is_late_move = 0;
    while (list_entry) {
      if (!(tte && move_equal(&tte->best_move, &list_entry->move))) {
        if (depth > 3 && n_legal_moves > 1) is_late_move = 1;
        if (search_move(job, parent_pv, &pv, position, depth, &best_score,
                        &alpha, beta, &list_entry->move, &best_move, &type,
                        &n_legal_moves, is_late_move)) {
          update_result(job, depth, &list_entry->move, beta);
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
      alpha = get_draw_score(position);
    }
    if (depth == job->depth) {
      if (in_check(position)) {
        job->result.type = SEARCH_RESULT_CHECKMATE;
      } else {
        job->result.type = SEARCH_RESULT_STALEMATE;
      }
    }
  }

  ASSERT(alpha > -INVALID_SCORE && alpha < INVALID_SCORE);

  /* Update the result if at root */
  update_result(job, depth, best_move, alpha);

  /* Update the transposition table at higher levels */
  if (depth > TT_MIN_DEPTH) {
    tt_update(position->hash, type, depth, alpha, best_move, position->total_a);
  }

  return alpha;
}

/* Perform a search */
void search(int target_depth, double time_budget, double time_margin,
            struct history *history, struct position *position,
            struct search_result *res, int show_thoughts) {
  /* Prepare for search */
  struct search_job job;
  memset(&job, 0, sizeof(job));
  job.start_time = time_now();
  job.history = history;
  job.show_thoughts = show_thoughts;
  tt_zero();

  double remaining_time_budget = time_budget;

  int min, max;
  if (target_depth == 0) {
    /* Search based on `time_budget` */
    min = MIN_ITERATION_DEPTH;
    max = MAX_ITERATION_DEPTH + 1;
    job.stop_time = job.start_time + time_budget - 0.01;
  } else if (target_depth < MIN_ITERATION_DEPTH) {
    /* Fixed-depth shallow search without iterative deepening */
    min = target_depth;
    max = target_depth + 1;
    job.stop_time = 0.0;
  } else {
    /* Fixed depth deep search with iterative deepening */
    min = MIN_ITERATION_DEPTH;
    max = target_depth + 1;
    job.stop_time = job.start_time + time_budget - 0.01;
  }

  for (int depth = min; depth < max; depth++) {
    double iteration_start_time = time_now();
    job.depth = depth;

    /* Enter recursive search with the current position as the root */
    struct pv pv;
    score_t score = search_position(&job, &pv, position, job.depth,
                                    -INVALID_SCORE, INVALID_SCORE, 1);

    if (job.result.type == SEARCH_RESULT_INVALID) break;

    /* Copy results and calculate stats */
    memcpy(res, &job.result, sizeof(*res));
    double branching_factor = pow((double)res->n_leaf, 1.0 / (double)depth);
    double iteration_time = time_now() - iteration_start_time;
    remaining_time_budget -= iteration_time;

    res->branching_factor = branching_factor;
    res->time = time_now() - job.start_time;
    res->collisions = tt_collisions();

    /* Break if a checkmate to either side has been found within depth */
    if (abs(score) + depth >= -CHECKMATE_SCORE) break;

    /* Estimate whether there is enough time for another iteration */
    double predicted_next_iteration_time = iteration_time * branching_factor;
    if (target_depth == 0 && predicted_next_iteration_time >
                                 remaining_time_budget * (1.0 + time_margin))
      break;
  }
  tt_new_age();
}
