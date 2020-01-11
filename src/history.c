#include "history.h"

void write_move_history(search_context_s *ctx, int depth, move_s *move,
  player_e to_move, score_t score)
{
  ctx->search_history[depth].from = move->from;
  ctx->search_history[depth].to = move->to;
  ctx->search_history[depth].player = to_move;
  ctx->search_history[depth].score = score;
}

int is_repeated_move(search_context_s *ctx, int depth, move_s *move)
{
  /* Super simple checking for moving back to the square you've
     just come from */
  /* TODO: Transposition table */
  if (depth > 1) {
    if (move->to == ctx->search_history[depth-2].from
	&& move->from == ctx->search_history[depth-2].to) {
      return 1;
    }
  } else if (ctx->n_ai_moves > 0) {
    if(move->to == ctx->repeat_history[ctx->n_ai_moves-1].from
       && move->from == ctx->repeat_history[ctx->n_ai_moves-1].to) {
      return 1;
    }
  }
  return 0;
}
