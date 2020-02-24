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

  using SuccessHandler = std::function<void(const std::vector<uint8_t>&, const uint256_t&)>;
  using ErrorHandler   = std::function<void(const Exception&, const std::vector<uint8_t>&, const uint256_t&)>;

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
    uint256_t gas_left;

    const size_t sm_checkpoint;
    const Account& caller;
    const Account& callee;
    uint256_t gas_limit;
    const bool is_static;
    const int64_t call_value;
    const std::vector<uint8_t> input;
    const Program prog;
    SuccessHandler success_cb;
    ErrorHandler error_cb;

    Context(
      const size_t sm_checkpoint,
      const Account& caller,
      const Account& callee,
      uint256_t gas_limit,
      const bool is_static,
      const int64_t call_value,
      std::vector<uint8_t>&& input,
      Program&& prog,
      SuccessHandler&& success_cb,
      ErrorHandler&& error_cb
    ) noexcept :
      sm_checkpoint(sm_checkpoint),
      caller(caller),
      callee(callee),
      gas_limit(gas_limit),
      gas_left(gas_limit),
      is_static(is_static),
      call_value(call_value),
      input(input),
      prog(prog),
      success_cb(success_cb),
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
    inline uint256_t gas_used() const { return gas_limit - gas_left; }
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

    #ifdef TESTING
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
    #endif /* TESTING */
  };
}