# loxperm - Integration with the Lox family

## Composition map

```
conditions from:
  - microhealth (boolean sensor states)
  - loxalarm    (alarm.is_active or alarm.needs_attention)
  - microconf   (operator mode flags)
        |
        v
     loxperm  -- deny_mask / first_out / bypass_mask --> HMI
        |
        v
  action gate (pump start, OTA begin, sequence step)
        |
        v
    microlog (audit trail)
```

## With microhealth

`microhealth` produces booleans (threshold crossings, range checks). Feed those into `loxperm`:

```c
loxperm_set(&chain, COND_LEVEL_OK,
            microhealth_get_bool(&hm, "level_above_min"),
            now_ms);
```

## With loxalarm

An alarm state can be a condition. Common patterns:

```c
/* "permit only if pressure alarm is NOT currently active" */
loxperm_set(&chain, COND_NO_PRESSURE_ALARM,
            !lox_alarm_is_active(&pressure_alarm),
            now_ms);

/* "permit only if no alarms need attention" */
loxperm_set(&chain, COND_ALARMS_QUIET,
            !lox_alarm_needs_attention(&pressure_alarm) &&
            !lox_alarm_needs_attention(&temp_alarm),
            now_ms);
```

## With microconf

Store qualifier times and latching flags in your config:

```c
typedef struct {
    uint32_t valve_qualifier_ms;
    uint32_t level_qualifier_ms;
    uint8_t  latching_bits;
    uint8_t  bypassable_bits;
} cfg_pump_chain_t;

static loxperm_condition_def_t pump_defs[4];

pump_defs[0] = (loxperm_condition_def_t){
    .tag           = "valve_open",
    .qualifier_ms  = cfg.pump.valve_qualifier_ms,
    .latching      = (cfg.pump.latching_bits   & 0x01) != 0,
    .bypassable    = (cfg.pump.bypassable_bits & 0x01) != 0,
};
```

## With microsh

Operator commands for the shell:

```c
static int sh_perm_status(int argc, char **argv) {
    (void)argc; (void)argv;
    loxperm_mask_t deny = loxperm_deny_mask(&chain);
    int fo  = loxperm_first_out(&chain);
    for (size_t i = 0; i < chain.condition_count; ++i) {
        printf(" [%c%c%c] %s%s\n",
            loxperm_get_qualified(&chain, i) ? 'Q' : '.',
            (deny >> i) & 1                  ? 'D' : '.',
            loxperm_is_bypassed(&chain, i)   ? 'B' : '.',
            loxperm_tag(&chain, i),
            (int)i == fo ? "  <-- first out" : "");
    }
    return 0;
}

static int sh_perm_bypass(int argc, char **argv) {
    if (argc < 3) return -1;
    size_t idx = (size_t)atoi(argv[1]);
    bool   on  = (argv[2][0] == 'o' && argv[2][1] == 'n');
    return loxperm_set_bypass(&chain, idx, on, now_ms(), 1);
}

microsh_register(&sh, "perm",         sh_perm_status);
microsh_register(&sh, "perm-bypass",  sh_perm_bypass);
```

## With microlog

Log denial transitions:

```c
loxperm_evaluate(&chain, now_ms);

if (loxperm_just_denied(&chain)) {
    int fo = loxperm_first_out(&chain);
    microlog_emit_event(&log, "PERM_DENY",
                        loxperm_tag(&chain, (size_t)fo));
}
if (loxperm_just_permitted(&chain)) {
    microlog_emit_event(&log, "PERM_OK", NULL);
}
```

## With loxseq

A sequencer can gate each step on a `loxperm` chain:

```c
loxseq_step_t step = {
    .id               = STEP_OPEN_VALVE,
    .precondition     = (loxseq_precond_fn)loxperm_is_permitted,
    .precondition_ctx = &valve_chain,
    .action           = action_open_valve,
};
```

## With loxguard

Guard the evaluation if the chain depends on parsers or other risky inputs:

```c
LOX_GUARD_BLOCK(perm_eval_guard) {
    loxperm_set(&chain, 0, parse_input(buf), now_ms);
    loxperm_evaluate(&chain, now_ms);
}
```

