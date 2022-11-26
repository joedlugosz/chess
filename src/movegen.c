/*
 *   Move list generation, sorting, and perft
 */

#include "movegen.h"

#include <stdio.h>

#include "history.h"
#include "io.h"
#include "position.h"

const score_t player_fact[N_PLAYERS] = {1, -1};

static inline void sort_compare(struct move_list **head, struct move_list *insert) {
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

static inline void sort_moves(struct move_list **head) {
  struct move_list *sorted = 0;
  struct move_list *current = *head;
  while (current) {
    struct move_list *next = current->next;
    sort_compare(&sorted, current);
    current = next;
  }
  *head = sorted;
}

static int sort_evaluate(struct position *position, struct move *move) {
  int score = 0;
  enum piece moving_type = piece_type[(int)position->piece_at[move->from]];

  if (move->promotion > PAWN) {
    score += 20 + move->promotion - PAWN;
  }
  if (position->piece_at[move->to] == EMPTY) {
    score += moving_type;
  } else {
    /* Moves which take get a bonus of 10 + difference between piece values */
    score += 10 - moving_type + piece_type[(int)position->piece_at[move->to]];
  }
  return score;
}

/* Add movelist entries for a given from and to position.  Add promotion
   moves if necessary */
static inline void add_movelist_entries(struct position *position, enum square from, enum square to,
                                        struct move_list *move_buf,          /* in */
                                        struct move_list **prev, int *index) /* in, out */
{
  enum piece piece = piece_type[position->piece_at[from]];
  enum piece promotion = (piece == PAWN && is_promotion_move(position, from, to)) ? QUEEN : PAWN;

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

/* Move generation - generates a linked list of moves within move_buf, sorted (or not) */
int generate_search_movelist(struct position *position, struct move_list **move_buf) {
  struct move_list *prev = 0;
  int count = 0;
  bitboard_t pieces = get_my_pieces(position);
  while (pieces) {
    enum square from = bit2square(take_next_bit_from(&pieces));
    bitboard_t moves = get_moves(position, from);
    while (moves) {
      bitboard_t to_mask = take_next_bit_from(&moves);
      enum square to = bit2square(to_mask);
      add_movelist_entries(position, from, to, *move_buf, &prev, &count);
    }
  }
  if (count) sort_moves(move_buf);
  return count;
}

/* Move generation - generates a linked list of moves within move_buf, sorted (or not) */
int generate_quiescence_movelist(struct position *position, struct move_list **move_buf) {
  struct move_list *prev = 0;
  int count = 0;
  bitboard_t victims = position->claim[position->turn] & get_opponents_pieces(position);
  while (victims) {
    bitboard_t to_mask = take_next_bit_from(&victims);
    enum square to = bit2square(to_mask);
    bitboard_t attackers = get_attacks(position, to, position->turn);
    while (attackers) {
      bitboard_t from_mask = take_next_bit_from(&attackers);
      enum square from = bit2square(from_mask);
      add_movelist_entries(position, from, to, *move_buf, &prev, &count);
    }
  }
  if (count) sort_moves(move_buf);
  return count;
}

void perft(perft_s *data, struct position *position, int depth, moveresult_t result) {
  struct move_list move_buf[N_MOVES];
  struct move_list *list_entry, *move_buf_head = move_buf;
  int i;
  struct position next_position;
  perft_s next_data;

  memset(data, 0, sizeof(*data));

  if (depth == 0) {
    data->moves = 1;
    if (result & CAPTURED) {
      data->captures = 1;
    }
    if (result & EN_PASSANT) {
      data->ep_captures = 1;
    }

    if (result & CASTLED) data->castles = 1;
    if (result & PROMOTED) data->promotions = 1;
    /* If in check, see if there are any valid moves out of check */
    if (in_check(position)) {
      data->checks = 1;
      generate_search_movelist(position, &move_buf_head);
      list_entry = move_buf_head;
      i = 0;
      while (list_entry) {
        copy_position(&next_position, position);
        make_move(&next_position, &list_entry->move);
        /* Count moves out of check */
        if (!in_check(&next_position)) {
          i++;
        }
        list_entry = list_entry->next;
      }
      /* If not, checkmate */
      if (i == 0) data->checkmates = 1;
    }
    return;
  }

  generate_search_movelist(position, &move_buf_head);
  list_entry = move_buf_head;
  i = 0;
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
      i++;
    }
    list_entry = list_entry->next;
  }
}

void perft_divide(struct position *position, int depth) {
  struct move_list move_buf[N_MOVES];
  struct move_list *list_entry, *move_buf_head = move_buf;
  struct position next_position;
  perft_s next_data;
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
      format_struct movean(buf, &list_entry->move);
      printf("%s: %lld\n", buf, next_data.moves);
    }
    list_entry = list_entry->next;
  }
}

void perft_total(struct position *position, int depth) {
  printf("%8s%16s%12s%12s%12s%12s%12s%12s%12s%12s\n", "Depth", "Nodes", "Captures", "E.P.",
         "Castles", "Promotions", "Checks", "Disco Chx", "Double Chx", "Checkmates");
  for (int i = 1; i <= depth; i++) {
    perft_s data;
    perft(&data, position, i, 0);
    printf("%8d%16lld%12ld%12ld%12ld%12ld%12ld%12s%12s%12ld\n", i, data.moves, data.captures,
           data.ep_captures, data.castles, data.promotions, data.checks, "X", "X", data.checkmates);
  }
}
