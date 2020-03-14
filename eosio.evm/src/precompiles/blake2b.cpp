#include <eosio.evm/eosio.evm.hpp>

/*
   BLAKE2 reference source code package - reference C implementations
   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
   terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
   your option.  The terms of these licenses can be found at:
   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0
   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/

struct blake2b_state
{
  uint64_t h[8];
  uint64_t t[2];
  uint64_t f[2];
  uint8_t  buf[128];
};

static inline uint64_t load64( const void *src )
{
  const uint8_t *p = ( const uint8_t * )src;
  return (( uint64_t )( p[0] ) <<  0) |
         (( uint64_t )( p[1] ) <<  8) |
         (( uint64_t )( p[2] ) << 16) |
         (( uint64_t )( p[3] ) << 24) |
         (( uint64_t )( p[4] ) << 32) |
         (( uint64_t )( p[5] ) << 40) |
         (( uint64_t )( p[6] ) << 48) |
         (( uint64_t )( p[7] ) << 56) ;
}

static inline uint64_t rotr64( const uint64_t w, const unsigned c )
{
  return ( w >> c ) | ( w << ( 64 - c ) );
}

static const uint64_t blake2b_IV[8] =
{
  0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t blake2b_sigma[12][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

inline void blake2b_g(const uint64_t& r, const uint64_t* m, const uint8_t i, uint64_t& a, uint64_t& b, uint64_t& c, uint64_t& d) {
  a = a + b + m[blake2b_sigma[r][2*i+0]];
  d = rotr64(d ^ a, 32);
  c = c + d;
  b = rotr64(b ^ c, 24);
  a = a + b + m[blake2b_sigma[r][2*i+1]];
  d = rotr64(d ^ a, 16);
  c = c + d;
  b = rotr64(b ^ c, 63);
}

static constexpr uint64_t blr[8][4] = {
  { 0, 4,  8, 12},
  { 1, 5,  9, 13},
  { 2, 6, 10, 14},
  { 3, 7, 11, 15},
  { 0, 5, 10, 15},
  { 1, 6, 11, 12},
  { 2, 7,  8, 13},
  { 3, 4,  9, 14}
};

inline void blake2b_round (const uint32_t& r, const uint64_t* m, uint64_t* v) {
  for ( uint8_t i = 0; i < 8; ++i ) {
    blake2b_g(r % 10, m, i, v[blr[i][0]], v[blr[i][1]], v[blr[i][2]], v[blr[i][3]]);
  }
}

inline void blake2b( const uint32_t rounds, blake2b_state *S )
{
  uint64_t m[16];
  uint64_t v[16];

  for( auto i = 0; i < 16; ++i ) {
    m[i] = load64( S->buf + i * sizeof( m[i] ) );
  }

  for( auto i = 0; i < 8; ++i ) {
    v[i] = S->h[i];
  }

  v[ 8] = blake2b_IV[0];
  v[ 9] = blake2b_IV[1];
  v[10] = blake2b_IV[2];
  v[11] = blake2b_IV[3];
  v[12] = blake2b_IV[4] ^ S->t[0];
  v[13] = blake2b_IV[5] ^ S->t[1];
  v[14] = blake2b_IV[6] ^ S->f[0];
  v[15] = blake2b_IV[7] ^ S->f[1];

  for ( auto round = 0; round < rounds; ++round ) {
    blake2b_round(round, m, v);
  }

  for( auto i = 0; i < 8; ++i ) {
    S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
  }
}

// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
namespace eosio_evm
{
  void Processor::precompile_blake2b()
  {
    // Validate size
    if (ctx->input.size() != 213) {
      throw_error(Exception(ET::OOB, "Out of Bounds"), {});
      return;
    }

    // Validate last byte
    if (ctx->input[212] > 1) {
      throw_error(Exception(ET::OOB, "Out of Bounds"), {});
      return;
    }

    // Get rounds
    uint8_t rounds_bytes[4];
    std::copy(ctx->input.begin(), ctx->input.begin() + 4, std::begin(rounds_bytes));
    uint32_t rounds = static_cast<uint32_t>(intx::be::load<uint256_t>(rounds_bytes));

    // Charge gas
    bool error = use_gas(GP_BLAKE2_ROUND * rounds);
    if (error) return;

    // Construct state
    blake2b_state state = {};
    std::memcpy(&state.h[0], &ctx->input[4], 64);
    std::memcpy(&state.buf[0], &ctx->input[68], 128);
    std::memcpy(&state.t[0], &ctx->input[196], 16);

    // Final
    if (ctx->input[212] == 1) {
      state.f[0] = std::numeric_limits<uint64_t>::max();
    }

    // BLAKE
    blake2b(rounds, &state);

    // Copy result
    std::vector<uint8_t> result(sizeof(state.h));
    std::memcpy(&result[0], std::begin(state.h), result.size());

    // Return
    precompile_return(result);
  }
} // namespace eosio_evm
