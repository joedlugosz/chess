#ifndef SEE_H
#define SEE_H

#include "position.h"

int see_after_move(const struct position *position, enum square from,
                   enum square to, enum piece moving_piece);
#endif  // ifndef SEE_H
