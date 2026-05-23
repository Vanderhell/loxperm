/*
 * loxperm.h - Permissive / interlock evaluator with explainable deny mask.
 *
 * SPDX-License-Identifier: MIT
 *
 * Single-header, heap-free, deterministic, C99.
 *
 * Public API + implementation. All functions are `static inline` so the
 * library is header-only: add `include/` to your include path.
 */

#ifndef LOXPERM_LOXPERM_H
#define LOXPERM_LOXPERM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- */
/* Version                                                                */
/* ---------------------------------------------------------------------- */

#define LOXPERM_VERSION_MAJOR 0
#define LOXPERM_VERSION_MINOR 1
#define LOXPERM_VERSION_PATCH 0

/* Default to 32 conditions / uint32_t mask. Define LOXPERM_WIDE_MASK
 * before including to get 64 conditions on platforms that have uint64_t. */
#ifndef LOXPERM_MAX_CONDITIONS
#  ifdef LOXPERM_WIDE_MASK
#    define LOXPERM_MAX_CONDITIONS 64
#  else
#    define LOXPERM_MAX_CONDITIONS 32
#  endif
#endif

#ifdef LOXPERM_WIDE_MASK
typedef uint64_t loxperm_mask_t;
#else
typedef uint32_t loxperm_mask_t;
#endif

/* ---------------------------------------------------------------------- */
/* Errors                                                                 */
/* ---------------------------------------------------------------------- */

typedef enum {
    LOXPERM_OK                 =  0,
    LOXPERM_ERR_INVALID_ARG    = -1,
    LOXPERM_ERR_INDEX          = -2,
    LOXPERM_ERR_TOO_MANY       = -3,
    LOXPERM_ERR_STATE          = -4,
    LOXPERM_ERR_NOT_BYPASSABLE = -5,
} loxperm_err_t;

/* ---------------------------------------------------------------------- */
/* Condition definition                                                   */
/* ---------------------------------------------------------------------- */

typedef struct {
    /* Human-readable tag for diagnostics. Pointer not copied. */
    const char *tag;

    /* If true, condition value must hold "true" for >= qualifier_ms before
     * counting as satisfied. Prevents transient OK readings from arming
     * an interlock. 0 = no qualifier. */
    uint32_t qualifier_ms;

    /* If true, once the condition transitions to denied, it stays denied
     * until loxperm_reset_condition() or loxperm_reset_chain() is called.
     * Use for trips that must require explicit operator acknowledgement. */
    bool latching;

    /* If true, loxperm_set_bypass() may force this condition to "OK"
     * regardless of input. Bypass is for maintenance and is audited. */
    bool bypassable;
} loxperm_condition_def_t;

/* ---------------------------------------------------------------------- */
/* Per-condition runtime state (size exposed for embedding)                */
/* ---------------------------------------------------------------------- */

typedef struct {
    bool     current;            /* raw input from caller            */
    bool     qualified;          /* current && held qualifier_ms     */
    bool     latched_bad;        /* sticky denial (if def->latching) */
    bool     bypassed;
    uint32_t became_true_at_ms;
    uint32_t last_set_at_ms;
    uint32_t last_denied_at_ms;  /* best-effort timestamp for first-out */
    uint16_t bypass_op_id;       /* last operator id that toggled bypass */
    bool     last_ok;            /* previous effective OK (for trip detect) */
} loxperm_condition_state_t;

/* ---------------------------------------------------------------------- */
/* Chain (caller-allocated)                                               */
/* ---------------------------------------------------------------------- */

typedef struct {
    /* --- public-readable --- */
    size_t          condition_count;
    loxperm_mask_t  current_deny_mask;   /* bit set => denying right now */
    int             first_out_index;     /* -1 if currently permitted    */
    uint32_t        denial_count;        /* lifetime; saturates at UINT32_MAX */

    /* --- internal --- */
    const loxperm_condition_def_t *defs;
    loxperm_condition_state_t      states[LOXPERM_MAX_CONDITIONS];
    uint32_t                       last_eval_ms;
    bool                           initialised;
    bool                           last_permitted;
    bool                           just_denied_flag;
    bool                           just_permitted_flag;
} loxperm_chain_t;

/* ---------------------------------------------------------------------- */
/* Snapshot for persistence                                               */
/* ---------------------------------------------------------------------- */

typedef struct {
    uint8_t        version;            /* = 1 */
    uint8_t        reserved;
    uint16_t       condition_count;
    loxperm_mask_t latched_bad_mask;
    loxperm_mask_t bypass_mask;
    int32_t        first_out_index;
    uint32_t       denial_count;
} loxperm_snapshot_t;

/* Snapshot integrity note:
 * loxperm_snapshot_t does not include a CRC or other torn-write protection.
 * If snapshots are stored in a backend that can corrupt or tear writes, the
 * caller/storage layer must provide integrity/atomicity. */

/* ---------------------------------------------------------------------- */
/* Internal helpers                                                       */
/* ---------------------------------------------------------------------- */

static inline uint32_t loxperm__elapsed_ms(uint32_t now_ms, uint32_t then_ms) {
    return (uint32_t)(now_ms - then_ms); /* wrap-safe for uint32_t */
}

static inline loxperm_err_t loxperm__require_init(const loxperm_chain_t *c) {
    return (c && c->initialised) ? LOXPERM_OK : LOXPERM_ERR_STATE;
}

static inline bool loxperm__index_ok(const loxperm_chain_t *c, size_t index) {
    return c && index < c->condition_count;
}

static inline loxperm_mask_t loxperm__bit(size_t i) {
    return ((loxperm_mask_t)1) << i;
}

static inline bool loxperm__mask_bits_within_count(loxperm_mask_t m, size_t count) {
    const size_t width = sizeof(loxperm_mask_t) * 8u;
    if (count == 0) return m == 0;
    if (count >= width) return true;
    loxperm_mask_t allowed = (((loxperm_mask_t)1) << count) - 1;
    return (m & ~allowed) == 0;
}

/* ---------------------------------------------------------------------- */
/* Lifecycle                                                              */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_chain_init(loxperm_chain_t *c,
                                               const loxperm_condition_def_t *defs,
                                               size_t count) {
    if (!c) return LOXPERM_ERR_INVALID_ARG;
    if (count > LOXPERM_MAX_CONDITIONS) return LOXPERM_ERR_TOO_MANY;
    if (count != 0 && defs == NULL) return LOXPERM_ERR_INVALID_ARG;

    memset(c, 0, sizeof(*c));
    c->condition_count = count;
    c->defs            = defs;
    c->first_out_index = -1;
    c->initialised     = true;
    c->last_permitted  = true; /* transitions computed on first evaluate */
    c->current_deny_mask = 0;
    c->last_eval_ms    = 0;
    c->just_denied_flag = false;
    c->just_permitted_flag = false;

    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_reset_chain(loxperm_chain_t *c, uint32_t now_ms) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;
    (void)now_ms;

    for (size_t i = 0; i < c->condition_count; ++i) {
        c->states[i].latched_bad = false;
        c->states[i].bypassed = false;
        c->states[i].bypass_op_id = 0;
    }
    /* Preserve denial_count; it is lifetime. */
    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_reset_condition(loxperm_chain_t *c,
                                                    size_t index,
                                                    uint32_t now_ms) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!loxperm__index_ok(c, index)) return LOXPERM_ERR_INDEX;
    (void)now_ms;

    c->states[index].latched_bad = false;
    return LOXPERM_OK;
}

/* ---------------------------------------------------------------------- */
/* Per-condition input                                                    */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_set(loxperm_chain_t *c,
                                        size_t index,
                                        bool value,
                                        uint32_t now_ms) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!loxperm__index_ok(c, index)) return LOXPERM_ERR_INDEX;

    loxperm_condition_state_t *s = &c->states[index];
    bool prev = s->current;

    s->current = value;
    s->last_set_at_ms = now_ms;

    if (value && !prev) {
        s->became_true_at_ms = now_ms;
    }
    if (!value) {
        s->qualified = false;
    }

    return LOXPERM_OK;
}

/* ---------------------------------------------------------------------- */
/* Aggregate evaluation                                                   */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_evaluate(loxperm_chain_t *c, uint32_t now_ms) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;

    loxperm_mask_t deny_mask = 0;

    /* Clear one-shot transition flags for this evaluation; getters clear on read too. */
    c->just_denied_flag = false;
    c->just_permitted_flag = false;

    for (size_t i = 0; i < c->condition_count; ++i) {
        const loxperm_condition_def_t *d = &c->defs[i];
        loxperm_condition_state_t *s = &c->states[i];
        bool ok_prev = s->last_ok;

        /* Update qualified based on current + qualifier_ms (no delay on going false). */
        if (s->current) {
            if (d->qualifier_ms == 0) {
                s->qualified = true;
            } else {
                uint32_t held = loxperm__elapsed_ms(now_ms, s->became_true_at_ms);
                s->qualified = (held >= d->qualifier_ms);
            }
        } else {
            s->qualified = false;
        }

        bool ok_now = false;
        if (s->bypassed) {
            ok_now = true;
        } else if (d->latching) {
            /* Latching only trips after the condition has previously been OK. */
            if (ok_prev && !s->qualified) {
                s->latched_bad = true;
                s->last_denied_at_ms = s->last_set_at_ms;
            }
            ok_now = s->qualified && !s->latched_bad;
        } else {
            ok_now = s->qualified;
        }

        if (!ok_now) {
            deny_mask |= loxperm__bit(i);
            if (ok_prev) {
                s->last_denied_at_ms = s->last_set_at_ms;
            }
        } else {
            s->last_denied_at_ms = 0;
        }

        s->last_ok = ok_now;
    }

    c->current_deny_mask = deny_mask;
    bool permitted = (deny_mask == 0);

    /* Transition bookkeeping. */
    if (c->last_permitted && !permitted) {
        c->just_denied_flag = true;
        if (c->denial_count != UINT32_MAX) {
            c->denial_count++;
        }

        /* First-out: choose the denying condition with the earliest denied timestamp.
         * If timestamps are equal/unknown, choose the lowest index for determinism. */
        int fo = -1;
        uint32_t best_t = 0;
        for (size_t i = 0; i < c->condition_count; ++i) {
            if (!(deny_mask & loxperm__bit(i))) continue;
            uint32_t t = c->states[i].last_denied_at_ms;
            if (fo < 0) {
                fo = (int)i;
                best_t = t;
                continue;
            }
            if (t < best_t || (t == best_t && (int)i < fo)) {
                fo = (int)i;
                best_t = t;
            }
        }
        c->first_out_index = fo;
    } else if (!c->last_permitted && permitted) {
        c->just_permitted_flag = true;
        c->first_out_index = -1;
    }

    c->last_permitted = permitted;
    c->last_eval_ms = now_ms;
    return LOXPERM_OK;
}

static inline bool loxperm_is_permitted(loxperm_chain_t *c, uint32_t now_ms) {
    if (loxperm_evaluate(c, now_ms) != LOXPERM_OK) return false;
    return c->current_deny_mask == 0;
}

static inline loxperm_mask_t loxperm_deny_mask(const loxperm_chain_t *c) {
    if (loxperm__require_init(c) != LOXPERM_OK) return 0;
    return c->current_deny_mask;
}

static inline int loxperm_first_out(const loxperm_chain_t *c) {
    if (loxperm__require_init(c) != LOXPERM_OK) return -1;
    return c->first_out_index;
}

static inline bool loxperm_just_denied(loxperm_chain_t *c) {
    if (loxperm__require_init(c) != LOXPERM_OK) return false;
    bool v = c->just_denied_flag;
    c->just_denied_flag = false;
    return v;
}

static inline bool loxperm_just_permitted(loxperm_chain_t *c) {
    if (loxperm__require_init(c) != LOXPERM_OK) return false;
    bool v = c->just_permitted_flag;
    c->just_permitted_flag = false;
    return v;
}

/* ---------------------------------------------------------------------- */
/* Bypass (maintenance override)                                          */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_set_bypass(loxperm_chain_t *c,
                                               size_t index,
                                               bool on,
                                               uint32_t now_ms,
                                               uint16_t op_id) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!loxperm__index_ok(c, index)) return LOXPERM_ERR_INDEX;

    const loxperm_condition_def_t *d = &c->defs[index];
    if (!d->bypassable) return LOXPERM_ERR_NOT_BYPASSABLE;

    loxperm_condition_state_t *s = &c->states[index];
    s->bypassed = on;
    s->last_set_at_ms = now_ms;
    s->bypass_op_id = op_id;
    return LOXPERM_OK;
}

static inline bool loxperm_is_bypassed(const loxperm_chain_t *c, size_t index) {
    if (loxperm__require_init(c) != LOXPERM_OK) return false;
    if (!loxperm__index_ok(c, index)) return false;
    return c->states[index].bypassed;
}

static inline loxperm_mask_t loxperm_bypass_mask(const loxperm_chain_t *c) {
    if (loxperm__require_init(c) != LOXPERM_OK) return 0;
    loxperm_mask_t m = 0;
    for (size_t i = 0; i < c->condition_count; ++i) {
        if (c->states[i].bypassed) m |= loxperm__bit(i);
    }
    return m;
}

/* ---------------------------------------------------------------------- */
/* Introspection / diagnostics                                            */
/* ---------------------------------------------------------------------- */

static inline const char *loxperm_tag(const loxperm_chain_t *c, size_t index) {
    if (loxperm__require_init(c) != LOXPERM_OK) return NULL;
    if (!loxperm__index_ok(c, index)) return NULL;
    return c->defs[index].tag;
}

static inline bool loxperm_get_qualified(const loxperm_chain_t *c, size_t index) {
    if (loxperm__require_init(c) != LOXPERM_OK) return false;
    if (!loxperm__index_ok(c, index)) return false;
    return c->states[index].qualified;
}

static inline bool loxperm_get_raw(const loxperm_chain_t *c, size_t index) {
    if (loxperm__require_init(c) != LOXPERM_OK) return false;
    if (!loxperm__index_ok(c, index)) return false;
    return c->states[index].current;
}

/* ---------------------------------------------------------------------- */
/* Snapshot for persistence                                               */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_snapshot_save(const loxperm_chain_t *c,
                                                  loxperm_snapshot_t *out) {
    loxperm_err_t st = loxperm__require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!out) return LOXPERM_ERR_INVALID_ARG;

    memset(out, 0, sizeof(*out));
    out->version = 1;
    out->condition_count = (uint16_t)c->condition_count;
    out->first_out_index = (int32_t)c->first_out_index;
    out->denial_count    = c->denial_count;

    loxperm_mask_t latched = 0;
    loxperm_mask_t bypass  = 0;
    for (size_t i = 0; i < c->condition_count; ++i) {
        if (c->states[i].latched_bad) latched |= loxperm__bit(i);
        if (c->states[i].bypassed)    bypass  |= loxperm__bit(i);
    }
    out->latched_bad_mask = latched;
    out->bypass_mask      = bypass;

    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_snapshot_load(loxperm_chain_t *c,
                                                  const loxperm_condition_def_t *defs,
                                                  size_t count,
                                                  const loxperm_snapshot_t *snap,
                                                  uint32_t now_ms) {
    if (!c) return LOXPERM_ERR_INVALID_ARG;
    if (!snap) return LOXPERM_ERR_INVALID_ARG;
    if (count != 0 && defs == NULL) return LOXPERM_ERR_INVALID_ARG;
    if (count > LOXPERM_MAX_CONDITIONS) return LOXPERM_ERR_TOO_MANY;
    if (snap->version != 1) return LOXPERM_ERR_INVALID_ARG;
    if (snap->condition_count != (uint16_t)count) return LOXPERM_ERR_INVALID_ARG;

    if (!loxperm__mask_bits_within_count(snap->latched_bad_mask, count)) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm__mask_bits_within_count(snap->bypass_mask, count)) return LOXPERM_ERR_INVALID_ARG;

    if (snap->first_out_index != -1) {
        if (snap->first_out_index < 0) return LOXPERM_ERR_INVALID_ARG;
        if ((size_t)snap->first_out_index >= count) return LOXPERM_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; ++i) {
        if ((snap->bypass_mask & loxperm__bit(i)) != 0 && !defs[i].bypassable) return LOXPERM_ERR_INVALID_ARG;
        if ((snap->latched_bad_mask & loxperm__bit(i)) != 0 && !defs[i].latching) return LOXPERM_ERR_INVALID_ARG;
    }

    loxperm_err_t st = loxperm_chain_init(c, defs, count);
    if (st != LOXPERM_OK) return st;

    c->denial_count    = snap->denial_count;
    c->first_out_index = (int)snap->first_out_index;

    for (size_t i = 0; i < c->condition_count; ++i) {
        c->states[i].latched_bad = (snap->latched_bad_mask & loxperm__bit(i)) != 0;
        c->states[i].bypassed    = (snap->bypass_mask      & loxperm__bit(i)) != 0;
        c->states[i].last_set_at_ms = now_ms;
    }

    /* Recompute enough runtime state for consistent diagnostic getters immediately after load.
     * Snapshot does not include inputs; caller is responsible for restoring inputs then calling
     * loxperm_evaluate() as appropriate for their application. */
    loxperm_mask_t deny_mask = 0;
    for (size_t i = 0; i < c->condition_count; ++i) {
        const loxperm_condition_def_t *d = &c->defs[i];
        loxperm_condition_state_t *s = &c->states[i];

        if (s->current) {
            if (d->qualifier_ms == 0) {
                s->qualified = true;
            } else {
                uint32_t held = loxperm__elapsed_ms(now_ms, s->became_true_at_ms);
                s->qualified = (held >= d->qualifier_ms);
            }
        } else {
            s->qualified = false;
        }

        bool ok_now = false;
        if (s->bypassed) {
            ok_now = true;
        } else if (d->latching) {
            ok_now = s->qualified && !s->latched_bad;
        } else {
            ok_now = s->qualified;
        }

        s->last_ok = ok_now;
        if (!ok_now) deny_mask |= loxperm__bit(i);
    }

    c->current_deny_mask = deny_mask;
    c->last_permitted = (deny_mask == 0);
    c->last_eval_ms = now_ms;

    if (deny_mask == 0) {
        c->first_out_index = -1;
    } else {
        int fo = (int)snap->first_out_index;
        if (fo >= 0 && (deny_mask & loxperm__bit((size_t)fo)) != 0) {
            c->first_out_index = fo;
        } else {
            int lowest = -1;
            for (size_t i = 0; i < c->condition_count; ++i) {
                if (deny_mask & loxperm__bit(i)) { lowest = (int)i; break; }
            }
            c->first_out_index = lowest;
        }
    }

    return LOXPERM_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* LOXPERM_LOXPERM_H */
