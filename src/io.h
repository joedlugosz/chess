#ifndef IO_H
#define IO_H

void print_board(FILE *f, state_s *state, plane_t hl1, plane_t hl2);
void print_plane(FILE *f, plane_t plane, plane_t indicator);
void print_plane_rank(FILE *f, unsigned char rank, unsigned char indicator);

#endif /* IO_H */