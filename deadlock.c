/* deadlock.c - kerangka Praktikum 2 bagian D */
#include <omp.h>
#include <stdio.h>

omp_lock_t kunci_a, kunci_b;
double saldo_a = 1000.0, saldo_b = 1000.0;

/* Memindahkan dana dari A ke B: mengunci A lalu B */
void transfer_ab(double x) {
  omp_set_lock(&kunci_a);
  omp_set_lock(&kunci_b);
  saldo_a -= x;
  saldo_b += x;
  omp_unset_lock(&kunci_b);
  omp_unset_lock(&kunci_a);
}

/* TODO D1: memindahkan dana dari B ke A dengan mengunci B lebih dulu, lalu A */
void transfer_ba(double x) {
  omp_set_lock(&kunci_b);
  omp_set_lock(&kunci_a);
  saldo_b -= x;
  saldo_a += x;
  omp_unset_lock(&kunci_a);
  omp_unset_lock(&kunci_b);
}

int main(void) {
  omp_init_lock(&kunci_a);
  omp_init_lock(&kunci_b);
#pragma omp parallel sections num_threads(2)
  {
#pragma omp section
    for (int i = 0; i < 1000000; i++)
      transfer_ab(1.0);
#pragma omp section
    for (int i = 0; i < 1000000; i++)
      transfer_ba(1.0);
  }
  printf("saldo A = %.1f, saldo B = %.1f, total = %.1f\n", saldo_a, saldo_b,
         saldo_a + saldo_b);
  omp_destroy_lock(&kunci_a);
  omp_destroy_lock(&kunci_b);
  return 0;
}