// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void Processor::precompile_ripemd160()
  {
    // Charge gas
    auto gas_cost = GP_RIPEMD160 + (GP_RIPEMD160_WORD * num_words(ctx->input.size()));
    bool error = use_gas(gas_cost);
    if (error) return;

    // Execute
    eosio::checksum160 checksum = eosio::ripemd160(
      const_cast<char*>(reinterpret_cast<const char*>(ctx->input.data())),
      ctx->input.size()
    );
    auto c_bytes = checksum.extract_as_byte_array();

    // Set the result.
    std::vector<uint8_t> result(32);
    std::copy(c_bytes.begin(), c_bytes.end(), std::begin(result) + 12);

    // Return the result.
    precompile_return(result);
  }
} // namespace eosio_evm
