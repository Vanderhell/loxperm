# loxperm - Quickstart

`loxperm` is a header-only C99 library. Include `loxperm/loxperm.h`, define your condition table, initialize a caller-owned chain, then feed raw booleans and timestamps into it.

## Minimal example

```c
#include <loxperm/loxperm.h>

enum { COND_VALVE_OPEN, COND_LEVEL_OK, COND_ARMED };

static const loxperm_condition_def_t pump_defs[] = {
    [COND_VALVE_OPEN] = { .tag = "valve_open" },
    [COND_LEVEL_OK]   = { .tag = "level_ok", .qualifier_ms = 3000 },
    [COND_ARMED]      = { .tag = "armed", .latching = true },
};

static loxperm_chain_t pump;

void pump_init(void) {
    (void)loxperm_chain_init(&pump, pump_defs, 3);
}

void pump_update(bool valve_open, bool level_ok, bool armed, uint32_t now_ms) {
    (void)loxperm_set(&pump, COND_VALVE_OPEN, valve_open, now_ms);
    (void)loxperm_set(&pump, COND_LEVEL_OK, level_ok, now_ms);
    (void)loxperm_set(&pump, COND_ARMED, armed, now_ms);
}

bool pump_can_start(uint32_t now_ms) {
    return loxperm_is_permitted(&pump, now_ms);
}
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install and consume

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix dist
```

```cmake
find_package(loxperm CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE loxperm::loxperm)
```

```c
#include <loxperm/loxperm.h>
```

## Portable snapshots

Use the fixed-size wire helpers when you need to persist state outside RAM:

```c
uint8_t wire[LOXPERM_SNAPSHOT_WIRE_SIZE];
loxperm_snapshot_t snap;
uint64_t config_id = LOXPERM_CONFIG_SIGNATURE;

(void)loxperm_snapshot_save(&pump, &snap);
(void)loxperm_snapshot_encode(&snap, config_id, 1u, wire, sizeof(wire));
```

Restore order matters:

1. Initialize or load the chain.
2. Restore raw inputs.
3. Run a baseline evaluation.
4. Then expose permission to the control path.
