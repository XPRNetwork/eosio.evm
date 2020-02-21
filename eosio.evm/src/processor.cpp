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
    transfer_internal(caller.get_address(), to_address, transaction.get_value());

    // Run
    const ExecResult exec_result = run(
      caller,
      callee,
      transaction.gas_left(),
      false, // Is Static
      {}, // Data is empty
      transaction.data, // Init data used as code here
      transaction.get_value()
    );

    // Success
    if (exec_result.er == ExitReason::returned) {
      // Validate size
      const auto output_size = exec_result.output.size();
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
        transaction.gas_used += output_size * GP_CREATE_DATA;
      }

      // Set code if not empty
      if (output_size > 0) {
        set_code(to_address, std::move(exec_result.output));
      }
    }
    // Error
    else
    {
      eosio::print("EVM Execution Error: ", int(exec_result.er), ", ", exec_result.exmsg);
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
    transfer_internal(caller.get_address(), to_address, transaction.get_value());

    // Run
    const ExecResult exec_result = run(
      caller,
      callee,
      transaction.gas_left(),
      false, // Is Static
      transaction.data,
      callee.get_code(),
      transaction.get_value()
    );

    // Success
    if (exec_result.er == ExitReason::returned)
    {
    }
    // Error
    else
    {
      // TODO revert if there was an error

      eosio::print("EVM Execution Error: ", int(exec_result.er), ", ", exec_result.exmsg);
    }

    // clean-up
    for (const auto& addr : transaction.selfdestruct_list) {
      selfdestruct(addr);
    }
  }

  ExecResult Processor::run(
    const Account& caller,
    const Account& callee,
    uint256_t gas_limit,
    const bool& is_static,
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& code,
    const int64_t& call_value
  )
  {
    // Create result and error callbacks
    ExecResult result;
    auto result_cb = [&result](const std::vector<uint8_t>& output) {
      result.er = ExitReason::returned;
      result.output = move(output);
    };
    auto error_cb = [&result](const Exception& ex_, const std::vector<uint8_t>& output) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
    };

    // Add new context
    auto c = std::make_unique<Context>(
      caller,
      callee,
      gas_limit,
      is_static,
      data,
      call_value,
      Program(code),
      result_cb,
      error_cb
    );
    ctxs.emplace_back(move(c));
    ctx = ctxs.back().get();

    // Execute code
    while(ctx->get_pc() < ctx->prog.code.size())
    {
      dispatch();

      // Break if result was found
      if (int(result.er))
        break;

      ctx->step();
    }

    // If no result yet
    if (!int(result.er)) {
      stop();
    }

    // Restore to parent context
    if (!ctxs.empty()) {
      ctx = ctxs.back().get();
    }

    return result;
  }

  uint16_t Processor::get_call_depth() const { return static_cast<uint16_t>(ctxs.size()); }
  Opcode Processor::get_op() const { return static_cast<Opcode>(ctx->prog.code[ctx->get_pc()]); }

  void Processor::revert(const uint64_t revert_to) {
    for (auto i = transaction.state_modifications.size(); i-- > revert_to; ) {
      const auto [type, index, key, oldvalue, amount] = transaction.state_modifications[i];
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

  void Processor::throw_error(const Exception& exception, const std::vector<uint8_t>& output) {
    // Consume all call gas on exception
    if (exception.type != ET::revert) {
      use_gas(ctx->gas_left);
    }

    ctx->error_cb(exception, output);
  }

  // Gas
  void Processor::use_gas(uint256_t amount) {
    // If higher than gas limit or more than gas left
    bool over_gas_limit = transaction.gas_used + amount > transaction.gas_limit;
    bool not_enough_gas_left = amount > ctx->gas_left;
    if (ctx->gas_left && (over_gas_limit || not_enough_gas_left)) {
      return throw_error(Exception(ET::outOfGas, "Out of Gas!"), {});
    }

    // Reflect gas changes
    transaction.gas_used += amount;
    ctx->gas_left -= amount;
  }
  void Processor::refund_gas(uint256_t amount) {
    transaction.gas_used -= amount;
    ctx->gas_left += amount;
  }

  // Complex calculation from EIP 2200
  // - Original value is the first value at start of TX
  // - Current value is what is currently stored in EOSIO
  // - New value is the value to be stored
  void Processor::process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value) {
    if (ctx->gas_left <= GP_SSTORE_MINIMUM) {
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

  void Processor::copy_mem_raw(
    const uint64_t offDst,
    const uint64_t offSrc,
    const uint64_t size,
    std::vector<uint8_t>& dst,
    const std::vector<uint8_t>& src,
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
  }

  void Processor::copy_mem(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src, const uint8_t pad)
  {
    const auto offDst = ctx->s.popu64();
    const auto offSrc = ctx->s.popu64();
    const auto size = ctx->s.popu64();

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
    const auto current_size = ctx->mem.size();

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
      ctx->mem.resize(end);
    }
  }

  std::vector<uint8_t> Processor::copy_from_mem(const uint64_t offset, const uint64_t size)
  {
    prepare_mem_access(offset, size);
    return {ctx->mem.begin() + offset, ctx->mem.begin() + offset + size};
  }

  void Processor::jump_to(const uint64_t newPc)
  {
    if (ctx->prog.jump_dests.find(newPc) == ctx->prog.jump_dests.end()) {
      return throw_error(Exception(ET::illegalInstruction, "Invalid Jump Destination"), {});
    }
    ctx->set_pc(newPc);
  }
} // namespace eosio_evm
