/*
 * examples/interlock_first_out.c
 *
 * Demonstrates the interlock pattern:
 *   - chain evaluated continuously while pump runs
 *   - first-out detection on simultaneous trips
 */

#include <stdio.h>
#include "loxperm/loxperm.h"

enum {
    COND_OVERLOAD_OK,
    COND_VIBRATION_OK,
    COND_BEARING_TEMP_OK,
    COND_OIL_PRESSURE_OK,
    COND_COUNT
};

static const loxperm_condition_def_t run_defs[COND_COUNT] = {
    [COND_OVERLOAD_OK]      = { .tag = "no_overload",
                                .qualifier_ms = 200,
                                .latching     = true },
    [COND_VIBRATION_OK]     = { .tag = "vibration_within_limit",
                                .qualifier_ms = 500,
                                .latching     = true },
    [COND_BEARING_TEMP_OK]  = { .tag = "bearing_temp_ok",
                                .qualifier_ms = 1000,
                                .latching     = true,
                                .bypassable   = true },
    [COND_OIL_PRESSURE_OK]  = { .tag = "oil_pressure_ok",
                                .qualifier_ms = 500,
                                .latching     = true },
};

int main(void) {
    loxperm_chain_t chain;
    loxperm_chain_init(&chain, run_defs, COND_COUNT);

    uint32_t now_ms = 0;

    /* All OK and qualified */
    for (int t = 0; t < 5; ++t) {
        now_ms = (uint32_t)t * 1000;
        loxperm_set(&chain, COND_OVERLOAD_OK,      true, now_ms);
        loxperm_set(&chain, COND_VIBRATION_OK,     true, now_ms);
        loxperm_set(&chain, COND_BEARING_TEMP_OK,  true, now_ms);
        loxperm_set(&chain, COND_OIL_PRESSURE_OK,  true, now_ms);
    }

    if (loxperm_is_permitted(&chain, now_ms)) {
        printf("[t=%u] all interlocks satisfied, pump can run\n", now_ms);
    }

    /* t=6000: oil pressure drops first (root cause). 50 ms later, bearing
     * temperature spikes (consequence). loxperm should report oil pressure
     * as first-out. */
    now_ms = 6000;
    loxperm_set(&chain, COND_OIL_PRESSURE_OK,  false, now_ms);

    now_ms = 6050;
    loxperm_set(&chain, COND_BEARING_TEMP_OK, false, now_ms);

    /* After qualifier_ms passes, both will be denied. */
    now_ms = 7100;
    bool permitted = loxperm_is_permitted(&chain, now_ms);
    printf("[t=%u] permitted=%d\n", now_ms, permitted);

    if (loxperm_just_denied(&chain)) {
        int fo = loxperm_first_out(&chain);
        printf("       TRIP, first-out: [%d] %s\n",
               fo, fo >= 0 ? loxperm_tag(&chain, (size_t)fo) : "?");

        loxperm_mask_t deny = loxperm_deny_mask(&chain);
        printf("       all denying: 0x%08lx\n", (unsigned long)deny);
        for (size_t i = 0; i < chain.condition_count; ++i) {
            if (deny & ((loxperm_mask_t)1 << i)) {
                printf("         - %s\n", loxperm_tag(&chain, i));
            }
        }
    }

    /* Maintenance: bypass bearing temp while oil pressure issue is fixed */
    loxperm_set_bypass(&chain, COND_BEARING_TEMP_OK, true, 8000, /*op=*/3);
    loxperm_set(&chain, COND_OIL_PRESSURE_OK, true, 8000);

    /* Reset the latched oil-pressure condition */
    loxperm_reset_condition(&chain, COND_OIL_PRESSURE_OK, 8000);

    now_ms = 8600;  /* qualifier satisfied for oil pressure */
    if (loxperm_is_permitted(&chain, now_ms)) {
        printf("[t=%u] permitted after bypass + reset\n", now_ms);
        printf("       bypass_mask=0x%08lx\n",
               (unsigned long)loxperm_bypass_mask(&chain));
    }

    return 0;
}

