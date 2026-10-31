# Heks

This is a fork of [zbjornson/fast-hex](https://github.com/zbjornson/fast-hex/) by Zach Bjornson
with some CMake work (following closely [friendlyanon/cmake-init](https://github.com/friendlyanon/cmake-init) style),
renaming and refactoring, testing (using [doctest](https://github.com/doctest/doctest)),
fuzzing (LLVM) and benchmarking ([google/benchmark](https://github.com/google/benchmark)).
It adds AVX-512 and NEON encoders, AVX-512 decoders, and encoders and decoders for 8, 16 and 32-byte integers.

## What is it?

Hexadecimal encoding and decoding for x86-64 (scalar, AVX2, AVX-512) and ARM (scalar, NEON).

By **encoding** we understand taking input binary data (e.g. `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`)
and producing an output hex string (from our previous example: `48656C6C6F` or `48656c6c6f`).

By **decoding** we understand taking an input hex string (e.g. `48656C6C6F` or `48656c6c6f`)
and producing a corresponding binary data (in our example: `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`).

## How to use it?

This is a header-only C++20 library: include the headers for the instruction sets you want and link the
`fast_hex::headers` CMake target, or copy the headers from [`include/fast_hex`](include/fast_hex) into a project.
Everything is in namespace `heks`.

| Header | Functions for | Runs on |
|--------|---------------|---------|
| [`fast_hex.hpp`](include/fast_hex/fast_hex.hpp) | any CPU | any CPU |
| [`fast_hex_x86_64_v3.hpp`](include/fast_hex/fast_hex_x86_64_v3.hpp) | AVX2 | Haswell, Zen 1 and later |
| [`fast_hex_x86_64_v4.hpp`](include/fast_hex/fast_hex_x86_64_v4.hpp) | AVX-512 F, BW, CD, DQ and VL | Skylake-X, Cascade Lake, Ice Lake, Zen 4 and later |
| [`fast_hex_x86_64_icelake.hpp`](include/fast_hex/fast_hex_x86_64_icelake.hpp) | AVX-512 F, BW, VL and VBMI | Ice Lake, Zen 4 and later, but not Skylake-X or Cascade Lake |
| [`fast_hex_arm_neon.hpp`](include/fast_hex/fast_hex_arm_neon.hpp) | NEON | AArch64, ARMv7 with NEON |

Each ISA header includes the ones it builds on. On other platforms it is empty, so it can be included
unconditionally. VBMI is not part of x86-64-v4, so a `-march=x86-64-v4` build is enough for the v4 functions but
not for the icelake ones.

```cpp
#include <fast_hex/fast_hex.hpp>

uint8_t hex[10];
heks::encodeHex(hex, data, heks::RawLength{5}, heks::upper);   // "48656C6C6F"
heks::decodeHex(data, hex, heks::RawLength{5});

uint8_t id[32];
heks::encodeHexInt(id, uint128_value, heks::lower);            // 32 hex chars, most significant first
auto value = heks::decodeHexInt<unsigned __int128>(id);        // and back
```

### Functions

| Function | Header | Available when |
|----------|--------|----------------|
| `encodeHex(dest, src, len, case)` | `fast_hex.hpp` | always |
| `decodeHex(dest, src, len)` | `fast_hex.hpp` | always |
| `decodeHexLUT(dest, src, len)`, `decodeHexLUT4(dest, src, len)` | `fast_hex.hpp` | always |
| `encodeHexInt(dest, value, case)`, `decodeHexInt<T>(src)` | `fast_hex.hpp` | always |
| `encodeHexInt2x8(dest, first, case)` | `fast_hex.hpp` | always |
| `encodeHexAvx2(dest, src, len, case)`, `decodeHexAvx2(dest, src, len)` | `fast_hex_x86_64_v3.hpp` | `FAST_HEX_X86_64` |
| `encodeHexIntAvx2(dest, value, case)`, `decodeHexIntAvx2<T>(src)` | `fast_hex_x86_64_v3.hpp` | `FAST_HEX_X86_64` |
| `encodeHexInt2x8Avx2(dest, first, case)` | `fast_hex_x86_64_v3.hpp` | `FAST_HEX_X86_64` |
| `cpuHasAvx2()` | `fast_hex_x86_64_v3.hpp` | `FAST_HEX_X86_64` |
| `encodeHexAvx512(dest, src, len, case)`, `decodeHexAvx512(dest, src, len)` | `fast_hex_x86_64_v4.hpp` | `FAST_HEX_X86_64` |
| `cpuHasAvx512()` | `fast_hex_x86_64_v4.hpp` | `FAST_HEX_X86_64` |
| `encodeHexAvx512Vbmi(dest, src, len, case)`, `decodeHexAvx512Vbmi(dest, src, len)` | `fast_hex_x86_64_icelake.hpp` | `FAST_HEX_X86_64` |
| `cpuHasAvx512Vbmi()` | `fast_hex_x86_64_icelake.hpp` | `FAST_HEX_X86_64` |
| `encodeHexNeon(dest, src, len, case)` | `fast_hex_arm_neon.hpp` | `FAST_HEX_NEON` |
| `encodeHexIntNeon(dest, value, case)`, `encodeHexInt2x8Neon(dest, first, case)` | `fast_hex_arm_neon.hpp` | `FAST_HEX_NEON` |

`FAST_HEX_X86_64` is defined for GCC and Clang on x86-64. MSVC and 32-bit x86 only get the scalar functions.
`FAST_HEX_NEON` is defined when the compiler targets NEON (`__ARM_NEON`).

The three scalar decoders suit different lengths:

- `decodeHex` computes each nibble with arithmetic, so the compiler turns it into vector code: Clang from `-O2`,
  GCC only at `-O3`. It is the fastest scalar decoder from about 8 bytes up, or 16 when the length is not a
  multiple of 8, as it then decodes the last bytes one at a time. There is no NEON decoder, as `decodeHex` is
  vectorized on ARM too.
- `decodeHexLUT` and `decodeHexLUT4` look each char up in a table, so they are never vectorized. They are faster
  for shorter inputs, and with GCC at `-O2`. `decodeHexLUT4` also looks up the high nibble already shifted.

On x86-64 with Clang, decoding 1 to 3 bytes takes about 1 to 3 ns with `decodeHexLUT4` against 2 to 5 ns with
`decodeHex` or `decodeHexAvx2`. `decodeHexAvx2` is the fastest from about 7 bytes.

### Contract

- `RawLength` is the length of the binary data. Encoding reads `len` bytes and writes `len * 2` hex chars.
  Decoding reads `len * 2` hex chars and writes `len` bytes. No function reads or writes outside those ranges.
- `case` is `heks::lower` or `heks::upper`. Decoding accepts either case, mixed too.
- Decoding does not validate its input: chars that are not hex decode to unspecified bytes, but never cause
  undefined behavior.
- Pointers must be valid for those ranges, and source and destination must not overlap.
- `encodeHexInt*` take any integer type stored in native byte order that can be copied with `memcpy`, including
  `unsigned __int128` and 256-bit integer classes, and write `sizeof(T) * 2` hex chars, most significant first.
  `decodeHexInt*<T>` read `sizeof(T) * 2` hex chars, most significant first, and return the `T`. The SIMD versions
  take 8, 16 and 32-byte types.
- `encodeHexInt2x8*` take a pointer to two 8-byte integers and write 32 hex chars: `first[0]` then `first[1]`,
  each most significant first. This is not `encodeHexInt` of a 16-byte value, which on little-endian would write
  `first[1]` first.

### Picking a function

The library never checks the CPU: the caller picks the function. The AVX2 and AVX-512 functions carry their own
target attributes, so they are always compiled in and need no build flags, but calling one is only valid on a CPU
that has the instruction set. Either:

- call it from code that already requires the instruction set, e.g. a build with `-march=x86-64-v3`, or
- check once with `cpuHasAvx2()`, `cpuHasAvx512()` or `cpuHasAvx512Vbmi()` and pick:

```cpp
#include <fast_hex/fast_hex.hpp>
#include <fast_hex/fast_hex_arm_neon.hpp>
#include <fast_hex/fast_hex_x86_64_v3.hpp>

void toHex(uint8_t * dest, const uint8_t * src, size_t size)
{
    const heks::RawLength len{size};
#if defined(FAST_HEX_X86_64)
    static const bool has_avx2 = heks::cpuHasAvx2();
    if (has_avx2)
        heks::encodeHexAvx2(dest, src, len, heks::lower);
    else
        heks::encodeHex(dest, src, len, heks::lower);
#elif defined(FAST_HEX_NEON)
    heks::encodeHexNeon(dest, src, len, heks::lower);
#else
    heks::encodeHex(dest, src, len, heks::lower);
#endif
}
```

Called from code built without AVX2, `encodeHexAvx2` is not inlined. That is fine for long inputs, but for many
short ones (e.g. one call per row) check the CPU once and run the whole loop in a function built for AVX2, with
`__attribute__((target("avx2")))` or a framework for it such as ClickHouse's `TargetSpecific`. The AVX2 functions
can then inline into it, but whether they do is still the compiler's call by size: clang keeps the AVX-512 decoders
out of line, so that loop makes one call per row. Put `[[clang::always_inline]]` on the call to force it (GCC inlines
them as is). Don't build that one file with `-march=x86-64-v3` instead: the plain inline functions it uses would then
be compiled with AVX2 too, which is the problem the next paragraph describes.

There are no functions that pick for you. All the functions are inline, so a picker compiled into files built with
different flags would have one definition chosen by the linker, which could be the AVX2 one on a CPU without AVX2.
The caller knows its build flags and is the one place where the choice is safe.

## How to develop?

### Submodules

This project uses submodules for testing/benchmarking. Be sure to init them:

```sh
git submodule update --init
```

### `pre-commit` hooks

Please install `pre-commit` to automatically run checks before each commit. Usually can be achieved with one of these:

```sh
pip install pre-commit
# or
brew install pre-commit
# or
conda install -c conda-forge pre-commit

# and then
pre-commit install
```

### CMake

#### Developer mode

Build system targets that are only useful for developers of this project are
hidden if the `fast_hex_DEVELOPER_MODE` option is disabled. Enabling this
option makes tests and other developer targets and options available. Not
enabling this option means that you are a consumer of this project and thus you
have no need for these targets and options.

Developer mode is always set to on in CI workflows.

#### Presets

This project makes use of [presets][1] to simplify the process of configuring
the project. As a developer, you are recommended to always have the [latest
CMake version][2] installed to make use of the latest Quality-of-Life
additions.

You have a few options to pass `fast_hex_DEVELOPER_MODE` to the configure
command, but this project prefers to use presets.

As a developer, you should create a `CMakeUserPresets.json` file at the root of
the project:

```json
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 14,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev",
      "binaryDir": "${sourceDir}/build/dev",
      "inherits": ["dev-mode", "ci-<os>"],
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug"
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

You should replace `<os>` in your newly created presets file with the name of
the operating system you have, which may be `win64`, `linux` or `darwin`. You
can see what these correspond to in the
[`CMakePresets.json`](CMakePresets.json) file.

`CMakeUserPresets.json` is also the perfect place in which you can put all
sorts of things that you would otherwise want to pass to the configure command
in the terminal.

#### Configure, build and test

If you followed the above instructions, then you can configure, build and test
the project respectively with the following commands from the project root on
any operating system with any build system:

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

Typical variables you might want to tweak:

- `fast_hex_ENABLE_FUZZ` - whether to build fuzzing tests in `test/fuzz`.
- `fast_hex_ENABLE_SANITIZERS` - build the tests with AddressSanitizer and UndefinedBehaviorSanitizer (GCC or Clang).
  The `ci-sanitize` preset turns it on.
- `fast_hex_ENABLE_BENCHMARK` - whether to build benchmarks in `test/benchmark`.
- `fast_hex_ENABLE_MARCH_NATIVE` - build the tests, fuzzers and benchmarks for the CPU of the build machine.
  It used to be on by default, so turn it on when comparing benchmark results with older builds.

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
