# Evidence matrix

This file maps repository claims to evidence in source and CI.

## Library properties

- Claim: Header-only
  - Evidence: Implementation is entirely in `include/loxperm/loxperm.h`; CMake target `loxperm` is `INTERFACE`.
- Claim: No heap allocation
  - Evidence: Source inspection: no `malloc/calloc/realloc/free` in `include/`, `tests/`, or `examples/`.
- Claim: Deterministic (no hidden global mutable runtime state)
  - Evidence: Source inspection: caller-owned `loxperm_chain_t` holds state; no global mutable state in `include/loxperm/loxperm.h`.

## Snapshot safety

- Claim: Snapshot load rejects invalid inputs (version/count/masks)
  - Evidence: Unit tests in `tests/test_loxperm.c`:
    - `test_snapshot_invalid_inputs_rejected`
    - `test_snapshot_valid_accept_and_runtime_consistent`
- Claim: Snapshot masks respect condition definitions
  - Evidence: `loxperm_snapshot_load()` validation in `include/loxperm/loxperm.h`.
- Claim: Portable wire snapshots are fixed-size and reject corruption/truncation/trailing bytes
  - Evidence: `test_P16_portable_snapshot_wire_roundtrip` in `tests/test_loxperm.c`.

## Configuration safety

- Claim: Invalid mask configurations fail at compile time
  - Evidence: CTest checks `loxperm_config_c_0`, `loxperm_config_c_33`, and `loxperm_config_c_65`.
- Claim: Boundary configs 1/32/64 compile successfully
  - Evidence: CTest checks `loxperm_config_c_1`, `loxperm_config_c_32`, and `loxperm_config_c_64`.
- Claim: Header is C++ compatible
  - Evidence: CTest checks `loxperm_cpp11_include`, `loxperm_cpp17_include`, and `loxperm_cpp20_include`.

## First-out determinism

- Claim: Equal timestamps choose lowest index
  - Evidence: Unit test `test_first_out_tie_break_lowest_index` in `tests/test_loxperm.c`.
- Claim: Denial-episode ordering uses monotonic transition sequencing
  - Evidence: `loxperm_evaluate()` implementation and `test_P07_first_out` / `test_P13_snapshot_roundtrip`.

## Wide mask support

- Claim: Wide mask supports 64 conditions
  - Evidence: `loxperm_tests_wide_mask` (`tests/test_loxperm_wide_mask.c`) verifies 64-condition init and bit 63 behavior.

## Package consumers

- Claim: Installed package works from an external C consumer
  - Evidence: Manual install smoke against `dist-check` during this audit.
- Claim: Installed package works from an external C++ consumer
  - Evidence: Manual install smoke against `dist-check-wide` during this audit.
