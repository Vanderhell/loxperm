# Contributing

## Quick start

- Build: `cmake -S . -B build && cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`

## Style

- C99.
- Keep `include/loxperm/loxperm.h` self-contained (no project-local includes).
- Prefer deterministic behavior and explicit timestamps (`uint32_t now_ms`).

## Changes

- Update `CHANGELOG.md` for user-visible changes.
- Add/extend tests in `tests/test_loxperm.c` for behavior changes.

