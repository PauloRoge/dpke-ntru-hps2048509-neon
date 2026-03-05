#include "binary_inversion.h"
#include <arm_neon.h>

#define NWORDS 2 

poly128_t binary_polynomial_multiplication_64x64_to_128(poly64_t a, poly64_t b) {
    // Perform polynomial multiplication using NEON intrinsics
    return vmull_p64(a, b);
}

void binary_polynomial_multiplication_128x128_to_256(const poly64_t a[NWORDS], const poly64_t b[NWORDS],
    poly128_t c[NWORDS]) {

    // inicializa para resultado
    uint64_t out_words[2*NWORDS] = {0};

    for (int i = 0; i < NWORDS; i++) {
        for (int j = 0; j < NWORDS; j++) {

            poly128_t p = vmull_p64(a[i], b[j]);

            uint64x2_t u = vreinterpretq_u64_p128(p);
            uint64_t p_lo = vgetq_lane_u64(u, 0);
            uint64_t p_hi = vgetq_lane_u64(u, 1);

            int k = i+j;
            out_words[k] ^= p_lo;
            out_words[k + 1] ^= p_hi;
        }
    }

    for (int t = 0; t < NWORDS; t++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out_words[2*t + 0], v, 0);
        v = vsetq_lane_u64(out_words[2*t + 1], v, 1);
        c[t] = vreinterpretq_p128_u64(v);
    }
}