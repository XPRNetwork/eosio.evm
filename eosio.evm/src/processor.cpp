// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..
// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <eosio.evm/eosio.evm.hpp>

using namespace std;

namespace eosio_evm
{
  ExecResult Processor::run(
    const Address& caller,
    const Account& callee,
    uint256_t gas_limit,
    const bool& is_static,
    const vector<uint8_t>& data,
    const vector<uint8_t>& code,
    const int64_t& call_value
  )
  {
    // Debug
    // eosio::print("\nProcessor:\n");
    // callee.print();
    // eosio::print("Data: ", bin2hex(data), "\n", "code: ", bin2hex(code), "\n", "ISCREATE?: ", transaction.is_create(), "\n", "call_value: ", call_value, "\n");

    // Create result and error callbacks
    ExecResult result;
    auto result_cb = [&result](const vector<uint8_t>& output) {
      result.er = ExitReason::returned;
      result.output = move(output);
    };
    auto error_cb = [&result](const Exception& ex_, const std::vector<uint8_t>& output) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
    };

    // Store parent context pointer to restore after execution
    auto parent_context = ctxt;

    // Add new context
    auto program = Program(code);
    auto c = make_unique<Context>(
      caller,
      callee,
      gas_limit,
      is_static,
      data,
      call_value,
      program,
      result_cb,
      error_cb
    );
    ctxts.emplace_back(move(c));
    ctxt = ctxts.back().get();

    // Execute code
    while(ctxt->get_pc() < ctxt->prog.code.size())
    {
      dispatch();

      if (int(result.er))
        break;

      ctxt->step();
    }

    ctxt = parent_context;

    // Mock success
    if (!int(result.er)) {
      result_cb({});
    }

    return result;
  }

  uint16_t Processor::get_call_depth() const { return static_cast<uint16_t>(ctxts.size()); }
  Opcode Processor::get_op() const { return static_cast<Opcode>(ctxt->prog.code[ctxt->get_pc()]); }

  void Processor::throw_error(const Exception& exception, const std::vector<uint8_t>& output) {
    // Consume all call gas on exception
    if (exception.type != ET::revert) {
      use_gas(ctxt->gas_left);
    }

    ctxt->error_cb(exception, output);
  }

  // Gas
  void Processor::use_gas(uint256_t amount) {
    transaction.gas_used += amount;
    ctxt->gas_left -= amount;

    // If higher than gas limit or more than gas left
    bool over_gas_limit = transaction.gas_used + amount > transaction.gas_limit;
    bool not_enough_gas_left = amount > ctxt->gas_left;
    if (ctxt->gas_left && (over_gas_limit || not_enough_gas_left)) {
      return throw_error(Exception(ET::outOfGas, "Out of Gas!"), {});
    }
  }
  void Processor::refund_gas(uint256_t amount) {
    transaction.gas_used -= amount;
    ctxt->gas_left += amount;
  }

  // Complex calculation from EIP 2200
  // - Original value is the first value at start of TX
  // - Current value is what is currently stored in EOSIO
  // - New value is the value to be stored
  void Processor::process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value) {
    if (ctxt->gas_left <= GP_SSTORE_MINIMUM) {
      return throw_error(Exception(ET::outOfGas, "Out of Gas!"), {});
    }

    if (current_value == new_value) {
      return use_gas(GP_SLOAD_GAS);
    }

    if (original_value == current_value) {
      if (original_value == 0) {
        return use_gas(GP_SSTORE_SET_GAS);
      } else {
        refund_gas(GP_SSTORE_RESET_GAS);
      }

      if (new_value == 0) {
        transaction.gas_refunds += GP_SSTORE_CLEARS_SCHEDULE;
      }
    } else {
      use_gas(GP_SLOAD_GAS);

      if (original_value != 0) {
        if (current_value == 0 && new_value != 0) {
          transaction.gas_refunds -= GP_SSTORE_CLEARS_SCHEDULE;
        }

        if (new_value == 0 && current_value != 0) {
          transaction.gas_refunds += GP_SSTORE_CLEARS_SCHEDULE;
        }
      }

      if (original_value == new_value) {
        if (original_value == 0) {
          transaction.gas_refunds += GP_SSTORE_SET_GAS - GP_SLOAD_GAS;
        } else {
          transaction.gas_refunds += GP_SSTORE_RESET_GAS - GP_SLOAD_GAS;
        }
      }
    }
  }

  // TODO fix memory gas usage (evmone check_memory)
  void Processor::copy_mem_raw(
    const uint64_t offDst,
    const uint64_t offSrc,
    const uint64_t size,
    vector<uint8_t>& dst,
    const vector<uint8_t>& src,
    const uint8_t pad
  )
  {
    if (!size)
      return;

    const auto lastDst = offDst + size;

    // Integer overflow or memory limit exceeded
    if (lastDst <= offDst || lastDst >= ProcessorConsts::MAX_MEM_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in copy_mem_raw!"), {});
    }

    if (lastDst > dst.size())
      dst.resize(lastDst);

    const auto lastSrc = offSrc + size;
    const auto endSrc = min(lastSrc, static_cast<decltype(lastSrc)>(src.size()));

    uint64_t remaining;
    if (endSrc > offSrc) {
      copy(src.begin() + offSrc, src.begin() + endSrc, dst.begin() + offDst);
      remaining = lastSrc - endSrc;
    } else {
      remaining = size;
    }

    // if there are more bytes to copy than available, add padding
    fill(dst.begin() + lastDst - remaining, dst.begin() + lastDst, pad);
  }

  void Processor::copy_mem(vector<uint8_t>& dst, const vector<uint8_t>& src, const uint8_t pad)
  {
    const auto offDst = ctxt->s.popu64();
    const auto offSrc = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();

    // Gas calculation (copy cost is 3)
    use_gas(GP_COPY * ((size + 31) / 32));

    copy_mem_raw(offDst, offSrc, size, dst, src, pad);
  }

  void Processor::prepare_mem_access(const uint64_t offset, const uint64_t size)
  {
    if (offset >= ProcessorConsts::MAX_BUFFER_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in buffer"), {});
    }

    const auto new_size = offset + size;
    const auto current_size = ctxt->mem.size();

    if (new_size > current_size)
    {
      const auto new_words = num_words(new_size);
      const auto current_words = static_cast<int64_t>(current_size / 32);
      const auto new_cost = 3 * new_words + new_words * new_words / 512;
      const auto current_cost = 3 * current_words + current_words * current_words / 512;
      const auto cost = new_cost - current_cost;

      // Gas
      use_gas(cost);

      // Resize
      const auto end = static_cast<size_t>(new_words * ProcessorConsts::WORD_SIZE);
      if (end >= ProcessorConsts::MAX_MEM_SIZE) {
        return throw_error(Exception(ET::outOfBounds, "Memory limit exceeded"), {});
      }
      ctxt->mem.resize(end);
    }
  }

  vector<uint8_t> Processor::copy_from_mem(const uint64_t offset, const uint64_t size)
  {
    prepare_mem_access(offset, size);
    return {ctxt->mem.begin() + offset, ctxt->mem.begin() + offset + size};
  }

  void Processor::jump_to(const uint64_t newPc)
  {
    if (ctxt->prog.jump_dests.find(newPc) == ctxt->prog.jump_dests.end()) {
      return throw_error(Exception(ET::illegalInstruction, "Invalid Jump Destination"), {});
    }
    ctxt->set_pc(newPc);
  }
} // namespace eosio_evm
