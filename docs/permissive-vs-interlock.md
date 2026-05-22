# loxperm — Permissive vs interlock

In process control, "permissive" and "interlock" are two different
patterns. `loxperm` supports both with one chain type.

## Definitions

A **permissive** is a condition that must be true *before* an action
starts. Once the action is running, the permissive does not stop it.

> Example: the suction valve must be open before the pump may start.
> Once running, if someone closes the suction valve, that is a separate
> problem; the permissive itself does not stop the pump.

An **interlock** is a condition that must be true *both* before the
action starts *and* while it runs. If the condition fails at any point,
the action stops.

> Example: the high-temperature trip on a motor. If it trips before
> start, the motor cannot start. If it trips during run, the motor
> must stop.

The boundary is sometimes blurry in practice. Most published references
(Rockwell, Siemens, Honeywell, Yokogawa) distinguish them this way:
permissives gate the *start*, interlocks gate the *continued operation*.

## Mapping to `loxperm`

`loxperm` is the same data structure for both. The difference is in how
you call it.

### Permissive pattern

You evaluate the chain only when the start command is received.

```c
if (start_button_pressed) {
    if (loxperm_is_permitted(&pump_start_chain, now_ms)) {
        start_pump();
        running = true;
    } else {
        log_denied_start(&pump_start_chain);
    }
}
```

The conditions in `pump_start_chain` should generally have
`latching=false`. They reset naturally as inputs change.

### Interlock pattern

You evaluate the chain every loop iteration while the action is active.

```c
if (running) {
    if (!loxperm_is_permitted(&pump_interlock_chain, now_ms)) {
        stop_pump();
        running = false;
        log_trip(&pump_interlock_chain);
    }
}
```

The conditions in `pump_interlock_chain` should generally have
`latching=true`. Once a trip happens, the chain stays denied until
explicit reset; otherwise a transient could cause the action to restart
on its own.

### Combined pattern

Most real start/run sequences want both:

```c
if (start_button_pressed && !running) {
    if (loxperm_is_permitted(&start_perm, now_ms)) {
        start_pump();
        running = true;
    }
}
if (running) {
    if (!loxperm_is_permitted(&run_interlocks, now_ms)) {
        stop_pump();
        running = false;
    }
}
```

Two chains, two purposes. They may share underlying conditions (the
same boolean fed into both), but the latching policy differs.

## Qualifier times

Both patterns benefit from `qualifier_ms`: a condition must hold true
for this duration before counting as satisfied. This prevents:

- a permissive starting on a transient noise spike that briefly reads OK
- an interlock failing to detect a real fault because the input glitched
  back to OK after one sample

Typical values: 200–500 ms for fast process signals, 1–5 s for slower
mechanical positions.

## First-out detection

When an interlock chain trips with multiple conditions failing
near-simultaneously, the operator needs to know **which one tripped
first**. That is the root cause; the rest are usually consequences.

`loxperm_first_out()` returns the index of the first condition that
transitioned from satisfied to denied during the current denial episode.
It is recomputed each time the chain re-enters the denied state and
held until the chain becomes permitted again.

```c
if (loxperm_just_denied(&run_interlocks)) {
    int idx = loxperm_first_out(&run_interlocks);
    const char *tag = loxperm_tag(&run_interlocks, (size_t)idx);
    log("INTERLOCK TRIP first-out: %s", tag ? tag : "?");
}
```

## Bypass with audit

For maintenance, a bypassable condition can be forced to "OK":

```c
loxperm_set_bypass(&run_interlocks, COND_LEVEL_OK, true,
                   now_ms, OPERATOR_ID);
```

Notes:

- Only definitions marked `bypassable=true` can be bypassed; others
  return `LOXPERM_ERR_NOT_BYPASSABLE`.
- The bypass mask is exposed via `loxperm_bypass_mask()` — render it on
  the HMI as a permanent reminder.
- Snapshot/restore preserves the bypass mask. After power loss, a
  bypass is **still in effect**, unless your application policy clears
  it on reboot.

## Why not just `if (a && b && c && d)`?

You can. For three conditions, you should. `loxperm` becomes worth its
weight when:

- the number of conditions grows past 4–5
- you need to know *which* condition failed at the time of failure
- you need to distinguish "first-out" from "subsequent"
- you need qualifier times per condition
- you need latching trips that survive a transient return to OK
- you need maintenance bypass with audit
- you need the same chain to drive HMI display, logging, and the
  control decision from one source of truth

If you have a single 3-condition permissive and no diagnostics
requirement, plain `&&` is fine and you do not need this library.

