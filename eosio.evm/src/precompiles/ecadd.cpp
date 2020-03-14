// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void Processor::precompile_identity()
  {
    // Check gas
    bool error = use_gas(GP_ECADD);
    if (error) return;

    // Return the result.
    const auto data = ctx->input;
    precompile_return(data);
  }
} // namespace eosio_evm
