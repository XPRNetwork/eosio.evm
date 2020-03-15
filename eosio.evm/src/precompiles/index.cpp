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
    else if (address == 5)
    {
      return precompile_expmod();
    }
    // 6. EXPMOD
    else if (address == 6)
    {
      return precompile_bnadd();
    }
    // 7. BNMUL
    // 8. SNARKV
    else if (address >= 6 && address <= 8)
    {
      throw_error(Exception(ET::notImplemented, "The precompiled contract at address 0x" + intx::hex(address) + " is not implemented."), {});
      return;
    }
    // 9. blake2b
    else if (address == 9)
    {
      return precompile_blake2b();
    }
    else
    {
      return; // No-op
    }
  }
} // namespace eosio_evm
