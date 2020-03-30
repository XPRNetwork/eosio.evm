// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void Processor::precompile_return(const std::vector<uint8_t>& output) {
    // invoke caller's return handler
    auto success_cb = ctx->success_cb;
    auto gas_used = ctx->gas_used();

    pop_context();
    success_cb(output, gas_used);
  }

  void Processor::precompile_execute(uint256_t address)
  {
    switch (static_cast<uint64_t>(address)) {
      case 1: return precompile_ecrecover();
      case 2: return precompile_sha256();
      case 3: return precompile_ripemd160();
      case 4: return precompile_identity();
      case 5: return precompile_expmod();
      #if (BN_CURVE == true)
      case 6: return precompile_bnadd();
      case 7: return precompile_bnmul();
      case 8: return precompile_bnpairing();
      #endif
      case 9: return precompile_blake2b();
      default: return;
    }
  }
} // namespace eosio_evm
