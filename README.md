# loxperm

Permissive / interlock evaluator with explainable deny mask for embedded C firmware.

`loxperm` is a small, heap-free **C99** single-header library that evaluates a set of
named conditions and decides whether an action is permitted to start (permissive)
or must continue to be allowed (interlock).

When the action is denied, `loxperm` tells you **which specific conditions caused
the denial**, including first-out detection.

```c
#include "loxperm/loxperm.h"

enum { COND_VALVE_OPEN, COND_LEVEL_OK, COND_TEMP_OK, COND_ARMED };

static const loxperm_condition_def_t pump_start_defs[] = {
    [COND_VALVE_OPEN] = { .tag = "valve_open",  .latching = false },
    [COND_LEVEL_OK]   = { .tag = "level_ok",    .latching = false,
                          .qualifier_ms = 3000 /* must hold 3 s */ },
    [COND_TEMP_OK]    = { .tag = "temp_ok",     .latching = false },
    [COND_ARMED]      = { .tag = "armed",       .latching = true },
};

static loxperm_chain_t pump_start;
loxperm_chain_init(&pump_start, pump_start_defs, 4);

loxperm_set(&pump_start, COND_VALVE_OPEN, valve_is_open(),    now_ms);
loxperm_set(&pump_start, COND_LEVEL_OK,   level >= MIN_LEVEL, now_ms);
loxperm_set(&pump_start, COND_TEMP_OK,    temp < MAX_TEMP,    now_ms);
loxperm_set(&pump_start, COND_ARMED,      operator_armed,     now_ms);

if (loxperm_is_permitted(&pump_start, now_ms)) {
    start_pump();
} else {
    loxperm_mask_t deny = loxperm_deny_mask(&pump_start);
    int first_out = loxperm_first_out(&pump_start);
}
```

## What `loxperm` is

- a set of named boolean conditions, each with its own qualifier time
- an aggregate **permitted / denied** verdict
- a **deny mask** showing which conditions caused the denial
- a **first-out** index identifying which condition tripped first
- optional **latching** (once tripped, condition stays denied until reset)
- optional **bypass** for maintenance

## What `loxperm` is not

- Not a safety library. No SIL claim, no IEC 61508 conformance.
- Not a configuration tool. You define conditions in C.
- Not an HMI. You render masks/tags yourself.

## Permissive vs interlock — which is which

- A **permissive** check is evaluated only at the *start* of an action.
  Once the action is running, the permissive does not stop it.
  → Configure conditions with `latching=false` and call `loxperm_is_permitted()`
  only at the start.

- An **interlock** check is evaluated continuously while the action is running.
  If any condition trips, the action must stop.
  → Configure conditions with `latching=true` (and/or keep evaluating every tick)
  and call `loxperm_is_permitted()` continuously.

More details: `docs/permissive-vs-interlock.md`.

## Integration with the Lox family

- `microhealth` — provides condition booleans.
- `loxalarm` — alarm states can be conditions in a chain.
- `microlog` — denial events with reason mask.
- `microsh` — `perm list`, `perm why-not`, `perm bypass-on/off`.
- `microconf` — chain configuration (qualifier times, latching flags).
- `loxseq` — sequencer can gate each step on a chain.

See: `docs/integration.md`.

## Layout

- `include/loxperm/loxperm.h` — public header + implementation
- `examples/` — example programs
- `tests/` — minimal unit tests (no external framework)
- `docs/` — guides and scenarios

## Build & test (CMake)

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

On Visual Studio generators (Windows), add a configuration:

```sh
cmake --build build --config Release
ctest --test-dir build -C Release
```

## License

MIT. See `LICENSE`.
