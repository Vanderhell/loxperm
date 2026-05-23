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

## Example smoke

CI builds examples (default CMake options) and runs them where the host platform permits.

