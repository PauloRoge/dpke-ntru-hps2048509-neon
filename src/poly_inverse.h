#ifndef POLY_INVERSE_H
#define POLY_INVERSE_H

#include <stdint.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif

void frobenius_square_vmull(const uint64_t a[8], uint64_t out[8]);
void r2_inverse(const uint64_t h[8], uint64_t hinv[8]);

void frobenius_square_perm(const uint64_t a[8], uint64_t out[8], const uint16_t perm[509]);
void r2_mul(const uint64_t a[8], const uint64_t b[8], uint64_t out[8]);
void r2_beta_step(const uint64_t beta_k[8], const uint16_t perm[509], const uint64_t beta_j[8], uint64_t out[8]);

#ifdef __cplusplus
}
#endif

#endif