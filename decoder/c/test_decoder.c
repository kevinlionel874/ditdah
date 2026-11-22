/* test_decoder.c — Runs every case in spec/morse_vectors.h against the C decoder.
 *
 * Build with the Makefile; exits 0 on full pass, non-zero on any failure.
 *
 * Runner logic (from design.md):
 *   t = 1000
 *   for each signal:
 *     collect keyDown(t, key)
 *     t += on;  collect keyUp(t, key)
 *     t += off; collect tick(t)
 *   collect flush()
 *   compare tokens with expected
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "morse_decoder.h"

/* Verify the size promise from requirement 9.3.
 * _Static_assert is C11; assert.h provides static_assert as an alias in C11+.
 * We use _Static_assert directly so this compiles cleanly under -std=c99 with
 * GCC, which accepts _Static_assert as a compiler built-in even in C99 mode. */
_Static_assert(sizeof(md_decoder_t) <= 64,
               "md_decoder_t must be 64 bytes or less");

/* Pull in the shared test vectors. */
#include "../../spec/morse_vectors.h"

/* -------------------------------------------------------------------------
 * Token buffer
 * Each output is formatted as a NUL-terminated token and stored in a flat
 * char array: tokens are separated by '|' so we can compare with expected.
 * Max tokens per case: 100 is more than any vector needs.
 * ------------------------------------------------------------------------- */
#define MAX_TOKENS 128
#define TOK_BUF    512

typedef struct {
    char buf[TOK_BUF];  /* "H|E|L|L|O| |W|O|R|L|D"  style */
    int  len;           /* current write position in buf    */
    int  count;         /* number of tokens appended        */
} token_buf_t;

static void tb_init(token_buf_t *tb) {
    tb->buf[0] = '\0';
    tb->len    = 0;
    tb->count  = 0;
}

static void tb_append(token_buf_t *tb, const md_out_t *o) {
    if (tb->len >= TOK_BUF - 16) return; /* safety: leave room for token */

    /* Separator */
    if (tb->count > 0) {
        tb->buf[tb->len++] = '|';
    }

    /* Token text */
    const char *tok;
    char ch_buf[2] = { o->ch, '\0' };

    switch (o->kind) {
        case MD_OUT_CHAR:      tok = ch_buf;    break;
        case MD_OUT_SPACE:     tok = " ";       break;
        case MD_OUT_BACKSPACE: tok = "<BS>";    break;
        case MD_OUT_ENTER:     tok = "<ENTER>"; break;
        case MD_OUT_ACCEPT:    tok = "<ACCEPT>"; break;
        case MD_OUT_UNKNOWN:   tok = "<?>";     break;
        default:               tok = "<?>";     break;
    }

    int tlen = (int)strlen(tok);
    memcpy(tb->buf + tb->len, tok, (size_t)tlen);
    tb->len += tlen;
    tb->buf[tb->len] = '\0';
    tb->count++;
}

/* -------------------------------------------------------------------------
 * Collect helper: append up to `n` outputs to the token buffer.
 * ------------------------------------------------------------------------- */
static void collect(token_buf_t *tb, const md_out_t *out, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        tb_append(tb, &out[i]);
    }
}

/* -------------------------------------------------------------------------
 * Run one test case; return 1 on pass, 0 on fail.
 * ------------------------------------------------------------------------- */
static int run_case(const mv_case_t *tc) {
    md_decoder_t dec;
    md_mode_t    mode = (tc->mode == 1) ? MD_DUAL : MD_SINGLE;
    md_init(&dec, tc->unit_ms, mode, false);

    token_buf_t tb;
    tb_init(&tb);

    md_out_t out[4]; /* at most 2 per call; 4 is generous */
    uint32_t t = 1000u;

    for (uint16_t i = 0; i < tc->n_signals; i++) {
        const mv_signal_t *sig = &tc->signals[i];
        md_key_t key = (md_key_t)sig->key; /* 0=SINGLE, 1=DIT, 2=DAH */

        /* keyDown */
        uint8_t n = md_key_down(&dec, t, key, out, 4);
        collect(&tb, out, n);

        /* keyUp */
        t += sig->on_ms;
        n = md_key_up(&dec, t, key, out, 4);
        collect(&tb, out, n);

        /* tick */
        t += sig->off_ms;
        n = md_tick(&dec, t, out, 4);
        collect(&tb, out, n);
    }

    /* flush */
    uint8_t n = md_flush(&dec, out, 4);
    collect(&tb, out, n);

    /* Compare */
    if (strcmp(tb.buf, tc->expected) == 0) {
        return 1;
    }

    fprintf(stderr, "FAIL  %s\n", tc->id);
    fprintf(stderr, "  expected: %s\n", tc->expected);
    fprintf(stderr, "  actual:   %s\n", tb.buf);
    return 0;
}

/* -------------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------------- */
int main(void) {
    int passed = 0;
    int failed = 0;

    for (int i = 0; i < MV_CASE_COUNT; i++) {
        if (run_case(&MV_CASES[i])) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("%d/%d passed", passed, MV_CASE_COUNT);
    if (failed > 0) {
        printf(", %d FAILED\n", failed);
        return 1;
    }
    printf("\n");
    return 0;
}
