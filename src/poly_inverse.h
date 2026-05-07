#ifndef POLY_INVERSE_H
#define POLY_INVERSE_H

#include <stdint.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif

void R2_inverse(const uint64_t h[8], uint64_t hinv[8]);

#ifdef __cplusplus
}
#endif

#endif