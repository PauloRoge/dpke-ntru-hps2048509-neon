#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <array>

#include "doctest.h"

extern "C" {
#include "binary_inversion.h"
#include "poly_mod.h"
}

template <size_t N>
struct binary_polynomial_multiplication_test_case_t {
    std::array<uint64_t, N>     a;
    std::array<uint64_t, N>     b;
    std::array<uint64_t, 2*N>   r;
};

template <size_t N>
static void poly128N_t_to_uint64_t_array(std::array<uint64_t, 2*N> &out, poly128_t (&poly)[N]) {
    for (size_t i = 0; i < N; i++) {
        uint64x2_t v = vreinterpretq_u64_p128(poly[i]);

        out[2*i] = vgetq_lane_u64(v, 0);
        out[2*i + 1] = vgetq_lane_u64(v, 1);
    }
}

// Funções para reference_mul
// Representar polinômios como vetor de coeficientes binários, para implementação de referência bit a bit
template <size_t N>
static void uint64N_to_uint8_array(const std::array<uint64_t, N>& poly, uint8_t v[64 * N]) {
    for (size_t i = 0; i < 64 * N; i++) {
        v[i] = (poly[i / 64] >> (i % 64)) & 1;
    }
}

static void ref_poly_mul(
    const uint8_t a[],
    const uint8_t b[],
    uint8_t c[],
    size_t ncoeffs
) {
    std::memset(c, 0, (2 * ncoeffs - 1) * sizeof(uint8_t));

    for (size_t j = 0; j < ncoeffs; j++) {
        for (size_t i = 0; i < ncoeffs; i++) {
            c[i + j] ^= static_cast<uint8_t>(a[i] & b[j]);
        }
    }
}

template <size_t N>
static void uint8_array_to_uint64_2N(const uint8_t in[128 * N - 1], std::array<uint64_t, 2 * N>& out) {
    out.fill(0);

    for (size_t i = 0; i < 128 * N - 1; i++) {
                out[i / 64] |= (uint64_t(in[i] &1) << (i % 64));
    }
}

// Funções para redução modular de referência.
static uint8_t get_coeff(const uint64_t a[], size_t n) {
    return (uint8_t)((a[n / 64] >> (n % 64)) & 1ULL);
}

static void set_coeff(uint64_t a[], size_t n, uint8_t v) {
    uint64_t mask = 1ULL << (n % 64);

    if (v) {
        a[n / 64] |= mask;
    } else {
        a[n / 64] &= ~mask;
    }
}

template <size_t N>
static void reference_reduce_mod_x509(const std::array<uint64_t, 2 * N>& c, std::array<uint64_t, 8>& h) {
    h.fill(0);

    for (size_t i = 0; i < 128 * N; i++) {
        uint8_t bit = get_coeff(c.data(), i);

        if (bit) {
            size_t j = i % 509;
            uint8_t old = get_coeff(h.data(), j);
            set_coeff(h.data(), j, old ^ 1);
        }
    }
}

template <size_t N, size_t cases>
static void test_multiplication_64Nx64N_to_128N(
    const binary_polynomial_multiplication_test_case_t<N> (&test_cases)[cases],
    void (*multiplication_func)(poly64_t*, poly64_t*, poly128_t*)
) {
    for (const auto& tc : test_cases) {
        poly64_t a[N];
        poly64_t b[N];

        for (size_t i = 0; i < N; i++) {
            a[i] = vget_lane_p64(vdup_n_p64(tc.a[i]), 0);
            b[i] = vget_lane_p64(vdup_n_p64(tc.b[i]), 0);
        }

        poly128_t c[N];
        multiplication_func(a, b, c);

        std::array<uint64_t, 2 * N> got;
        poly128N_t_to_uint64_t_array<N>(got, c);

        uint8_t a_ref[64 * N];
        uint8_t b_ref[64 * N];
        uint8_t c_ref[128 * N - 1];

        uint64N_to_uint8_array(tc.a, a_ref);
        uint64N_to_uint8_array(tc.b, b_ref);
        ref_poly_mul(a_ref, b_ref, c_ref, 64 * N);

        std::array<uint64_t, 2 * N> expected;
        uint8_array_to_uint64_2N<N>(c_ref, expected);

        REQUIRE(got == expected);
        REQUIRE(expected == tc.r);
    }
}

template <size_t cases>
static void test_multiplication_mod_x509(
    const binary_polynomial_multiplication_test_case_t<8> (&test_cases)[cases]
) {
    const uint64_t mask61 = (UINT64_C(1) << 61) - 1;

    for (const auto& tc : test_cases) {
        std::array<uint64_t, 8> a_mask = tc.a;
        std::array<uint64_t, 8> b_mask = tc.b;

        a_mask[7] &= mask61;
        b_mask[7] &= mask61;

        poly64_t a[8];
        poly64_t b[8];

        for (size_t i = 0; i < 8; i++) {
            a[i] = vget_lane_p64(vdup_n_p64(a_mask[i]), 0);
            b[i] = vget_lane_p64(vdup_n_p64(b_mask[i]), 0);
        }

        std::array<uint64_t, 8> got;
        binary_polynomial_mul_mod509(a, b, got.data());

        uint8_t a_ref[64 * 8];
        uint8_t b_ref[64 * 8];
        uint8_t c_ref[128 * 8 - 1];

        uint64N_to_uint8_array(a_mask, a_ref);
        uint64N_to_uint8_array(b_mask, b_ref);
        ref_poly_mul(a_ref, b_ref, c_ref, 64 * 8);

        std::array<uint64_t, 16> product_ref;
        uint8_array_to_uint64_2N<8>(c_ref, product_ref);

        std::array<uint64_t, 8> expected;
        reference_reduce_mod_x509<8>(product_ref, expected);

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

const binary_polynomial_multiplication_test_case_t<8> test_cases_512[] = {
    // 0 * 0 = 0
    {{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},

    // 1 * 1 = 1
    {{0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},

    // x^64 * 1 = x^64
    {{0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},

    // x^448 * 1 = x^448
    {{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1},
     {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
     {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}},

    // random
    {{UINT64_C(0x123456789abcdef0), UINT64_C(0x0fedcba987654321),
      UINT64_C(0x55aa55aa55aa55aa), UINT64_C(0xaa55aa55aa55aa55),
      UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210),
      UINT64_C(0x0f0f0f0f0f0f0f0f), UINT64_C(0xf0f0f0f0f0f0f0f0)},

     {UINT64_C(0xfedcba9876543210), UINT64_C(0x0123456789abcdef),
      UINT64_C(0xaa55aa55aa55aa55), UINT64_C(0x55aa55aa55aa55aa),
      UINT64_C(0x0fedcba987654321), UINT64_C(0x123456789abcdef0),
      UINT64_C(0xf0f0f0f0f0f0f0f0), UINT64_C(0x0f0f0f0f0f0f0f0f)},

     {UINT64_C(0x0a0789828c810f00), UINT64_C(0x0b4688c28dc00e44),
      UINT64_C(0x399a508895c9fcdf), UINT64_C(0x5d9e88b389b55c98),
      UINT64_C(0x44bffb7d6e6ad1a8), UINT64_C(0x20ab7347320621aa),
      UINT64_C(0x2b5c8748c60e2a5a), UINT64_C(0x1a1d5a27cf378f0d),
      UINT64_C(0x3a1390c111c7bb15), UINT64_C(0x0b421daf58ee4e07),
      UINT64_C(0x399ec5e540e7bc9c), UINT64_C(0x6d8a91f014f3e88d),
      UINT64_C(0x5bf2a40aa40d5bf5), UINT64_C(0x5bf2a40aa40d5bf5),
      UINT64_C(0x5005500550055005), UINT64_C(0x0550055005500550)}},

    // all ones
    {{UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},

     {UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff),
      UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},

     {UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
      UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555),
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

TEST_CASE("binary polynomial multiplication 512x512->1024 works") {
    test_multiplication_64Nx64N_to_128N(test_cases_512, binary_polynomial_multiplication_512x512_to_1024);
}

TEST_CASE("binary polynomial multiplication mod x^509 - 1 works") {
    test_multiplication_mod_x509(test_cases_512);
}