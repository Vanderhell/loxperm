# loxperm - Limitations and non-goals

## Not a safety library

`loxperm` is not safety-rated. No SIL claim. No IEC 61508 conformance.
No ISO 26262 ASIL conformance.

It is suitable for the basic process control layer (BPCS), where its
failure is not by itself a hazard. Safety-instrumented functions belong
in a certified SIS.

## Not a code generator

There is no IDE, no graphical editor, no Logic Matrix workflow. You
write the condition definitions in C. If you want generation from a
schema, build that on top of `loxperm`.

## Conditions are booleans

`loxperm` does not evaluate analogue thresholds, ratios, or rate-of-change.
Compute the boolean upstream and feed it in.

## No quorum / voting

`loxperm` is "all conditions must be true". It does not implement
2-out-of-3 or other voting topologies. For voted inputs, use a separate
voter module and feed the consolidated boolean into `loxperm`.

## No dynamic chain

The chain size is fixed at init. You cannot add or remove conditions at
runtime. If you need that, build multiple chains.

## Per-condition flags are fixed at init

`latching`, `bypassable`, and `qualifier_ms` are set in the
`loxperm_condition_def_t` array and cannot be changed without re-init.
This is deliberate: changing them at runtime breaks the audit trail.

## First-out is per-chain, not per-process

The first-out index is local to the chain. If you have multiple chains
and want to identify the absolute first event across them, time-stamp
the denial transitions in your log and compare.

## Bypass policy is the application's job

`loxperm` lets you bypass a condition and records who did it. It does
not:

- enforce bypass timeouts (max duration before forced reset)
- require a second-person authorisation
- prevent bypass during plant alarms
- track bypass uptime against a maintenance window

The latest bypass operator ID is readable through `loxperm_bypass_operator_id()`.
The portable snapshot wire format stores bypass state, but not every runtime diagnostic field.

Those are policy decisions. Implement them in the layer above.

## Snapshot integrity is the caller's job

`loxperm_snapshot_t` is an in-memory transfer object. It is not a stable on-disk format.

Use the fixed wire helpers (`loxperm_snapshot_encode()`, `loxperm_snapshot_decode()`, and
`loxperm_snapshot_load_wire()`) when you need portable bytes.

The wire format does not include a CRC or other integrity checks. If you persist snapshots to
flash/EEPROM or any backend that can tear writes, you must provide integrity and atomicity at that layer
(for example: CRC, double-buffering with sequence numbers, or transactional storage).

Snapshot load validates:

- snapshot version and condition count
- masks are within the chain size
- bypass/latched bits are consistent with condition definitions
- portable wire snapshots also validate magic, byte order, fixed size, reserved bytes, schema ID,
  and caller-supplied config ID before loading

It does not protect against corrupted or torn snapshots beyond these checks.

## Snapshot stores bypass state

If you persist the snapshot and restore on boot, bypassed conditions remain bypassed. If your operating policy
is "all bypasses clear on power cycle", call `loxperm_reset_chain()` after restore.

## denial_count saturation

`denial_count` is a lifetime counter. When it reaches `UINT32_MAX`, it saturates and no longer increments.
This avoids changing the permit/deny verdict due to counter overflow.

## Re-entrancy

Two threads may safely operate on two different chains. Two threads on
the same chain require external synchronisation. The library has no
mutex.

## Max conditions / wide mask

`LOXPERM_MAX_CONDITIONS` defaults to 32 (fits in a `uint32_t` mask).
Define `LOXPERM_WIDE_MASK` for 64 (uses `uint64_t`).

Unit tests include a dedicated wide-mask test (`loxperm_tests_wide_mask`) that initializes 64 conditions and
verifies bit 63 behavior.

## Clock requirements

Caller provides monotonic `uint32_t` ms. Wrap is handled (49.7 days).
Single-delay maximum is half the wrap window (~24.85 days). Clock jumps
cause undefined behaviour for that update.

## Byte requirements

The library requires 8-bit bytes. That is asserted at compile time because
the portable wire format is defined in raw bytes.
