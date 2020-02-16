/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _UECC_CURVE_SPECIFIC_H_
#define _UECC_CURVE_SPECIFIC_H_

#define num_bytes_secp256k1 32

#if (uECC_WORD_SIZE == 1)

#define num_words_secp256k1 32

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) \
    0x##a, 0x##b, 0x##c, 0x##d, 0x##e, 0x##f, 0x##g, 0x##h

#elif (uECC_WORD_SIZE == 4)

#define num_words_secp256k1 8

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) 0x##d##c##b##a, 0x##h##g##f##e

#elif (uECC_WORD_SIZE == 8)

#define num_words_secp256k1 4

#define BYTES_TO_WORDS_8(a, b, c, d, e, f, g, h) 0x##h##g##f##e##d##c##b##a##ull

#endif /* uECC_WORD_SIZE */

/* Computes result = x^3 + ax + b. result must not overlap x. */
static void x_side_default(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve) {
    uECC_word_t _3[uECC_MAX_WORDS] = {3}; /* -a = 3 */
    wordcount_t num_words = curve->num_words;

    uECC_vli_modSquare_fast(result, x, curve);                             /* r = x^2 */
    uECC_vli_modSub(result, result, _3, curve->p, num_words);       /* r = x^2 - 3 */
    uECC_vli_modMult_fast(result, result, x, curve);                       /* r = x^3 - 3x */
    uECC_vli_modAdd(result, result, curve->b, curve->p, num_words); /* r = x^3 - 3x + b */
}
#endif /* uECC_SUPPORTS_secp... */

#if uECC_SUPPORT_COMPRESSED_POINT
#if uECC_SUPPORTS_secp256k1
/* Compute a = sqrt(a) (mod curve_p). */
static void mod_sqrt_default(uECC_word_t *a, uECC_Curve curve) {
    bitcount_t i;
    uECC_word_t p1[uECC_MAX_WORDS] = {1};
    uECC_word_t l_result[uECC_MAX_WORDS] = {1};
    wordcount_t num_words = curve->num_words;

    /* When curve->p == 3 (mod 4), we can compute
       sqrt(a) = a^((curve->p + 1) / 4) (mod curve->p). */
    uECC_vli_add(p1, curve->p, p1, num_words); /* p1 = curve_p + 1 */
    for (i = uECC_vli_numBits(p1, num_words) - 1; i > 1; --i) {
        uECC_vli_modSquare_fast(l_result, l_result, curve);
        if (uECC_vli_testBit(p1, i)) {
            uECC_vli_modMult_fast(l_result, l_result, a, curve);
        }
    }
    uECC_vli_set(a, l_result, num_words);
}
#endif /* uECC_SUPPORTS_secp... */
#endif /* uECC_SUPPORT_COMPRESSED_POINT */


#if uECC_SUPPORTS_secp256k1

static void x_side_secp256k1(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve);
#if (uECC_OPTIMIZATION_LEVEL > 0)
static void vli_mmod_fast_secp256k1(uECC_word_t *result, uECC_word_t *product);
#endif

static const struct uECC_Curve_t curve_secp256k1 = {
    num_words_secp256k1,
    num_bytes_secp256k1,
    256, /* num_n_bits */
    { BYTES_TO_WORDS_8(2F, FC, FF, FF, FE, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(41, 41, 36, D0, 8C, 5E, D2, BF),
        BYTES_TO_WORDS_8(3B, A0, 48, AF, E6, DC, AE, BA),
        BYTES_TO_WORDS_8(FE, FF, FF, FF, FF, FF, FF, FF),
        BYTES_TO_WORDS_8(FF, FF, FF, FF, FF, FF, FF, FF) },
    { BYTES_TO_WORDS_8(98, 17, F8, 16, 5B, 81, F2, 59),
        BYTES_TO_WORDS_8(D9, 28, CE, 2D, DB, FC, 9B, 02),
        BYTES_TO_WORDS_8(07, 0B, 87, CE, 95, 62, A0, 55),
        BYTES_TO_WORDS_8(AC, BB, DC, F9, 7E, 66, BE, 79),

        BYTES_TO_WORDS_8(B8, D4, 10, FB, 8F, D0, 47, 9C),
        BYTES_TO_WORDS_8(19, 54, 85, A6, 48, B4, 17, FD),
        BYTES_TO_WORDS_8(A8, 08, 11, 0E, FC, FB, A4, 5D),
        BYTES_TO_WORDS_8(65, C4, A3, 26, 77, DA, 3A, 48) },
    { BYTES_TO_WORDS_8(07, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00),
        BYTES_TO_WORDS_8(00, 00, 00, 00, 00, 00, 00, 00) },
#if uECC_SUPPORT_COMPRESSED_POINT
    &mod_sqrt_default,
#endif
    &x_side_secp256k1,
#if (uECC_OPTIMIZATION_LEVEL > 0)
    &vli_mmod_fast_secp256k1
#endif
};

inline uECC_Curve uECC_secp256k1(void) { return &curve_secp256k1; }

/* Computes result = x^3 + b. result must not overlap x. */
static void x_side_secp256k1(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve) {
    uECC_vli_modSquare_fast(result, x, curve);                                /* r = x^2 */
    uECC_vli_modMult_fast(result, result, x, curve);                          /* r = x^3 */
    uECC_vli_modAdd(result, result, curve->b, curve->p, num_words_secp256k1); /* r = x^3 + b */
}

#if (uECC_OPTIMIZATION_LEVEL > 0 && !asm_mmod_fast_secp256k1)
static void omega_mult_secp256k1(uECC_word_t *result, const uECC_word_t *right);
static void vli_mmod_fast_secp256k1(uECC_word_t *result, uECC_word_t *product) {
    uECC_word_t tmp[2 * num_words_secp256k1];
    uECC_word_t carry;

    uECC_vli_clear(tmp, num_words_secp256k1);
    uECC_vli_clear(tmp + num_words_secp256k1, num_words_secp256k1);

    omega_mult_secp256k1(tmp, product + num_words_secp256k1); /* (Rq, q) = q * c */

    carry = uECC_vli_add(result, product, tmp, num_words_secp256k1); /* (C, r) = r + q       */
    uECC_vli_clear(product, num_words_secp256k1);
    omega_mult_secp256k1(product, tmp + num_words_secp256k1); /* Rq*c */
    carry += uECC_vli_add(result, result, product, num_words_secp256k1); /* (C1, r) = r + Rq*c */

    while (carry > 0) {
        --carry;
        uECC_vli_sub(result, result, curve_secp256k1.p, num_words_secp256k1);
    }
    if (uECC_vli_cmp_unsafe(result, curve_secp256k1.p, num_words_secp256k1) > 0) {
        uECC_vli_sub(result, result, curve_secp256k1.p, num_words_secp256k1);
    }
}

#if uECC_WORD_SIZE == 1
static void omega_mult_secp256k1(uint8_t * result, const uint8_t * right) {
    /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t k;

    /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    muladd(0xD1, right[0], &r0, &r1, &r2);
    result[0] = r0;
    r0 = r1;
    r1 = r2;
    /* r2 is still 0 */

    for (k = 1; k < num_words_secp256k1; ++k) {
        muladd(0x03, right[k - 1], &r0, &r1, &r2);
        muladd(0xD1, right[k], &r0, &r1, &r2);
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    muladd(0x03, right[num_words_secp256k1 - 1], &r0, &r1, &r2);
    result[num_words_secp256k1] = r0;
    result[num_words_secp256k1 + 1] = r1;
    /* add the 2^32 multiple */
    result[4 + num_words_secp256k1] =
        uECC_vli_add(result + 4, result + 4, right, num_words_secp256k1);
}
#elif uECC_WORD_SIZE == 4
static void omega_mult_secp256k1(uint32_t * result, const uint32_t * right) {
    /* Multiply by (2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    uint32_t carry = 0;
    wordcount_t k;

    for (k = 0; k < num_words_secp256k1; ++k) {
        uint64_t p = (uint64_t)0x3D1 * right[k] + carry;
        result[k] = (uint32_t) p;
        carry = p >> 32;
    }
    result[num_words_secp256k1] = carry;
    /* add the 2^32 multiple */
    result[1 + num_words_secp256k1] =
        uECC_vli_add(result + 1, result + 1, right, num_words_secp256k1);
}
#else
static void omega_mult_secp256k1(uint64_t * result, const uint64_t * right) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t k;

    /* Multiply by (2^32 + 2^9 + 2^8 + 2^7 + 2^6 + 2^4 + 1). */
    for (k = 0; k < num_words_secp256k1; ++k) {
        muladd(0x1000003D1ull, right[k], &r0, &r1, &r2);
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    result[num_words_secp256k1] = r0;
}
#endif /* uECC_WORD_SIZE */
#endif /* (uECC_OPTIMIZATION_LEVEL > 0 &&  && !asm_mmod_fast_secp256k1) */

#endif /* uECC_SUPPORTS_secp256k1 */
