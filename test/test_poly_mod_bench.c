#include "benchmark.h"
#include "binary_poly.h"

int main(void) {
    poly64_t a[4] = { 0xdeadbeef, 0xcafebabe, 0x12345678, 0xabcdef01 };
    poly64_t b[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
    poly128_t c[4] = { 0 };

    BENCH_RUN("binary_polynomial_multiplication_256x256_to_512",
        ; /* setup already done above */,
        binary_polynomial_multiplication_256x256_to_512(a, b, c));

    return 0;
}