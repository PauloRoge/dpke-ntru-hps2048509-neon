#include "binary_poly.h"
#include "poly_mod.h"

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

#define R2_NWORDS 8
#define MASK ((uint64_t)((1ULL << 61) - 1ULL))

static void r2_mul(const uint64_t a[8], const uint64_t b[8], uint64_t out[8]) {
    poly64_t ap[8];
    poly64_t bp[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        ap[i] = (poly64_t)a[i];
        bp[i] = (poly64_t)b[i];
    }

    mul_karatsuba_512x512_to_1024_mod_x509m1(ap, bp, out);

    out[7] &= MASK;
}

void R2_inverse(const uint64_t h[8], uint64_t hinv[8]) {
    uint64_t acc[8];
    uint64_t aux[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        acc[i] = h[i];
    }

    acc[7] &= MASK;

    for (size_t i = 0; i < 506; i++) {
        r2_mul(acc, acc, aux);
        r2_mul(aux, h, acc);
    }

    r2_mul(acc, acc, hinv);
    hinv[7] &= MASK;
}