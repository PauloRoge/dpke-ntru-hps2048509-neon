cmake_minimum_required(VERSION 3.12)
project(BinaryInversion C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-march=armv8-a+crypto)

include(CTest)
enable_testing()
include(cmake/doctest.cmake)

add_library(binary_inversion STATIC src/binary_inversion.c)
target_include_directories(binary_inversion PUBLIC src)

add_executable(test_binary_inversion test/test_binary_inversion.cpp)
target_include_directories(test_binary_inversion PRIVATE doctest test)
target_link_libraries(test_binary_inversion PRIVATE binary_inversion)

doctest_discover_tests(test_binary_inversion)
