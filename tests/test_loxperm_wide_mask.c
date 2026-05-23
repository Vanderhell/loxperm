#include <stdio.h>
#include <stdint.h>
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

static int test_wide_mask_basics(void) {
    TASSERT(sizeof(loxperm_mask_t) == 8u);

    loxperm_condition_def_t defs[64] = {0};
    for (size_t i = 0; i < 64; ++i) defs[i] = (loxperm_condition_def_t){ .tag = "x" };

    loxperm_chain_t c = {0};
    TASSERT(loxperm_chain_init(&c, defs, 64) == LOXPERM_OK);

    /* Make all satisfied, then trip index 63 and check highest bit. */
    for (size_t i = 0; i < 64; ++i) TASSERT(loxperm_set(&c, i, true, 0) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 0) == true);

    TASSERT(loxperm_set(&c, 63, false, 1) == LOXPERM_OK);
    TASSERT(loxperm_is_permitted(&c, 1) == false);
    TASSERT((loxperm_deny_mask(&c) & (((loxperm_mask_t)1) << 63)) != 0);

    /* Index 64 is out of range. */
    volatile size_t oob = 64;
    TASSERT(loxperm_set(&c, oob, true, 2) == LOXPERM_ERR_INDEX);
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_wide_mask_basics();
    if (rc == 0) printf("OK\n");
    return rc ? 1 : 0;
}
