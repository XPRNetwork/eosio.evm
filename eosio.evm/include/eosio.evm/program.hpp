// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace eosio_evm
{
  class Program
  {
  public:
    const std::vector<uint8_t> code;
    const std::set<uint64_t> jump_dests;

    Program(const std::vector<uint8_t>&& c)
      : code(c),
        jump_dests(compute_jump_dests(code)) {}

  private:
    std::set<uint64_t> compute_jump_dests(const std::vector<uint8_t>& code)
    {
      std::set<uint64_t> dests;

      for (uint64_t i = 0; i < code.size(); i++) {
        const auto op = code[i];
        if (op == JUMPDEST) {
          dests.insert(i);
        } else if (op >= PUSH1 && op <= PUSH32) {
          i += op - PUSH1 + 1;
        }
      }
      return dests;
    }
  };
}