#ifndef BINARY_INVERSION_H
#define BINARY_INVERSION_H

#include <arm_neon.h>

poly128_t binary_polynomial_multiplication_64x64_to_128(poly64_t a, poly64_t b);

void binary_polynomial_multiplication_128x128_to_256(const poly64_t a[2], const poly64_t b[2],
    poly128_t c[2]);
    
#endif // BINARY_INVERSION_H
