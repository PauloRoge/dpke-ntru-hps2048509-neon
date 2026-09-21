# ---------------------------------------------------------------------------
# Cross-compile this project for AArch64 (arm64) Linux and run the resulting
# binaries on the build host under qemu-user.
#
#   cmake -B build-aarch64 -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.toolchain.cmake \
#         -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-aarch64
#   ctest --test-dir build-aarch64
#
# Debian/Ubuntu prerequisites:
#   sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user
#
# Overridable from the command line:
#   -DAARCH64_TRIPLE=aarch64-none-linux-gnu    (toolchain prefix)
#   -DAARCH64_SYSROOT=/path/to/sysroot         (qemu interpreter prefix)
# ---------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT DEFINED AARCH64_TRIPLE)
    set(AARCH64_TRIPLE aarch64-linux-gnu)
endif()

if(NOT DEFINED AARCH64_SYSROOT)
    set(AARCH64_SYSROOT /usr/${AARCH64_TRIPLE})
endif()

# find_* for headers/libraries/packages must look in the target sysroot, never
# in the host's /usr. Programs are the exception: python3, make and friends are
# build-host tools, so they are resolved against the host PATH as usual.
set(CMAKE_FIND_ROOT_PATH ${AARCH64_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ----- compilers -----------------------------------------------------------
#
# Deliberately no CMAKE_SYSROOT: the Debian cross gcc already knows where its
# own headers, libc and crt objects live, and forcing --sysroot here hides the
# gcc-internal include directory, which lives outside ${AARCH64_SYSROOT}.

find_program(AARCH64_C_COMPILER   ${AARCH64_TRIPLE}-gcc)
find_program(AARCH64_CXX_COMPILER ${AARCH64_TRIPLE}-g++)

if(NOT AARCH64_C_COMPILER OR NOT AARCH64_CXX_COMPILER)
    message(FATAL_ERROR
        "AArch64 cross compiler not found (looked for ${AARCH64_TRIPLE}-gcc and "
        "${AARCH64_TRIPLE}-g++ on PATH).\n"
        "On Debian/Ubuntu: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu")
endif()

set(CMAKE_C_COMPILER   ${AARCH64_C_COMPILER})
set(CMAKE_CXX_COMPILER ${AARCH64_CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${AARCH64_C_COMPILER})

# ----- running target binaries under qemu-user -----------------------------
#
# CMAKE_CROSSCOMPILING_EMULATOR drives everything that executes a target
# binary: ctest's add_test(), doctest's build-time test discovery (see
# cmake/doctest.cmake, which forwards the CROSSCOMPILING_EMULATOR target
# property), and any add_custom_command(COMMAND <target>).
#
# -L is what lets qemu find ld-linux-aarch64.so.1 and the target libc.

find_program(QEMU_AARCH64_EXECUTABLE NAMES qemu-aarch64 qemu-aarch64-static)

if(NOT QEMU_AARCH64_EXECUTABLE)
    message(FATAL_ERROR
        "qemu-aarch64 not found on PATH.\n"
        "On Debian/Ubuntu: sudo apt install qemu-user")
endif()

set(CMAKE_CROSSCOMPILING_EMULATOR ${QEMU_AARCH64_EXECUTABLE} -L ${AARCH64_SYSROOT})

# try_compile() configures a throwaway project that re-reads this file; carry
# the resolved values across so it does not repeat the searches.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    AARCH64_TRIPLE
    AARCH64_SYSROOT
    AARCH64_C_COMPILER
    AARCH64_CXX_COMPILER
    QEMU_AARCH64_EXECUTABLE
)
