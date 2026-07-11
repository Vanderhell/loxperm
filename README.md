# loxperm

[![ci](https://github.com/Vanderhell/loxperm/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/loxperm/actions/workflows/ci.yml)
[![release](https://github.com/Vanderhell/loxperm/actions/workflows/release.yml/badge.svg)](https://github.com/Vanderhell/loxperm/actions/workflows/release.yml)

`loxperm` - Liquid Oxygen Permissive / Interlock Evaluator.

`loxperm` is a small, heap-free C99 single-header library that evaluates a set of named conditions and decides whether an action is permitted to start
(permissive) or must continue to be allowed (interlock).

When the action is denied, `loxperm` tells you which specific conditions caused the denial, including first-out detection.

```c
#include <loxperm/loxperm.h>

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
    (void)deny;
    (void)first_out;
}
```

## Properties

- Header-only: all public API + implementation in `include/loxperm/loxperm.h` (caller includes the header).
- Heap-free: caller-owned state in `loxperm_chain_t`.
- Deterministic: no floating point, no hidden global mutable runtime state.
- Portable snapshots: fixed-byte wire helpers with explicit config/schema IDs.
- Compatibility: C99 primary, with C11/C17/C23 and C++ consumers verified by compile checks.

## Layout

- `include/loxperm/loxperm.h` - public header + implementation
- `examples/` - example programs
- `tests/` - unit tests (no external framework)
- `docs/` - guides, scenarios, limitations, evidence

## Build & test (CMake)

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

On single-config generators (Makefiles/Ninja), `--config Release` / `-C Release` are not needed.

## Install & consume (CMake package)

Install:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix dist
```

Consume:

```cmake
find_package(loxperm CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE loxperm::loxperm)
```

And include:

```c
#include <loxperm/loxperm.h>
```

## Docs

- Quickstart: `docs/quickstart.md`
- API: `docs/api.md`
- Cookbook: `docs/cookbook.md`
- Scenarios and test mapping: `docs/scenarios.md`
- Limitations: `docs/limitations.md`
- Test plan: `docs/test-plan.md`
- Evidence matrix: `docs/evidence-matrix.md`
- Release checklist: `docs/release-checklist.md`

## License

MIT. See `LICENSE`.
