/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#include "uECC.h"
#include "uECC_vli.h"

#if uECC_ENABLE_VLI_API
    #define uECC_VLI_API
#else
    #define uECC_VLI_API static
#endif

#if (uECC_WORD_SIZE == 1)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 21 /* Due to the size of curve_n. */
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 24
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 28
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 32
    #endif
#elif (uECC_WORD_SIZE == 4)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 6 /* Due to the size of curve_n. */
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 6
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 7
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 8
    #endif
#elif (uECC_WORD_SIZE == 8)
    #if uECC_SUPPORTS_secp160r1
        #define uECC_MAX_WORDS 3
    #endif
    #if uECC_SUPPORTS_secp192r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 3
    #endif
    #if uECC_SUPPORTS_secp224r1
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
    #if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
        #undef uECC_MAX_WORDS
        #define uECC_MAX_WORDS 4
    #endif
#endif /* uECC_WORD_SIZE */

#define BITS_TO_WORDS(num_bits) ((num_bits + ((uECC_WORD_SIZE * 8) - 1)) / (uECC_WORD_SIZE * 8))
#define BITS_TO_BYTES(num_bits) ((num_bits + 7) / 8)

struct uECC_Curve_t {
    wordcount_t num_words;
    wordcount_t num_bytes;
    bitcount_t num_n_bits;
    uECC_word_t p[uECC_MAX_WORDS];
    uECC_word_t n[uECC_MAX_WORDS];
    uECC_word_t G[uECC_MAX_WORDS * 2];
    uECC_word_t b[uECC_MAX_WORDS];
#if uECC_SUPPORT_COMPRESSED_POINT
    void (*mod_sqrt)(uECC_word_t *a, uECC_Curve curve);
#endif
    void (*x_side)(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    void (*mmod_fast)(uECC_word_t *result, uECC_word_t *product);
#endif
};

#if uECC_VLI_NATIVE_LITTLE_ENDIAN
static void bcopy(uint8_t *dst,
                  const uint8_t *src,
                  unsigned num_bytes) {
    while (0 != num_bytes) {
        num_bytes--;
        dst[num_bytes] = src[num_bytes];
    }
}
#endif

static cmpresult_t uECC_vli_cmp_unsafe(const uECC_word_t *left,
                                       const uECC_word_t *right,
                                       wordcount_t num_words);

#if (uECC_PLATFORM == uECC_arm || uECC_PLATFORM == uECC_arm_thumb || \
        uECC_PLATFORM == uECC_arm_thumb2)
    #include "asm_arm.inc"
#endif

#if (uECC_PLATFORM == uECC_avr)
    #include "asm_avr.inc"
#endif

#if !asm_clear
uECC_VLI_API void uECC_vli_clear(uECC_word_t *vli, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        vli[i] = 0;
    }
}
#endif /* !asm_clear */

/* Constant-time comparison to zero - secure way to compare long integers */
/* Returns 1 if vli == 0, 0 otherwise. */
uECC_VLI_API uECC_word_t uECC_vli_isZero(const uECC_word_t *vli, wordcount_t num_words) {
    uECC_word_t bits = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        bits |= vli[i];
    }
    return (bits == 0);
}

/* Returns nonzero if bit 'bit' of vli is set. */
uECC_VLI_API uECC_word_t uECC_vli_testBit(const uECC_word_t *vli, bitcount_t bit) {
    return (vli[bit >> uECC_WORD_BITS_SHIFT] & ((uECC_word_t)1 << (bit & uECC_WORD_BITS_MASK)));
}

/* Counts the number of words in vli. */
static wordcount_t vli_numDigits(const uECC_word_t *vli, const wordcount_t max_words) {
    wordcount_t i;
    /* Search from the end until we find a non-zero digit.
       We do it in reverse because we expect that most digits will be nonzero. */
    for (i = max_words - 1; i >= 0 && vli[i] == 0; --i) {
    }

    return (i + 1);
}

/* Counts the number of bits required to represent vli. */
uECC_VLI_API bitcount_t uECC_vli_numBits(const uECC_word_t *vli, const wordcount_t max_words) {
    uECC_word_t i;
    uECC_word_t digit;

    wordcount_t num_digits = vli_numDigits(vli, max_words);
    if (num_digits == 0) {
        return 0;
    }

    digit = vli[num_digits - 1];
    for (i = 0; digit; ++i) {
        digit >>= 1;
    }

    return (((bitcount_t)(num_digits - 1) << uECC_WORD_BITS_SHIFT) + i);
}

/* Sets dest = src. */
#if !asm_set
uECC_VLI_API void uECC_vli_set(uECC_word_t *dest, const uECC_word_t *src, wordcount_t num_words) {
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        dest[i] = src[i];
    }
}
#endif /* !asm_set */

/* Returns sign of left - right. */
static cmpresult_t uECC_vli_cmp_unsafe(const uECC_word_t *left,
                                       const uECC_word_t *right,
                                       wordcount_t num_words) {
    wordcount_t i;
    for (i = num_words - 1; i >= 0; --i) {
        if (left[i] > right[i]) {
            return 1;
        } else if (left[i] < right[i]) {
            return -1;
        }
    }
    return 0;
}

uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words);

/* Returns sign of left - right, in constant time. */
uECC_VLI_API cmpresult_t uECC_vli_cmp(const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t tmp[uECC_MAX_WORDS];
    uECC_word_t neg = !!uECC_vli_sub(tmp, left, right, num_words);
    uECC_word_t equal = uECC_vli_isZero(tmp, num_words);
    return (!equal - 2 * neg);
}

/* Computes vli = vli >> 1. */
#if !asm_rshift1
uECC_VLI_API void uECC_vli_rshift1(uECC_word_t *vli, wordcount_t num_words) {
    uECC_word_t *end = vli;
    uECC_word_t carry = 0;

    vli += num_words;
    while (vli-- > end) {
        uECC_word_t temp = *vli;
        *vli = (temp >> 1) | carry;
        carry = temp << (uECC_WORD_BITS - 1);
    }
}
#endif /* !asm_rshift1 */

/* Computes result = left + right, returning carry. Can modify in place. */
#if !asm_add
uECC_VLI_API uECC_word_t uECC_vli_add(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t carry = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        uECC_word_t sum = left[i] + right[i] + carry;
        if (sum != left[i]) {
            carry = (sum < left[i]);
        }
        result[i] = sum;
    }
    return carry;
}
#endif /* !asm_add */

/* Computes result = left - right, returning borrow. Can modify in place. */
#if !asm_sub
uECC_VLI_API uECC_word_t uECC_vli_sub(uECC_word_t *result,
                                      const uECC_word_t *left,
                                      const uECC_word_t *right,
                                      wordcount_t num_words) {
    uECC_word_t borrow = 0;
    wordcount_t i;
    for (i = 0; i < num_words; ++i) {
        uECC_word_t diff = left[i] - right[i] - borrow;
        if (diff != left[i]) {
            borrow = (diff > left[i]);
        }
        result[i] = diff;
    }
    return borrow;
}
#endif /* !asm_sub */

#if !asm_mult || (uECC_SQUARE_FUNC && !asm_square) || \
    (uECC_SUPPORTS_secp256k1 && (uECC_OPTIMIZATION_LEVEL > 0) && \
        ((uECC_WORD_SIZE == 1) || (uECC_WORD_SIZE == 8)))
static void muladd(uECC_word_t a,
                   uECC_word_t b,
                   uECC_word_t *r0,
                   uECC_word_t *r1,
                   uECC_word_t *r2) {
#if uECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1) { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    uECC_dword_t p = (uECC_dword_t)a * b;
    uECC_dword_t r01 = ((uECC_dword_t)(*r1) << uECC_WORD_BITS) | *r0;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> uECC_WORD_BITS;
    *r0 = (uECC_word_t)r01;
#endif
}
#endif /* muladd needed */

#if !asm_mult
uECC_VLI_API void uECC_vli_mult(uECC_word_t *result,
                                const uECC_word_t *left,
                                const uECC_word_t *right,
                                wordcount_t num_words) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;
    wordcount_t i, k;

    /* Compute each digit of result in sequence, maintaining the carries. */
    for (k = 0; k < num_words; ++k) {
        for (i = 0; i <= k; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    for (k = num_words; k < num_words * 2 - 1; ++k) {
        for (i = (k + 1) - num_words; i < num_words; ++i) {
            muladd(left[i], right[k - i], &r0, &r1, &r2);
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_mult */

#if uECC_SQUARE_FUNC

#if !asm_square
static void mul2add(uECC_word_t a,
                    uECC_word_t b,
                    uECC_word_t *r0,
                    uECC_word_t *r1,
                    uECC_word_t *r2) {
#if uECC_WORD_SIZE == 8 && !SUPPORTS_INT128
    uint64_t a0 = a & 0xffffffffull;
    uint64_t a1 = a >> 32;
    uint64_t b0 = b & 0xffffffffull;
    uint64_t b1 = b >> 32;

    uint64_t i0 = a0 * b0;
    uint64_t i1 = a0 * b1;
    uint64_t i2 = a1 * b0;
    uint64_t i3 = a1 * b1;

    uint64_t p0, p1;

    i2 += (i0 >> 32);
    i2 += i1;
    if (i2 < i1)
    { /* overflow */
        i3 += 0x100000000ull;
    }

    p0 = (i0 & 0xffffffffull) | (i2 << 32);
    p1 = i3 + (i2 >> 32);

    *r2 += (p1 >> 63);
    p1 = (p1 << 1) | (p0 >> 63);
    p0 <<= 1;

    *r0 += p0;
    *r1 += (p1 + (*r0 < p0));
    *r2 += ((*r1 < p1) || (*r1 == p1 && *r0 < p0));
#else
    uECC_dword_t p = (uECC_dword_t)a * b;
    uECC_dword_t r01 = ((uECC_dword_t)(*r1) << uECC_WORD_BITS) | *r0;
    *r2 += (p >> (uECC_WORD_BITS * 2 - 1));
    p *= 2;
    r01 += p;
    *r2 += (r01 < p);
    *r1 = r01 >> uECC_WORD_BITS;
    *r0 = (uECC_word_t)r01;
#endif
}

uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  wordcount_t num_words) {
    uECC_word_t r0 = 0;
    uECC_word_t r1 = 0;
    uECC_word_t r2 = 0;

    wordcount_t i, k;

    for (k = 0; k < num_words * 2 - 1; ++k) {
        uECC_word_t min = (k < num_words ? 0 : (k + 1) - num_words);
        for (i = min; i <= k && i <= k - i; ++i) {
            if (i < k-i) {
                mul2add(left[i], left[k - i], &r0, &r1, &r2);
            } else {
                muladd(left[i], left[k - i], &r0, &r1, &r2);
            }
        }
        result[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }

    result[num_words * 2 - 1] = r0;
}
#endif /* !asm_square */

#else /* uECC_SQUARE_FUNC */

#if uECC_ENABLE_VLI_API
uECC_VLI_API void uECC_vli_square(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  wordcount_t num_words) {
    uECC_vli_mult(result, left, left, num_words);
}
#endif /* uECC_ENABLE_VLI_API */

#endif /* uECC_SQUARE_FUNC */

/* Computes result = (left + right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void uECC_vli_modAdd(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  const uECC_word_t *right,
                                  const uECC_word_t *mod,
                                  wordcount_t num_words) {
    uECC_word_t carry = uECC_vli_add(result, left, right, num_words);
    if (carry || uECC_vli_cmp_unsafe(mod, result, num_words) != 1) {
        /* result > mod (result = mod + remainder), so subtract mod to get remainder. */
        uECC_vli_sub(result, result, mod, num_words);
    }
}

/* Computes result = (left - right) % mod.
   Assumes that left < mod and right < mod, and that result does not overlap mod. */
uECC_VLI_API void uECC_vli_modSub(uECC_word_t *result,
                                  const uECC_word_t *left,
                                  const uECC_word_t *right,
                                  const uECC_word_t *mod,
                                  wordcount_t num_words) {
    uECC_word_t l_borrow = uECC_vli_sub(result, left, right, num_words);
    if (l_borrow) {
        /* In this case, result == -diff == (max int) - diff. Since -x % d == d - x,
           we can get the correct result from result + mod (with overflow). */
        uECC_vli_add(result, result, mod, num_words);
    }
}

/* Computes result = product % mod, where product is 2N words long. */
/* Currently only designed to work for curve_p or curve_n. */
uECC_VLI_API void uECC_vli_mmod(uECC_word_t *result,
                                uECC_word_t *product,
                                const uECC_word_t *mod,
                                wordcount_t num_words) {
    uECC_word_t mod_multiple[2 * uECC_MAX_WORDS];
    uECC_word_t tmp[2 * uECC_MAX_WORDS];
    uECC_word_t *v[2] = {tmp, product};
    uECC_word_t index;

    /* Shift mod so its highest set bit is at the maximum position. */
    bitcount_t shift = (num_words * 2 * uECC_WORD_BITS) - uECC_vli_numBits(mod, num_words);
    wordcount_t word_shift = shift / uECC_WORD_BITS;
    wordcount_t bit_shift = shift % uECC_WORD_BITS;
    uECC_word_t carry = 0;
    uECC_vli_clear(mod_multiple, word_shift);
    if (bit_shift > 0) {
        for(index = 0; index < (uECC_word_t)num_words; ++index) {
            mod_multiple[word_shift + index] = (mod[index] << bit_shift) | carry;
            carry = mod[index] >> (uECC_WORD_BITS - bit_shift);
        }
    } else {
        uECC_vli_set(mod_multiple + word_shift, mod, num_words);
    }

    for (index = 1; shift >= 0; --shift) {
        uECC_word_t borrow = 0;
        wordcount_t i;
        for (i = 0; i < num_words * 2; ++i) {
            uECC_word_t diff = v[index][i] - mod_multiple[i] - borrow;
            if (diff != v[index][i]) {
                borrow = (diff > v[index][i]);
            }
            v[1 - index][i] = diff;
        }
        index = !(index ^ borrow); /* Swap the index if there was no borrow */
        uECC_vli_rshift1(mod_multiple, num_words);
        mod_multiple[num_words - 1] |= mod_multiple[num_words] << (uECC_WORD_BITS - 1);
        uECC_vli_rshift1(mod_multiple + num_words, num_words);
    }
    uECC_vli_set(result, v[index], num_words);
}

/* Computes result = (left * right) % mod. */
uECC_VLI_API void uECC_vli_modMult(uECC_word_t *result,
                                   const uECC_word_t *left,
                                   const uECC_word_t *right,
                                   const uECC_word_t *mod,
                                   wordcount_t num_words) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_mult(product, left, right, num_words);
    uECC_vli_mmod(result, product, mod, num_words);
}

uECC_VLI_API void uECC_vli_modMult_fast(uECC_word_t *result,
                                        const uECC_word_t *left,
                                        const uECC_word_t *right,
                                        uECC_Curve curve) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_mult(product, left, right, curve->num_words);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    uECC_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#if uECC_SQUARE_FUNC
uECC_VLI_API void uECC_vli_modSquare_fast(uECC_word_t *result,
                                          const uECC_word_t *left,
                                          uECC_Curve curve) {
    uECC_word_t product[2 * uECC_MAX_WORDS];
    uECC_vli_square(product, left, curve->num_words);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    curve->mmod_fast(result, product);
#else
    uECC_vli_mmod(result, product, curve->p, curve->num_words);
#endif
}

#else /* uECC_SQUARE_FUNC */

uECC_VLI_API void uECC_vli_modSquare_fast(uECC_word_t *result,
                                          const uECC_word_t *left,
                                          uECC_Curve curve) {
    uECC_vli_modMult_fast(result, left, left, curve);
}

#endif /* uECC_SQUARE_FUNC */

#define EVEN(vli) (!(vli[0] & 1))
static void vli_modInv_update(uECC_word_t *uv,
                              const uECC_word_t *mod,
                              wordcount_t num_words) {
    uECC_word_t carry = 0;
    if (!EVEN(uv)) {
        carry = uECC_vli_add(uv, uv, mod, num_words);
    }
    uECC_vli_rshift1(uv, num_words);
    if (carry) {
        uv[num_words - 1] |= HIGH_BIT_SET;
    }
}

/* ------ Point operations ------ */

#include "curve-specific.inc"

#if uECC_WORD_SIZE == 1

uECC_VLI_API void uECC_vli_nativeToBytes(uint8_t *bytes,
                                         int num_bytes,
                                         const uint8_t *native) {
    wordcount_t i;
    for (i = 0; i < num_bytes; ++i) {
        bytes[i] = native[(num_bytes - 1) - i];
    }
}

uECC_VLI_API void uECC_vli_bytesToNative(uint8_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    uECC_vli_nativeToBytes(native, num_bytes, bytes);
}

#else

uECC_VLI_API void uECC_vli_nativeToBytes(uint8_t *bytes,
                                         int num_bytes,
                                         const uECC_word_t *native) {
    wordcount_t i;
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        bytes[i] = native[b / uECC_WORD_SIZE] >> (8 * (b % uECC_WORD_SIZE));
    }
}

uECC_VLI_API void uECC_vli_bytesToNative(uECC_word_t *native,
                                         const uint8_t *bytes,
                                         int num_bytes) {
    wordcount_t i;
    uECC_vli_clear(native, (num_bytes + (uECC_WORD_SIZE - 1)) / uECC_WORD_SIZE);
    for (i = 0; i < num_bytes; ++i) {
        unsigned b = num_bytes - 1 - i;
        native[b / uECC_WORD_SIZE] |=
            (uECC_word_t)bytes[i] << (8 * (b % uECC_WORD_SIZE));
    }
}

#endif /* uECC_WORD_SIZE */

#if uECC_SUPPORT_COMPRESSED_POINT
inline void uECC_compress(const uint8_t *public_key, uint8_t *compressed, uECC_Curve curve) {
    wordcount_t i;
    for (i = 0; i < curve->num_bytes; ++i) {
        compressed[i+1] = public_key[i];
    }
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    compressed[0] = 2 + (public_key[curve->num_bytes] & 0x01);
#else
    compressed[0] = 2 + (public_key[curve->num_bytes * 2 - 1] & 0x01);
#endif
}

inline void uECC_decompress(const uint8_t *compressed, uint8_t *public_key, uECC_Curve curve) {
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    uECC_word_t *point = (uECC_word_t *)public_key;
#else
    uECC_word_t point[uECC_MAX_WORDS * 2];
#endif
    uECC_word_t *y = point + curve->num_words;
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
    bcopy(public_key, compressed+1, curve->num_bytes);
#else
    uECC_vli_bytesToNative(point, compressed + 1, curve->num_bytes);
#endif
    curve->x_side(y, point, curve);
    curve->mod_sqrt(y, curve);

    if ((y[0] & 0x01) != (compressed[0] & 0x01)) {
        uECC_vli_sub(y, curve->p, y, curve->num_words);
    }

#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    uECC_vli_nativeToBytes(public_key, curve->num_bytes, point);
    uECC_vli_nativeToBytes(public_key + curve->num_bytes, curve->num_bytes, y);
#endif
}
#endif /* uECC_SUPPORT_COMPRESSED_POINT */

/* -------- ECDSA code -------- */
#if uECC_ENABLE_VLI_API
const uECC_word_t *uECC_curve_n(uECC_Curve curve) {
    return curve->n;
}
#endif /* uECC_ENABLE_VLI_API */
