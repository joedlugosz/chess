/*
 *   Move list generation, sorting, and perft
 */

#include "movegen.h"
#include "history.h"
#include "io.h"
#include "state.h"

const score_t player_fact[N_PLAYERS] = { 1, -1 };

static inline void sort_compare(movelist_s **head, movelist_s *insert)
{
  movelist_s *current;
  if(!*head || (*head)->score <= insert->score) {
    insert->next = *head;
    *head = insert;
  } else {
    current = *head;
    while(current->next && current->next->score > insert->score) {
      current = current->next;
    }
    insert->next = current->next;
    current->next = insert;
  }
}

static inline void sort_moves(movelist_s **head)
{
  movelist_s *sorted = 0;
  movelist_s *current = *head;
  while(current) {
    movelist_s *next = current->next;
    sort_compare(&sorted, current);
    current = next;
  }
  *head = sorted;
}

static int sort_evaluate(state_s *state, move_s *move)
{
  int score = 0;
  piece_e moving_type = piece_type[(int)state->piece_at[move->from]];

  if(move->promotion > PAWN) {
    score += 20 + move->promotion - PAWN;
  }
  if(state->piece_at[move->to] == EMPTY) {
    score += moving_type;
  } else {
    /* Moves which take get a bonus of 10 + difference between piece values */
    score += 10 - moving_type + piece_type[(int)state->piece_at[move->to]];
  }
  return score;
}

/* Add movelist entries for a given from and to position.  Add promotion
   moves if necessary */
static inline void add_movelist_entries(
  state_s *state, pos_t from, pos_t to, movelist_s *move_buf, /* in */ 
  movelist_s **prev, int *index) /* in, out */
{
  piece_e promotion = (piece_type[state->piece_at[from]] == PAWN 
    && is_promotion_move(state, from, to)) ? QUEEN : PAWN;
  do {
    ASSERT(*index < N_MOVES);
    movelist_s *current = &move_buf[*index];
    ASSERT(current != *prev);
    current->move.from = from;
    current->move.to = to;
    current->move.promotion = promotion;
    current->score = sort_evaluate(state, &current->move);
    current->next = 0;
    if(*prev) (*prev)->next = current;
    *prev = current;
    (*index)++;
  } while(--promotion > PAWN);
}

/* Move generation - generates a linked list of moves within move_buf, sorted (or not) */
int generate_search_movelist(state_s *state, movelist_s **move_buf)
{
  movelist_s *prev = 0;
  int count = 0; 
  bitboard_t pieces = get_my_pieces(state);
  while(pieces) {
    pos_t from = mask2pos(next_bit_from(&pieces));
    bitboard_t moves = get_moves(state, from);
    while(moves) {
      bitboard_t to_mask = next_bit_from(&moves);
      pos_t to = mask2pos(to_mask);
      add_movelist_entries(state, from, to, *move_buf, &prev, &count);
    }
  }
  if(count) sort_moves(move_buf);
  return count;
}

/* Move generation - generates a linked list of moves within move_buf, sorted (or not) */
int generate_quiescence_movelist(state_s *state, movelist_s **move_buf)
{
  movelist_s *prev = 0;
  int count = 0;
  bitboard_t victims = state->claim[state->to_move] & get_opponents_pieces(state);
  while(victims) {
    bitboard_t to_mask = next_bit_from(&victims);
    pos_t to = mask2pos(to_mask);
    bitboard_t attackers = get_attacks(state, to, state->to_move);
    while(attackers) {
      bitboard_t from_mask = next_bit_from(&attackers);
      pos_t from = mask2pos(from_mask);
      add_movelist_entries(state, from, to, *move_buf, &prev, &count);
    }
  }
  if(count) sort_moves(move_buf);
  return count;
}

void perft(perft_s *data, state_s *state, int depth, moveresult_t result)
{
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry, *move_buf_head = move_buf;
  int i;
  state_s next_state;
  perft_s next_data;
  
  memset(data, 0, sizeof(*data));
  
  if(depth == 0) {
    data->moves = 1;
    if(result & CAPTURED) {
      data->captures = 1;
    }
    if(result & EN_PASSANT) {
      data->ep_captures = 1;
    }
    
    if(result & CASTLED) data->castles = 1;
    if(result & PROMOTED) data->promotions = 1;
    /* If in check, see if there are any valid moves out of check */
    if(in_check(state)) {
      data->checks = 1;
      generate_search_movelist(state, &move_buf_head);
      list_entry = move_buf_head;
      i = 0;
      while(list_entry) {
	      copy_state(&next_state, state);
	      make_move(&next_state, &list_entry->move);
	      /* Count moves out of check */
	      if(!in_check(&next_state)) {
	        i++;
	      }
	      list_entry = list_entry->next;
      }
      /* If not, checkmate */
      if(i == 0) data->checkmates = 1;
    }
    return;
  }

  generate_search_movelist(state, &move_buf_head);
  list_entry = move_buf_head;
  i = 0;
  while(list_entry) {
    copy_state(&next_state, state);
    make_move(&next_state, &list_entry->move);
    /* Can't move into check */
    if(!in_check(&next_state)) {
      change_player(&next_state);
      perft(&next_data, &next_state, depth - 1, list_entry->move.result);
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

void perft_divide(state_s *state, int depth)
{
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry, *move_buf_head = move_buf;
  state_s next_state;
  perft_s next_data;
  char buf[6];

  generate_search_movelist(state, &move_buf_head);
  list_entry = move_buf_head;
  while(list_entry) {
    copy_state(&next_state, state);
    make_move(&next_state, &list_entry->move);
    /* Can't move into check */
    if(!in_check(&next_state)) {
      change_player(&next_state);
      perft(&next_data, &next_state, depth - 1, list_entry->move.result);
      format_move(buf, &list_entry->move, 1);
      printf("%s: %lld\n", buf, next_data.moves);
    }
    list_entry = list_entry->next;
  }
}

void perft_total(state_s *state, int depth) 
{
  printf("%8s%16s%12s%12s%12s%12s%12s%12s%12s%12s\n", "Depth", "Nodes", 
    "Captures", "E.P.", "Castles", "Promotions", "Checks", "Disco Chx", 
    "Double Chx", "Checkmates");
  for(int i = 1; i <= depth; i++) {
    perft_s data;
    perft(&data, state, i, 0);
    printf("%8d%16lld%12ld%12ld%12ld%12ld%12ld%12s%12s%12ld\n", 
      i, data.moves, data.captures, data.ep_captures, data.castles, 
      data.promotions, data.checks, 
      "X", "X", data.checkmates);
  }
}
