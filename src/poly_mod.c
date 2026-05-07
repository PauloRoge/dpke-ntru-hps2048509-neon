#include <stddef.h>
#include <stdint.h>
#include <arm_neon.h>

#include "poly_mod.h"
#include "binary_poly.h"

void reduce_mod_x509m1(const uint64_t c[16], uint64_t h[8]) {
    uint64_t hi[8];

    for (size_t i = 0; i < 7; i++)
        hi[i] = (c[i + 7] >> 61) | (c[i + 8] << 3);

    hi[7] = (c[14] >> 61) | (c[15] << 3);

    for (size_t i = 0; i < 8; i++)
        h[i] = c[i] ^ hi[i];

    h[7] &= (UINT64_C(1) << 61) - 1;
}

void binary_polynomial_mul_reduce_mod_x509m1(poly64_t a[8], poly64_t b[8], uint64_t h[8]) {
    poly128_t c[8];
    uint64_t x[16];

    binary_polynomial_multiplication_512x512_to_1024(a, b, c);

    for (size_t i = 0; i < 8; i++) {
        uint64x2_t v = vreinterpretq_u64_p128(c[i]);
        x[2 * i] = vgetq_lane_u64(v, 0);
        x[2 * i + 1] = vgetq_lane_u64(v, 1);    
    }

    reduce_mod_x509m1(x, h);
}

void mul_karatsuba_512x512_to_1024_mod_x509m1(poly64_t a[8], poly64_t b[8], uint64_t h[8]) {
    poly128_t c[8];
    uint64_t x[16];

    mul_karatsuba_512x512_to_1024_unfolded(a, b, c);

    for (size_t i = 0; i < 8; i++) {
        uint64x2_t v = vreinterpretq_u64_p128(c[i]);
        x[2 * i] = vgetq_lane_u64(v, 0);
        x[2 * i + 1] = vgetq_lane_u64(v, 1);
    }

    reduce_mod_x509m1(x, h);
}