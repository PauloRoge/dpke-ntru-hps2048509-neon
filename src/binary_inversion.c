#include "binary_inversion.h"

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

poly128_t binary_polynomial_multiplication_64x64_to_128(poly64_t a, poly64_t b) {
    // Perform polynomial multiplication using NEON intrinsics
    return vmull_p64(a, b);
}

static void binary_polynomial_multiplication_words_to_words(const poly64_t *a, const poly64_t *b,
     poly128_t *c, size_t nwords){
    
    uint64_t out_words[2 * nwords];

    for (size_t i = 0; i < 2 * nwords; i++) {
        out_words[i] = 0;
    }

    for (size_t i = 0; i < nwords; i++) {
        for (size_t j = 0; j < nwords; j++) {
            poly128_t p = vmull_p64(a[i], b[j]);

            uint64x2_t u = vreinterpretq_u64_p128(p);
            uint64_t p_lo = vgetq_lane_u64(u, 0);
            uint64_t p_hi = vgetq_lane_u64(u, 1);

            size_t k = i + j;
            out_words[k]     ^= p_lo;
            out_words[k+1] ^= p_hi;
        }
    }

    for (size_t t = 0; t < nwords; t++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out_words[2*t], v, 0);
        v = vsetq_lane_u64(out_words[2*t + 1], v, 1);
        c[t] = vreinterpretq_p128_u64(v);
    }
}

void binary_polynomial_multiplication_128x128_to_256(const poly64_t a[2], const poly64_t b[2], poly128_t c[2]){
    binary_polynomial_multiplication_words_to_words(a, b, c, 2);
}

void binary_polynomial_multiplication_256x256_to_512(const poly64_t a[4], const poly64_t b[4], poly128_t c[4]){
    binary_polynomial_multiplication_words_to_words(a, b, c, 4);
}

