#include "binary_poly.h"
#include "poly_mod.h"

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define R2_NWORDS 8

#define MASK ((uint64_t)((1ULL << 61) - 1ULL))

static inline uint8_t get_coeff(const uint64_t a[], size_t n) {
    return (uint8_t)((a[n / 64] >> (n % 64)) & 1ULL);
}

void frobenius_square_vmull(const uint64_t a[8], uint64_t out[8]) {
    poly64_t ap[8];
    poly128_t sq[8];
    uint64_t expanded[16];

    for (size_t i = 0; i < 16; i++) {
        expanded[i] = 0;
    }

    for (size_t i = 0; i < R2_NWORDS; i++) {
        ap[i] = (poly64_t)a[i];
    }

    ap[7] &= (poly64_t)MASK;

    binary_polynomial_square(ap, sq, R2_NWORDS);

    for (size_t i = 0; i < R2_NWORDS; i++) {
        uint64x2_t v = vreinterpretq_u64_p128(sq[i]);

        expanded[2 * i]     = vgetq_lane_u64(v, 0);
        expanded[2 * i + 1] = vgetq_lane_u64(v, 1);
    }

    reduce_mod_x509m1(expanded, out);

    out[7] &= MASK;
}

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

// pior caso onde todos bits são 1; [(2^508) - 1] 
void r2_inverse_ltr(const uint64_t h[8], uint64_t hinv[8]) {
    uint64_t r[8];
    uint64_t aux[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        r[i] = h[i];
    }

    r[7] &= MASK;

    for (size_t i = 0; i < 506; i++) {
        r2_mul(r, r, aux);
        r2_mul(aux, h, r);
    }

    r2_mul(r, r, hinv);
    hinv[7] &= MASK;
}

static void frobenius_square(const uint64_t a[8], uint64_t out[8]) {
    memset(out, 0, sizeof(uint64_t) * R2_NWORDS);

    for (size_t i = 0; i < 509; i++) {
        if (get_coeff(a, i)) {
            size_t j = (2 * i) % 509;
            out[j / 64] |= 1ULL << (j % 64);
        }
    }

    out[7] &= MASK;
}

// pior caso onde todos bits são 1; [(2^508) - 1]
void r2_inverse(const uint64_t h[8], uint64_t hinv[8]) {
    uint64_t r[8];
    uint64_t aux[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        r[i] = h[i];
    }

    r[7] &= MASK;

    for (size_t i = 0; i < 506; i++) {
        frobenius_square_vmull(r, aux);
        r2_mul(aux, h, r);
    }

    frobenius_square_vmull(r, hinv);
    hinv[7] &= MASK;
}

//beta_{k+j} = beta_k^(2^j) * beta_j
static void r2_beta_step(const uint64_t beta_k[8], size_t j, const uint64_t beta_j[8], uint64_t out[8]) {
    
    // beta_k^(2^j) pode ser calculado por j iterações de squaring, ou seja, aplicando a função de Frobenius j vezes
    uint64_t a[8];
    uint64_t tmp[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        a[i] = beta_k[i];
    }
    a[7] &= MASK;

    for (size_t i = 0; i < j; i++) {
        r2_mul(a, a, tmp);

        for (size_t i = 0; i < R2_NWORDS; i++) {
        a[i] = tmp[i];
        }
        a[7] &= MASK;
    }

    for (size_t i = 0; i < R2_NWORDS; i++) {
        out[i] = a[i];
    }
    out[7] &= MASK;
    //

    r2_mul(tmp, beta_j, out);
    out[7] &= MASK;
}

void r2_inverse_itoh(const uint64_t h[8], uint64_t hinv[8]) {
    uint64_t beta1[8];
    uint64_t beta2[8];
    uint64_t beta3[8];
    uint64_t beta6[8];
    uint64_t beta12[8];
    uint64_t beta15[8];
    uint64_t beta30[8];
    uint64_t beta60[8];
    uint64_t beta63[8];
    uint64_t beta126[8];
    uint64_t beta252[8];
    uint64_t beta504[8];
    uint64_t beta507[8];

    // beta_i = h^(2^i - 1)    
    for (size_t i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    // beta2 = beta1^2 * beta1 = h^(2^2 - 1)
    r2_beta_step(beta1, 1, beta1, beta2);

    // 1, 2, 3, 6, 12, 15, 30, 60, 63, 126, 252, 504, 507
    r2_beta_step(beta2,   1,   beta1,   beta3); // beta3 = beta2^(2^1) * beta1
    r2_beta_step(beta3,   3,   beta3,   beta6); // beta6 = beta3^(2^3) * beta3
    r2_beta_step(beta6,   6,   beta6,   beta12); // beta12 = beta6^(2^6) * beta6
    r2_beta_step(beta12,  3,   beta3,   beta15); // beta15 = beta12^(2^3) * beta3
    r2_beta_step(beta15,  15,  beta15,  beta30); // beta30 = beta15^(2^15) * beta15
    r2_beta_step(beta30,  30,  beta30,  beta60); // beta60 = beta30^(2^30) * beta30
    r2_beta_step(beta60,  3,   beta3,   beta63); // beta63 = beta60^(2^3) * beta3
    r2_beta_step(beta63,  63,  beta63,  beta126); // beta126 = beta63^(2^63) * beta63
    r2_beta_step(beta126, 126, beta126, beta252); // beta252 = beta126^(2^126) * beta126
    r2_beta_step(beta252, 252, beta252, beta504); // beta504 = beta252^(2^252) * beta252
    r2_beta_step(beta504, 3,   beta3,   beta507); // beta507 = beta504^(2^3) * beta3

    r2_mul(beta507, beta507, hinv);
    hinv[7] &= MASK;
}

