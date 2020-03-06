// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void Processor::process_transaction(const Account& caller) {
    auto result = transaction.is_create()
      ? initialize_create(caller)
      : initialize_call(caller);

    // If error
    if (result.er != ExitReason::returned) {
      // Revert to initial state
      revert_state(0);
      transaction.errors.push_back(result.exmsg);

      // Use all gas if not revert
      if (result.ex != ET::revert) {
        transaction.gas_used = transaction.gas_limit;
      }
    }

    // Print Result
    transaction.print_receipt(result);

    // clean-up
    for (const auto& addr : transaction.selfdestruct_list) {
      selfdestruct(addr);
    }
  }

  ExecResult Processor::initialize_create(const Account& caller) {
    // Nonce - 1, since the caller's nonce has been incremented in "raw" action
    const Address to_address = generate_address(caller.get_address(), caller.get_nonce() - 1);

    // Prevent collision
    const auto [callee, error] = create_account(to_address, true);
    if (error) return {"EVM Execution Error: " + intx::hex(to_address) + " already exists."};
    transaction.created_address = to_address;

    // Transfer value
    bool transfer_error = transfer_internal(caller.get_address(), to_address, transaction.value);
    if (transfer_error) return {"EVM Execution Error: Unable to process value transfer, check your balance."};

    // Push initial context
    ExecResult result;
    auto success_cb = [&result](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::returned;
      result.output = move(output);
      result.gas_used = sub_gas_used;
    };
    auto error_cb = [&](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
      result.output = output;
      result.gas_used = sub_gas_used;

      // Reset refunds
      transaction.gas_refunds = 0;
    };
    push_context(
      transaction.state_modifications.size(),
      caller,
      callee,
      transaction.gas_left(),
      false,
      transaction.value,
      std::move(std::vector<uint8_t>{}), // Data is empty
      std::move(transaction.data),  // Init data used as code here
      std::move(success_cb),
      std::move(error_cb)
    );

    // Run
    run();

    // Use gas
    transaction.gas_used += result.gas_used;

    // If success
    if (result.er == ExitReason::returned) {
      // Validate size
      const auto output_size = result.output.size();
      if (output_size >= MAX_CODE_SIZE) {
        return {"EVM Execution Error: Code is larger than max code size, out of gas!"};
      }

      // Charge create data gas
      const auto create_gas_cost = output_size * GP_CREATE_DATA;
      if (transaction.gas_left() < create_gas_cost) {
        return {"EVM Execution Error: Out of Gas"};
      } else {
        transaction.gas_used += create_gas_cost;
      }

      // Set code
      set_code(to_address, std::move(result.output));
    }

    return result;
  }

  ExecResult Processor::initialize_call(const Account& caller)
  {
    Address to_address = *transaction.to_address;
    const Account& callee = get_account(to_address);

    // Transfer value
    bool error = transfer_internal(caller.get_address(), to_address, transaction.value);
    if (error) return {"EVM Execution Error: Unable to process value transfer, check your balance."};

    // Push initial context
    ExecResult result;
    auto success_cb = [&result](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::returned;
      result.output = move(output);
      result.gas_used = sub_gas_used;
    };
    auto error_cb = [&](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      result.er = ExitReason::threw;
      result.ex = ex_.type;
      result.exmsg = ex_.what();
      result.output = move(output);
      result.gas_used = sub_gas_used;

      // Reset refunds
      transaction.gas_refunds = 0;
    };
    push_context(
      transaction.state_modifications.size(),
      caller,
      callee,
      transaction.gas_left(),
      false,
      transaction.value,
      std::move(transaction.data),
      std::move(callee.get_code()),
      std::move(success_cb),
      std::move(error_cb)
    );

    // Executes precompile
    if (is_precompile(to_address)) {
      precompile_execute(to_address);
    }

    // Run
    run();

    // Use gas
    transaction.gas_used += result.gas_used;

    // Call does nothing with result unlike create which sets code for contract
    return result;
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
    for (unsigned i = transaction.state_modifications.size() ; i-- > revert_to ; ) {
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
          transaction.logs.pop();
          break;
        case SMT::SELF_DESTRUCT:
          transaction.selfdestruct_list.pop_back();
          break;
        default:
          break;
      }
    }

    // Slice vector
    transaction.state_modifications = std::vector<StateModification>(
      transaction.state_modifications.begin(),
      transaction.state_modifications.begin() + revert_to
    );
  }

  void Processor::push_context(
    const size_t sm_checkpoint,
    const Account& caller,
    const Account& callee,
    uint256_t gas_left,
    const bool is_static,
    const uint256_t call_value,
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
  bool Processor::throw_error(const Exception& exception, const std::vector<uint8_t>& output)
  {
    // Add to error log
    transaction.errors.push_back(exception.what());

    // Consume all call gas on exception
    if (exception.type != ET::revert) {
      ctx->gas_left = 0;
    }

    // Revert all state changes
    revert_state(ctx->sm_checkpoint);

    // Pop context and Call error callback
    auto error_cb = ctx->error_cb;
    auto gas_used = ctx->gas_used();
    pop_context();
    error_cb(exception, output, gas_used);

    // Always true for error
    return true;
  }
  void Processor::throw_stack() {
    throw_error(Exception(ET::OOB, *ctx->s.stack_error), {});
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
        if (error) return true;
      }

      if (new_value == 0) {
        transaction.gas_refunds += GP_SSTORE_CLEARS_SCHEDULE;
      }
    } else {
      bool error = use_gas(GP_SLOAD_GAS);
      if (error) return true;

      if (original_value != 0) {
        if (current_value == 0 && new_value != 0) {
          // Sets to 0 if refunds would go negative
          if (transaction.gas_refunds < GP_SSTORE_CLEARS_SCHEDULE) {
            transaction.gas_refunds = 0;
          } else {
            transaction.gas_refunds -= GP_SSTORE_CLEARS_SCHEDULE;
          }
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
  bool Processor::jump_to(const uint256_t& newPc)
  {
    if (newPc > ctx->prog.code.size() || newPc > std::numeric_limits<uint64_t>::max()) {
      return throw_error(Exception(ET::illegalInstruction, "Invalid Jump Destination"), {});
    }

    auto bounded_pc = static_cast<uint64_t>(newPc);

    if (ctx->prog.jump_dests.find(bounded_pc) == ctx->prog.jump_dests.end()) {
      return throw_error(Exception(ET::illegalInstruction, "Invalid Jump Destination"), {});
    }

    // Set PC
    ctx->set_pc(bounded_pc);
    return false;
  }
} // namespace eosio_evm
