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

  void Processor::precompile_not_implemented()
  {
    throw_error(Exception(ET::notImplemented, "This precompiled contract is not implemented."), {});
  }

  void Processor::precompile_execute(uint256_t address)
  {
    // 1.ECDSARECOVER
    if (address == 1)
    {
      return precompile_ecrecover();
    }
    // 2. SHA256
    else if (address == 2)
    {
      return precompile_sha256();
    }
    // 3. RIPEMD160
    else if (address == 3)
    {
      return precompile_ripemd160();
    }
    // 4. IDENTITY
    else if (address == 4)
    {
      return precompile_identity();
    }
    // 5. EXPMOD
    // 6. SNARKV
    // 7. BNADD
    // 8. BNMUL
    else if (address >= 5 && address <= 8)
    {
      return precompile_not_implemented();
    }
    else
    {
      return; // No-op
    }
  }
} // namespace eosio_evm
