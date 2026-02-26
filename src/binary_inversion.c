#include "binary_inversion.h"

#include <arm_neon.h>

poly128_t binary_polynomial_multiplication_64x64_to_128(poly64_t a, poly64_t b) {
    // Perform polynomial multiplication using NEON intrinsics
    return vmull_p64(a, b);
}

void binary_polynomial_multiplication_128x128_to_256(const poly64_t a[2],
    const poly64_t b[2], poly128_t c[2]) {
    const poly64_t a0 = a[0];
    const poly64_t a1 = a[1];
    const poly64_t b0 = b[0];
    const poly64_t b1 = b[1];

    // (aL + x^64*aH)
    const poly128_t p00 = vmull_p64(a0, b0); // a0*b0
    const poly128_t p01 = vmull_p64(a0, b1); // a0*b1
    const poly128_t p10 = vmull_p64(a1, b0); // a1*b0
    const poly128_t p11 = vmull_p64(a1, b1); // a1*b1

    // aL, aH
    const uint64x2_t u00 = vreinterpretq_u64_p128(p00);
    const uint64x2_t u01 = vreinterpretq_u64_p128(p01);
    const uint64x2_t u10 = vreinterpretq_u64_p128(p10);
    const uint64x2_t u11 = vreinterpretq_u64_p128(p11);

    const uint64_t p00_lo = vgetq_lane_u64(u00, 0);
    const uint64_t p00_hi = vgetq_lane_u64(u00, 1);

    const uint64_t p01_lo = vgetq_lane_u64(u01, 0);
    const uint64_t p01_hi = vgetq_lane_u64(u01, 1);

    const uint64_t p10_lo = vgetq_lane_u64(u10, 0);
    const uint64_t p10_hi = vgetq_lane_u64(u10, 1);

    const uint64_t p11_lo = vgetq_lane_u64(u11, 0);
    const uint64_t p11_hi = vgetq_lane_u64(u11, 1);

    const uint64_t s_lo = p01_lo ^ p10_lo;
    const uint64_t s_hi = p01_hi ^ p10_hi;

    uint64x2_t c0 = vdupq_n_u64(0);
    uint64x2_t c1 = vdupq_n_u64(0);

    c0 = vsetq_lane_u64(p00_lo, c0, 0);
    c0 = vsetq_lane_u64(p00_hi ^ s_lo, c0, 1);

    c1 = vsetq_lane_u64(s_hi ^ p11_lo, c1, 0);
    c1 = vsetq_lane_u64(p11_hi, c1, 1);

    c[0] = vreinterpretq_p128_u64(c0);
    c[1] = vreinterpretq_p128_u64(c1);
}