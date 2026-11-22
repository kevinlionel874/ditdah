/* morse_decoder.h — C99 Morse decoder for DitDah
 *
 * Plain C99. No dynamic memory. No HAL includes.
 * State is opaque; callers allocate md_decoder_t on the stack or in a struct.
 */
#ifndef MORSE_DECODER_H
#define MORSE_DECODER_H

#include <stdint.h>
#include <stdbool.h>

/* Operating mode ---------------------------------------------------------- */
typedef enum { MD_SINGLE = 0, MD_DUAL = 1 } md_mode_t;

/* Key identity (only used in dual mode) ------------------------------------ */
typedef enum { MD_KEY_SINGLE = 0, MD_KEY_DIT = 1, MD_KEY_DAH = 2 } md_key_t;

/* Output kinds ------------------------------------------------------------- */
typedef enum {
    MD_OUT_CHAR,
    MD_OUT_SPACE,
    MD_OUT_BACKSPACE,
    MD_OUT_ENTER,
    MD_OUT_ACCEPT,
    MD_OUT_UNKNOWN
} md_kind_t;

/* One output token --------------------------------------------------------- */
typedef struct {
    md_kind_t kind;
    char      ch;   /* non-zero only when kind == MD_OUT_CHAR */
} md_out_t;

/* Opaque decoder state (64 bytes max) -------------------------------------- */
typedef union  { uint32_t _align; uint8_t _opaque[64]; } md_decoder_t;

/* API ---------------------------------------------------------------------- */

/**
 * Initialise the decoder.
 * @param d        Decoder to initialise (zeroed by this function).
 * @param unit_ms  Unit length in ms. 0 → default 120. Clamped to 48–200.
 * @param mode     MD_SINGLE or MD_DUAL.
 * @param adaptive Enable adaptive speed tracking.
 */
void md_init(md_decoder_t *d, uint16_t unit_ms, md_mode_t mode, bool adaptive);

/**
 * Signal key-down at timestamp now_ms.
 * Runs the silence check first; may output a completed letter or word-space.
 * Returns number of outputs written (0–2).
 */
uint8_t md_key_down(md_decoder_t *d, uint32_t now_ms, md_key_t key,
                    md_out_t *out, uint8_t max_out);

/**
 * Signal key-up at timestamp now_ms.
 * Classifies the press as dit or dah and appends it to the current pattern.
 * Returns 0 (outputs arrive at the next silence check).
 */
uint8_t md_key_up(md_decoder_t *d, uint32_t now_ms, md_key_t key,
                  md_out_t *out, uint8_t max_out);

/**
 * Poll the decoder at timestamp now_ms (call regularly during silence).
 * Runs the silence check; may output a completed letter or word-space.
 * Returns number of outputs written (0–2).
 */
uint8_t md_tick(md_decoder_t *d, uint32_t now_ms,
                md_out_t *out, uint8_t max_out);

/**
 * Flush the in-progress letter (if any) without waiting for a silence.
 * Does NOT output a word-space.
 * Returns 0 or 1.
 */
uint8_t md_flush(md_decoder_t *d, md_out_t *out, uint8_t max_out);

/** Return the current unit in ms. */
uint16_t md_unit_ms(const md_decoder_t *d);

#endif /* MORSE_DECODER_H */
