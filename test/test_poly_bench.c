#include "benchmark.h"
#include "binary_inversion.h"

int main(void) {
    size_t N = 4;
    poly64_t a[4] = { 0xdeadbeef, 0xcafebabe, 0x12345678, 0xabcdef01 };
    poly64_t b[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
    poly128_t c[4] = { 0 };

    BENCH_RUN("binary_polynomial_multiplication (N=4)",
        ; /* setup already done above */,
        binary_polynomial_multiplication(a, b, c, N));

    return 0;
}