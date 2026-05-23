# Changelog

## [Unreleased]

## [0.1.0]

### Added
- Header-only implementation: `include/loxperm/loxperm.h`.
- Docs: permissive vs interlock, integration, limitations, scenarios (`docs/`).
- Examples: `examples/pump_start.c`, `examples/interlock_first_out.c`.
- Minimal unit tests: `tests/test_loxperm.c`.
- CMake build for examples + tests.

### Changed
- Hardened `loxperm_snapshot_load()` validation and recomputed post-load runtime state for consistent getters.
- Defined `denial_count` overflow behavior: saturates at `UINT32_MAX`.
- Expanded unit tests for invalid snapshot inputs, API edge cases, wrap-around timing, deterministic first-out tie-break, and reset behavior.
- Added wide-mask unit test target (`loxperm_tests_wide_mask`) and CTest entry.
- Improved CMake install + package export (`find_package(loxperm CONFIG REQUIRED)`), added options for tests/examples/werror/sanitizers, and added `loxperm::loxperm` alias.
- Strengthened GitHub Actions CI matrix (OS, GCC/Clang, sanitizers, wide mask, install + consumer smoke, example smoke, header self-contained checks).
- Updated GitHub Actions release workflow to require matching `CHANGELOG.md` section and attach source and install-tree archives.
- Added repository hygiene files: `SECURITY.md`, `.editorconfig`, `.gitattributes`.
- Added evidence documentation: `docs/test-plan.md`, `docs/release-checklist.md`, `docs/evidence-matrix.md`.
