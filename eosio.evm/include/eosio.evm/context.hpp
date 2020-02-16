// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "processor.hpp"

namespace evm4eos {
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
    Stack s;
    ChangeLog changelog;

    const Address caller;
    const Account& callee;
    uint256_t gas_left;
    const bool is_static;
    const std::vector<uint8_t> input;
    const int64_t call_value;
    const Program prog;
    ReturnHandler result_cb;
    HaltHandler halt_cb;
    ExceptionHandler error_cb;

    Context(
      const Address& caller,
      const Account& callee,
      uint256_t gas_left,
      const bool& is_static,
      std::vector<uint8_t>&& input,
      const int64_t& call_value,
      Program&& prog,
      ReturnHandler&& result_cb,
      HaltHandler&& halt_cb,
      ExceptionHandler&& error_cb
    ) :
      caller(caller),
      callee(callee),
      gas_left(gas_left),
      is_static(is_static),
      input(input),
      call_value(call_value),
      prog(prog),
      result_cb(result_cb),
      halt_cb(halt_cb),
      error_cb(error_cb)
    {}

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
  };
}