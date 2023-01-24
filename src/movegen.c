/*
 *   Functions to generate lists of moves for searching, and perft
 */

#include "movegen.h"

#include <stdio.h>

#include "history.h"
#include "io.h"
#include "position.h"
#include "see.h"

/* Comparison and insertion for insertion sort.  Insert `insert` at the first
 * position in the list where `insert->score` > the score of the next item.  If
 * this results in insertion at the head, update the value of `head`. */
static inline void sort_compare_insert(struct move_list **head /* in/out */,
                                       struct move_list *insert) {
  struct move_list *current;
  if (!*head || (*head)->score <= insert->score) {
    insert->next = *head;
    *head = insert;
  } else {
    current = *head;
    while (current->next && current->next->score > insert->score) {
      current = current->next;
    }
    insert->next = current->next;
    current->next = insert;
  }
}

/* In-place insertion sort.  Iterate over all list items beginning at initial
 * value of `head`, calling `sort_compare_insert` to sort them.  Update `head`
 * with the head of the sorted list. */
static inline void sort_moves(struct move_list **head /* in/out */) {
  struct move_list *sorted = 0;
  struct move_list *current = *head;
  while (current) {
    struct move_list *next = current->next;
    sort_compare_insert(&sorted, current);
    current = next;
  }
  *head = sorted;
}

/* Move-ordering heuristic.  Return a quick evaluation score for the move in the
 * context of the current position, which can be performed without making the
 * move.  This is used for search movelist generation in an attempt to search
 * the best moves first. */
static int sort_evaluate(const struct position *position,
                         const struct move *move) {
  /* For non-capture moves, score +value of moving piece.
   * For captures, score +10 +/-difference in piece values (e.g. pawn takes
   * queen is best).
   * Additionally, for promotion moves, score +20 +value of piece after
   * promotion.
   */

  int score = 0;
  enum piece moving_type = piece_type[(int)position->piece_at[move->from]];

  if (position->piece_at[move->to] == EMPTY) {
    score += moving_type;
  } else {
    score += 10 - moving_type + piece_type[(int)position->piece_at[move->to]];
  }

  if (move->promotion > PAWN) {
    score += 20 + move->promotion - PAWN;
  }

  return score;
}

/* Add movelist entries for a given `from` and `to` position.  For promoting
 * moves, add entries for all possible promotion pieces, otherwise add a single
 * entry. Add a move-ordering heuristic score from `sort_evaluate` to enable
 * sorting. */
static inline void add_movelist_entries(const struct position *position,
                                        enum square from, enum square to,
                                        struct move_list *move_buf,
                                        struct move_list **prev, /* in/out */
                                        int *index)              /* in/out */
{
  enum piece piece = piece_type[position->piece_at[from]];
  enum piece promotion =
      (piece == PAWN && is_promotion_move(position, from, to)) ? QUEEN : PAWN;

  do {
    ASSERT(*index < N_MOVES);
    struct move_list *current = &move_buf[*index];
    ASSERT(current != *prev);
    current->move.from = from;
    current->move.to = to;
    current->move.piece = piece;
    current->move.promotion = promotion;
    current->score = sort_evaluate(position, &current->move);
    current->next = 0;
    if (*prev) (*prev)->next = current;
    *prev = current;
    (*index)++;
  } while (--promotion > PAWN);
}

int generate_test_movelist(const struct position *position,
                           struct move_list **move_buf) {
  struct move_list *prev = 0;
  int count = 0;
  bitboard_t pieces = get_my_pieces(position);
  if (position->turn == WHITE) {
    if (pieces & square2bit[B1])
      add_movelist_entries(position, B1, C3, *move_buf, &prev, &count);
    else if (pieces & square2bit[C3])
      add_movelist_entries(position, C3, B1, *move_buf, &prev, &count);
  } else {
    if (pieces & square2bit[B8])
      add_movelist_entries(position, B8, C6, *move_buf, &prev, &count);
    else if (pieces & square2bit[C6])
      add_movelist_entries(position, C6, B8, *move_buf, &prev, &count);
  }
  return count;
}

/* Generate a sorted linked list of moves at the buffer beginning at `move_buf`,
 * for a normal search from `position`, including all possible moves.  Update
 * `move_buf` to the head of the sorted list. */
int generate_search_movelist(const struct position *position,
                             struct move_list **move_buf /* in/out */) {
  struct move_list *prev = 0;
  int count = 0;
  bitboard_t pieces = get_my_pieces(position);
  while (pieces) {
    enum square from = bit2square(take_next_bit_from(&pieces));
    bitboard_t moves = get_moves(position, from) & ~get_my_pieces(position);
    while (moves) {
      enum square to = bit2square(take_next_bit_from(&moves));
      add_movelist_entries(position, from, to, *move_buf, &prev, &count);
    }
  }
  if (count) sort_moves(move_buf);
  return count;
}

/* Generate a sorted linked list of moves at the buffer beginning at `move_buf`,
 * for a quiescence search from `position`.  Update `move_buf` to the head of
 * the sorted list.  Quiescence search consists of all attacks to all pieces. */
int generate_quiescence_movelist(const struct position *position,
                                 struct move_list **move_buf /* in/out */) {
  struct move_list *prev = 0;
  int count = 0;
  bitboard_t victims =
      position->claim[position->turn] & get_opponents_pieces(position);
  while (victims) {
    bitboard_t to_mask = take_next_bit_from(&victims);
    enum square to = bit2square(to_mask);
    bitboard_t attackers = get_attacks(position, to, position->turn);
    while (attackers) {
      bitboard_t from_mask = take_next_bit_from(&attackers);
      enum square from = bit2square(from_mask);

      if (see_after_move(position, from, to, position->piece_at[from]) >= 0)
        add_movelist_entries(position, from, to, *move_buf, &prev, &count);
    }
  }
  if (count) sort_moves(move_buf);
  return count;
}

/* perft - A standard test to perform high-level validation of move generation
 * by recursively generating a tree of all moves for a given position to a given
 * depth, and counting the total moves and features (captured, en-passant,
 * castled, etc.) at leaf nodes, which can be compared to reference data. */
void perft(struct perft_stats *data, struct position *position, int depth,
           moveresult_t result) {
  struct move_list move_buf[N_MOVES];
  struct move_list *list_entry, *move_buf_head = move_buf;
  struct position next_position;
  struct perft_stats next_data;

  memset(data, 0, sizeof(*data));

  /* Leaf node - count one instance of a move and assess the features of the
   * move_result of the move made by the parent iteration.  Test for check in
   * the resulting position.  If in check, extend the search to test for
   * checkmate by trying to find moves out of check. */
  if (depth == 0) {
    data->moves = 1;
    if (result & CAPTURED) data->captures = 1;
    if (result & EN_PASSANT) data->ep_captures = 1;
    if (result & CASTLED) data->castles = 1;
    if (result & PROMOTED) data->promotions = 1;

    if (in_check(position)) {
      data->checks = 1;
      generate_search_movelist(position, &move_buf_head);
      list_entry = move_buf_head;
      int found_escape = 0;
      while (list_entry) {
        copy_position(&next_position, position);
        make_move(&next_position, &list_entry->move);
        if (!in_check(&next_position)) {
          found_escape = 1;
          break;
        }
        list_entry = list_entry->next;
      }
      if (!found_escape) data->checkmates = 1;
    }
    return;
  }

  /* Branch node - generate all moves, make each one, recurse into `perft`, and
   * total the data for all moves. */
  generate_search_movelist(position, &move_buf_head);
  list_entry = move_buf_head;
  while (list_entry) {
    copy_position(&next_position, position);
    make_move(&next_position, &list_entry->move);
    /* Can't move into check */
    if (!in_check(&next_position)) {
      change_player(&next_position);
      perft(&next_data, &next_position, depth - 1, list_entry->move.result);
      data->moves += next_data.moves;
      data->captures += next_data.captures;
      data->promotions += next_data.promotions;
      data->castles += next_data.castles;
      data->checks += next_data.checks;
      data->checkmates += next_data.checkmates;
      data->ep_captures += next_data.ep_captures;
    }
    list_entry = list_entry->next;
  }
}

/* For each move from a given position, perform a perft on the following
 * position, and report the number of moves. */
void perft_divide(struct position *position, int depth) {
  struct move_list move_buf[N_MOVES];
  struct move_list *list_entry, *move_buf_head = move_buf;
  struct position next_position;
  struct perft_stats next_data;
  char buf[6];

  generate_search_movelist(position, &move_buf_head);
  list_entry = move_buf_head;
  while (list_entry) {
    copy_position(&next_position, position);
    make_move(&next_position, &list_entry->move);
    /* Can't move into check */
    if (!in_check(&next_position)) {
      change_player(&next_position);
      perft(&next_data, &next_position, depth - 1, list_entry->move.result);
      format_move_san(buf, &list_entry->move);
      printf("%s: %lld\n", buf, next_data.moves);
    }
    list_entry = list_entry->next;
  }
}

/* Perform multiple perft tests to increasing depths and print the results in a
 * formatted table. */
void perft_total(struct position *position, int depth) {
  printf("%8s%16s%12s%12s%12s%12s%12s%12s%12s%12s\n", "Depth", "Nodes",
         "Captures", "E.P.", "Castles", "Promotions", "Checks", "Disco Chx",
         "Double Chx", "Checkmates");
  for (int i = 1; i <= depth; i++) {
    struct perft_stats data;
    perft(&data, position, i, 0);
    printf("%8d%16lld%12ld%12ld%12ld%12ld%12ld%12s%12s%12ld\n", i, data.moves,
           data.captures, data.ep_captures, data.castles, data.promotions,
           data.checks, "X", "X", data.checkmates);
  }
}
