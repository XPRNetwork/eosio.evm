// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..
// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void Processor::initialize_create(const Account& caller) {
    // Nonce - 1, since the caller's nonce has been incremented in "raw" action
    const Address to_address = generate_address(caller.get_address(), caller.get_nonce() - 1);

    // Prevent collision
    const auto [callee, error] = create_account(to_address, 0, true);
    if (error) {
      eosio::print("EVM Execution Error: ", intx::hex(to_address), " already exists.");
      return;
    }

    // Transfer value
    bool transfer_error = transfer_internal(caller.get_address(), to_address, transaction.get_value());
    if (transfer_error) return;

    // Push initial context
    ExecResult result;
    auto success_cb = [&result](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::returned;
      result.output = move(output);
      result.gas_used = sub_gas_used;
    };
    auto error_cb = [&result](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
      result.gas_used = sub_gas_used;
    };
    push_context(
      transaction.state_modifications.size(),
      caller,
      callee,
      transaction.gas_left(),
      false,
      transaction.get_value(),
      std::move(std::vector<uint8_t>{}), // Data is empty
      std::move(transaction.data),  // Init data used as code here
      std::move(success_cb),
      std::move(error_cb)
    );

    // Run
    run();

    // Success
    if (result.er == ExitReason::returned) {
      // Validate size
      const auto output_size = result.output.size();
      if (output_size >= MAX_CODE_SIZE) {
        eosio::print("EVM Execution Error: Code is larger than max code size, out of gas!");
        return;
      }

      // Charge create data gas
      const auto create_gas_cost = output_size * GP_CREATE_DATA;
      const bool out_of_gas = create_gas_cost > transaction.gas_limit;
      if (out_of_gas) {
        eosio::print("EVM Execution Error: Out of Gas");
        return;
      } else {
        transaction.gas_used += create_gas_cost;
      }

      // Set code
      set_code(to_address, std::move(result.output));
    }
    // Error
    else
    {
      eosio::print("\nEVM Execution Error: ", int(result.er), ", ", result.exmsg);
    }

    // clean-up
    for (const auto& addr : transaction.selfdestruct_list) {
      selfdestruct(addr);
    }
  }

  void Processor::initialize_call(const Account& caller)
  {
    Address to_address = *transaction.to_address;
    const Account& callee = get_account(to_address);

    // Transfer value
    bool error = transfer_internal(caller.get_address(), to_address, transaction.get_value());
    if (error) return;

    // Push initial context
    ExecResult result;
    auto success_cb = [&result](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::returned;
      result.output = move(output);
      result.gas_used = sub_gas_used;
    };
    auto error_cb = [&result](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
      result.gas_used = sub_gas_used;
    };
    push_context(
      transaction.state_modifications.size(),
      caller,
      callee,
      transaction.gas_left(),
      false,
      transaction.get_value(),
      std::move(transaction.data),
      std::move(callee.get_code()),
      std::move(success_cb),
      std::move(error_cb)
    );

    // Run
    run();

    // Success
    if (result.er == ExitReason::returned)
    {
    }
    // Error
    else
    {
      eosio::print("\nEVM Execution Error: ", int(result.er), ", ", result.exmsg);
    }

    // clean-up
    for (const auto& addr : transaction.selfdestruct_list) {
      selfdestruct(addr);
    }
  }

  void Processor::run()
  {
    // Execute code
    while(!ctxs.empty())
    {
      ctx->step();

      // Execute
      if (ctx->get_pc() < ctx->prog.code.size())
      {
        dispatch();
      }
      // Stop
      else
      {
        stop();
      }
    }
  }

  uint16_t Processor::get_call_depth() const { return static_cast<uint16_t>(ctxs.size()); }
  const uint8_t Processor::get_op() const {
    return ctx->prog.code[ctx->get_pc()];
  }

  void Processor::revert_state(const size_t& revert_to) {
    for (auto i = transaction.state_modifications.size(); i-- > revert_to; ) {
      const auto [type, index, key, oldvalue, amount, newvalue] = transaction.state_modifications[i];
      switch (type) {
        case SMT::STORE_KV:
          storekv(index, key, oldvalue);
          break;
        case SMT::CREATE_ACCOUNT:
          remove_account(key);
          break;
        case SMT::SET_CODE:
          remove_code(key);
          break;
        case SMT::INCREMENT_NONCE:
          decrement_nonce(key);
          break;
        case SMT::TRANSFER:
          transfer_internal(oldvalue, key, amount);
          break;
        case SMT::LOG:
          transaction.log_handler.pop();
          break;
        case SMT::SELF_DESTRUCT:
          transaction.selfdestruct_list.pop_back();
          break;
        default:
          break;
      }

      // Remove item
      transaction.state_modifications.pop_back();
    }
  }

  void Processor::push_context(
    const size_t sm_checkpoint,
    const Account& caller,
    const Account& callee,
    uint256_t gas_left,
    const bool is_static,
    const int64_t call_value,
    std::vector<uint8_t>&& input,
    Program&& prog,
    SuccessHandler&& success_cb,
    ErrorHandler&& error_cb
  ) {
    auto c = std::make_unique<Context>(
      sm_checkpoint,
      caller,
      callee,
      gas_left,
      is_static,
      call_value,
      std::move(input),
      std::move(prog),
      std::move(success_cb),
      std::move(error_cb)
    );
    ctxs.emplace_back(move(c));
    ctx = ctxs.back().get();
  }

  void Processor::pop_context() {
    ctxs.pop_back();
    if (!ctxs.empty()) {
      ctx = ctxs.back().get();
    } else {
      ctx = nullptr;
    }
  }

  void Processor::refund_gas(uint256_t amount) {
    ctx->gas_left += amount;
  }

  // Returns true if error
  bool Processor::throw_error(const Exception& exception, const std::vector<uint8_t>& output) {
    eosio::print("\nException: ", exception.what(), "\n");

    // Consume all call gas on exception
    if (exception.type != ET::revert) {
      ctx->gas_left = 0;
    } else {
      last_return_data = move(output);
    }

    auto error_cb = ctx->error_cb;
    auto gas_used = ctx->gas_used();
    pop_context();
    error_cb(exception, output, gas_used);

    // Always true for error
    return true;
  }

  // Returns true if error
  bool Processor::use_gas(uint256_t amount) {
    if (amount > ctx->gas_left) {
      return throw_error(Exception(ET::outOfGas, "Out of Gas!"), {});
    }

    // Reflect gas changes
    ctx->gas_left -= amount;
    return false;
  }

  // Complex calculation from EIP 2200
  // - Original value is the first value at start of TX
  // - Current value is what is currently stored in EOSIO
  // - New value is the value to be stored
  //
  // Returns true if gas error
  bool Processor::process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value) {
    if (ctx->gas_left <= GP_SSTORE_MINIMUM) {
      return throw_error(Exception(ET::outOfGas, "Out of Gas!"), {});
    }

    // No-op
    if (current_value == new_value) {
      return use_gas(GP_SLOAD_GAS);
    }

    if (original_value == current_value) {
      if (original_value == 0) {
        return use_gas(GP_SSTORE_SET_GAS);
      } else {
        bool error = use_gas(GP_SSTORE_RESET_GAS);
        if (error) return false;
      }

      if (new_value == 0) {
        transaction.gas_refunds += GP_SSTORE_CLEARS_SCHEDULE;
      }
    } else {
      bool error = use_gas(GP_SLOAD_GAS);
      if (error) return false;

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

    return false;
  }

  // Return true if error
  bool Processor::copy_mem_raw(
    const uint64_t offDst,
    const uint64_t offSrc,
    const uint64_t size,
    std::vector<uint8_t>& dst,
    const std::vector<uint8_t>& src,
    const uint8_t pad
  )
  {
    if (!size)
      return false;

    const auto lastDst = offDst + size;

    // Integer overflow or memory limit exceeded
    if (lastDst <= offDst || lastDst >= ProcessorConsts::MAX_MEM_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in copy_mem_raw!"), {});
    }

    if (lastDst > dst.size())
      dst.resize(lastDst);

    const auto lastSrc = offSrc + size;
    const auto endSrc = std::min(lastSrc, static_cast<decltype(lastSrc)>(src.size()));

    uint64_t remaining;
    if (endSrc > offSrc) {
      copy(src.begin() + offSrc, src.begin() + endSrc, dst.begin() + offDst);
      remaining = lastSrc - endSrc;
    } else {
      remaining = size;
    }

    // if there are more bytes to copy than available, add padding
    fill(dst.begin() + lastDst - remaining, dst.begin() + lastDst, pad);

    // Success
    return false;
  }

  // Return true if error
  bool Processor::copy_mem(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src, const uint8_t pad)
  {
    const auto offDst = ctx->s.popu64();
    const auto offSrc = ctx->s.popu64();
    const auto size = ctx->s.popu64();

    // Gas calculation (copy cost is 3)
    bool error = use_gas(GP_COPY * ((size + 31) / 32));
    if (error) return true;

    return copy_mem_raw(offDst, offSrc, size, dst, src, pad);
  }

  // Return true if error
  bool Processor::prepare_mem_access(const uint64_t offset, const uint64_t size)
  {
    if (offset >= ProcessorConsts::MAX_BUFFER_SIZE) {
      return throw_error(Exception(ET::overflow, "overflow in buffer"), {});
    }

    const auto new_size = offset + size;
    const auto current_size = ctx->mem.size();

    if (new_size > current_size)
    {
      const auto new_words = num_words(new_size);
      const auto current_words = static_cast<int64_t>(current_size / 32);
      const auto new_cost = 3 * new_words + new_words * new_words / 512;
      const auto current_cost = 3 * current_words + current_words * current_words / 512;
      const auto cost = new_cost - current_cost;

      // Gas
      bool error = use_gas(cost);
      if (error) return true;

      // Resize
      const auto end = static_cast<size_t>(new_words * ProcessorConsts::WORD_SIZE);
      if (end >= ProcessorConsts::MAX_MEM_SIZE) {
        return throw_error(Exception(ET::outOfBounds, "Memory limit exceeded"), {});
      }
      ctx->mem.resize(end);
    }

    // Success
    return false;
  }

  // Return true if error
  bool Processor::jump_to(const uint64_t newPc)
  {
    if (ctx->prog.jump_dests.find(newPc) == ctx->prog.jump_dests.end()) {
      return throw_error(Exception(ET::illegalInstruction, "Invalid Jump Destination"), {});
    }

    // Set PC
    ctx->set_pc(newPc);
    return false;
  }
} // namespace eosio_evm
