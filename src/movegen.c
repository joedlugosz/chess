/*
 *   movegen.c
 *
 *   Move generation, sorting, and perft
 *
 *   5.0              Original
 *   5.1   24/02/2019 En passant
 */

#include "movegen.h"
#include "history.h"
#include "sys.h"
#include "board.h"

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

int gen_eval(state_s *state, pos_t from, pos_t to)
{
  int score = 0;
  piece_e moving_type = piece_type[(int)state->piece_at[from]];

  if(state->piece_at[to] == EMPTY) {
    score = moving_type;
  } else {
    /* Moves which take get a bonus of 10 + difference between piece values */
    score += 10 - moving_type + piece_type[(int)state->piece_at[to]];
  }
  return score;
}

/* Move generation - generates a linked list of moves within move_buf, sorted (or not) */
int gen_moves(state_s *state, movelist_s **move_buf_head)
{
  plane_t pieces, moves;
  pos_t from, to;
  movelist_s *prev = 0;
  movelist_s *move_buf = *move_buf_head;
  int i = 0;
  
  pieces = get_my_pieces(state);
  while(pieces) {
    from = mask2pos(next_bit_from(&pieces));
    ASSERT(is_valid_pos(from));
    moves = get_moves(state, from);
    while(moves) {
      ASSERT(i < N_MOVES);
      plane_t to_mask = next_bit_from(&moves);
      to = mask2pos(to_mask);
      ASSERT(is_valid_pos(to));
      piece_e promotion;
      if(is_promotion_move(state, from, to_mask)) {
        promotion = QUEEN;
      } else {
        promotion = PAWN;
      }
      do {
        /* Enter the move info into the buffer */
        move_buf[i].score = gen_eval(state, from, to);
        move_buf[i].move.from = from;
        move_buf[i].move.to = to;
        move_buf[i].move.promotion = promotion;
        move_buf[i].next = 0;
        /* Link this entry to the previous entry in the list */
        if(prev) {
          prev->next = &move_buf[i];
        }
        prev = &move_buf[i];
        /* Next move in buffer */
        i++;
      } while(--promotion > PAWN);
    }
  }
  if(i) sort_moves(move_buf_head);
  return i;
}

void perft(perft_s *data, state_s *state, int depth)
{
  movelist_s move_buf[N_MOVES];
  movelist_s *list_entry, *move_buf_head = move_buf;
  int i;
  state_s next_state;
  perft_s next_data;
  
  memset(data, 0, sizeof(*data));
  
  if(depth == 0) {
    data->moves = 1;
    if(state->captured) {
      data->captures = 1;
    }
      if(state->ep_captured) {
	      data->ep_captures = 1;
      }
    
    if(state->castled) data->castles = 1;
    if(state->promoted) data->promotions = 1;
    /* If in check, see if there are any valid moves out of check */
    if(in_check(state)) {
      data->checks = 1;
      gen_moves(state, &move_buf_head);
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

  gen_moves(state, &move_buf_head);
  list_entry = move_buf_head;
  i = 0;
  while(list_entry) {
    copy_state(&next_state, state);
    make_move(&next_state, &list_entry->move);
    /* Can't move into check */
    if(!in_check(&next_state)) {
      change_player(&next_state);
      perft(&next_data, &next_state, depth - 1);
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

  gen_moves(state, &move_buf_head);
  list_entry = move_buf_head;
  while(list_entry) {
    copy_state(&next_state, state);
    make_move(&next_state, &list_entry->move);
    /* Can't move into check */
    if(!in_check(&next_state)) {
      change_player(&next_state);
      perft(&next_data, &next_state, depth - 1);
      format_move_bare(buf, &list_entry->move);
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
    perft(&data, state, i);
    printf("%8d%16lld%12ld%12ld%12ld%12ld%12ld%12s%12s%12ld\n", 
      i, data.moves, data.captures, data.ep_captures, data.castles, 
      data.promotions, data.checks, 
      "X", "X", data.checkmates);
  }
}
