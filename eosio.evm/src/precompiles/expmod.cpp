// rust-evm.
// Copyright 2020 Stewart Mackenzie, Wei Tang, Matt Brubeck, Isaac Ardis, Elaine Ou.
// Licensed under the Apache License, Version 2.0.
// https://github.com/sorpaas/rust-evm/blob/dcd6e79a8a8ddaf955efe58131c6835e3aa59cfc/precompiled/modexp/src/lib.rs
// e-wasm
// Copyright 2020 e-wasm team
// Licensed under the Apache License, Version 2.0.
// https://github.com/ewasm/ewasm-precompiles/blob/master/modexp/src/lib.rs

#include <eosio.evm/eosio.evm.hpp>
namespace eosio_evm
{
  uint256_t Processor::mult_complexity(const uint256_t& len) {
    uint256_t term1 = len * len;
    uint256_t term2 = 0;
    uint256_t term3 = 0;

    if (len > 1024) {
      term1 /= 16;
      term2 = 46080;
      term3 = 199680;
    } else if (len > 64) {
      term1 /= 4;
      term2 = len * 96;
      term3 = 3072;
    }

    return term1 + term2 - term3;
  }

  uint256_t Processor::read_input(uint64_t offset, uint64_t length) {
    // Read past end
    if (offset >= ctx->input.size()) {
      return {};
    }

    uint8_t data[32] = {};
    auto end = (offset + length) > ctx->input.size() ? ctx->input.size() : offset + length;
    auto data_offset = (end - offset) < 32 ? 32 - (end - offset) : 0;
    std::copy(std::begin(ctx->input) + offset, std::begin(ctx->input) + end, std::begin(data) + data_offset );

    auto padding = (length - (end - offset)) * 8;
    uint256_t result = intx::be::load<uint256_t>(data) << padding;

    return result;
  }

  BN Processor::read_input_large(uint256_t off, uint256_t len, Context* ctx) {
    // Read past end
    if (off >= ctx->input.size()) {
      return {};
    }

    // Bound
    uint64_t offset = static_cast<uint64_t>(off);
    uint64_t length = static_cast<uint64_t>(len);

    // Subset params
    auto end = (offset + length) > ctx->input.size() ? ctx->input.size() : offset + length;
    auto data_offset = (end - offset) < length ? length - (end - offset) : 0;

    // Subset memory
    std::vector<uint8_t> data(length);
    std::copy(std::begin(ctx->input) + offset, std::begin(ctx->input) + end, std::begin(data) + data_offset );

    // Create result
    auto padding = (length - (end - offset)) * 8;
    BN num = BN(bytes_to_bmi(data)) << padding;

    return num;
  }

  uint256_t Processor::adjusted_exponent_length(uint256_t el, uint256_t bl) {
    // Params
    bool oversize  = el > 32;
    auto input_el  = oversize ? 32 : el;
    auto adjust_el = oversize ? (8 * el) - 256 : 0;

    // Read input
    BN e = read_input_large(bl, input_el, ctx);

    // MSB
    auto i = e ? -1 : 0;
    for (; e; e = e >> 1ull, ++i);

    // Adjustment
    return i + adjust_el;
  }

  void Processor::precompile_expmod()
  {
    // Get b,e,m lengths
    uint256_t blen = read_input(0, 32);
    uint256_t elen = read_input(32, 32);
    uint256_t mlen = read_input(64, 32);

    // Gas param - complexity
    auto max = blen > mlen ? blen : mlen;
    auto complexity = mult_complexity(max);

    // Gas param - adjusted length
    auto adjusted = adjusted_exponent_length(elen, blen + 96);
    if (adjusted < 1) adjusted = 1;

    // Charge gas
    auto gas_cost = (adjusted * complexity) / GP_MODEXP;
    bool error = use_gas(gas_cost);
    if (error) return;

    if ((blen == 0 && mlen == 0) || mlen == 0) {
      return precompile_return({});
    }

    // Bounded lengths
    uint64_t blen2 = static_cast<uint64_t>(blen);
    uint64_t elen2 = static_cast<uint64_t>(elen);
    uint64_t mlen2 = static_cast<uint64_t>(mlen);

    // Get b,e,m
    auto b = read_input_large(96, blen2, ctx);
    auto e = read_input_large(96 + blen2, elen2, ctx);
    auto m = read_input_large(96 + blen2 + elen2, mlen2, ctx);

    // Empty vector of mod length
    std::vector<uint8_t> vec(static_cast<uint64_t>(mlen));

    // Mod less than equal 1
    if (m <= 1) {
      return precompile_return(vec);
    }

    // Execute
    bmi_to_bytes(boost::multiprecision::powm(b, e, m), vec);

    // Return the result.
    precompile_return(vec);
  }
} // namespace eosio_evm
