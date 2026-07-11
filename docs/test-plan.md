# Test plan

This repository uses small C executables (no external unit test framework) and CTest.

## Unit tests (default mask)

- CMake target: `loxperm_tests`
- CTest name: `loxperm_tests`
- Source: `tests/test_loxperm.c`

## Wide-mask tests (64-bit mask)

- CMake target: `loxperm_tests_wide_mask`
- CTest name: `loxperm_tests_wide_mask`
- Source: `tests/test_loxperm_wide_mask.c`
- Built with: `LOXPERM_WIDE_MASK=1`

## Compile-time configuration checks

- CTest names:
  - `loxperm_config_c_1`
  - `loxperm_config_c_32`
  - `loxperm_config_c_64`
  - `loxperm_config_c_0`
  - `loxperm_config_c_33`
  - `loxperm_config_c_65`
- Source coverage:
  - default mask boundary success/failure
  - wide-mask boundary success/failure

## C++ inclusion checks

- CTest names:
  - `loxperm_cpp11_include`
  - `loxperm_cpp17_include`
  - `loxperm_cpp20_include`
- These compile a small C++ translation unit that includes `loxperm/loxperm.h`.

## Sanitizers (ASan/UBSan)

- Enabled by CMake option: `LOXPERM_ENABLE_SANITIZERS=ON`
- Intended for GCC/Clang on Linux in CI.

## Header self-contained checks

CI compiles a translation unit that includes only:

- `#include <loxperm/loxperm.h>`

With:

- `-std=c99 -Wall -Wextra -Wpedantic -Werror`
- Both GCC and Clang

## Install + consumer smoke tests

CI performs:

- `cmake --install ... --prefix dist`
- External consumer build that uses:
  - `find_package(loxperm CONFIG REQUIRED)`
  - `target_link_libraries(... loxperm::loxperm)`
  - `#include <loxperm/loxperm.h>`
  - C99, C11, and C++17 consumers

## Example smoke

CI builds examples (default CMake options) and runs them where the host platform permits.
