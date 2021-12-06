#ifndef FEN_H
#define FEN_H

#include "state.h"
#include "io.h"

int get_fen(const state_s *state, char *out, size_t outsize);
int load_fen(state_s *state, const char *placement, const char *active,
             const char *castling, const char *en_passant);

#endif /* FEN_H */