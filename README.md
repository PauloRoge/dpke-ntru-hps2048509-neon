# dpke-ntru-hps2048509-neon

AArch64/NEON implementation of the binary-polynomial arithmetic used by
NTRU HPS 2048-509: multiplication in `GF(2)[x]` via `PMULL`, Karatsuba,
reduction mod `x^509 - 1`, and inversion in `R2 = GF(2)[x]/(x^509 - 1)`
(Itoh–Tsujii with Frobenius permutations).

The code requires the ARMv8 crypto extensions — `CMakeLists.txt` builds
everything with `-march=armv8-a+crypto`.

## Building

### On an AArch64 host

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Cross-compiling from x86-64, running under qemu

`cmake/aarch64-linux-gnu.toolchain.cmake` sets up the cross compiler and wires
`CMAKE_CROSSCOMPILING_EMULATOR` to `qemu-aarch64`, so the test suite runs on the
build host with no extra steps.

Prerequisites (Debian/Ubuntu):

```sh
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user
```

> On Ubuntu the cross-gcc packages conflict with the `gcc-multilib`
> *metapackage*. Letting apt remove it is safe: its only real content is the
> `/usr/include/asm -> x86_64-linux-gnu/asm` symlink, and the packages that
> actually provide 32-bit support (`gcc-15-multilib`, `lib32gcc-15-dev`,
> `libc6-dev-i386`) are kept, so `gcc -m32` keeps working. Recreate the symlink
> afterwards if you need it:
> `sudo ln -s x86_64-linux-gnu/asm /usr/include/asm`

Then:

```sh
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`ctest` transparently invokes each AArch64 binary through
`qemu-aarch64 -L /usr/aarch64-linux-gnu`. The same applies to doctest's
build-time test discovery, so the individual test cases show up in `ctest -N`
exactly as they do in a native build.

To run a target by hand:

```sh
qemu-aarch64 -L /usr/aarch64-linux-gnu ./build-aarch64/test_binary_inversion
```

#### Toolchain file options

| Variable | Default | Purpose |
| --- | --- | --- |
| `AARCH64_TRIPLE` | `aarch64-linux-gnu` | Cross toolchain prefix |
| `AARCH64_SYSROOT` | `/usr/${AARCH64_TRIPLE}` | qemu interpreter prefix (`-L`) |

For example, to use a toolchain from the Arm GNU Toolchain releases:

```sh
cmake -B build-aarch64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.toolchain.cmake \
      -DAARCH64_TRIPLE=aarch64-none-linux-gnu \
      -DAARCH64_SYSROOT=/opt/arm-gnu-toolchain/aarch64-none-linux-gnu/libc
```

## Benchmarks

The `*_bench` targets are built alongside the tests. They are not registered
with CTest, so run them directly:

```sh
qemu-aarch64 -L /usr/aarch64-linux-gnu ./build-aarch64/poly_bench
```

**Benchmark numbers from qemu are not meaningful.** qemu-user cannot expose the
ARM PMU, so libcpucycles falls back from `arm64-pmc` to `default-monotonic` and
reports emulator wall-clock time scaled by the host clock rather than target
cycle counts. Use the cross build for correctness testing and code inspection;
take timings on real AArch64 hardware.

## Layout

| Path | Contents |
| --- | --- |
| `src/` | Library: `binary_poly.c`, `poly_mod.c`, `poly_inverse.c` |
| `test/` | doctest correctness tests and libcpucycles benchmarks |
| `cmake/` | Toolchain file, doctest and libcpucycles integration |
| `scripts/` | Generators for the precomputed Frobenius permutation tables |
