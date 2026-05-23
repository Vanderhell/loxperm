#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "loxperm/loxperm.h"

static int tassert_int(int cond, const char *expr, const char *file, int line) {
    if (!cond) {
        fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
        return 1;
    }
    return 0;
}

#define TASSERT(expr) for (int _tassert_once = 0; _tassert_once == 0; _tassert_once = 1) { \
    int _tassert_rc = tassert_int((expr) ? 1 : 0, #expr, __FILE__, __LINE__); \
    if (_tassert_rc) return _tassert_rc; \
}

static int test_P01_empty_chain(void) {
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, NULL, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_deny_mask(&c) == 0);
    TASSERT(loxperm_first_out(&c) == -1);
    return 0;
}

static int test_P02_single_satisfied(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_deny_mask(&c) == 0);
    return 0;
}

static int test_P03_single_denies(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, false, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == false);
    TASSERT((loxperm_deny_mask(&c) & 1u) != 0);
    TASSERT(loxperm_first_out(&c) == 0);
    return 0;
}

static int test_P04_qualifier(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .qualifier_ms = 1000 } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 500) == false);
    TASSERT(loxperm_is_permitted(&c, 1000) == true);
    return 0;
}

static int test_P05_qualifier_resets(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .qualifier_ms = 1000 } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, false, 500) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 600) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 1400) == false);
    TASSERT(loxperm_is_permitted(&c, 1600) == true);
    return 0;
}

static int test_P06_latching_trip(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .latching = true } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_set(&c, 0, false, 100) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 100) == false);
    TASSERT(loxperm_set(&c, 0, true, 200) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 200) == false); /* latched */
    TASSERT(loxperm_reset_condition(&c, 0, 300) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 300) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 300) == true);
    return 0;
}

static int test_P07_first_out(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a" }, { .tag = "b" }, { .tag = "c" }
    };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 3) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 2, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    TASSERT(loxperm_set(&c, 0, false, 100) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 100) == false);
    TASSERT(loxperm_first_out(&c) == 0);

    TASSERT(loxperm_set(&c, 1, false, 110) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 110) == false);
    TASSERT(loxperm_first_out(&c) == 0);

    loxperm_mask_t deny = loxperm_deny_mask(&c);
    TASSERT((deny & (1u << 0)) != 0);
    TASSERT((deny & (1u << 1)) != 0);
    return 0;
}

static int test_P08_first_out_resets(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "a" }, { .tag = "b" }, { .tag = "c" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 3) == LOXPERM_OK);

    for (size_t i = 0; i < 3; ++i) TASSERT(loxperm_set(&c, i, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    TASSERT(loxperm_set(&c, 1, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);
    TASSERT(loxperm_first_out(&c) == 1);

    for (size_t i = 0; i < 3; ++i) TASSERT(loxperm_set(&c, i, true, 20) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 20) == true);
    TASSERT(loxperm_first_out(&c) == -1);

    TASSERT(loxperm_set(&c, 2, false, 30) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 30) == false);
    TASSERT(loxperm_first_out(&c) == 2);
    return 0;
}

static int test_P09_bypass_rejected(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .bypassable = false } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set_bypass(&c, 0, true, 0, 1) == LOXPERM_ERR_NOT_BYPASSABLE);
    return 0;
}

static int test_P10_bypass_forces_ok(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .bypassable = true } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, false, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == false);
    TASSERT(loxperm_set_bypass(&c, 0, true, 100, 1) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 100) == true);
    TASSERT(loxperm_set(&c, 0, false, 200) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 200) == true);
    return 0;
}

static int test_P11_one_shot_flags(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_just_denied(&c) == false);
    TASSERT(loxperm_just_permitted(&c) == false);

    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);
    TASSERT(loxperm_just_denied(&c) == true);
    TASSERT(loxperm_just_denied(&c) == false);

    TASSERT(loxperm_set(&c, 0, true, 20) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 20) == true);
    TASSERT(loxperm_just_permitted(&c) == true);
    TASSERT(loxperm_just_permitted(&c) == false);
    return 0;
}

static int test_P12_snapshot_roundtrip(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true },
        { .tag = "b", .bypassable = true },
        { .tag = "c" },
    };

    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 3) == LOXPERM_OK);

    /* Make it permitted once, then trip condition 0 to latch it. */
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 2, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);

    /* Bypass condition 1. */
    TASSERT(loxperm_set_bypass(&c, 1, true, 20, 7) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 20);

    loxperm_snapshot_t snap;
    TASSERT(loxperm_snapshot_save(&c, &snap) == LOXPERM_OK);

    loxperm_chain_t d;
    TASSERT(loxperm_snapshot_load(&d, defs, 3, &snap, 100) == LOXPERM_OK);

    TASSERT(d.denial_count == c.denial_count);
    TASSERT(loxperm_bypass_mask(&d) == loxperm_bypass_mask(&c));
    /* Condition 0 should still be latched-bad; bypass does not clear it. */
    TASSERT((snap.latched_bad_mask & 1u) != 0);
    return 0;
}

static int test_P13_reset_chain_clears_latch_and_bypass(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true },
        { .tag = "b", .bypassable = true },
    };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 2) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 10);
    TASSERT(loxperm_set_bypass(&c, 1, true, 20, 1) == LOXPERM_OK);

    uint32_t deny_count_before = c.denial_count;
    TASSERT(loxperm_reset_chain(&c, 30) == LOXPERM_OK);
    TASSERT(loxperm_bypass_mask(&c) == 0);
    TASSERT(c.denial_count == deny_count_before);
    return 0;
}

static int test_P14_index_out_of_range(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_ERR_INDEX);
    return 0;
}

static int test_chain_init_invalid_args(void) {
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(NULL, NULL, 0) == LOXPERM_ERR_INVALID_ARG);
    TASSERT(loxperm_chain_init(&c, NULL, 1) == LOXPERM_ERR_INVALID_ARG);

    loxperm_condition_def_t defs[LOXPERM_MAX_CONDITIONS + 1];
    memset(defs, 0, sizeof(defs));
    TASSERT(loxperm_chain_init(&c, defs, LOXPERM_MAX_CONDITIONS + 1) == LOXPERM_ERR_TOO_MANY);
    return 0;
}

static int test_api_null_chain_safe_values(void) {
    TASSERT(loxperm_tag(NULL, 0) == NULL);
    TASSERT(loxperm_get_qualified(NULL, 0) == false);
    TASSERT(loxperm_get_raw(NULL, 0) == false);
    TASSERT(loxperm_is_bypassed(NULL, 0) == false);
    TASSERT(loxperm_bypass_mask(NULL) == 0);
    TASSERT(loxperm_deny_mask(NULL) == 0);
    TASSERT(loxperm_first_out(NULL) == -1);
    TASSERT(loxperm_just_denied(NULL) == false);
    TASSERT(loxperm_just_permitted(NULL) == false);
    return 0;
}

static int test_api_uninitialised_chain_safe_values(void) {
    loxperm_chain_t c;
    memset(&c, 0, sizeof(c));
    TASSERT(loxperm_bypass_mask(&c) == 0);
    TASSERT(loxperm_deny_mask(&c) == 0);
    TASSERT(loxperm_first_out(&c) == -1);
    TASSERT(loxperm_tag(&c, 0) == NULL);
    TASSERT(loxperm_get_qualified(&c, 0) == false);
    TASSERT(loxperm_get_raw(&c, 0) == false);
    TASSERT(loxperm_is_bypassed(&c, 0) == false);
    TASSERT(loxperm_just_denied(&c) == false);
    TASSERT(loxperm_just_permitted(&c) == false);
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_ERR_STATE);
    TASSERT(loxperm_set_bypass(&c, 0, true, 0, 1) == LOXPERM_ERR_STATE);
    TASSERT(loxperm_reset_chain(&c, 0) == LOXPERM_ERR_STATE);
    TASSERT(loxperm_reset_condition(&c, 0, 0) == LOXPERM_ERR_STATE);
    return 0;
}

static int test_qualifier_wraparound(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .qualifier_ms = 10 } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);

    uint32_t t0 = 0xFFFFFFF0u; /* near wrap */
    TASSERT(loxperm_set(&c, 0, true, t0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, (uint32_t)(t0 + 5u)) == false);
    TASSERT(loxperm_is_permitted(&c, 5u) == true); /* wrapped */
    return 0;
}

static int test_first_out_tie_break_lowest_index(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "a" }, { .tag = "b" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 2) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    /* Deny both at same timestamp; choose lowest index deterministically. */
    TASSERT(loxperm_set(&c, 0, false, 100) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, false, 100) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 100) == false);
    TASSERT(loxperm_first_out(&c) == 0);
    return 0;
}

static int test_bypass_latched_bad_visibility(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x", .latching = true, .bypassable = true } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);

    /* Permit once, then trip to latch-bad. */
    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);

    /* Bypass forces permitted but does not clear latch. */
    TASSERT(loxperm_set_bypass(&c, 0, true, 20, 1) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 20) == true);

    loxperm_snapshot_t snap;
    TASSERT(loxperm_snapshot_save(&c, &snap) == LOXPERM_OK);
    TASSERT((snap.latched_bad_mask & 1u) != 0);

    /* Turning bypass off exposes latched-bad denial again. */
    TASSERT(loxperm_set_bypass(&c, 0, false, 30, 1) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 30) == false);
    TASSERT((loxperm_deny_mask(&c) & 1u) != 0);
    return 0;
}

static int test_reset_condition_only_clears_one(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true },
        { .tag = "b", .latching = true },
    };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 2) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, false, 11) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 11);

    TASSERT(loxperm_reset_condition(&c, 0, 20) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 0, true, 20) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 20) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 20) == false);
    TASSERT((loxperm_deny_mask(&c) & (1u << 1)) != 0);
    return 0;
}

static int test_reset_chain_clears_latch_and_bypass_preserves_denial_count(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true },
        { .tag = "b", .bypassable = true },
    };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 2) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_set(&c, 1, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    /* Latch-bad condition 0 and bypass condition 1. */
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 10);
    TASSERT(loxperm_set_bypass(&c, 1, true, 20, 1) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 20);

    uint32_t deny_count_before = c.denial_count;
    loxperm_snapshot_t snap_before;
    TASSERT(loxperm_snapshot_save(&c, &snap_before) == LOXPERM_OK);
    TASSERT((snap_before.latched_bad_mask & 1u) != 0);
    TASSERT((snap_before.bypass_mask & (1u << 1)) != 0);

    TASSERT(loxperm_reset_chain(&c, 30) == LOXPERM_OK);
    TASSERT(loxperm_bypass_mask(&c) == 0);
    TASSERT(c.denial_count == deny_count_before);

    loxperm_snapshot_t snap_after;
    TASSERT(loxperm_snapshot_save(&c, &snap_after) == LOXPERM_OK);
    TASSERT(snap_after.latched_bad_mask == 0);
    TASSERT(snap_after.bypass_mask == 0);
    return 0;
}

static int test_denial_count_saturates(void) {
    const loxperm_condition_def_t defs[] = { { .tag = "x" } };
    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 1) == LOXPERM_OK);

    TASSERT(loxperm_set(&c, 0, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    c.denial_count = UINT32_MAX;
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);
    TASSERT(c.denial_count == UINT32_MAX);
    return 0;
}

static int test_snapshot_invalid_inputs_rejected(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true,  .bypassable = false },
        { .tag = "b", .latching = false, .bypassable = true  },
    };

    loxperm_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));
    snap.version = 1;
    snap.condition_count = 2;
    snap.first_out_index = -1;

    loxperm_chain_t out;

    /* Version mismatch. */
    snap.version = 2;
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.version = 1;

    /* condition_count mismatch. */
    snap.condition_count = 1;
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.condition_count = 2;

    /* mask bits outside count. */
    snap.bypass_mask = (loxperm_mask_t)(1u << 2);
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.bypass_mask = 0;

    /* first_out_index outside range. */
    snap.first_out_index = 2;
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.first_out_index = -1;

    /* bypass bit for non-bypassable condition rejected. */
    snap.bypass_mask = (loxperm_mask_t)(1u << 0);
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.bypass_mask = 0;

    /* latched_bad bit for non-latching condition rejected. */
    snap.latched_bad_mask = (loxperm_mask_t)(1u << 1);
    TASSERT(loxperm_snapshot_load(&out, defs, 2, &snap, 0) == LOXPERM_ERR_INVALID_ARG);
    snap.latched_bad_mask = 0;

    return 0;
}

static int test_snapshot_valid_accept_and_runtime_consistent(void) {
    const loxperm_condition_def_t defs[] = {
        { .tag = "a", .latching = true },
        { .tag = "b", .bypassable = true },
        { .tag = "c" },
    };

    loxperm_chain_t c;
    TASSERT(loxperm_chain_init(&c, defs, 3) == LOXPERM_OK);

    for (size_t i = 0; i < 3; ++i) TASSERT(loxperm_set(&c, i, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    /* Latch-bad condition 0; bypass condition 1. */
    TASSERT(loxperm_set(&c, 0, false, 10) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 10) == false);
    TASSERT(loxperm_set_bypass(&c, 1, true, 20, 7) == LOXPERM_OK);
    (void)loxperm_is_permitted(&c, 20);

    loxperm_snapshot_t snap;
    TASSERT(loxperm_snapshot_save(&c, &snap) == LOXPERM_OK);

    loxperm_chain_t d;
    TASSERT(loxperm_snapshot_load(&d, defs, 3, &snap, 100) == LOXPERM_OK);

    TASSERT(d.denial_count == c.denial_count);
    TASSERT(loxperm_bypass_mask(&d) == (loxperm_mask_t)(1u << 1));
    TASSERT(loxperm_first_out(&d) == 0);
    TASSERT((loxperm_deny_mask(&d) & 1u) != 0);

    /* Current inputs are not snapshotted; after load, inputs are default-false. */
    TASSERT((loxperm_deny_mask(&d) & (1u << 2)) != 0);

    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_P01_empty_chain();
    rc |= test_P02_single_satisfied();
    rc |= test_P03_single_denies();
    rc |= test_P04_qualifier();
    rc |= test_P05_qualifier_resets();
    rc |= test_P06_latching_trip();
    rc |= test_P07_first_out();
    rc |= test_P08_first_out_resets();
    rc |= test_P09_bypass_rejected();
    rc |= test_P10_bypass_forces_ok();
    rc |= test_P11_one_shot_flags();
    rc |= test_P12_snapshot_roundtrip();
    rc |= test_P13_reset_chain_clears_latch_and_bypass();
    rc |= test_P14_index_out_of_range();
    rc |= test_chain_init_invalid_args();
    rc |= test_api_null_chain_safe_values();
    rc |= test_api_uninitialised_chain_safe_values();
    rc |= test_qualifier_wraparound();
    rc |= test_first_out_tie_break_lowest_index();
    rc |= test_bypass_latched_bad_visibility();
    rc |= test_reset_condition_only_clears_one();
    rc |= test_reset_chain_clears_latch_and_bypass_preserves_denial_count();
    rc |= test_denial_count_saturates();
    rc |= test_snapshot_invalid_inputs_rejected();
    rc |= test_snapshot_valid_accept_and_runtime_consistent();

    if (rc == 0) {
        printf("OK\n");
    }
    return rc ? 1 : 0;
}
