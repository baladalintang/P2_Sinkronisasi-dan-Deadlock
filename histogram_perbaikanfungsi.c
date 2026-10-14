/* histogram.c - kerangka Praktikum 2 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BIN 256

static void hist_serial(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
  for (long i = 0; i < n; i++)
    h[data[i]]++;
}

/* Versi salah: mengandung race condition. Jangan diperbaiki di sini. */
static void hist_race(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
#pragma omp parallel for
  for (long i = 0; i < n; i++)
    h[data[i]]++;
}

/* TODO C1-C4: lengkapi empat versi perbaikan berikut */

static void hist_critical(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
/* TODO C1: pakai critical */
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
#pragma omp critical
    {
      h[data[i]]++;
    }
  }
}

static void hist_atomic(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
/* TODO C2: pakai atomic */
#pragma omp parallel for
  for (long i = 0; i < n; i++) {
#pragma omp atomic
    h[data[i]]++;
  }
}

static void hist_reduction(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
/* TODO C3: pakai reduction pada array h */
#pragma omp parallel for reduction(+ : h[ : BIN])
  for (long i = 0; i < n; i++) {
    h[data[i]]++;
  }
}

static void hist_lokal(const unsigned char *data, long n, long *h) {
  memset(h, 0, BIN * sizeof(long));
/* TODO C4: setiap thread mengisi histogram lokal, lalu digabung ke h */
#pragma omp parallel
  {
    long h_lokal[BIN];
    memset(h_lokal, 0, BIN * sizeof(long));

#pragma omp for
    for (long i = 0; i < n; i++) {
      h_lokal[data[i]]++;
    }

#pragma omp critical
    {
      for (int k = 0; k < BIN; k++) {
        h[k] += h_lokal[k];
      }
    }
  }
}

static int sama(const long *a, const long *b) {
  for (int k = 0; k < BIN; k++)
    if (a[k] != b[k])
      return 0;
  return 1;
}

typedef void (*fungsi_hist)(const unsigned char *, long, long *);

int main(int argc, char **argv) {
  long n = (argc > 1) ? atol(argv[1]) : 100000000L; /* 10^8 */
  unsigned char *data = malloc(n);
  srand(42);
  for (long i = 0; i < n; i++)
    data[i] = (unsigned char)(rand() % BIN);

  long acuan[BIN], h[BIN];
  hist_serial(data, n, acuan);

  const char *nama[] = {"race", "critical", "atomic", "reduction", "lokal"};
  fungsi_hist f[] = {hist_race, hist_critical, hist_atomic, hist_reduction,
                     hist_lokal};

  for (int v = 0; v < 5; v++) {
    double t0 = omp_get_wtime();
    f[v](data, n, h);
    double t1 = omp_get_wtime();
    long total = 0;
    for (int k = 0; k < BIN; k++)
      total += h[k];
    printf("%-10s waktu=%.3f total=%ld benar=%s\n", nama[v], t1 - t0, total,
           sama(h, acuan) ? "ya" : "TIDAK");
  }

  free(data);
  return 0;
}