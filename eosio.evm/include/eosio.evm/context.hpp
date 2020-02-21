// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <deque>
#include "constants.hpp"
#include "exception.hpp"
#include "processor.hpp"
#include "program.hpp"
#include "stack.hpp"

namespace eosio_evm {
  using ReturnHandler    = std::function<void(std::vector<uint8_t>)>;
  using HaltHandler      = std::function<void()>;
  using ExceptionHandler = std::function<void(const Exception&, std::vector<uint8_t>)>;

  /**
   * execution context of a call
   */
  class Context
  {
  private:
    uint64_t pc = 0;
    bool pc_changed = true;

    using PcType = decltype(pc);

  public:
    std::vector<uint8_t> mem;
    ChangeLog changelog;
    Stack s;

    const Account& caller;
    const Account& callee;
    uint256_t gas_left;
    const bool& is_static;
    const std::vector<uint8_t>& input;
    const int64_t& call_value;
    const Program& prog;
    const ReturnHandler& result_cb;
    const ExceptionHandler& error_cb;

    // Local storage
    std::map<uint64_t, std::map<uint256_t, uint256_t>> local_account_states;
    std::map<uint256_t, const Account&> local_accounts;

    Context(
      const Account& caller,
      const Account& callee,
      uint256_t gas_left,
      const bool& is_static,
      const std::vector<uint8_t>& input,
      const int64_t& call_value,
      const Program& prog,
      const ReturnHandler& result_cb,
      const ExceptionHandler& error_cb
    ) noexcept :
      caller(caller),
      callee(callee),
      gas_left(gas_left),
      is_static(is_static),
      input(input),
      call_value(call_value),
      prog(prog),
      result_cb(result_cb),
      error_cb(error_cb),
      s(this)
    {
      // Reserve 4 KB of memory if code
      if (!prog.code.empty()) {
        mem.reserve(4096);
      }
    }

    bool pc_valid() const { return pc < prog.code.size(); }
    auto get_used_mem() const { return (mem.size() + ProcessorConsts::WORD_SIZE - 1) / ProcessorConsts::WORD_SIZE;  }
    PcType get_pc() const { return pc; }

    void set_pc(const PcType pc_)
    {
      pc = pc_;
      pc_changed = true;
    }

    /// increment the pc if it wasn't changed before
    void step() {
      if (pc_changed) {
        pc_changed = false;
      } else {
        pc++;
      }
    }

    // void store_state(const uint64_t& index, const uint256_t& key, const uint256_t& value)
    // {
    //   local_account_states[index] = {key, value};
    // }
    // void store_account(const uint256_t& address, const Account& account)
    // {
    //   local_changes[address] = account;
    // }

    // uint256_t load_key(const uint64_t& address_index, const uint256_t& key)
    // {
    //   const auto local_states_itr = local_account_states.find(key);
    //   if (local_states_itr != local_account_states.end()) {
    //     const auto local_states_kv_itr = local_states_itr->second.find(key);
    //     if (local_states_kv_itr != local_states_itr->second.end()) {
    //       return local_states_kv_itr->second
    //     }
    //   }

    //   // If not found, return from permanent store
    //   return contract->loadkv(address_index, key)
    // }

    // uint256_t load_account(const uint256_t& address)
    // {
    //   const auto local_accounts_itr = local_accounts.find(address);
    //   if (local_accounts_itr != local_accounts.end()) {
    //     return local_accounts_itr->second;
    //   } else {
    //     // If not found, return from permanent store
    //     return contract->loadkv(address_index, key)
    //   }
    // }

    // void apply()
    // {
    //   // For accounts
    //   for (const auto& [address, account]: local_accounts) {
    //     permanent_state[k] = v;
    //   }

    //   // For account states
    //   for (const auto& [index, kv]: local_changes) {
    //     for (const auto& [k, v]: local_changes) {
    //       permanent_state[k] = v;
    //     }
    //     permanent_state[k] = v;
    //   }
    // }
  };
}