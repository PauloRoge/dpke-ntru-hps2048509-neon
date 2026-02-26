#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <array>

#include "doctest.h"

extern "C" {
#include "binary_inversion.h"
}

using uint64_t_array_2 = std::array<uint64_t, 2>;
using uint64_t_array_4 = std::array<uint64_t, 4>;

static void poly128_t_to_uint64_t_array(uint64_t_array_2 &out, poly128_t poly) {
    vst1q_u64(out.data(), vreinterpretq_u64_p128(poly));
}

static void poly256_t_to_uint64_t_array(uint64_t_array_4 &out, const poly128_t c[2]) {
    uint64_t_array_2 lo, hi;
    poly128_t_to_uint64_t_array(lo, c[0]); // graus 0..127  -> [lo64, hi64]
    poly128_t_to_uint64_t_array(hi, c[1]); // graus 128..255 -> [lo64, hi64]

    out[0] = lo[0]; 
    out[1] = lo[1]; 
    out[2] = hi[0]; 
    out[3] = hi[1]; 
}

typedef struct {
    uint64_t a, b, cl, ch;
} binary_polynomial_multiplication_test_case_t;

const binary_polynomial_multiplication_test_case_t test_cases[] = {
    {0x0, 0x0, 0x0, 0x0},                                                              // 0 * 0 = 0
    {0x1, 0x1, 0x1, 0x0},                                                              // 1 * 1 = 1
    {0x2, 0x1, 0x2, 0x0},                                                              // x * 1 = x
    {0x2, 0x2, 0x4, 0x0},                                                              // x * x = x^2
    {UINT64_C(1) << 32, UINT64_C(1) << 32, 0x0, 0x1},                                  // x^32 * x^32 = x^64
    {UINT64_C(0xc9f7d8ab7f2d1a29), UINT64_C(0xa3dace72b96811d5),
         UINT64_C(0x7dc6194b9f13f7dd), UINT64_C(0x7cc29844002f16cb)},                  // random
    
    {UINT64_C(1) << 63, 0x1, UINT64_C(1) << 63, 0x0},                                  // x^63 * 1 = x^63
    {UINT64_C(1) << 63, 0x2, 0x0, 0x1},                                                // x^63 * x = x^64
    {UINT64_C(1) << 63, UINT64_C(1) << 63, 0x0, UINT64_C(1) << 62},                    // x^63 * x^63 = x^126
    {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
         UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)},
};

TEST_CASE("binary polynomial multiplication 64x64->128 works") {
    for (const auto &test_case : test_cases) {
        poly64_t a = vget_lane_p64(vdup_n_p64(test_case.a), 0);
        poly64_t b = vget_lane_p64(vdup_n_p64(test_case.b), 0);
        uint64_t_array_2 expected_result = {test_case.cl, test_case.ch};

        poly128_t result = binary_polynomial_multiplication_64x64_to_128(a, b);

        uint64_t_array_2 result_array;
        poly128_t_to_uint64_t_array(result_array, result);

        REQUIRE(result_array == expected_result);
    }
}

// caso 128
typedef struct {
    uint64_t a0, a1;
    uint64_t b0, b1;
    uint64_t r0, r1, r2, r3; // resultado 256 em 4 words de 64
} binary_polynomial_multiplication_128_test_case_t;

const binary_polynomial_multiplication_128_test_case_t test_cases_128[] = {
    
    {0x0, 0x0, 0x0, 0x0,
     0x0, 0x0, 0x0, 0x0},   // 0*0 = 0

    {0x1, 0x0, 0x1, 0x0,
     0x1, 0x0, 0x0, 0x0},   // 1*1 = 1

    {0x0, 0x1, 0x1, 0x0,
     0x0, 0x1, 0x0, 0x0},   // x^64*1 = x^64

    {0x0, 0x1, 0x0, 0x1,
     0x0, 0x0, 0x1, 0x0}, // x^64*x^64 = x^128

    {0x0, (UINT64_C(1) << 63), 0x2, 0x0,
     0x0, 0x0, 0x1, 0x0}, // x^127 * x = x^128  (a1 bit63, b0 bit1)

    // random
    {UINT64_C(0x168c1c323e0ce3f9), UINT64_C(0x89548d484c8a6c7c), UINT64_C(0x6510545849916e1b), UINT64_C(0xa9c2aa96a9d5a697),
     UINT64_C(0xd51a927bea83ba53), UINT64_C(0x15b7d2daeee743dc), UINT64_C(0xd5189e4d4a3a2bba), UINT64_C(0x51249ab14afe4904)},


    // (1 + ... + x^127)^2 = 1 + x^2 + x^4 + ... + x^254  -> padrão 0x55.. em todos os limbs
    {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
     UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
     UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
     UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)},
};

TEST_CASE("binary polynomial multiplication 128x128->256 works") {
    for (const auto &tc : test_cases_128) {
        poly64_t a[2] = {
            vget_lane_p64(vdup_n_p64(tc.a0), 0),
            vget_lane_p64(vdup_n_p64(tc.a1), 0),
        };
        poly64_t b[2] = {
            vget_lane_p64(vdup_n_p64(tc.b0), 0),
            vget_lane_p64(vdup_n_p64(tc.b1), 0),
        };

        uint64_t_array_4 expected = {tc.r0, tc.r1, tc.r2, tc.r3};

        poly128_t c[2];
        binary_polynomial_multiplication_128x128_to_256(a, b, c);

        uint64_t_array_4 got;
        poly256_t_to_uint64_t_array(got, c);

        REQUIRE(got == expected);
    }
}