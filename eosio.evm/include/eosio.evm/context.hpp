// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <deque>
#include "constants.hpp"
#include "exception.hpp"
#include "program.hpp"
#include "stack.hpp"

namespace eosio_evm {
  class evm;

  using ReturnHandler    = std::function<void(std::vector<uint8_t>)>;
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
    std::map<uint256_t, Account> local_accounts;

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
      // TODO needed?
      if (!prog.code.empty()) {
        mem.reserve(4096);
      }
    }

    inline auto get_used_mem() const { return (mem.size() + ProcessorConsts::WORD_SIZE - 1) / ProcessorConsts::WORD_SIZE;  }
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

    void print() {
      eosio::print("\nmemory\":",  bin2hex(mem));
      eosio::print("\nstack\":",   s.asArray());
      eosio::print("\ncaller\":",  caller.by_address());
      eosio::print("\ncallee\":",  callee.by_address());
      eosio::print("\ngasLeft\":", intx::to_string(gas_left));
      eosio::print("\nIs Static\":", is_static);
      eosio::print("\nInput\":", bin2hex(input));
      eosio::print("\nCall Value\":", call_value, "\n");
    }
  };
}