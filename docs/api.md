# loxperm - API overview

## Core types

- `loxperm_condition_def_t` - static per-condition definition.
- `loxperm_condition_state_t` - caller-visible runtime state.
- `loxperm_chain_t` - caller-owned chain object.
- `loxperm_snapshot_t` - in-memory snapshot for persistence.

## Core lifecycle

- `loxperm_chain_init()` - initialize a chain over a definition array.
- `loxperm_reset_chain()` - clear latched and bypass state.
- `loxperm_reset_condition()` - clear one latched condition.

## Input and evaluation

- `loxperm_set()` - update a raw boolean input.
- `loxperm_evaluate()` - explicitly evaluate the current chain state.
- `loxperm_is_permitted()` - evaluate and return the current permission result.

## Diagnostics

- `loxperm_deny_mask()` - current deny mask.
- `loxperm_first_out()` - first condition that entered denial in the current episode.
- `loxperm_just_denied()` - one-shot transition flag, consumed on read.
- `loxperm_just_permitted()` - one-shot transition flag, consumed on read.
- `loxperm_tag()` - definition tag lookup.
- `loxperm_get_raw()` - raw input state.
- `loxperm_get_qualified()` - qualified input state.

## Bypass

- `loxperm_set_bypass()` - enable or disable bypass for a bypassable condition.
- `loxperm_is_bypassed()` - query bypass state.
- `loxperm_bypass_mask()` - mask of bypassed conditions.
- `loxperm_bypass_operator_id()` - latest bypass operator ID recorded for that condition.

## Snapshot and wire format

- `loxperm_snapshot_save()` - copy current runtime state into `loxperm_snapshot_t`.
- `loxperm_snapshot_load()` - restore in-memory state from `loxperm_snapshot_t`.
- `loxperm_snapshot_wire_size()` - fixed wire size in bytes.
- `loxperm_snapshot_encode()` - encode a portable wire snapshot.
- `loxperm_snapshot_decode()` - decode a portable wire snapshot.
- `loxperm_snapshot_load_wire()` - transactional decode + load with expected config/schema IDs.

## Configuration

- `LOXPERM_MAX_CONDITIONS` - default 32, or 64 when `LOXPERM_WIDE_MASK` is defined.
- `LOXPERM_CONFIG_SIGNATURE` - stable configuration signature for TU and snapshot checks.
- `LOXPERM_SNAPSHOT_WIRE_SIZE` - fixed portable snapshot size.

## Contract notes

- `loxperm` requires 8-bit bytes.
- The library is header-only and heap-free.
- Same-chain use from multiple threads requires external synchronization.
