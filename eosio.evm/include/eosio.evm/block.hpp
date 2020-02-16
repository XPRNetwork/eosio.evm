// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

namespace evm4eos
{
  struct Block
  {
    uint64_t  number     = 0;
    uint64_t  difficulty = 0;
    uint64_t  gas_limit  = 10000000;  // Average on ethereum mainnet
    uint64_t  timestamp  = eosio::time_point().sec_since_epoch();
    uint256_t coinbase   = 0;
  };

  inline const Block get_current_block() {
    Block current_block = {};
    return current_block;
  };
  inline uint256_t get_block_hash(uint8_t offset) {
    return 0;
  };
} // namespace evm4eos