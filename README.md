# Heks

This is a fork of [zbjornson/fast-hex](https://github.com/zbjornson/fast-hex/) by Zach Bjornson
with some CMake work (following closely [friendlyanon/cmake-init](https://github.com/friendlyanon/cmake-init) style),
renaming and refactoring, testing (using [doctest](https://github.com/doctest/doctest)),
fuzzing (LLVM) and benchmarking ([google/benchmark](https://github.com/google/benchmark)).
It adds AVX-512, NEON and SVE code, and encoders and decoders for 8, 16 and 32-byte integers.

## What is it?

Hexadecimal encoding and decoding for x86-64 (scalar, AVX2, AVX-512) and ARM (scalar, NEON, SVE).

By **encoding** we understand taking input binary data (e.g. `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`)
and producing an output hex string (from our previous example: `48656C6C6F` or `48656c6c6f`).

By **decoding** we understand taking an input hex string (e.g. `48656C6C6F` or `48656c6c6f`)
and producing a corresponding binary data (in our example: `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`).

## How to use it?

This is a header-only C++20 library. Include the headers you need and link the `fast_hex::headers` CMake target,
or copy [`include/fast_hex`](include/fast_hex) into your project. Everything is in namespace `heks`.

```cpp
#include <fast_hex/fast_hex.hpp>

uint8_t hex[10];
heks::encodeHex(hex, data, heks::RawLength{5}, heks::upper);   // "48656C6C6F"
heks::decodeHex(data, hex, heks::RawLength{5});

uint8_t id[32];
heks::encodeHexInt(id, uint128_value, heks::lower);            // 32 hex chars, most significant first
auto value = heks::decodeHexInt<unsigned __int128>(id);        // and back
heks::encodeHexInt2x8(id, two_uint64s, heks::lower);           // two_uint64s[0], then two_uint64s[1]
```

The SIMD functions take the same arguments as the scalar ones.

| Header | Instruction set | Functions |
|--------|-----------------|-----------|
| [`fast_hex.hpp`](include/fast_hex/fast_hex.hpp) | any | `encodeHex`, `decodeHex`, `decodeHexLUT`, `decodeHexLUT4`, `encodeHexInt`, `decodeHexInt`, `encodeHexInt2x8` |
| [`fast_hex_x86_64_v3.hpp`](include/fast_hex/fast_hex_x86_64_v3.hpp) | AVX2 (Haswell, Zen 1 and later) | `encodeHexAvx2`, `decodeHexAvx2`, `encodeHexIntAvx2`, `decodeHexIntAvx2`, `encodeHexInt2x8Avx2`, `cpuHasAvx2` |
| [`fast_hex_x86_64_v4.hpp`](include/fast_hex/fast_hex_x86_64_v4.hpp) | AVX-512 F, BW, CD, DQ and VL (Skylake-X, Zen 4 and later) | `encodeHexAvx512`, `decodeHexAvx512`, `cpuHasAvx512` |
| [`fast_hex_x86_64_icelake.hpp`](include/fast_hex/fast_hex_x86_64_icelake.hpp) | AVX-512 F, BW, VL and VBMI (Ice Lake, Zen 4 and later) | `encodeHexAvx512Vbmi`, `decodeHexAvx512Vbmi`, `cpuHasAvx512Vbmi` |
| [`fast_hex_arm_neon.hpp`](include/fast_hex/fast_hex_arm_neon.hpp) | NEON (AArch64, ARMv7) | `encodeHexNeon`, `decodeHexNeon`, `encodeHexIntNeon`, `decodeHexIntNeon`, `encodeHexInt2x8Neon` |
| [`fast_hex_arm_sve.hpp`](include/fast_hex/fast_hex_arm_sve.hpp) | SVE (Graviton 3 and 4) | `encodeHexSve`, `decodeHexSve`, `cpuHasSve` |

Each SIMD header includes the ones it builds on, and is empty where the compiler can't target it, so you can include
all of them. Guard calls with the macro for that instruction set:

- `FAST_HEX_X86_64`: GCC or Clang on x86-64. MSVC and 32-bit x86 only get the scalar functions.
- `FAST_HEX_NEON`: the compiler targets NEON.
- `FAST_HEX_ARM_SVE`: GCC 14+ or Clang 18+ on AArch64, but not on Apple, whose CPUs have no SVE.
  `cpuHasSve()` only works on Linux and FreeBSD, and returns false elsewhere.

VBMI is not part of x86-64-v4, so `-march=x86-64-v4` is enough for the v4 functions but not for the icelake ones.

### Contract

- `RawLength` is the length of the binary data. Encoding reads `len` bytes and writes `len * 2` hex chars.
  Decoding does the reverse. No function reads or writes outside those ranges.
- Pointers must be valid for those ranges, and source and destination must not overlap.
- `case` is `heks::lower` or `heks::upper`. Decoding accepts either case, or a mix.
- Decoding does not validate its input. Chars that are not hex give unspecified bytes, but never undefined behavior.
- `encodeHexInt*` takes any integer type in native byte order that `memcpy` can copy, including
  `unsigned __int128` and 256-bit integer classes, and writes `sizeof(T) * 2` hex chars, most significant first.
  `decodeHexInt*<T>` reads them back. The SIMD versions take 8, 16 and 32-byte types.
- `encodeHexInt2x8*` writes two 8-byte integers as 32 hex chars, `first[0]` then `first[1]`. On little-endian
  this is not the same as `encodeHexInt` of one 16-byte value, which writes `first[1]` first.

### Which function is fastest

- `decodeHexLUT4` is the fastest decoder below about 8 bytes. It uses table lookups, which compilers don't
  vectorize. Use it with GCC at `-O2` too, as GCC only vectorizes `decodeHex` at `-O3`.
- `decodeHex` is the fastest scalar decoder above that, as the compiler vectorizes it.
- On x86-64, `decodeHexAvx2` is the fastest from about 7 bytes.
- On ARM, the NEON functions beat the scalar ones at most lengths. They use only base AArch64 NEON, so building
  for a newer CPU, or with `+sve`, gives the same code.
- The SVE functions run the NEON code up to 32 bytes, so they only help on longer inputs.

### Picking a function

The library never checks the CPU: the caller picks the function. The AVX2, AVX-512 and SVE functions carry their own
target attributes, so they always compile without build flags. Only call one where the CPU supports it:

- from code built for that instruction set, e.g. with `-march=x86-64-v3`, or
- after checking once with `cpuHasAvx2()`, `cpuHasAvx512()`, `cpuHasAvx512Vbmi()` or `cpuHasSve()`:

```cpp
#include <fast_hex/fast_hex.hpp>
#include <fast_hex/fast_hex_arm_neon.hpp>
#include <fast_hex/fast_hex_arm_sve.hpp>
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
#    if defined(FAST_HEX_ARM_SVE)
    static const bool has_sve = heks::cpuHasSve();
    if (has_sve)
        return heks::encodeHexSve(dest, src, len, heks::lower);
#    endif
    heks::encodeHexNeon(dest, src, len, heks::lower);
#else
    heks::encodeHex(dest, src, len, heks::lower);
#endif
}
```

The library has no function that picks for you. All functions are inline, so a picker compiled in files with
different build flags would end up as one copy chosen by the linker, which could be the AVX2 one on a CPU without
AVX2. Only the caller knows its build flags.

Called from code built without AVX2, `encodeHexAvx2` is not inlined. That is fine for long inputs. For many short
ones (e.g. one call per row), check the CPU once and run the whole loop in a function marked
`__attribute__((target("avx2")))`, or `target("+sve")` for SVE, so the calls can inline. Clang may still keep the
AVX-512 decoders out of line: put `[[clang::always_inline]]` on the call to force it. Don't build that file with
`-march=x86-64-v3` instead: the inline scalar functions it uses would then be built with AVX2 too, which is the
linker problem above.

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

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
