#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <array>

#include "doctest.h"

extern "C" {
#include "binary_inversion.h"
}

using uint64_t_array_2 = std::array<uint64_t, 2>;

static void poly128_t_to_uint64_t_array(uint64_t_array_2 &out, poly128_t poly) {
    vst1q_u64(out.data(), vreinterpretq_u64_p128(poly));
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
    {0xc9f7d8ab7f2d1a29, 0xa3dace72b96811d5, 0x7dc6194b9f13f7dd, 0x7cc29844002f16cb},  // random
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
