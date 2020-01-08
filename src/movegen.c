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

static inline void sort_compare(move_s **head, move_s *insert)
{
  move_s *current;
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

static inline void sort_moves(move_s **head)
{
  move_s *sorted = 0;
  move_s *current = *head;
  while(current) {
    move_s *next = current->next;
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
int gen_moves(state_s *state, move_s **move_buf_head)
{
  plane_t pieces, moves;
  pos_t from, to;
  move_s *prev = 0;
  move_s *move_buf = *move_buf_head;
  //state_s *next_state;
  int i = 0;
  
  /* Get set of pieces this player can move */
  pieces = get_my_pieces(state);
  /* For each piece */
  while(pieces) {
    /* Get from-position and its valid moves, from the piece */
    from = mask2pos(next_bit_from(&pieces));
    ASSERT(is_valid_pos(from));
    moves = get_moves(state, from);
    /* For each move */
    while(moves) {
      ASSERT(i < N_MOVES);
      /* Get to-position from the valid moves */
      plane_t to_mask = next_bit_from(&moves);
      to = mask2pos(to_mask);
      ASSERT(is_valid_pos(to));
      int n;
      /* Promotion */
      if((to_mask & 0xff000000000000ffull) && piece_type[(int)state->piece_at[from]] == PAWN) {
        n = KING;
        while(--n > PAWN) {
          /* Enter the move info into the buffer */
          move_buf[i].score = gen_eval(state, from, to);
          move_buf[i].from = from;
          move_buf[i].to = to;
          move_buf[i].promotion = n;
          move_buf[i].next = 0;
          /* Link this entry to the previous entry in the list */
          if(prev) {
            prev->next = &move_buf[i];
          }
          prev = &move_buf[i];
          /* Next move in buffer */
          i++;
        }
      } else {
        /* Enter the move info into the buffer */
        move_buf[i].score = gen_eval(state, from, to);
        move_buf[i].from = from;
        move_buf[i].to = to;
        move_buf[i].promotion = 0;
        move_buf[i].next = 0;
        /* Link this entry to the previous entry in the list */
        if(prev) {
          prev->next = &move_buf[i];
        }
        prev = &move_buf[i];
        /* Next move in buffer */
        i++;
      }
    }
  }
  if(i) sort_moves(move_buf_head);
  return i;
}

void perft(perft_s *data, state_s *state, int depth)
{
  move_s move_buf[N_MOVES];
  move_s *move, *move_buf_head = move_buf;
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
      move = move_buf_head;
      i = 0;
      while(move) {
	      copy_state(&next_state, state);
	      do_move(&next_state, move->from, move->to, move->promotion);
	      /* Count moves out of check */
	      if(!in_check(&next_state)) {
	        i++;
	      }
	      move = move->next;
      }
      /* If not, checkmate */
      if(i == 0) data->checkmates = 1;
    }
    return;
  }

  gen_moves(state, &move_buf_head);
  move = move_buf_head;
  i = 0;
  while(move) {
    copy_state(&next_state, state);
    do_move(&next_state, move->from, move->to, move->promotion);
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
    move = move->next;
  }
}

void perft_divide(state_s *state, int depth)
{
  move_s move_buf[N_MOVES];
  move_s *move, *move_buf_head = move_buf;
  state_s next_state;
  perft_s next_data;
  char buf[6];

  gen_moves(state, &move_buf_head);
  move = move_buf_head;
  while(move) {
    copy_state(&next_state, state);
    do_move(&next_state, move->from, move->to, move->promotion);
    /* Can't move into check */
    if(!in_check(&next_state)) {
      change_player(&next_state);
      perft(&next_data, &next_state, depth - 1);
      format_move_bare(buf, move->from, move->to);
      printf("%s: %lld\n", buf, next_data.moves);
  /*    data->captures += next_data.captures;
      data->promotions += next_data.promotions;
      data->castles += next_data.castles;
      data->checks += next_data.checks;
      data->checkmates += next_data.checkmates;
      data->ep_captures += next_data.ep_captures;*/
    }
    move = move->next;
  }
}
  //printf("%4d %20s %12lld\n", depth, buf, p);
  //printf("Perft: %lld\n", perft(&(e->game), depth));

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
