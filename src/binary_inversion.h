#ifndef BINARY_INVERSION_H
#define BINARY_INVERSION_H

#include <arm_neon.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void binary_polynomial_multiplication_64x64_to_128(poly64_t *a, poly64_t *b, poly128_t *c);

void binary_polynomial_multiplication_128x128_to_256(poly64_t a[2], poly64_t b[2], poly128_t c[2]);

void binary_polynomial_multiplication_256x256_to_512(poly64_t a[4], poly64_t b[4], poly128_t c[4]);

void binary_polynomial_multiplication_512x512_to_1024(poly64_t a[8], poly64_t b[8], poly128_t c[8]);

void mul_karatsuba_128x128_to_256(poly64_t a[2], poly64_t b[2], poly128_t c[2]);

void mul_karatsuba_256x256_to_512(poly64_t a[4], poly64_t b[4], poly128_t c[4]);

void mul_karatsuba_512x512_to_1024(poly64_t a[8], poly64_t b[8], poly128_t c[8]);

void mul_karatsuba_64Nx64N_to_128N(const poly64_t *a, const poly64_t *b, poly128_t *c, size_t N);

#ifdef __cplusplus
}
#endif

#endif