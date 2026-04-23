# Inspired by https://github.com/SQISign/the-sqisign/blob/main/.cmake/gmpconfig.cmake

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

cmake_host_system_information(RESULT N QUERY NUMBER_OF_PHYSICAL_CORES)

if(N EQUAL 0)
    # Choose a "safe" amount
    set(N 8)
endif()

set(libcpucycles_PARALLEL_BUILD_ARGS -j${N})

if(APPLE AND BUILD_X86_64)
    set(libcpucycles_HOST "--host=x86_64")
    set(libcpucycles_PATCH_COMMAND patch -p0 < ${CMAKE_CURRENT_SOURCE_DIR}/libcpucycles-x86-64.patch)
endif()

include(ExternalProject)
find_program(MAKE_EXE NAMES make gmake nmake)
set(libcpucycles_INSTALL_DIR "${CMAKE_BINARY_DIR}/libcpucycles")

ExternalProject_Add(libcpucycles_external
    PREFIX ${libcpucycles_INSTALL_DIR}
    URL https://cpucycles.cr.yp.to/libcpucycles-20260105.tar.gz
    URL_HASH SHA256=e87dcaaa28e905b574ccf3d49e23e05c73edb3f99136dcd566bca16829ab6694
    CONFIGURE_COMMAND ${libcpucycles_INSTALL_DIR}/src/libcpucycles_external/configure --prefix=${libcpucycles_INSTALL_DIR} ${libcpucycles_HOST}
    BUILD_COMMAND ${MAKE_EXE} ${libcpucycles_PARALLEL_BUILD_ARGS}
    BUILD_IN_SOURCE ON
    PATCH_COMMAND ${libcpucycles_PATCH_COMMAND}
    INSTALL_COMMAND ${MAKE_EXE} install
    BUILD_BYPRODUCTS ${libcpucycles_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cpucycles${CMAKE_STATIC_LIBRARY_SUFFIX}
)

file(MAKE_DIRECTORY ${libcpucycles_INSTALL_DIR}/include)

add_library(libcpucycles STATIC IMPORTED)
set_target_properties(libcpucycles PROPERTIES
    IMPORTED_LOCATION ${libcpucycles_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cpucycles${CMAKE_STATIC_LIBRARY_SUFFIX}
    INTERFACE_INCLUDE_DIRECTORIES ${libcpucycles_INSTALL_DIR}/include
)

add_dependencies(libcpucycles libcpucycles_external)
