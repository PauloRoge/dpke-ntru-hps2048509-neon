#include "binary_inversion.h"

#include <arm_neon.h>

poly128_t binary_polynomial_multiplication_64x64_to_128(poly64_t a, poly64_t b) {
    // Perform polynomial multiplication using NEON intrinsics
    return vmull_p64(a, b);
}
