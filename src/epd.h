/*
 * Functions which run Extended Position Description (EPD) test cases
 */

#ifndef EPD_H
#define EPD_H

struct position;

int epd_run(const char *epd_line, int depth);
int epd_test(const char *filename, int depth);

#endif  // EPD_H
