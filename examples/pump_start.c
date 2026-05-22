/*
 * examples/pump_start.c - permissive chain for pump start.
 *
 * Demonstrates the basic permissive pattern:
 *   - evaluate at the moment of start command
 *   - if denied, show which condition blocked and which tripped first
 */

#include <stdio.h>
#include "loxperm/loxperm.h"

enum {
    COND_VALVE_OPEN,
    COND_LEVEL_OK,
    COND_TEMP_OK,
    COND_OPERATOR_ARMED,
    COND_COUNT
};

static const loxperm_condition_def_t pump_start_defs[COND_COUNT] = {
    [COND_VALVE_OPEN]      = { .tag = "valve_open",
                               .qualifier_ms = 500 },
    [COND_LEVEL_OK]        = { .tag = "level_above_min",
                               .qualifier_ms = 3000 },
    [COND_TEMP_OK]         = { .tag = "motor_temp_ok",
                               .qualifier_ms = 1000 },
    [COND_OPERATOR_ARMED]  = { .tag = "operator_armed",
                               .latching     = true },
};

static loxperm_chain_t pump_chain;

int main(void) {
    loxperm_chain_init(&pump_chain, pump_start_defs, COND_COUNT);

    /* Simulate readings over time. */
    uint32_t now_ms = 0;

    /* t=0: nothing OK yet */
    loxperm_set(&pump_chain, COND_VALVE_OPEN,     false, now_ms);
    loxperm_set(&pump_chain, COND_LEVEL_OK,       false, now_ms);
    loxperm_set(&pump_chain, COND_TEMP_OK,        true,  now_ms);
    loxperm_set(&pump_chain, COND_OPERATOR_ARMED, false, now_ms);

    /* t=1000: operator arms, valve opens, level rises */
    now_ms = 1000;
    loxperm_set(&pump_chain, COND_VALVE_OPEN,     true,  now_ms);
    loxperm_set(&pump_chain, COND_LEVEL_OK,       true,  now_ms);
    loxperm_set(&pump_chain, COND_OPERATOR_ARMED, true,  now_ms);

    /* t=1400: try to start — level qualifier not yet met (needs 3 s) */
    now_ms = 1400;
    if (loxperm_is_permitted(&pump_chain, now_ms)) {
        printf("[t=%u] would start pump\n", now_ms);
    } else {
        loxperm_mask_t deny = loxperm_deny_mask(&pump_chain);
        printf("[t=%u] start DENIED, deny_mask=0x%08lx\n",
               now_ms, (unsigned long)deny);
        int fo = loxperm_first_out(&pump_chain);
        if (fo >= 0) {
            printf("       first-out: [%d] %s\n", fo,
                   loxperm_tag(&pump_chain, (size_t)fo));
        }
    }

    /* t=4500: now level has held > 3 s */
    now_ms = 4500;
    loxperm_set(&pump_chain, COND_VALVE_OPEN,     true,  now_ms);
    loxperm_set(&pump_chain, COND_LEVEL_OK,       true,  now_ms);
    loxperm_set(&pump_chain, COND_TEMP_OK,        true,  now_ms);
    loxperm_set(&pump_chain, COND_OPERATOR_ARMED, true,  now_ms);

    if (loxperm_is_permitted(&pump_chain, now_ms)) {
        printf("[t=%u] PERMITTED, starting pump\n", now_ms);
    }

    return 0;
}

