# loxperm — Test scenarios (v0.1 acceptance)

All tests use a virtual clock; `now_ms` is supplied explicitly.

## P01 — Empty chain is trivially permitted

```
init(c, defs=NULL, count=0)
expect: is_permitted(c, 0) == true
        deny_mask == 0
        first_out == -1
```

## P02 — Single satisfied condition

```
defs: [ {tag:"x"} ]
set(c, 0, true, 0)
expect: is_permitted == true after qualifier (0 here)
        deny_mask == 0
```

## P03 — Single condition denies

```
defs: [ {tag:"x"} ]
set(c, 0, false, 0)
expect: is_permitted == false
        deny_mask bit 0 set
        first_out == 0
```

## P04 — Qualifier suppresses transient OK

```
defs: [ {tag:"x", qualifier_ms:1000} ]
set(c, 0, true, 0)
evaluate(c, 500)   --> still denied (not qualified yet)
evaluate(c, 1000)  --> permitted
expect: matches above
```

## P05 — Transient OK during qualifier resets

```
defs: [ {tag:"x", qualifier_ms:1000} ]
set(c, 0, true,  0)
set(c, 0, false, 500)
set(c, 0, true,  600)
evaluate(c, 1400) --> still not qualified (only 800 ms held)
evaluate(c, 1600) --> qualified
```

## P06 — Latching trip stays denied until reset

```
defs: [ {tag:"x", latching:true} ]
set(c, 0, true,  0)    --> permitted
set(c, 0, false, 100)  --> denied, latched
set(c, 0, true,  200)  --> still denied (latched)
reset_condition(c, 0, 300)
set(c, 0, true,  300)
expect: now permitted after qualifier
```

## P07 — First-out captures earliest transition

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

## P08 — First-out resets when chain becomes permitted again

```
preconditions: chain denied with first_out == 1
set all conditions true at t=N
evaluate at t=N + qualifier
expect: is_permitted == true
        first_out == -1
later, condition 2 trips:
expect: first_out == 2 (not 1)
```

## P09 — Bypass on non-bypassable condition rejected

```
defs: [ {tag:"x", bypassable:false} ]
set_bypass(c, 0, true, 0, 1)
expect: return LOXPERM_ERR_NOT_BYPASSABLE
```

## P10 — Bypass forces OK regardless of input

```
defs: [ {tag:"x", bypassable:true} ]
set(c, 0, false, 0)         --> denied
set_bypass(c, 0, true, 100, 1)
evaluate(c, 100)            --> permitted (bypassed)
set(c, 0, false, 200)
evaluate(c, 200)            --> permitted (still bypassed)
```

## P11 — Just-denied / just-permitted are one-shot

```
sequence that transitions permitted -> denied -> permitted -> denied
each transition: just_* flag set once and cleared on read
```

## P12 — Snapshot round-trip preserves latched_bad and bypass

```
build chain with mixed latched-bad and bypass states
snap = snapshot_save
load into fresh chain
expect: latched_bad_mask, bypass_mask, first_out match
```

## P13 — Reset chain clears latched and bypass

```
preconditions: some conditions latched-bad, some bypassed
reset_chain(c, now)
expect: latched_bad_mask == 0
        bypass_mask == 0
        denial_count preserved
```

## P14 — Index out of range rejected

```
set(c, c.condition_count, true, 0)
expect: LOXPERM_ERR_INDEX
```

## P15 — Wide mask compile path

```
build with -DLOXPERM_WIDE_MASK
verify sizeof(loxperm_mask_t) == 8
verify chain with 64 conditions works
```

