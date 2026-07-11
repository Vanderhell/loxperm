# loxperm - Test scenarios (v0.1 acceptance)

All tests use a virtual clock; `now_ms` is supplied explicitly.

## P01 - Empty chain is trivially permitted

Test: `loxperm_tests` (`test_P01_empty_chain`)

```
init(c, defs=NULL, count=0)
expect: is_permitted(c, 0) == true
        deny_mask == 0
        first_out == -1
```

## P02 - Single satisfied condition

Test: `loxperm_tests` (`test_P02_single_satisfied`)

```
defs: [ {tag:"x"} ]
set(c, 0, true, 0)
expect: is_permitted == true after qualifier (0 here)
        deny_mask == 0
```

## P03 - Single condition denies

Test: `loxperm_tests` (`test_P03_single_denies`)

```
defs: [ {tag:"x"} ]
set(c, 0, false, 0)
expect: is_permitted == false
        deny_mask bit 0 set
        first_out == 0
```

## P04 - Qualifier suppresses transient OK

Test: `loxperm_tests` (`test_P04_qualifier`)

```
defs: [ {tag:"x", qualifier_ms:1000} ]
set(c, 0, true, 0)
evaluate(c, 500)   --> still denied (not qualified yet)
evaluate(c, 1000)  --> permitted
expect: matches above
```

## P05 - Transient OK during qualifier resets

Test: `loxperm_tests` (`test_P05_qualifier_resets`)

```
defs: [ {tag:"x", qualifier_ms:1000} ]
set(c, 0, true,  0)
set(c, 0, false, 500)
set(c, 0, true,  600)
evaluate(c, 1400) --> still not qualified (only 800 ms held)
evaluate(c, 1600) --> qualified
```

## P06 - Latching trip stays denied until reset

Test: `loxperm_tests` (`test_P06_latching_trip`)

```
defs: [ {tag:"x", latching:true} ]
set(c, 0, true,  0)    --> permitted
set(c, 0, false, 100)  --> denied, latched
set(c, 0, true,  200)  --> still denied (latched)
reset_condition(c, 0, 300)
set(c, 0, true,  300)
expect: now permitted after qualifier
```

## P07 - First-out captures earliest transition

Test: `loxperm_tests` (`test_P07_first_out`)

```
defs: [ a:{}, b:{}, c:{} ]  // all instant qualifier
set all true at t=0
set a=false at t=100
evaluate(c, 100)
set b=false at t=110
evaluate(c, 110)
expect: first_out == index(a)
        deny_mask has bits a and b
```

## P08 - First-out resets when chain becomes permitted again

Test: `loxperm_tests` (`test_P08_first_out_resets`)

```
preconditions: chain denied with first_out == 1
set all conditions true at t=N
evaluate at t=N + qualifier
expect: is_permitted == true
        first_out == -1
later, condition 2 trips:
expect: first_out == 2 (not 1)
```

## P09 - Bypass on non-bypassable condition rejected

Test: `loxperm_tests` (`test_P09_bypass_rejected`)

```
defs: [ {tag:"x", bypassable:false} ]
set_bypass(c, 0, true, 0, 1)
expect: return LOXPERM_ERR_NOT_BYPASSABLE
```

## P10 - Bypass forces OK regardless of input

Test: `loxperm_tests` (`test_P10_bypass_forces_ok`)

```
defs: [ {tag:"x", bypassable:true} ]
set(c, 0, false, 0)         --> denied
set_bypass(c, 0, true, 100, 1)
evaluate(c, 100)            --> permitted (bypassed)
set(c, 0, false, 200)
evaluate(c, 200)            --> permitted (still bypassed)
```

## P11 - Just-denied / just-permitted are one-shot

Test: `loxperm_tests` (`test_P11_one_shot_flags`)

```
sequence that transitions permitted -> denied -> permitted -> denied
each transition: just_* flag persists across no-transition re-evaluation
until explicitly consumed, then clears on read
```

## P12 - Bypass operator metadata tracks the last toggle

Test: `loxperm_tests` (`test_P12_bypass_operator_metadata`)

```
enable bypass with operator 7
disable bypass with operator 9
expect: latest operator ID is 9
reset clears metadata to 0
```

## P13 - Snapshot round-trip preserves latched_bad and bypass

Test: `loxperm_tests` (`test_P12_snapshot_roundtrip`)

```
build chain with mixed latched-bad and bypass states
snap = snapshot_save
load into fresh chain
expect: latched_bad_mask, bypass_mask, first_out match
```

## P14 - Reset chain clears latched and bypass

Test: `loxperm_tests` (`test_P13_reset_chain_clears_latch_and_bypass`)

```
preconditions: some conditions latched-bad, some bypassed
reset_chain(c, now)
expect: latched_bad_mask == 0
        bypass_mask == 0
        denial_count preserved
```

## P15 - Index out of range rejected

Test: `loxperm_tests` (`test_P14_index_out_of_range`)

```
set(c, c.condition_count, true, 0)
expect: LOXPERM_ERR_INDEX
```

## P16 - Wide mask compile path

Test: `loxperm_tests_wide_mask` (`test_wide_mask_basics`)

```
build with -DLOXPERM_WIDE_MASK
verify sizeof(loxperm_mask_t) == 8
verify chain with 64 conditions works
```
