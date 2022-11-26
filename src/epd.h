/*
 * Functions which run Extended Position Description (EPD) test cases
 */

#ifndef EPD_H
#define EPD_H

struct state_s_;
int epd_run(struct state_s_ *state, char *epd_line);
int epd_test(const char *filename, int depth);

#endif  // EPD_H
