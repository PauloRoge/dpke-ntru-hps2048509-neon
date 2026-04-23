#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cpucycles.h>

/* --- Configuration (override these via -D flags if desired) --- */
#ifndef BENCH_LOOP_ITERATIONS
#define BENCH_LOOP_ITERATIONS     50
#endif
#ifndef BENCH_ITERATIONS
#define BENCH_ITERATIONS          10000
#endif
#ifndef BENCH_WARMUP_ITERATIONS
#define BENCH_WARMUP_ITERATIONS   200
#endif

static int bench_compare_uint64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (int)((x > y) - (x < y));
}

/*
 * BENCH_RUN(name, setup_code, func_call)
 *
 * Executes the benchmark harness:
 *   1. Runs setup_code once (e.g., allocate/initialize arrays)
 *   2. Runs WARMUP_ITERATIONS untimed calls of func_call
 *   3. Runs LOOP_ITERATIONS timed batches, each with ITERATIONS calls of func_call
 *   4. Sorts measurements, computes median
 *   5. Prints results to stdout
 *
 * All code is inlined — no function pointers used.
 */
#define BENCH_RUN(name, setup_code, func_call) \
do { \
    setup_code \
    uint64_t measurements[BENCH_LOOP_ITERATIONS]; \
    \
    /* Warmup (untimed) */ \
    for (int _w = 0; _w < BENCH_WARMUP_ITERATIONS; _w++) { \
        func_call; \
    } \
    \
    /* Timed measurement loop */ \
    for (int _l = 0; _l < BENCH_LOOP_ITERATIONS; _l++) { \
        long long _start = cpucycles(); \
        for (int _i = 0; _i < BENCH_ITERATIONS; _i++) { \
            func_call; \
        } \
        long long _end = cpucycles(); \
        measurements[_l] = (uint64_t)(_end - _start); \
    } \
    \
    /* Sort for median */ \
    qsort(measurements, BENCH_LOOP_ITERATIONS, sizeof(uint64_t), bench_compare_uint64); \
    \
    /* Compute median */ \
    uint64_t _median; \
    if (BENCH_LOOP_ITERATIONS % 2 == 1) { \
        _median = measurements[BENCH_LOOP_ITERATIONS / 2]; \
    } else { \
        _median = (measurements[BENCH_LOOP_ITERATIONS / 2 - 1] + measurements[BENCH_LOOP_ITERATIONS / 2]) / 2; \
    } \
    \
    long long _hz = cpucycles_persecond(); \
    double _median_per_call = (double)_median / (double)BENCH_ITERATIONS; \
    double _median_us = _median_per_call / (double)_hz * 1e6; \
    \
    printf("\n=== %s ===\n", name); \
    printf("cpucycles implementation : %s\n", cpucycles_implementation()); \
    printf("Median cycles (per call) : %.0f cycles\n", _median_per_call); \
    printf("CPU Hz                   : %.3f GHz\n", (double)_hz / 1e9); \
    printf("Min (batch)              : %llu cycles\n", (unsigned long long)measurements[0]); \
    printf("Max (batch)              : %llu cycles\n", (unsigned long long)measurements[BENCH_LOOP_ITERATIONS - 1]); \
    printf("\n"); \
} while (0)

#endif /* BENCHMARK_H */