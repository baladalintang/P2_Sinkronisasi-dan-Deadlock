# Praktikum 2: Pemrograman Paralel dan Sinkronisasi dengan OpenMP

Repositori ini berisi implementasi, analisis, dan solusi untuk tugas **Praktikum 2 Pemrograman Paralel**, yang berfokus pada penanganan *Race Condition* dan *Deadlock* menggunakan pustaka OpenMP dalam bahasa C.

## 📚 Daftar Isi

1. [Prasyarat & Lingkungan Pengembangan](#1-prasyarat--lingkungan-pengembangan)
2. [Struktur Repositori](#2-struktur-repositori)
3. [Tugas 1: Sinkronisasi Histogram (`histogram.c` & `histogram_perbaikanfungsi.c`)](#3-tugas-1-sinkronisasi-histogram-histogramc--histogram_perbaikanfungsic)
   * [Masalah Race Condition](#masalah-race-condition)
   * [Metode Perbaikan (C1 - C4)](#metode-perbaikan-c1---c4)
   * [Kompilasi & Cara Menjalankan](#kompilasi--cara-menjalankan)
   * [Hasil Performa & Analisis](#hasil-performa--analisis)
4. [Tugas 2: Penanganan Deadlock (`deadlock.c` & `deadlock_perbaikan.c`)](#4-tugas-2-penanganan-deadlock-deadlockc--deadlock_perbaikanc)
   * [Penyebab Deadlock](#penyebab-deadlock)
   * [Solusi Lock Ordering (D1 - D2)](#solusi-lock-ordering-d1---d2)
   * [Kompilasi & Cara Menjalankan](#kompilasi--cara-menjalankan-1)
5. [Kesimpulan](#5-kesimpulan)

---

## 1. Prasyarat & Lingkungan Pengembangan

* **Compiler**: GCC (dengan dukungan OpenMP)
* **Pustaka**: OpenMP (`<omp.h>`)
* **Sistem Operasi**: Linux / WSL (Ubuntu)

---

## 2. Struktur Repositori

```text
.
├── src/
│   ├── histogram.c                 # Kode kerangka awal histogram
│   ├── histogram_perbaikanfungsi.c # Kode histogram lengkap dengan perbaikan C1-C4
│   ├── deadlock.c                  # Kode simulasi deadlock (D1)
│   └── deadlock_perbaikan.c        # Kode perbaikan deadlock (D2)
├── hasil/
│   └── histogram.txt               # Log output eksekusi pengujian histogram (C5)
└── README.md                       # Dokumentasi resmi repositori
```

---

## 3. Tugas 1: Sinkronisasi Histogram (`histogram.c` & `histogram_perbaikanfungsi.c`)

### Masalah Race Condition

Pada fungsi `hist_race`, operasi pembentukan histogram dieksekusi secara paralel tanpa sinkronisasi:

```c
#pragma omp parallel for
for (long i = 0; i < n; i++) h[data[i]]++;
```

Karena operasi *increment* (`h[...]++`) melibatkan tahapan *Read-Modify-Write*, beberapa thread yang memperbarui elemen array secara bersamaan menderita *data race* / *lost updates*. Akibatnya, total elemen yang terhitung jauh di bawah $100.000.000$ (`benar=TIDAK`).

### Metode Perbaikan (C1 - C4)

1. **`hist_critical` (`#pragma omp critical`)**:
   Memaksa pembaruan histogram dilakukan secara bergantian (*mutual exclusion*) oleh satu thread pada satu waktu.
2. **`hist_atomic` (`#pragma omp atomic`)**:
   Memanfaatkan instruksi *hardware-level atomic update* untuk memperbarui lokasi memori spesifik secara efisien.
3. **`hist_reduction` (`#pragma omp parallel for reduction(+:h[:256])`)**:
   OpenMP secara otomatis memprivatisasi array histogram untuk setiap thread dan menggabungkannya di akhir loop.
4. **`hist_lokal` (Histogram Lokal Per-Thread)**:
   Masing-masing thread mengalokasikan array histogram lokal di memori privat, lalu hasilnya digabungkan ke array utama menggunakan blok `critical` di akhir ekskekusi.

### Kompilasi & Cara Menjalankan

```bash
# Kompilasi kode perbaikan
gcc -O2 -fopenmp src/histogram_perbaikanfungsi.c -o histogram_perbaikanfungsi

# Uji coba dengan 8 Thread
OMP_NUM_THREADS=8 ./histogram_perbaikanfungsi

# Pengujian dengan 1 Thread (Membuktikan tidak ada race condition saat sekuensial)
OMP_NUM_THREADS=1 ./histogram_perbaikanfungsi | grep race
```

### Hasil Performa & Analisis

Pengujian dilakukan dengan $n = 10^8$ elemen menggunakan 8 thread CPU:

| Versi Implementasi | Waktu Eksekusi (detik) | Total Data | Status (`benar`) | Analisis Performa |
| :--- | :---: | :---: | :---: | :--- |
| **`race`** | \~0.742 s | \~57.4 M | **TIDAK** | *Data race* menyebabkan *lost updates* jutaan hitungan. |
| **`critical`** | \~31.548 s | 100.0 M | **ya** | **Sangat Lambat**: *Lock contention* ekstrem karena thread saling antre. |
| **`atomic`** | \~1.739 s | 100.0 M | **ya** | Lebih cepat dari `critical`, namun masih mengalami *memory bus contention*. |
| **`reduction`** | **\~0.059 s** | 100.0 M | **ya** | **Sangat Cepat**: Bebas *lock contention* berkat privatisasi otomatis OpenMP. |
| **`lokal`** | **\~0.056 s** | 100.0 M | **ya** | **Sangat Cepat**: Bebas *lock contention* berkat privatisasi manual per-thread. |

---

## 4. Tugas 2: Penanganan Deadlock (`deadlock.c` & `deadlock_perbaikan.c`)

### Penyebab Deadlock

*Deadlock* terjadi pada skenario transaksi transfer bank inter-thread ketika terjadi penguncian berlawanan (*circular wait*):

* **Thread 1 (`transfer_ab`)**: Mengunci `kunci_a` $\rightarrow$ mencoba mengunci `kunci_b`.
* **Thread 2 (`transfer_ba`)**: Mengunci `kunci_b` $\rightarrow$ mencoba mengunci `kunci_a`.

Kedua thread saling menunggu kunci lawan yang sedang terikat, menyebabkan program mengalami *hang* permanen.

### Solusi Lock Ordering (D1 - D2)

Untuk mencegah *deadlock* tanpa mengubah `transfer_ab`, fungsi `transfer_ba` disesuaikan agar selalu mengambil kunci dengan urutan hirarki yang konsisten (**Lock Ordering**):

```c
void transfer_ba(double x) {
    omp_set_lock(&kunci_a); /* Selalu kunci A terlebih dahulu */
    omp_set_lock(&kunci_b); /* Kemudian kunci B */
    saldo_b -= x;
    saldo_a += x;
    omp_unset_lock(&kunci_b);
    omp_unset_lock(&kunci_a);
}
```

### Kompilasi & Cara Menjalankan

```bash
# Kompilasi
gcc -O2 -fopenmp src/deadlock_perbaikan.c -o deadlock_perbaikan

# Eksekusi pengujian 5 kali beruntun
for i in {1..5}; do ./deadlock_perbaikan; done
```

**Output Konsisten (Bebas Deadlock):**
```text
saldo A = 1000.0, saldo B = 1000.0, total = 2000.0
saldo A = 1000.0, saldo B = 1000.0, total = 2000.0
saldo A = 1000.0, saldo B = 1000.0, total = 2000.0
saldo A = 1000.0, saldo B = 1000.0, total = 2000.0
saldo A = 1000.0, saldo B = 1000.0, total = 2000.0
```

---

## 5. Kesimpulan

1. **Penggunaan Mutual Exclusion (`critical` / `atomic`)** yang berlebihan pada pembaruan memori frekuensi tinggi menderita penurunan performa yang ekstrim akibat *lock contention*.
2. **Teknik Privatisasi Memori (`reduction` & `lokal`)** merupakan pendekatan terbaik untuk kalkulasi histogram paralel karena menghilangkan interferensi antar-thread secara total.
3. **Pencegahan Deadlock** paling efektif dilakukan dengan menerapkan aturan **Lock Ordering** yang konsisten di seluruh fungsi yang mengakses *resource* bersama.