#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <array>
#include "doctest.h"

extern "C" {
#include "binary_inversion.h"
}

template <size_t N>
struct binary_polynomial_multiplication_test_case_t {
    std::array<uint64_t, N> a;
    std::array<uint64_t, N> b;
    std::array<uint64_t, 2*N> r;
};

template <size_t N>
static void poly128N_t_to_uint64_t_array(std::array<uint64_t, 2*N> &out, poly128_t (&poly)[N]) {
    for (size_t i = 0; i < N; i++) {
        uint64x2_t v = vreinterpretq_u64_p128(poly[i]);

        out[2*i]     = vgetq_lane_u64(v, 0);
        out[2*i + 1] = vgetq_lane_u64(v, 1);
    }
}

template <size_t N, size_t cases>
static void test_multiplication_64Nx64N_to_128N(
    const binary_polynomial_multiplication_test_case_t<N> (&test_cases)[cases],
    void (*multiplication_func)(poly64_t*, poly64_t*, poly128_t*)) {
    for (const auto &tc : test_cases) {
        poly64_t a[N];
        poly64_t b[N];

        for (size_t i = 0; i < N; i++) {
            a[i] = vget_lane_p64(vdup_n_p64(tc.a[i]), 0);
            b[i] = vget_lane_p64(vdup_n_p64(tc.b[i]), 0);
        }

        const auto expected = tc.r;

        poly128_t c[N];
        multiplication_func(a, b, c);

        std::array<uint64_t, 2*N> got;
        poly128N_t_to_uint64_t_array<N>(got, c);

        REQUIRE(got == expected);
    }
}

const binary_polynomial_multiplication_test_case_t<1> test_cases_64[] = {
    {{0x0}, {0x0}, {0x0, 0x0}},
    {{0x1}, {0x1}, {0x1, 0x0}},
    {{0x2}, {0x1}, {0x2, 0x0}},
    {{0x2}, {0x2}, {0x4, 0x0}},
    {{UINT64_C(1) << 32}, {UINT64_C(1) << 32}, {0x0, 0x1}},
    {{UINT64_C(0xc9f7d8ab7f2d1a29)}, {UINT64_C(0xa3dace72b96811d5)},
     {UINT64_C(0x7dc6194b9f13f7dd), UINT64_C(0x7cc29844002f16cb)}},

    {{UINT64_C(1) << 63}, {0x1}, {UINT64_C(1) << 63, 0x0}},
    {{UINT64_C(1) << 63}, {0x2}, {0x0, 0x1}},
    {{UINT64_C(1) << 63}, {UINT64_C(1) << 63}, {0x0, UINT64_C(1) << 62}},
    {{UINT64_C(0xffffffffffffffff)}, {UINT64_C(0xffffffffffffffff)},
     {UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)}},
};

const binary_polynomial_multiplication_test_case_t<2> test_cases_128[] = {
    {{0x0, 0x0}, {0x0, 0x0}, {0x0, 0x0, 0x0, 0x0}},
    {{0x1, 0x0}, {0x1, 0x0}, {0x1, 0x0, 0x0, 0x0}},
    {{0x0, 0x1}, {0x1, 0x0}, {0x0, 0x1, 0x0, 0x0}},
    {{0x0, 0x1}, {0x0, 0x1}, {0x0, 0x0, 0x1, 0x0}},
    {{0x0, (UINT64_C(1) << 63)}, {0x2, 0x0}, {0x0, 0x0, 0x1, 0x0}},

    {{UINT64_C(0x168c1c323e0ce3f9), UINT64_C(0x89548d484c8a6c7c)},
     {UINT64_C(0x6510545849916e1b), UINT64_C(0xa9c2aa96a9d5a697)},
     {UINT64_C(0xd51a927bea83ba53), UINT64_C(0x15b7d2daeee743dc),
      UINT64_C(0xd5189e4d4a3a2bba), UINT64_C(0x51249ab14afe4904)}},

    {{UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},
     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},
     {UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)}},
};

const binary_polynomial_multiplication_test_case_t<4> test_cases_256[] = {
    {{0x0, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}, // 0 * 0 = 0

    {{0x1, 0x0, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}, // 1 * 1 = 1

    {{0x0, 0x1, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0},
     {0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}, // x^64 * 1 = x^64
     
    {{0x0, 0x0, 0x1, 0x0},
     {0x1, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0}}, // x^128 * 1 = x^128
    
    {{0x0, 0x0, 0x0, 0x1},
     {0x1, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0}}, // x^192 * 1 = x^192
     
    // random
{{UINT64_C(0x7a16b2fe325764e7), UINT64_C(0xaf5a130c243f79e7), UINT64_C(0x9ca82c06b15263dd), UINT64_C(0xc8de8343f474e889)},
     {UINT64_C(0xa31d4a1841315e16), UINT64_C(0x9e71014ca372e0b6), UINT64_C(0x6d467d5e8cb24e20), UINT64_C(0x92317b5a01283f42)},
     {UINT64_C(0xa4e097b0fef8ce22), UINT64_C(0xd92755f774ebe1dc), UINT64_C(0x87ac0d6568c417f8), UINT64_C(0x41763e456d6ad35e),
      UINT64_C(0x966209aa7345a5b5), UINT64_C(0xfc357c6401a4510c), UINT64_C(0x9b8fbe52a4a79202), UINT64_C(0x69666af2ff52e3b5)}},

    {{UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},

     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},

     {UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555)}},
};

TEST_CASE("binary polynomial multiplication 64x64->128 works") {
    test_multiplication_64Nx64N_to_128N(test_cases_64, binary_polynomial_multiplication_64x64_to_128);
}

TEST_CASE("binary polynomial multiplication 128x128->256 works") {
    test_multiplication_64Nx64N_to_128N(test_cases_128, binary_polynomial_multiplication_128x128_to_256);
}

TEST_CASE("binary polynomial multiplication 256x256->512 works") {
    test_multiplication_64Nx64N_to_128N(test_cases_256, binary_polynomial_multiplication_256x256_to_512);
}