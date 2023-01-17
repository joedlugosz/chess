#include "moves.h"

#include "clock.h"
#include "fen.h"
#include "position.h"

/* 10 million */
const int REPEATS = 10000000;
const int BUF_LEN = 1000;

struct total {
  double calculate_moves;
  double king_attacks;
};

/* Function to run a test on a position, returns time taken in ms */
typedef double (*test_func)(struct position *);

/* A test consists of running a function or set of functions */
struct test {
  char name[20];     /* Name of test */
  test_func func;    /* Function to run the test */
  double total_time; /* Total time taken for all test cases */
};

/*
 * Test functions
 */
double time_calculate_moves(struct position *position) {
  double start = time_now();
  for (int i = 0; i < REPEATS; i++) {
    calculate_moves(position);
  }
  return (time_now() - start) * 1000.0 / (double)REPEATS;
}

double time_king_attacks(struct position *position) {
  double start = time_now();
  for (int i = 0; i < REPEATS; i++) {
    for (enum player player = WHITE; player != N_PLAYERS; player++) {
      enum square square =
          bit2square(position->a[player ? KING + N_PIECE_T : KING]);
      get_attacks(position, square, !player);
    }
  }
  return (time_now() - start) * 1000.0 / (double)REPEATS;
}

/* Tests */
struct test tests[] = {
    {"calc_moves", time_calculate_moves, 0.0},
    {"king_attacks", time_king_attacks, 0.0},
};
const int n_tests = sizeof(tests) / sizeof(tests[0]);

/* Test cases */
const char test_case_fen[][100] = {
    "1b1qrr2/1p4pk/1np4p/p3Np1B/Pn1P4/R1N3B1/1Pb2PPP/2Q1R1K1 b",
    "1k1r2r1/1b4p1/p4n1p/1pq1pPn1/2p1P3/P1N2N2/1PB1Q1PP/3R1R1K b",
    "1k1r3r/pb1q2p1/B4p2/2p4p/Pp1bPPn1/7P/1P2Q1P1/R1BN1R1K b",
    "1k1r4/1br2p2/3p1p2/pp2pPb1/2q1P2p/P1PQNB1P/1P4P1/1K1RR3 b",
    "1k1r4/4bp2/p1q1pnr1/6B1/NppP3P/6P1/1P3P2/2RQR1K1 w",
    "1k5r/1pq1b2r/p2p1p2/4n1p1/R3P1p1/1BP3B1/PP1Q3P/1K1R4 w",
    "1kb4r/1p3pr1/3b1p1p/q2B1p2/p7/P1P3P1/1P1Q2NP/K2RR3 b",
    "1kr5/1b3ppp/p4n2/3p4/2qN1P2/2r2B2/PQ4PP/R2R3K b",
    "1n1r4/p1q2pk1/b2bp2p/4N1p1/3P1P2/1QN1P3/5PBP/1R5K w",
    "1n1rr1k1/1pq2pp1/3b2p1/2p3N1/P1P5/P3B2P/2Q2PP1/R2R2K1 w",
    "1n1rr1k1/5pp1/1qp4p/3p3P/3P4/pP1Q1N2/P1R2PP1/1KR5 w",
    "1r1qr1k1/2Q1bp1p/2n3p1/2PN4/4B3/2N3P1/5P1P/5RK1 b",
    "1r6/8/p2b3p/2nN1kpP/2P1p3/3rP3/3NKP2/1R1R4 w",
    "1b1qrr2/1p4pk/1np4p/p3Np1B/Pn1P4/R1N3B1/1Pb2PPP/2Q1R1K1 b",
};
const int n_test_cases = sizeof(test_case_fen) / sizeof(test_case_fen[0]);

/* Run all tests for a single test case */
void run_case(const char *fen) {
  char buf[BUF_LEN];
  strcpy(buf, fen);
  const char *pieces = strtok(buf, " ");
  const char *turn = strtok(0, "");
  struct position position;
  load_fen(&position, pieces, turn, "-", "-", "1", "1");
  printf("%-60s ", fen);
  fflush(stdout);

  for (int i = 0; i < n_tests; i++) {
    double time = (tests[i].func)(&position);
    tests[i].total_time += time;
    printf("%-15lf ", time);
    fflush(stdout);
  }
  printf("\n");
}

/* Run all test cases */
int main(int argc, char *argv[]) {
  init_board();
  printf("%-60s %s\n", "Test position", "Time, ms");
  printf("%-60s ", "");

  for (int i = 0; i < n_tests; i++) {
    printf("%-15s ", tests[i].name);
  }
  printf("\n");

  for (int i = 0; i < n_test_cases; i++) {
    run_case(test_case_fen[i]);
  }

  printf("%-60s ", "Total");
  for (int i = 0; i < n_tests; i++) {
    printf("%-15lf ", tests[i].total_time);
  }
  printf("\n");

  printf("%-60s ", "Mean");
  for (int i = 0; i < n_tests; i++) {
    printf("%-15lf ", tests[i].total_time / (double)n_test_cases);
  }
  printf("\n");

  return 0;
}
