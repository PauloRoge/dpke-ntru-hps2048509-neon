#include "benchmark.h"
#include "poly_mod.h"
#include "poly_inverse.h"
#include "r2_frobenius_perms.h"

#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R2_NWORDS 8

#define MASK ((uint64_t)((1ULL << 61) - 1ULL))

static inline uint8_t get_coeff(const uint64_t a[], size_t n) {
    return (uint8_t)((a[n / 64] >> (n % 64)) & 1ULL);
}

// beta_{k+j} = beta_k^(2^j) * beta_j, com frobenius por j lacos de vmull
static void r2_beta_step_vmull(const uint64_t beta_k[8], size_t j, const uint64_t beta_j[8], uint64_t out[8]) {
    uint64_t a[8];
    uint64_t tmp[8];

    for (size_t i = 0; i < R2_NWORDS; i++) {
        a[i] = beta_k[i];
    }

    a[7] &= MASK;

    for (size_t i = 0; i < j; i++) {
        frobenius_square_vmull(a, tmp);

        for (size_t w = 0; w < R2_NWORDS; w++) {
            a[w] = tmp[w];
        }
    }

    r2_mul(a, beta_j, out);
    out[7] &= MASK;
}

// V0: todas as permutacoes por tabela
static void r2_inverse_v0(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step(beta1,   perm_1,   beta1,   beta2);
    r2_beta_step(beta2,   perm_1,   beta1,   beta3);
    r2_beta_step(beta3,   perm_3,   beta3,   beta6);
    r2_beta_step(beta6,   perm_6,   beta6,   beta12);
    r2_beta_step(beta12,  perm_3,   beta3,   beta15);
    r2_beta_step(beta15,  perm_15,  beta15,  beta30);
    r2_beta_step(beta30,  perm_30,  beta30,  beta60);
    r2_beta_step(beta60,  perm_3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step(beta504, perm_3,   beta3,   beta507);

    frobenius_square_perm(beta507, hinv, perm_1);
    hinv[7] &= MASK;
}

// V1: k=1 via vmull
static void r2_inverse_v1(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step(beta3,   perm_3,   beta3,   beta6);
    r2_beta_step(beta6,   perm_6,   beta6,   beta12);
    r2_beta_step(beta12,  perm_3,   beta3,   beta15);
    r2_beta_step(beta15,  perm_15,  beta15,  beta30);
    r2_beta_step(beta30,  perm_30,  beta30,  beta60);
    r2_beta_step(beta60,  perm_3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step(beta504, perm_3,   beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V2: k<=3 via vmull
static void r2_inverse_v2(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,    beta3,   beta6);
    r2_beta_step(beta6,   perm_6,   beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,   beta3,   beta15);
    r2_beta_step(beta15,  perm_15,  beta15,  beta30);
    r2_beta_step(beta30,  perm_30,  beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,  beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V3: k<=6 via vmull
static void r2_inverse_v3(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,    beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,    beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,   beta3,   beta15);
    r2_beta_step(beta15,  perm_15,  beta15,  beta30);
    r2_beta_step(beta30,  perm_30,  beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,  beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V4: k<=15 via vmull
static void r2_inverse_v4(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,    beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,    beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,   beta3,   beta15);
    r2_beta_step_vmull(beta15, 15,  beta15,  beta30);
    r2_beta_step(beta30,  perm_30,  beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,  beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V5: k<=30 via vmull
static void r2_inverse_v5(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,    beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,    beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,   beta3,   beta15);
    r2_beta_step_vmull(beta15, 15,  beta15,  beta30);
    r2_beta_step_vmull(beta30, 30,  beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,   beta3,   beta63);
    r2_beta_step(beta63,  perm_63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,  beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V6: k<=63 via vmull
static void r2_inverse_v6(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,    beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,    beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,    beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,    beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,   beta3,   beta15);
    r2_beta_step_vmull(beta15, 15,  beta15,  beta30);
    r2_beta_step_vmull(beta30, 30,  beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,   beta3,   beta63);
    r2_beta_step_vmull(beta63, 63,  beta63,  beta126);
    r2_beta_step(beta126, perm_126, beta126, beta252);
    r2_beta_step(beta252, perm_252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,  beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V7: k<=126 via vmull
static void r2_inverse_v7(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,     beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,     beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,     beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,     beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,    beta3,   beta15);
    r2_beta_step_vmull(beta15, 15,   beta15,  beta30);
    r2_beta_step_vmull(beta30, 30,   beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,    beta3,   beta63);
    r2_beta_step_vmull(beta63, 63,   beta63,  beta126);
    r2_beta_step_vmull(beta126, 126, beta126, beta252);
    r2_beta_step(beta252, perm_252,  beta252, beta504);
    r2_beta_step_vmull(beta504, 3,   beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

// V8: todas via vmull
static void r2_inverse_v8(const uint64_t h[8], uint64_t hinv[8]) {
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

    for (int i = 0; i < R2_NWORDS; i++) {
        beta1[i] = h[i];
    }

    beta1[7] &= MASK;

    r2_beta_step_vmull(beta1, 1,     beta1,   beta2);
    r2_beta_step_vmull(beta2, 1,     beta1,   beta3);
    r2_beta_step_vmull(beta3, 3,     beta3,   beta6);
    r2_beta_step_vmull(beta6, 6,     beta6,   beta12);
    r2_beta_step_vmull(beta12, 3,    beta3,   beta15);
    r2_beta_step_vmull(beta15, 15,   beta15,  beta30);
    r2_beta_step_vmull(beta30, 30,   beta30,  beta60);
    r2_beta_step_vmull(beta60, 3,    beta3,   beta63);
    r2_beta_step_vmull(beta63, 63,   beta63,  beta126);
    r2_beta_step_vmull(beta126, 126, beta126, beta252);
    r2_beta_step_vmull(beta252, 252, beta252, beta504);
    r2_beta_step_vmull(beta504, 3,   beta3,   beta507);

    frobenius_square_vmull(beta507, hinv);
    hinv[7] &= MASK;
}

int main(void) {
    // polinomio de teste; h[7] respeita MASK = 2^61 - 1
    static const uint64_t h[8] = {
        0xdeadbeefcafe1357ULL,
        0x0123456789abcdefULL,
        0xfedcba9876543210ULL,
        0xabcdef0123456789ULL,
        0x1111222233334444ULL,
        0x5555666677778888ULL,
        0x9999aaaabbbbccccULL,
        0x0dddeefff1234567ULL,
    };

    uint64_t ref[8];
    uint64_t hinv[8];

    // verifica se cada variante resulta em mesma inversa que r2_inverse
    r2_inverse(h, ref);

    void (*variants[9])(const uint64_t[8], uint64_t[8]) = {
        r2_inverse_v0, r2_inverse_v1, r2_inverse_v2,
        r2_inverse_v3, r2_inverse_v4, r2_inverse_v5,
        r2_inverse_v6, r2_inverse_v7, r2_inverse_v8,
    };

    for (int v = 0; v < 9; v++) {
        variants[v](h, hinv);

        if (memcmp(hinv, ref, sizeof(ref)) != 0) {
            printf("ERRO: V%d difere de r2_inverse — benchmark abortado\n", v);
            return 1;
        }
    }

    BENCH_RUN("V0 r2_inverse perm_all",
        ; /* setup already done above */,
        r2_inverse_v0(h, hinv));

    BENCH_RUN("V1 r2_inverse vmull k=1",
        ; /* setup already done above */,
        r2_inverse_v1(h, hinv));

    BENCH_RUN("V2 r2_inverse vmull k<=3",
        ; /* setup already done above */,
        r2_inverse_v2(h, hinv));

    BENCH_RUN("V3 r2_inverse vmull k<=6",
        ; /* setup already done above */,
        r2_inverse_v3(h, hinv));

    BENCH_RUN("V4 r2_inverse vmull k<=15",
        ; /* setup already done above */,
        r2_inverse_v4(h, hinv));

    BENCH_RUN("V5 r2_inverse vmull k<=30",
        ; /* setup already done above */,
        r2_inverse_v5(h, hinv));

    BENCH_RUN("V6 r2_inverse vmull k<=63",
        ; /* setup already done above */,
        r2_inverse_v6(h, hinv));

    BENCH_RUN("V7 r2_inverse vmull k<=126",
        ; /* setup already done above */,
        r2_inverse_v7(h, hinv));

    BENCH_RUN("V8 r2_inverse vmull_all",
        ; /* setup already done above */,
        r2_inverse_v8(h, hinv));

    return 0;
}