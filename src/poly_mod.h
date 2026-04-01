#ifndef POLY_MOD_H
#define POLY_MOD_H

#include <stdint.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif

void reduce_mod509(const uint64_t x[16], uint64_t h[8]);

void binary_polynomial_mul_mod509(poly64_t a[8], poly64_t b[8], uint64_t h[8]);

#ifdef __cplusplus
}
#endif

#endif