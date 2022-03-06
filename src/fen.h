/*
 *   Forsyth-Edwards Notation input and output
 */

#ifndef FEN_H
#define FEN_H

#include "io.h"
#include "state.h"

int get_fen(const state_s *state, char *out, size_t outsize);
int load_fen(state_s *state, const char *placement, const char *active, const char *castling,
             const char *en_passant, const char *halfmove_text, const char *fullmove_text);

#endif /* FEN_H */
