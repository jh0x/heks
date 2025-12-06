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

CMake setup allows for two different types of usage:

- static/shared library (i.e. [`fast_hex.hpp`](https://github.com/jh0x/heks/blob/master/include/fast_hex/fast_hex.hpp))
  (`fast_hex::fast_hex` target and `fast_hex_BUILD_LIBS` CMake variable)
- header only library (i.e. [`fast_hex_inline.hpp`](https://github.com/jh0x/heks/blob/master/include/fast_hex/fast_hex_inline.hpp))
  (`fast_hex::headers` target),

One could also grab the desired files and use them directly as there are no external dependencies.

### Static/shared library

When `fast_hex_BUILD_LIBS` CMake flag is enabled static/shared libraries will be build (according
to `fast_hex_BUILD_SHARED_LIBS`). The following functions are provided:

#### Decoding

| Function                    | Description                                                                                   |
|-----------------------------|-----------------------------------------------------------------------------------------------|
| `decodeHexLUT`              | Decodes hex string using a scalar look-up table. Requires a shift for decoding two chars at a time. |
| `decodeHexLUT4`             | Similar to `decodeHexLUT`, but uses two look-up tables to avoid shifts.                        |
| `decodeHexBMI`              | Uses bit manipulation instructions to decode the hex string by directly applying bit operations. |
| `decodeHexVec`              | AVX2-optimized version for vectorized decoding. Can decode in parallel for better performance. |

#### Encoding

| Function                    | Description                                                                                   |
|-----------------------------|-----------------------------------------------------------------------------------------------|
| `encodeHexLower` / `encodeHexUpper` | Encodes bytes into a hex string. Each byte is converted into two hex characters.  |
| `encodeHexLowerVec` / `encodeHexUpperVec`         | AVX2-optimized version for encoding.    |
| `encodeHexNeonLower` / `encodeHexNeonUpper`         | NEON-optimized version for encoding.    |
| `encodeHex8LowerFast` / `encodeHex8UpperFast`| AVX-optimized version for inputs of length of exactly 8 bytes |
| `encodeHex16LowerFast` / `encodeHex16UpperFast`| AVX2-optimized version for inputs of length of exactly 16 bytes |
| `encodeHex8LowerNeon` / `encodeHex8UpperNeon`| NEON-optimized version for inputs of length of exactly 8 bytes |

#### Encoding of integral types (accounting for endianness)

| Function | Description |
|----------|-------------|
| `encode_integral_naive` | Generic, portable implementation for encoding any integral type (8–128 bitintegers) into hex. Handles machine endianness explicitly using byte-by-byte extraction. Uses a lookup table for upper/lower hex. |
| `encode_integral8` (AVX) | AVX-optimized encoder for 64-bit integers. Performs a 64-bit byte swap (to normalize endianness) and then routes the 8-byte block into `encodeHex8Fast` routine. |
| `encode_integral8` (NEON) | NEON-optimized encoder for 64-bit integers. Uses NEON fast 8-byte encoding routine (`encodeHexNeon8_impl`) and reverses bytes at runtime if needed depending on host endianness. |
| `encode_integral16` (AVX2) | AVX2-optimized encoder for 128-bit integral types. Converts a full 16-byte block using `encodeHex16Fast` routine with an appropriate shuffle mask. |
| `encode_integral2x8` (AVX2) | AVX2-optimized function for encoding two consecutive 64-bit integers (2×8 bytes = 16 bytes) in one pass. Treats the pair as a contiguous 16-byte block and uses `encodeHex16Fast` routine with an appropriate shuffle mask. |

You might want to pass specific flags for the target architecture (e.g. `-mavx2`). There is a convenience CMake option available
for forcing the native architecture: `fast_hex_ENABLE_MARCH_NATIVE`.

### Header only library

It might be preferred if one wishes to give compiler more potential for optimization - as it will have
the full visibility into the function declarations.

In addition to the same set of functions as in case described above, the header only library provides
two convenience functions that attempt to make it easier to select the "best" algorithm given the target architecture:

```cpp
// For X64 prefer: encodeHexVec, encodeHex
// For ARM with NEON: encodeHexNeon
// Otherwise: encodeHex
heks::encode_auto(dst, src, heks::RawLength{len}, heks::upper);

// For X64 prefer: decodeHexVec, decodeHexBMI, decodeHexLUT4
// For ARM prefer: decodeHexBMI, decodeHexLut
// Otherwise: decodeHexLUT4
heks::decode_auto(dst, src, heks::RawLength{len});
```

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

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
