#include "binary_inversion.h"

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

void binary_polynomial_multiplication_64x64_to_128(poly64_t *a, poly64_t *b, poly128_t *c) {
    // Perform polynomial multiplication using NEON intrinsics
    *c = vmull_p64(*a, *b);
}

void binary_polynomial_multiplication(poly64_t *a, poly64_t *b,
     poly128_t *c, size_t N){
    
    uint64_t out[2 * N];

    for (size_t i = 0; i < 2 * N; i++) {
        out[i] = 0;
    }

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < N; j++) {
            poly128_t p;
            binary_polynomial_multiplication_64x64_to_128(&a[i], &b[j], &p);

            uint64x2_t u = vreinterpretq_u64_p128(p);
            uint64_t p_lo = vgetq_lane_u64(u, 0);
            uint64_t p_hi = vgetq_lane_u64(u, 1);

            size_t k = i + j;
            out[k] ^= p_lo;
            out[k+1] ^= p_hi;
        }
    }

    for (size_t i = 0; i < N; i++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out[2*i], v, 0);
        v = vsetq_lane_u64(out[2*i + 1], v, 1);
        c[i] = vreinterpretq_p128_u64(v);
    }
}

void binary_polynomial_multiplication_128x128_to_256(poly64_t a[2], poly64_t b[2], poly128_t c[2]){
    binary_polynomial_multiplication(a, b, c, 2);
}

void binary_polynomial_multiplication_256x256_to_512(poly64_t a[4], poly64_t b[4], poly128_t c[4]){
    binary_polynomial_multiplication(a, b, c, 4);
}

