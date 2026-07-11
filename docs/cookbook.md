# loxperm - Cookbook

## Pump permissive

Use non-latching conditions for start permissives.

```c
if (start_button_pressed && loxperm_is_permitted(&pump_start, now_ms)) {
    start_pump();
}
```

## Run interlock

Use latching conditions for faults that require operator reset.

```c
if (running && !loxperm_is_permitted(&run_interlocks, now_ms)) {
    stop_pump();
}
```

## Latching fault reset

Clear an individual latched condition after operator acknowledgement.

```c
(void)loxperm_reset_condition(&run_interlocks, COND_FAULT, now_ms);
```

## Qualifier or debounce

Use `qualifier_ms` to require a condition to stay true for a minimum time.

```c
{ .tag = "level_ok", .qualifier_ms = 3000 }
```

## Bypass metadata

Enable or disable a bypassable condition and record the operator ID.

```c
(void)loxperm_set_bypass(&chain, COND_LEVEL_OK, true, now_ms, 42);
uint16_t op_id = loxperm_bypass_operator_id(&chain, COND_LEVEL_OK);
```

## First-out diagnostics

Use `loxperm_first_out()` after a denial transition.

```c
if (loxperm_just_denied(&chain)) {
    int first = loxperm_first_out(&chain);
    const char *tag = loxperm_tag(&chain, (size_t)first);
}
```

## Portable snapshot save/load

Use a fixed wire buffer and expected config/schema IDs.

```c
uint8_t wire[LOXPERM_SNAPSHOT_WIRE_SIZE];
loxperm_snapshot_t snap;
uint64_t config_id = LOXPERM_CONFIG_SIGNATURE;

(void)loxperm_snapshot_save(&chain, &snap);
(void)loxperm_snapshot_encode(&snap, config_id, 1u, wire, sizeof(wire));
(void)loxperm_snapshot_load_wire(&chain, defs, count, config_id, 1u, wire, sizeof(wire), now_ms);
```

## Default and wide masks

- Default build: `LOXPERM_MAX_CONDITIONS` is 32.
- Wide build: define `LOXPERM_WIDE_MASK` and the mask grows to 64.

## Installed C consumer

```cmake
find_package(loxperm CONFIG REQUIRED)
target_link_libraries(app PRIVATE loxperm::loxperm)
```

## Installed C++ consumer

```cpp
#include <loxperm/loxperm.h>
```

Same-chain concurrency still requires external synchronization.
