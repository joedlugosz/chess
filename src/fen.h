/*
 *   Forsyth-Edwards Notation input and output
 */

#ifndef FEN_H
#define FEN_H

#include "io.h"
#include "position.h"

int get_fen(const struct position *position, char *out, size_t outsize);
int load_fen(struct position *position, const char *placement, const char *active,
             const char *castling, const char *en_passant, const char *halfmove_text,
             const char *fullmove_text);

#endif /* FEN_H */
