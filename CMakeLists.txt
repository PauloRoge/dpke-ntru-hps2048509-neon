cmake_minimum_required(VERSION 3.12)
project(BinaryInversion C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-march=armv8-a+crypto)

include(CTest)
enable_testing()
include(cmake/doctest.cmake)
include(cmake/libcpucycles.cmake)

add_library(binary_poly STATIC
    src/binary_poly.c
    src/poly_mod.c
)
target_include_directories(binary_poly PUBLIC src)

add_executable(test_binary_inversion test/test_binary_inversion.cpp)
target_include_directories(test_binary_inversion PRIVATE doctest test)
target_link_libraries(test_binary_inversion PRIVATE binary_poly)

doctest_discover_tests(test_binary_inversion)

add_executable(mul_karatsuba_bench test/test_mul_karatsuba_bench.c)
target_link_libraries(mul_karatsuba_bench PRIVATE binary_poly libcpucycles)

add_executable(mul_karatsuba_bench2 test/test_mul_karatsuba_bench2.c)
target_link_libraries(mul_karatsuba_bench2 PRIVATE binary_poly libcpucycles)

add_executable(poly_mod_bench test/test_poly_mod_bench.c)
target_link_libraries(poly_mod_bench PRIVATE binary_poly libcpucycles)

add_executable(poly_bench test/test_poly_bench.c)
target_link_libraries(poly_bench PRIVATE binary_poly libcpucycles)
