# Heks

## Warning

This is a fork of [zbjornson/fast-hex](https://github.com/zbjornson/fast-hex/) by Zach Bjornson
with some CMake work (following closely [friendlyanon/cmake-init](https://github.com/friendlyanon/cmake-init) style),
renaming and refactoring, testing (using [doctest](https://github.com/doctest/doctest)),
fuzzing (LLVM) and benchmarking ([google/benchmark](https://github.com/google/benchmark))
Specialized functions for encoding 8 and 16 bytes have been added as well as NEON
encoder.

## What is it?

This library provides a set of hexadecimal encoding and decoding functionality
for various CPU architectures.

By **encoding** we understand taking input binary data (e.g. `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`)
and producing an output hex string (from our previous example: `48656C6C6F` or `48656c6c6f`).

By **decoding** we understand taking an input hex string (e.g. `48656C6C6F` or `48656c6c6f`)
and producing a corresponding binary data (in our example: `[0x48, 0x65, 0x6C, 0x6C, 0x6F]`).

### Constraints and requirements

- Input parameters cannot be `nullptr`.
- Input parameters should not alias - as we are using `__restrict__`.
- All functions take `RawLength` parameter. This is always the length of the binary data
  (hence it is output for encoding or input for decoding). As a consequence,
  when encoding we will read `2 * RawLength` bytes.
- There is no checking whether input hex string for decoding is valid. In case of invalid
  inputs for decoding the result will be undefined.

## How to use it?

This is a header-only library: include
[`fast_hex_inline.hpp`](https://github.com/jh0x/heks/blob/master/include/fast_hex/fast_hex_inline.hpp)
and link the `fast_hex::headers` CMake target. As there are no external dependencies, one could also copy
`fast_hex_inline.hpp` and `fast_hex_common.hpp` into a project. The compiler sees every function body, so it can
inline them. To get a compiled library instead, build a `.cpp` that includes the header into one.

### Functions

#### Decoding

| Function                    | Description                                                                                   |
|-----------------------------|-----------------------------------------------------------------------------------------------|
| `decodeHexLUT`              | Decodes hex string using a scalar look-up table. Requires a shift for decoding two chars at a time. |
| `decodeHexLUT4`             | Similar to `decodeHexLUT`, but uses two look-up tables to avoid shifts.                        |
| `decodeHexBMI`              | Scalar version that computes each nibble with bit operations instead of a lookup table. Plain C++, it does not need the BMI instruction set. |
| `decodeHexAuto`             | Picks the fastest of the decoders below that are compiled in and the CPU supports, checked at runtime. Always available. |
| `decodeHexVec`              | AVX2-optimized version for vectorized decoding. Can decode in parallel for better performance. |
| `decodeHexVec512`           | AVX-512 (BW/VL/VBMI) version. Lengths under 32 use overlapping blocks with plain loads/stores (no masking, partly AVX2 code) and longer inputs end with an overlapping full block, so there is no scalar tail. |

#### Encoding

| Function                    | Description                                                                                   |
|-----------------------------|-----------------------------------------------------------------------------------------------|
| `encodeHexLower` / `encodeHexUpper` | Encodes bytes into a hex string. Each byte is converted into two hex characters.  |
| `encodeHexLowerAuto` / `encodeHexUpperAuto` | Picks the fastest of the encoders below that are compiled in and the CPU supports, checked at runtime. Always available. |
| `encodeHexLowerVec` / `encodeHexUpperVec`         | AVX2-optimized version for encoding.    |
| `encodeHexLowerVec512` / `encodeHexUpperVec512`   | AVX-512 (BW/VL/VBMI) version for encoding. |
| `encodeHexNeonLower` / `encodeHexNeonUpper`         | NEON-optimized version for encoding.    |
| `encodeHex8LowerFast` / `encodeHex8UpperFast`| AVX-optimized version for inputs of length of exactly 8 bytes |
| `encodeHex16LowerFast` / `encodeHex16UpperFast`| AVX2-optimized version for inputs of length of exactly 16 bytes |
| `encodeHex8LowerScalar` / `encodeHex8UpperScalar`, `encodeHex16LowerScalar` / `encodeHex16UpperScalar` | Scalar versions of the 8 and 16 byte `Fast` functions. Run on any CPU. |
| `encodeHex8LowerNeon` / `encodeHex8UpperNeon`| NEON-optimized version for inputs of length of exactly 8 bytes |

On x86-64 with GCC or Clang all the AVX/AVX2/AVX-512 functions are always compiled in, each with its own
target attribute, so no `-mavx*` flags or options are needed and the rest of the library runs on any x86-64 CPU.
The `Auto` functions pick the path at runtime. Calling a SIMD function directly (including `encode_integral8`,
`encode_integral16`, `encode_integral2x8` and `decode_integral8`) on a CPU without its instruction set crashes.
`FAST_HEX_AVX`/`FAST_HEX_AVX2` only say the code is compiled in, not that the CPU can run it.
AVX-512 here means AVX-512 BW/VL/VBMI. MSVC builds and 32-bit x86 builds only get the scalar functions.

The fixed-size helpers also have `Scalar` / `_scalar` versions that run on any CPU, so callers can check the CPU
once and pick. `encodeHex8*Fast`, `encode_integral8` and `decode_integral8` need AVX; `encodeHex16*Fast`,
`encode_integral16` and `encode_integral2x8` need AVX2:

```cpp
#if defined(FAST_HEX_AVX2)
const bool has_avx2 = __builtin_cpu_supports("avx2"); // the 16-byte / 2x8 helpers need AVX2, not just AVX
#else
const bool has_avx2 = false;
#endif
if (has_avx2)
    encode_integral2x8(dest, pair, lower);
else
    encode_integral2x8_scalar(dest, pair, lower);
```

To leave a path out (e.g. for a toolchain without the AVX-512 VBMI intrinsics) define `FAST_HEX_NO_AVX`,
`FAST_HEX_NO_AVX2` or `FAST_HEX_NO_AVX512`. These replace the old `fast_hex_ENABLE_AVX`/`fast_hex_ENABLE_AVX2`
CMake options. Defining `FAST_HEX_AVX512=0` does not work, as only the `NO_` macros are checked.

`fast_hex_ENABLE_MARCH_NATIVE` (off by default) builds everything for the CPU of the build machine,
so the result may not run on other CPUs.

### Templates picking the best algorithm

`encode_auto` and `decode_auto` (what `encodeHex*Auto` / `decodeHexAuto` call) pick the "best" algorithm
for the target architecture. On x86-64 they pick, at runtime, the fastest path the CPU supports:

```cpp
// For X64 prefer: encodeHexVec512, encodeHexVec, encodeHex
// For ARM with NEON: encodeHexNeon
// Otherwise: encodeHex
heks::encode_auto(dst, src, heks::RawLength{len}, heks::upper);

// For X64 prefer: decodeHexVec512, decodeHexVec, decodeHexBMI, decodeHexLUT4
// For ARM prefer: decodeHexBMI, decodeHexLut
// Otherwise: decodeHexLUT4
heks::decode_auto(dst, src, heks::RawLength{len});
```

### Integral types

#### Decoding of integral types (accounting for endianness)

| Function | Description |
|----------|-------------|
|  `decode_integral_naive` | Naive implementation that decodes hex string to integral type T by processing 2 hex chars per byte. |
| `decode_integral8`       | AVX-optimized version that decodes 16 hex characters at once into an 8-byte integer using SIMD operations. |
| `decode_integral8_scalar` | Scalar version of `decode_integral8`. Runs on any CPU. |

#### Encoding of integral types (accounting for endianness)

| Function | Description |
|----------|-------------|
| `encode_integral_naive` | Generic, portable implementation for encoding any integral type (8–128 bitintegers) into hex. Handles machine endianness explicitly using byte-by-byte extraction. Uses a lookup table for upper/lower hex. |
| `encode_integral8` (AVX) | AVX-optimized encoder for 64-bit integers. Performs a 64-bit byte swap (to normalize endianness) and then routes the 8-byte block into `encodeHex8Fast` routine. |
| `encode_integral8` (NEON) | NEON-optimized encoder for 64-bit integers. Uses NEON fast 8-byte encoding routine (`encodeHexNeon8_impl`) and reverses bytes at runtime if needed depending on host endianness. |
| `encode_integral16` (AVX2) | AVX2-optimized encoder for 128-bit integral types. Converts a full 16-byte block using `encodeHex16Fast` routine with an appropriate shuffle mask. |
| `encode_integral2x8` (AVX2) | AVX2-optimized function for encoding two consecutive 64-bit integers (2×8 bytes = 16 bytes) in one pass. Treats the pair as a contiguous 16-byte block and uses `encodeHex16Fast` routine with an appropriate shuffle mask. |
| `encode_integral8_scalar` / `encode_integral16_scalar` / `encode_integral2x8_scalar` | Scalar versions of the three functions above. Run on any CPU. |

The x86 SIMD functions (`Vec`, `Fast` and the AVX integral helpers) carry a target attribute, so the compiler only
inlines them into code built for that instruction set: with `-mavx2` or `-march=x86-64-v3`, or inside a function
marked `__attribute__((target("avx2")))` (ClickHouse wraps its callers in `#pragma clang attribute push` for this).
Called from code built with the default flags, each one is a real function call.

The `Auto` functions check the CPU on every call. That costs little, but for many short inputs it is faster to check
once (e.g. with `__builtin_cpu_supports`) and run the whole loop in a function built for AVX2.

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
- `fast_hex_ENABLE_BENCHMARK` - whether to build benchmarks in `test/benchmark`.
- `fast_hex_ENABLE_MARCH_NATIVE` - build everything for the CPU of the build machine. It used to be on by default,
  so turn it on when comparing benchmark results with older builds.

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
