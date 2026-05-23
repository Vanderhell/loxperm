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

## First-out determinism

- Claim: Equal timestamps choose lowest index
  - Evidence: Unit test `test_first_out_tie_break_lowest_index` in `tests/test_loxperm.c`.

## Wide mask support

- Claim: Wide mask supports 64 conditions
  - Evidence: `loxperm_tests_wide_mask` (`tests/test_loxperm_wide_mask.c`) verifies 64-condition init and bit 63 behavior.

