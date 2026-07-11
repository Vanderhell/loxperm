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
#include <limits.h>
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

#if CHAR_BIT != 8
#  error "loxperm requires 8-bit bytes"
#endif

#ifdef LOXPERM_WIDE_MASK
#  define LOXPERM_INTERNAL_WIDE_MASK 1u
#else
#  define LOXPERM_INTERNAL_WIDE_MASK 0u
#endif

#ifdef LOXPERM_WIDE_MASK
typedef uint64_t loxperm_mask_t;
#else
typedef uint32_t loxperm_mask_t;
#endif

enum {
    LOXPERM_INTERNAL_MASK_WIDTH = (int)(sizeof(loxperm_mask_t) * CHAR_BIT)
};

typedef char loxperm_internal_config_must_have_valid_mask_width[
    (LOXPERM_MAX_CONDITIONS >= 1 &&
     LOXPERM_MAX_CONDITIONS <= LOXPERM_INTERNAL_MASK_WIDTH) ? 1 : -1
];

#define LOXPERM_CONFIG_SIGNATURE \
    ((uint64_t)0x4C4F58504D000000ull ^ \
     ((uint64_t)LOXPERM_MAX_CONDITIONS << 32) ^ \
     ((uint64_t)LOXPERM_INTERNAL_MASK_WIDTH << 16) ^ \
     ((uint64_t)LOXPERM_INTERNAL_WIDE_MASK << 8) ^ \
     ((uint64_t)CHAR_BIT))

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
    uint32_t denial_seq;         /* monotonic ordering of denial entry */
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
    uint32_t                       next_transition_seq;
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

enum {
    LOXPERM_SNAPSHOT_WIRE_VERSION = 1u,
    LOXPERM_SNAPSHOT_WIRE_BYTE_ORDER_LITTLE = 1u,
    LOXPERM_SNAPSHOT_WIRE_SIZE = 56u,
};

typedef struct {
    uint32_t magic;
    uint8_t  version;
    uint8_t  byte_order;
    uint16_t wire_size;
    uint32_t schema_id;
    uint64_t config_id;
    uint16_t condition_count;
    uint16_t reserved0;
    int32_t  first_out_index;
    uint32_t denial_count;
    uint64_t latched_bad_mask;
    uint64_t bypass_mask;
    uint64_t reserved1;
} loxperm_snapshot_wire_t;

static inline bool loxperm_internal_mask_bits_within_count(loxperm_mask_t m, size_t count);
static inline loxperm_err_t loxperm_snapshot_load(loxperm_chain_t *c,
                                                  const loxperm_condition_def_t *defs,
                                                  size_t count,
                                                  const loxperm_snapshot_t *snap,
                                                  uint32_t now_ms);

static inline void loxperm_internal_write_u16_le(uint8_t *dst, uint16_t v) {
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static inline void loxperm_internal_write_u32_le(uint8_t *dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    dst[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static inline void loxperm_internal_write_u64_le(uint8_t *dst, uint64_t v) {
    for (size_t i = 0; i < 8; ++i) {
        dst[i] = (uint8_t)((v >> (8u * i)) & 0xFFu);
    }
}

static inline uint16_t loxperm_internal_read_u16_le(const uint8_t *src) {
    return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

static inline uint32_t loxperm_internal_read_u32_le(const uint8_t *src) {
    return (uint32_t)((uint32_t)src[0] |
                      ((uint32_t)src[1] << 8) |
                      ((uint32_t)src[2] << 16) |
                      ((uint32_t)src[3] << 24));
}

static inline uint64_t loxperm_internal_read_u64_le(const uint8_t *src) {
    uint64_t v = 0;
    for (size_t i = 0; i < 8; ++i) {
        v |= ((uint64_t)src[i]) << (8u * i);
    }
    return v;
}

static inline size_t loxperm_snapshot_wire_size(void) {
    return (size_t)LOXPERM_SNAPSHOT_WIRE_SIZE;
}

static inline loxperm_err_t loxperm_snapshot_encode(const loxperm_snapshot_t *snap,
                                                    uint64_t config_id,
                                                    uint32_t schema_id,
                                                    uint8_t *buffer,
                                                    size_t buffer_size) {
    if (!snap || !buffer) return LOXPERM_ERR_INVALID_ARG;
    if (buffer_size != LOXPERM_SNAPSHOT_WIRE_SIZE) return LOXPERM_ERR_INVALID_ARG;
    if (snap->version != 1u) return LOXPERM_ERR_INVALID_ARG;

    const uint16_t count = snap->condition_count;
    if (count > LOXPERM_MAX_CONDITIONS) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm_internal_mask_bits_within_count(snap->latched_bad_mask, count)) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm_internal_mask_bits_within_count(snap->bypass_mask, count)) return LOXPERM_ERR_INVALID_ARG;
    if (snap->first_out_index != -1) {
        if (snap->first_out_index < 0) return LOXPERM_ERR_INVALID_ARG;
        if ((uint16_t)snap->first_out_index >= count) return LOXPERM_ERR_INVALID_ARG;
    }

    memset(buffer, 0, buffer_size);
    loxperm_internal_write_u32_le(buffer + 0, 0x4C50584Du); /* "LPXM" */
    buffer[4] = (uint8_t)LOXPERM_SNAPSHOT_WIRE_VERSION;
    buffer[5] = (uint8_t)LOXPERM_SNAPSHOT_WIRE_BYTE_ORDER_LITTLE;
    loxperm_internal_write_u16_le(buffer + 6, (uint16_t)LOXPERM_SNAPSHOT_WIRE_SIZE);
    loxperm_internal_write_u32_le(buffer + 8, schema_id);
    loxperm_internal_write_u64_le(buffer + 12, config_id);
    loxperm_internal_write_u16_le(buffer + 20, count);
    loxperm_internal_write_u16_le(buffer + 22, 0);
    loxperm_internal_write_u32_le(buffer + 24, (uint32_t)snap->first_out_index);
    loxperm_internal_write_u32_le(buffer + 28, snap->denial_count);
    loxperm_internal_write_u64_le(buffer + 32, (uint64_t)snap->latched_bad_mask);
    loxperm_internal_write_u64_le(buffer + 40, (uint64_t)snap->bypass_mask);
    loxperm_internal_write_u64_le(buffer + 48, 0);
    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_snapshot_decode(loxperm_snapshot_t *snap,
                                                    uint64_t *config_id,
                                                    uint32_t *schema_id,
                                                    const uint8_t *buffer,
                                                    size_t buffer_size) {
    if (!snap || !config_id || !schema_id || !buffer) return LOXPERM_ERR_INVALID_ARG;
    if (buffer_size != LOXPERM_SNAPSHOT_WIRE_SIZE) return LOXPERM_ERR_INVALID_ARG;

    if (loxperm_internal_read_u32_le(buffer + 0) != 0x4C50584Du) return LOXPERM_ERR_INVALID_ARG;
    if (buffer[4] != LOXPERM_SNAPSHOT_WIRE_VERSION) return LOXPERM_ERR_INVALID_ARG;
    if (buffer[5] != LOXPERM_SNAPSHOT_WIRE_BYTE_ORDER_LITTLE) return LOXPERM_ERR_INVALID_ARG;
    if (loxperm_internal_read_u16_le(buffer + 6) != LOXPERM_SNAPSHOT_WIRE_SIZE) return LOXPERM_ERR_INVALID_ARG;
    if (buffer[22] != 0 || buffer[23] != 0) return LOXPERM_ERR_INVALID_ARG;
    if (loxperm_internal_read_u64_le(buffer + 48) != 0) return LOXPERM_ERR_INVALID_ARG;

    uint16_t count = loxperm_internal_read_u16_le(buffer + 20);
    uint64_t latched = loxperm_internal_read_u64_le(buffer + 32);
    uint64_t bypass = loxperm_internal_read_u64_le(buffer + 40);
    if (count > LOXPERM_MAX_CONDITIONS) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm_internal_mask_bits_within_count((loxperm_mask_t)latched, count)) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm_internal_mask_bits_within_count((loxperm_mask_t)bypass, count)) return LOXPERM_ERR_INVALID_ARG;

    int32_t first_out = (int32_t)loxperm_internal_read_u32_le(buffer + 24);
    if (first_out != -1) {
        if (first_out < 0) return LOXPERM_ERR_INVALID_ARG;
        if ((uint16_t)first_out >= count) return LOXPERM_ERR_INVALID_ARG;
    }

    memset(snap, 0, sizeof(*snap));
    snap->version = 1u;
    snap->condition_count = count;
    snap->first_out_index = first_out;
    snap->denial_count = loxperm_internal_read_u32_le(buffer + 28);
    snap->latched_bad_mask = (loxperm_mask_t)latched;
    snap->bypass_mask = (loxperm_mask_t)bypass;
    *config_id = loxperm_internal_read_u64_le(buffer + 12);
    *schema_id = loxperm_internal_read_u32_le(buffer + 8);
    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_snapshot_load_wire(loxperm_chain_t *c,
                                                       const loxperm_condition_def_t *defs,
                                                       size_t count,
                                                       uint64_t expected_config_id,
                                                       uint32_t expected_schema_id,
                                                       const uint8_t *buffer,
                                                       size_t buffer_size,
                                                       uint32_t now_ms) {
    loxperm_snapshot_t snap;
    uint64_t config_id = 0;
    uint32_t schema_id = 0;
    loxperm_err_t st = loxperm_snapshot_decode(&snap, &config_id, &schema_id, buffer, buffer_size);
    if (st != LOXPERM_OK) return st;
    if (config_id != expected_config_id) return LOXPERM_ERR_INVALID_ARG;
    if (schema_id != expected_schema_id) return LOXPERM_ERR_INVALID_ARG;
    return loxperm_snapshot_load(c, defs, count, &snap, now_ms);
}

/* Snapshot integrity note:
 * loxperm_snapshot_t does not include a CRC or other torn-write protection.
 * If snapshots are stored in a backend that can corrupt or tear writes, the
 * caller/storage layer must provide integrity/atomicity. */

/* ---------------------------------------------------------------------- */
/* Internal helpers                                                       */
/* ---------------------------------------------------------------------- */

static inline uint32_t loxperm_internal_elapsed_ms(uint32_t now_ms, uint32_t then_ms) {
    return (uint32_t)(now_ms - then_ms); /* wrap-safe for uint32_t */
}

static inline loxperm_err_t loxperm_internal_require_init(const loxperm_chain_t *c) {
    return (c && c->initialised) ? LOXPERM_OK : LOXPERM_ERR_STATE;
}

static inline bool loxperm_internal_index_ok(const loxperm_chain_t *c, size_t index) {
    return c && index < c->condition_count;
}

static inline size_t loxperm_internal_bounded_count(const loxperm_chain_t *c) {
    if (!c || c->condition_count > LOXPERM_MAX_CONDITIONS) {
        return 0;
    }
    return c->condition_count;
}

static inline loxperm_mask_t loxperm_internal_bit(size_t i) {
    return (i < (size_t)LOXPERM_INTERNAL_MASK_WIDTH)
        ? (((loxperm_mask_t)1) << i)
        : (loxperm_mask_t)0;
}

static inline bool loxperm_internal_mask_bits_within_count(loxperm_mask_t m, size_t count) {
    const size_t width = sizeof(loxperm_mask_t) * CHAR_BIT;
    if (count == 0) return m == 0;
    if (count > width) return false;
    if (count == width) return true;
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
    c->next_transition_seq = 1;
    c->just_denied_flag = false;
    c->just_permitted_flag = false;

    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_reset_chain(loxperm_chain_t *c, uint32_t now_ms) {
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;
    (void)now_ms;

    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        c->states[i].latched_bad = false;
        c->states[i].bypassed = false;
        c->states[i].bypass_op_id = 0;
        c->states[i].last_ok = false;
        c->states[i].last_denied_at_ms = 0;
        c->states[i].denial_seq = 0;
    }
    c->current_deny_mask = 0;
    c->first_out_index = -1;
    c->last_permitted = true;
    c->just_denied_flag = false;
    c->just_permitted_flag = false;
    /* Preserve denial_count; it is lifetime. */
    return LOXPERM_OK;
}

static inline loxperm_err_t loxperm_reset_condition(loxperm_chain_t *c,
                                                    size_t index,
                                                    uint32_t now_ms) {
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!loxperm_internal_index_ok(c, index)) return LOXPERM_ERR_INDEX;
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
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;
    size_t count = loxperm_internal_bounded_count(c);
    if (index >= count) return LOXPERM_ERR_INDEX;

    loxperm_condition_state_t *s = c->states + index;
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
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;

    loxperm_mask_t deny_mask = 0;

    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        const loxperm_condition_def_t *d = &c->defs[i];
        loxperm_condition_state_t *s = &c->states[i];
        bool ok_prev = s->last_ok;

        /* Update qualified based on current + qualifier_ms (no delay on going false). */
        if (s->current) {
            if (d->qualifier_ms == 0) {
                s->qualified = true;
            } else {
                uint32_t held = loxperm_internal_elapsed_ms(now_ms, s->became_true_at_ms);
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
                s->bypassed = false;
                s->denial_seq = c->next_transition_seq;
                if (c->next_transition_seq != UINT32_MAX) {
                    c->next_transition_seq++;
                }
            }
            ok_now = s->qualified && !s->latched_bad;
        } else {
            ok_now = s->qualified;
        }

        if (!ok_now) {
            if (ok_prev) {
                s->last_denied_at_ms = s->last_set_at_ms;
                s->denial_seq = c->next_transition_seq;
                if (c->next_transition_seq != UINT32_MAX) {
                    c->next_transition_seq++;
                }
            }
            deny_mask |= loxperm_internal_bit(i);
        } else {
            s->last_denied_at_ms = 0;
            s->denial_seq = 0;
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

        /* First-out: choose the denying condition with the earliest denial transition.
         * If transitions are equal/unknown, choose the lowest index for determinism. */
        int fo = -1;
        uint32_t best_seq = 0;
        for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
            if (!(deny_mask & loxperm_internal_bit(i))) continue;
            uint32_t seq = c->states[i].denial_seq;
            if (fo < 0) {
                fo = (int)i;
                best_seq = seq;
                continue;
            }
            if (seq < best_seq || (seq == best_seq && (int)i < fo)) {
                fo = (int)i;
                best_seq = seq;
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
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return 0;
    return c->current_deny_mask;
}

static inline int loxperm_first_out(const loxperm_chain_t *c) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return -1;
    return c->first_out_index;
}

static inline bool loxperm_just_denied(loxperm_chain_t *c) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return false;
    bool v = c->just_denied_flag;
    c->just_denied_flag = false;
    return v;
}

static inline bool loxperm_just_permitted(loxperm_chain_t *c) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return false;
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
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;
    size_t count = loxperm_internal_bounded_count(c);
    if (index >= count) return LOXPERM_ERR_INDEX;

    const loxperm_condition_def_t *d = &c->defs[index];
    if (!d->bypassable) return LOXPERM_ERR_NOT_BYPASSABLE;

    loxperm_condition_state_t *s = c->states + index;
    s->bypassed = on;
    s->last_set_at_ms = now_ms;
    s->bypass_op_id = op_id;
    return LOXPERM_OK;
}

static inline bool loxperm_is_bypassed(const loxperm_chain_t *c, size_t index) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return false;
    if (!loxperm_internal_index_ok(c, index)) return false;
    return c->states[index].bypassed;
}

static inline uint16_t loxperm_bypass_operator_id(const loxperm_chain_t *c, size_t index) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return 0;
    if (!loxperm_internal_index_ok(c, index)) return 0;
    return c->states[index].bypass_op_id;
}

static inline loxperm_mask_t loxperm_bypass_mask(const loxperm_chain_t *c) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return 0;
    loxperm_mask_t m = 0;
    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        if (c->states[i].bypassed) m |= loxperm_internal_bit(i);
    }
    return m;
}

/* ---------------------------------------------------------------------- */
/* Introspection / diagnostics                                            */
/* ---------------------------------------------------------------------- */

static inline const char *loxperm_tag(const loxperm_chain_t *c, size_t index) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return NULL;
    if (!loxperm_internal_index_ok(c, index)) return NULL;
    return c->defs[index].tag;
}

static inline bool loxperm_get_qualified(const loxperm_chain_t *c, size_t index) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return false;
    if (!loxperm_internal_index_ok(c, index)) return false;
    return c->states[index].qualified;
}

static inline bool loxperm_get_raw(const loxperm_chain_t *c, size_t index) {
    if (loxperm_internal_require_init(c) != LOXPERM_OK) return false;
    if (!loxperm_internal_index_ok(c, index)) return false;
    return c->states[index].current;
}

/* ---------------------------------------------------------------------- */
/* Snapshot for persistence                                               */
/* ---------------------------------------------------------------------- */

static inline loxperm_err_t loxperm_snapshot_save(const loxperm_chain_t *c,
                                                  loxperm_snapshot_t *out) {
    loxperm_err_t st = loxperm_internal_require_init(c);
    if (st != LOXPERM_OK) return st;
    if (!out) return LOXPERM_ERR_INVALID_ARG;

    memset(out, 0, sizeof(*out));
    out->version = 1;
    out->condition_count = (uint16_t)c->condition_count;
    out->first_out_index = (int32_t)c->first_out_index;
    out->denial_count    = c->denial_count;

    loxperm_mask_t latched = 0;
    loxperm_mask_t bypass  = 0;
    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        if (c->states[i].latched_bad) latched |= loxperm_internal_bit(i);
        if (c->states[i].bypassed)    bypass  |= loxperm_internal_bit(i);
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

    if (!loxperm_internal_mask_bits_within_count(snap->latched_bad_mask, count)) return LOXPERM_ERR_INVALID_ARG;
    if (!loxperm_internal_mask_bits_within_count(snap->bypass_mask, count)) return LOXPERM_ERR_INVALID_ARG;

    if (snap->first_out_index != -1) {
        if (snap->first_out_index < 0) return LOXPERM_ERR_INVALID_ARG;
        if ((size_t)snap->first_out_index >= count) return LOXPERM_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; ++i) {
        if ((snap->bypass_mask & loxperm_internal_bit(i)) != 0 && !defs[i].bypassable) return LOXPERM_ERR_INVALID_ARG;
        if ((snap->latched_bad_mask & loxperm_internal_bit(i)) != 0 && !defs[i].latching) return LOXPERM_ERR_INVALID_ARG;
    }

    loxperm_err_t st = loxperm_chain_init(c, defs, count);
    if (st != LOXPERM_OK) return st;

    c->denial_count    = snap->denial_count;
    c->first_out_index = (int)snap->first_out_index;
    c->next_transition_seq = 1u;

    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        c->states[i].latched_bad = (snap->latched_bad_mask & loxperm_internal_bit(i)) != 0;
        c->states[i].bypassed    = (snap->bypass_mask      & loxperm_internal_bit(i)) != 0;
        c->states[i].last_set_at_ms = now_ms;
        c->states[i].last_ok = false;
        c->states[i].denial_seq = 0;
    }

    /* Recompute enough runtime state for consistent diagnostic getters immediately after load.
     * Snapshot does not include inputs; caller is responsible for restoring inputs then calling
     * loxperm_evaluate() as appropriate for their application. */
    loxperm_mask_t deny_mask = 0;
    for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
        const loxperm_condition_def_t *d = &c->defs[i];
        loxperm_condition_state_t *s = &c->states[i];

        if (s->current) {
            if (d->qualifier_ms == 0) {
                s->qualified = true;
            } else {
                uint32_t held = loxperm_internal_elapsed_ms(now_ms, s->became_true_at_ms);
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
        if (!ok_now) deny_mask |= loxperm_internal_bit(i);
    }

    c->current_deny_mask = deny_mask;
    c->last_permitted = (deny_mask == 0);
    c->last_eval_ms = now_ms;

    if (deny_mask == 0) {
        c->first_out_index = -1;
    } else {
        int fo = (int)snap->first_out_index;
        if (fo >= 0 && (deny_mask & loxperm_internal_bit((size_t)fo)) != 0) {
            c->first_out_index = fo;
        } else {
            int lowest = -1;
            for (size_t i = 0, n = loxperm_internal_bounded_count(c); i < n; ++i) {
                if (deny_mask & loxperm_internal_bit(i)) { lowest = (int)i; break; }
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
