#include "binary_inversion.h"

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

void binary_polynomial_multiplication_64x64_to_128(poly64_t *a, poly64_t *b, poly128_t *c) {
    // Perform polynomial multiplication using NEON intrinsics
    *c = vmull_p64(*a, *b);
}

void binary_polynomial_multiplication(poly64_t *a, poly64_t *b,
     poly128_t *c, size_t N){
    
    uint64_t out[2 * N];
     
    // 0 é elemento neutro da operação XOR
    for (size_t i = 0; i < (2*N); i++) {
        out[i] = 0;
    }

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < N; j++) {
            poly128_t p;
            binary_polynomial_multiplication_64x64_to_128(&a[i], &b[j], &p);

            uint64x2_t u = vreinterpretq_u64_p128(p);
            uint64_t p_lo = vgetq_lane_u64(u, 0);
            uint64_t p_hi = vgetq_lane_u64(u, 1);

            size_t k = i + j;
            out[k] ^= p_lo;
            out[k+1] ^= p_hi;
        }
    }

    for (size_t i = 0; i < N; i++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out[2*i], v, 0);
        v = vsetq_lane_u64(out[2*i + 1], v, 1);
        c[i] = vreinterpretq_p128_u64(v);
    }
}

void mul_karatsuba(poly64_t a[2], poly64_t b[2], poly128_t c[2]) {
    poly128_t z0, z1, z2;

    // z0 = a0 * b0
    binary_polynomial_multiplication_64x64_to_128(&a[0], &b[0], &z0);

    // z2 = a1 * b1
    binary_polynomial_multiplication_64x64_to_128(&a[1], &b[1], &z2);

    // z1 = (a0 ^ a1) * (b0 ^ b1)
    poly64_t a_xor = a[0] ^ a[1];
    poly64_t b_xor = b[0] ^ b[1];
    binary_polynomial_multiplication_64x64_to_128(&a_xor, &b_xor, &z1);

    // extrai low/high de cada produto 128-bit
    uint64x2_t u0 = vreinterpretq_u64_p128(z0);
    uint64x2_t u1 = vreinterpretq_u64_p128(z1);
    uint64x2_t u2 = vreinterpretq_u64_p128(z2);

    uint64_t z0_lo = vgetq_lane_u64(u0, 0);
    uint64_t z0_hi = vgetq_lane_u64(u0, 1);

    uint64_t z1_lo = vgetq_lane_u64(u1, 0);
    uint64_t z1_hi = vgetq_lane_u64(u1, 1);

    uint64_t z2_lo = vgetq_lane_u64(u2, 0);
    uint64_t z2_hi = vgetq_lane_u64(u2, 1);

    // termo cruzado: z1 ^ z0 ^ z2
    uint64_t mid_lo = z1_lo ^ z0_lo ^ z2_lo;
    uint64_t mid_hi = z1_hi ^ z0_hi ^ z2_hi;

    // recomposição em 4 words de 64 bits:
    uint64_t out0 = z0_lo;
    uint64_t out1 = z0_hi ^ mid_lo;
    uint64_t out2 = mid_hi ^ z2_lo;
    uint64_t out3 = z2_hi;

    // empacota em dois poly128_t
    uint64x2_t v0 = vdupq_n_u64(0);
    v0 = vsetq_lane_u64(out0, v0, 0);
    v0 = vsetq_lane_u64(out1, v0, 1);
    c[0] = vreinterpretq_p128_u64(v0);

    uint64x2_t v1 = vdupq_n_u64(0);
    v1 = vsetq_lane_u64(out2, v1, 0);
    v1 = vsetq_lane_u64(out3, v1, 1);
    c[1] = vreinterpretq_p128_u64(v1);
}

void mul_karatsuba256x256(poly64_t a[4], poly64_t b[4], poly128_t c[4]) {
    poly64_t a0[2] = {a[0], a[1]};
    poly64_t a1[2] = {a[2], a[3]};
    poly64_t b0[2] = {b[0], b[1]};
    poly64_t b1[2] = {b[2], b[3]};

    poly128_t z0[2];
    poly128_t z1[2];
    poly128_t z2[2];

    // z0 = a0 * b0
    mul_karatsuba(a0, b0, z0);

    // z2 = a1 * b1
    mul_karatsuba(a1, b1, z2);

    // (a0 ^ a1) e (b0 ^ b1)
    poly64_t a_xor[2];
    poly64_t b_xor[2];

    a_xor[0] = a0[0] ^ a1[0];
    a_xor[1] = a0[1] ^ a1[1];

    b_xor[0] = b0[0] ^ b1[0];
    b_xor[1] = b0[1] ^ b1[1];

    // z1 = (a0 ^ a1)(b0 ^ b1)
    mul_karatsuba(a_xor, b_xor, z1);

    // Converter z0, z1, z2 para words (4 words cada)
    uint64_t z0w[4];
    uint64_t z1w[4];
    uint64_t z2w[4];

    for (int i = 0; i < 2; i++) {
        uint64x2_t v0 = vreinterpretq_u64_p128(z0[i]);
        uint64x2_t v1 = vreinterpretq_u64_p128(z1[i]);
        uint64x2_t v2 = vreinterpretq_u64_p128(z2[i]);

        z0w[2*i]     = vgetq_lane_u64(v0, 0); // fixo segundo parametro tem que ser constante 
        z0w[2*i + 1] = vgetq_lane_u64(v0, 1);

        z1w[2*i]     = vgetq_lane_u64(v1, 0);
        z1w[2*i + 1] = vgetq_lane_u64(v1, 1);

        z2w[2*i]     = vgetq_lane_u64(v2, 0);
        z2w[2*i + 1] = vgetq_lane_u64(v2, 1);
    }

    // z1 ^ z0 ^ z2
    uint64_t mid[4];
    for (int i = 0; i < 4; i++) {
        mid[i] = z1w[i] ^ z0w[i] ^ z2w[i];
    }

    // Resultado final (8 words)
    uint64_t out[8] = {0};

    out[0] ^= z0w[0];
    out[1] ^= z0w[1];
    out[2] ^= z0w[2];
    out[3] ^= z0w[3];

    out[2] ^= mid[0];
    out[3] ^= mid[1];
    out[4] ^= mid[2];
    out[5] ^= mid[3];

    out[4] ^= z2w[0];
    out[5] ^= z2w[1];
    out[6] ^= z2w[2];
    out[7] ^= z2w[3];

    for (int i = 0; i < 4; i++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out[2*i], v, 0);
        v = vsetq_lane_u64(out[2*i + 1], v, 1);
        c[i] = vreinterpretq_p128_u64(v);
    }
}

void mul_karatsuba512x512(poly64_t a[8], poly64_t b[8], poly128_t c[8]) {
    // a = a1 * x^256 + a0
    // b = b1 * x^256 + b0

    poly64_t a0[4] = {a[0], a[1], a[2], a[3]};
    poly64_t a1[4] = {a[4], a[5], a[6], a[7]};
    poly64_t b0[4] = {b[0], b[1], b[2], b[3]};
    poly64_t b1[4] = {b[4], b[5], b[6], b[7]};

    poly128_t z0[4];
    poly128_t z1[4];
    poly128_t z2[4];

    // z0 = a0 * b0
    mul_karatsuba256x256(a0, b0, z0);

    // z2 = a1 * b1
    mul_karatsuba256x256(a1, b1, z2);

    // z1 = (a0 ^ a1) * (b0 ^ b1)
    poly64_t a_xor[4];
    poly64_t b_xor[4];

    for (int i = 0; i < 4; i++) {
        a_xor[i] = a0[i] ^ a1[i];
        b_xor[i] = b0[i] ^ b1[i];
    }

    mul_karatsuba256x256(a_xor, b_xor, z1);

    // Converter z0, z1, z2 para words 8 cada
    uint64_t z0w[8];
    uint64_t z1w[8];
    uint64_t z2w[8];

    for (int i = 0; i < 4; i++) {
        uint64x2_t v0 = vreinterpretq_u64_p128(z0[i]);
        uint64x2_t v1 = vreinterpretq_u64_p128(z1[i]);
        uint64x2_t v2 = vreinterpretq_u64_p128(z2[i]);

        z0w[2 * i]     = vgetq_lane_u64(v0, 0);
        z0w[2 * i + 1] = vgetq_lane_u64(v0, 1);

        z1w[2 * i]     = vgetq_lane_u64(v1, 0);
        z1w[2 * i + 1] = vgetq_lane_u64(v1, 1);

        z2w[2 * i]     = vgetq_lane_u64(v2, 0);
        z2w[2 * i + 1] = vgetq_lane_u64(v2, 1);
    }

    uint64_t mid[8];
    
    for (int i = 0; i < 8; i++)
        mid[i] = z1w[i] ^ z0w[i] ^ z2w[i];

    uint64_t out[16] = {0};

    for (int i = 0; i < 8; i++)
        out[i] ^= z0w[i];

    for (int i = 0; i < 8; i++)
        out[i + 4] ^= mid[i];

    for (int i = 0; i < 8; i++)
        out[i + 8] ^= z2w[i];

    // Empacotar em poly128_t c[8]
    for (int i = 0; i < 8; i++) {
        uint64x2_t v = vdupq_n_u64(0);
        v = vsetq_lane_u64(out[2 * i], v, 0);
        v = vsetq_lane_u64(out[2 * i + 1], v, 1);
        c[i] = vreinterpretq_p128_u64(v);
    }
}

void binary_polynomial_multiplication_128x128_to_256(poly64_t a[2], poly64_t b[2], poly128_t c[2]){
    binary_polynomial_multiplication(a, b, c, 2);
}

void binary_polynomial_multiplication_256x256_to_512(poly64_t a[4], poly64_t b[4], poly128_t c[4]){
    binary_polynomial_multiplication(a, b, c, 4);
}

void binary_polynomial_multiplication_512x512_to_1024(poly64_t a[8], poly64_t b[8], poly128_t c[8]){
    binary_polynomial_multiplication(a, b, c, 8);
}

