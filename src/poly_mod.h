#ifndef POLY_MOD_H
#define POLY_MOD_H

#include <stdint.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif

void binary_polynomial_mul_reduce_mod_x509m1(poly64_t a[8], poly64_t b[8], uint64_t h[8]);

#ifdef __cplusplus
}
#endif

#endif