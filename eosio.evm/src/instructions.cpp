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
  void Processor::dispatch()
  {
    const auto op = get_op();

    // Debug
    #if (OPTRACE == true)
    eosio::print(
     "\n",
      "{",
      "\"pc\":",      ctx->get_pc(), ",",
      "\"gasLeft\":", ctx->gas_left > 0 ? intx::to_string(ctx->gas_left) : 0, ",",
      "\"gasCost\":", std::to_string(OpFees::by_code[op]), ",",
      "\"stack\":",   ctx->s.asArray(), ",",
      "\"depth\":",   std::to_string(get_call_depth() - 1), ",",
      "\"opName\": \"",  opcodeToString[op], "\",",
      "\"sm\": ",  transaction.state_modifications.size(),
      "}",
      ","
    );
    #endif /* OPTRACE */

    // Charge gas
    bool error = use_gas(OpFees::by_code[op]);
    if (error) return;

    switch (op)
    {
      case Opcode::PUSH1 ... Opcode::PUSH32:
        push();
        break;
      case Opcode::POP:
        pop();
        break;
      case Opcode::SWAP1 ... Opcode::SWAP16:
        swap();
        break;
      case Opcode::DUP1 ... Opcode::DUP16:
        dup();
        break;
      case Opcode::LOG0 ... Opcode::LOG4:
        log();
        break;
      case Opcode::ADD:
        add();
        break;
      case Opcode::MUL:
        mul();
        break;
      case Opcode::SUB:
        sub();
        break;
      case Opcode::DIV:
        div();
        break;
      case Opcode::SDIV:
        sdiv();
        break;
      case Opcode::MOD:
        mod();
        break;
      case Opcode::SMOD:
        smod();
        break;
      case Opcode::ADDMOD:
        addmod();
        break;
      case Opcode::MULMOD:
        mulmod();
        break;
      case Opcode::EXP:
        exp();
        break;
      case Opcode::SIGNEXTEND:
        signextend();
        break;
      case Opcode::LT:
        lt();
        break;
      case Opcode::GT:
        gt();
        break;
      case Opcode::SLT:
        slt();
        break;
      case Opcode::SGT:
        sgt();
        break;
      case Opcode::EQ:
        eq();
        break;
      case Opcode::ISZERO:
        isZero();
        break;
      case Opcode::AND:
        and_();
        break;
      case Opcode::OR:
        or_();
        break;
      case Opcode::XOR:
        xor_();
        break;
      case Opcode::NOT:
        not_();
        break;
      case Opcode::BYTE:
        byte();
        break;
      case Opcode::SHL:
        shl();
        break;
      case Opcode::SHR:
        shr();
        break;
      case Opcode::SAR:
        sar();
        break;
      case Opcode::JUMP:
        jump();
        break;
      case Opcode::JUMPI:
        jumpi();
        break;
      case Opcode::PC:
        pc();
        break;
      case Opcode::MSIZE:
        msize();
        break;
      case Opcode::MLOAD:
        mload();
        break;
      case Opcode::MSTORE:
        mstore();
        break;
      case Opcode::MSTORE8:
        mstore8();
        break;
      case Opcode::CODESIZE:
        codesize();
        break;
      case Opcode::CODECOPY:
        codecopy();
        break;
      case Opcode::EXTCODESIZE:
        extcodesize();
        break;
      case Opcode::EXTCODECOPY:
        extcodecopy();
        break;
      case Opcode::SLOAD:
        sload();
        break;
      case Opcode::SSTORE:
        sstore();
        break;
      case Opcode::ADDRESS:
        address();
        break;
      case Opcode::BALANCE:
        balance();
        break;
      case Opcode::ORIGIN:
        origin();
        break;
      case Opcode::CALLER:
        caller();
        break;
      case Opcode::CALLVALUE:
        callvalue();
        break;
      case Opcode::CALLDATALOAD:
        calldataload();
        break;
      case Opcode::CALLDATASIZE:
        calldatasize();
        break;
      case Opcode::CALLDATACOPY:
        calldatacopy();
        break;
      case Opcode::RETURNDATASIZE:
        returndatasize();
        break;
      case Opcode::RETURNDATACOPY:
        returndatacopy();
        break;
      case Opcode::EXTCODEHASH:
        extcodehash();
        break;
      case Opcode::RETURN:
        return_();
        break;
      case Opcode::SELFDESTRUCT:
        selfdestruct();
        break;
      case Opcode::CREATE:
        create();
        break;
      case Opcode::CREATE2:
        create2();
        break;
      case Opcode::CALL:
      case Opcode::STATICCALL:
      case Opcode::CALLCODE:
      case Opcode::DELEGATECALL:
        call();
        break;
      case Opcode::JUMPDEST:
        jumpdest();
        break;
      case Opcode::BLOCKHASH:
        blockhash();
        break;
      case Opcode::NUMBER:
        number();
        break;
      case Opcode::GASPRICE:
        gasprice();
        break;
      case Opcode::COINBASE:
        coinbase();
        break;
      case Opcode::TIMESTAMP:
        timestamp();
        break;
      case Opcode::DIFFICULTY:
        difficulty();
        break;
      case Opcode::GASLIMIT:
        gaslimit();
        break;
      case Opcode::CHAINID:
        chainid();
        break;
      case Opcode::SELFBALANCE:
        selfbalance();
        break;
      case Opcode::GAS:
        gas();
        break;
      case Opcode::SHA3:
        sha3();
        break;
      case Opcode::STOP:
        stop();
        break;
      case Opcode::INVALID:
        invalid();
        break;
      case Opcode::REVERT:
        revert();
        break;
      default:
        illegal();
        break;
    };
  }

  /**
   * OP Code Implementations.
   */
  void Processor::stop()
  {
    auto success_cb = ctx->success_cb;
    auto gas_used = ctx->gas_used();
    pop_context();
    success_cb({}, gas_used);
  }

  void Processor::add()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x + y);
  }

  void Processor::mul()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x * y);
  }

  void Processor::sub()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x - y);
  }

  void Processor::div()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();

    if (y == 0) {
      ctx->s.push(0);
    } else {
      ctx->s.push(x / y);
    }
  }

  void Processor::sdiv()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    const auto min = (std::numeric_limits<uint256_t>::max() / 2) + 1;

    if (y == 0)
      ctx->s.push(0);
    // special "overflow case" from the yellow paper
    else if (x == min && y == -1)
      ctx->s.push(x);
    else
      ctx->s.push(intx::sdivrem(x, y).quot);
  }

  void Processor::mod()
  {
    const auto x = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(x % m);
  }

  void Processor::addmod()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(intx::addmod(x, y, m));
  }

  void Processor::smod()
  {
    const auto x = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(intx::sdivrem(x, m).rem);
  }

  void Processor::mulmod()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(intx::mulmod(x, y, m));
  }

  void Processor::exp()
  {
    const auto b = ctx->s.pop();
    const auto e = ctx->s.pop();

    // Optimize: X^0 = 1
    if (e == 0) {
      ctx->s.push(1);
      return;
    }

    // Gas
    const auto sig_bytes = static_cast<int>(intx::count_significant_words<uint8_t>(e));
    bool error = use_gas(sig_bytes * GP_EXP_BYTE);
    if (error) return;

    // Push result
    const auto res = intx::exp(b, uint256_t(e));
    ctx->s.push(res);
  }

  void Processor::signextend()
  {
    const auto ext = ctx->s.pop();
    const auto x = ctx->s.pop();

    if (ext >= 32) {
      ctx->s.push(x);
      return;
    }

    const auto sign_bit = static_cast<int>(ext) * 8 + 7;
    const auto sign_mask = uint256_t{1} << sign_bit;
    const auto value_mask = sign_mask - 1;
    const auto is_neg = (x & sign_mask) != 0;
    ctx->s.push(is_neg ? x | ~value_mask : x & value_mask);
  }

  void Processor::lt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x < y);
  }

  void Processor::gt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x > y);
  }

  void Processor::slt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();

    auto x_neg = static_cast<bool>(x >> 255);
    auto y_neg = static_cast<bool>(y >> 255);
    ctx->s.push((x_neg ^ y_neg) ? x_neg : x < y);
  }

  void Processor::sgt()
  {
    ctx->s.swap(1);
    slt();
  }

  void Processor::eq()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x == y);
  }

  void Processor::isZero()
  {
    const auto x = ctx->s.pop();
    ctx->s.push(x == 0);
  }

  void Processor::and_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x & y);
  }

  void Processor::or_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x | y);
  }

  void Processor::xor_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x ^ y);
  }

  void Processor::not_()
  {
    const auto x = ctx->s.pop();
    ctx->s.push(~x);
  }

  void Processor::byte()
  {
    const auto n = ctx->s.pop();
    const auto x = ctx->s.pop();

    if (n > 31)
      ctx->s.push(0);
    else
    {
      auto sh = (31 - static_cast<unsigned>(n)) * 8;
      auto y = x >> sh;
      ctx->s.push(y & 0xff);
    }
  }

  void Processor::shl()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();
    ctx->s.push(value << shift);
  }

  void Processor::shr()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();
    ctx->s.push(value >> shift);
  }

  void Processor::sar()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();

    if ((value & (uint256_t{1} << 255)) == 0) {
      ctx->s.push(value >> shift);
      return;
    }

    constexpr auto ones = ~uint256_t{};
    if (shift >= 256) {
      ctx->s.push(ones);
    } else {
      ctx->s.push((value >> shift) | (ones << (256 - shift)));
    }
  }

  void Processor::sha3()
  {
    const auto offset = ctx->s.popu64();
    const auto size = ctx->s.popu64();
    bool memory_error = prepare_mem_access(offset, size);
    if (memory_error) return;

    // Update gas (ceiling)
    bool gas_error = use_gas(((size + 31) / 32) * GP_SHA3_WORD);
    if (gas_error) return;

    // Find keccak 256 hash
    uint8_t h[32];
    keccak_256(ctx->mem.data() + offset, static_cast<unsigned int>(size), h);

    ctx->s.push(intx::be::load<uint256_t>(h));
  }

  void Processor::address()
  {
    ctx->s.push(ctx->callee.get_address());
  }

  void Processor::balance()
  {
    const auto address = ctx->s.pop_addr();
    const Account& given_account = get_account(address);
    ctx->s.push(given_account.get_balance_u64());
  }

  void Processor::origin()
  {
    const auto address = checksum160ToAddress(*transaction.sender);
    ctx->s.push(address);
  }

  void Processor::caller()
  {
    ctx->s.push(ctx->caller.get_address());
  }

  void Processor::callvalue()
  {
    ctx->s.push(ctx->call_value);
  }

  void Processor::calldataload()
  {
    const auto index = ctx->s.pop();
    const auto input_size = ctx->input.size();

    if (input_size < index)
      ctx->s.push(0);
    else
    {
      const auto begin = static_cast<size_t>(index);
      const auto end = std::min(begin + 32, input_size);

      uint8_t data[32] = {};
      for (size_t i = 0; i < (end - begin); ++i) {
        data[i] = ctx->input[begin + i];
      }

      ctx->s.push(intx::be::load<uint256_t>(data));
    }
  }

  void Processor::calldatasize()
  {
    ctx->s.push(ctx->input.size());
  }

  void Processor::calldatacopy()
  {
    copy_to_mem(ctx->input, 0);
  }

  void Processor::codesize()
  {
    ctx->s.push(ctx->prog.code.size());
  }

  void Processor::codecopy()
  {
    const auto mem_index = ctx->s.popu64();
    const auto input_index = ctx->s.popu64();
    const auto size = ctx->s.popu64();

    bool memory_error = prepare_mem_access(mem_index, size);
    if (memory_error) return;

    const auto code_size = ctx->prog.code.size();
    auto dst = static_cast<size_t>(mem_index);
    auto src = code_size < input_index ? code_size : static_cast<size_t>(input_index);
    auto s = static_cast<size_t>(size);
    auto copy_size = std::min(s, code_size - src);

    // Gas cost
    bool gas_error = use_gas((num_words(s) * GP_COPY));
    if (gas_error) return;

    if (copy_size > 0)
        std::memcpy(&ctx->mem[dst], &ctx->prog.code[src], copy_size);

    if (s - copy_size > 0)
        std::memset(&ctx->mem[dst + copy_size], 0, s - copy_size);
  }

  void Processor::gasprice()
  {
    ctx->s.push(GAS_PRICE);
  }

  void Processor::extcodesize()
  {
    auto address = ctx->s.pop_addr();
    auto code = get_account(address).get_code();
    ctx->s.push(code.size());
  }

  void Processor::extcodecopy()
  {
    auto address = ctx->s.pop_addr();
    auto code = get_account(address).get_code();
    copy_to_mem(code, Opcode::STOP);
  }

  void Processor::returndatasize()
  {
    ctx->s.push(last_return_data.size());
  }

  void Processor::returndatacopy()
  {
    copy_to_mem(last_return_data, 0);
  }

  void Processor::extcodehash()
  {
    auto address = ctx->s.pop_addr();

    // Fetch code account
    const Account& code_account = get_account(address);

    // If account is empty, return 0
    if (code_account.is_empty()) {
      ctx->s.push(0);
      return;
    }

    // Get account code
    auto code = code_account.get_code();

    // Hash code
    uint8_t h[32];
    keccak_256(code.data(), code.size(), h);

    ctx->s.push(intx::be::load<uint256_t>(h));
  }

  void Processor::blockhash()
  {
    const auto i = ctx->s.popu64();
    if (i >= 256)
      ctx->s.push(0);
    else
      ctx->s.push(get_block_hash(i % 256));
  }

  void Processor::coinbase()
  {
    ctx->s.push(get_current_block().coinbase);
  }

  void Processor::timestamp()
  {
    ctx->s.push(get_current_block().timestamp);
  }

  void Processor::number()
  {
    ctx->s.push(eosio::tapos_block_num());
  }

  void Processor::difficulty()
  {
    ctx->s.push(get_current_block().difficulty);
  }

  void Processor::gaslimit()
  {
    ctx->s.push(get_current_block().gas_limit);
  }

  void Processor::chainid()
  {
    ctx->s.push(CURRENT_CHAIN_ID);
  }

  // TODO check if balance of contract is correct if it changes mid operation from start
  void Processor::selfbalance()
  {
    ctx->s.push(ctx->callee.get_balance_u64());
  }

  void Processor::pop()
  {
    ctx->s.pop();
  }

  void Processor::mload()
  {
    const auto offset = ctx->s.popu64();
    bool error = prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
    if (error) return;
    auto res = intx::be::unsafe::load<uint256_t>(&ctx->mem[offset]);
    ctx->s.push(res);
  }

  void Processor::mstore()
  {
    const auto offset = ctx->s.popu64();
    const auto word = ctx->s.pop();
    bool error = prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
    if (error) return;
    intx::be::unsafe::store(ctx->mem.data() + offset, word);
  }

  void Processor::mstore8()
  {
    const auto offset = ctx->s.popu64();
    const auto b = shrink<uint8_t>(ctx->s.pop());
    bool error = prepare_mem_access(offset, sizeof(b));
    if (error) return;
    ctx->mem[offset] = b;
  }

  void Processor::sload()
  {
    const auto k = ctx->s.pop();
    uint256_t loaded = loadkv(ctx->callee.primary_key(), k);
    ctx->s.push(loaded);
  }

  void Processor::sstore()
  {
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    // Get items from stack
    const auto k = ctx->s.pop();
    const auto v = ctx->s.pop();

    // Load current value
    uint256_t current_value = loadkv(ctx->callee.primary_key(), k);

    // Charge gas
    auto original_value = transaction.find_original(ctx->callee.primary_key(), k);
    bool error = process_sstore_gas(original_value, current_value, v);
    if (error) return;

    // Store
    storekv(ctx->callee.primary_key(), k, v);
  }

  void Processor::jump()
  {
    const auto newPc = ctx->s.popu64();
    bool error = jump_to(newPc);
    if (error) return;
  }

  void Processor::jumpi()
  {
    const auto newPc = ctx->s.popu64();
    const auto cond = ctx->s.pop();
    if (cond) {
      bool error = jump_to(newPc);
      if (error) return;
    }
  }

  void Processor::pc()
  {
    ctx->s.push(ctx->get_pc());
  }

  void Processor::msize()
  {
    ctx->s.push(ctx->get_used_mem() * 32);
  }

  void Processor::gas()
  {
    ctx->s.push(ctx->gas_left);
  }

  void Processor::jumpdest() {}

  void Processor::push()
  {
    const uint8_t bytes = get_op() - PUSH1 + 1;
    const auto end = ctx->get_pc() + bytes;

    if (end < ctx->get_pc()) {
      throw_error(Exception(ET::outOfBounds, "Integer overflow in push"), {});
      return;
    }
    if (end >= ctx->prog.code.size()) {
      throw_error(Exception(ET::outOfBounds, "Push immediate exceeds size of program "), {});
      return;
    }

    // TODO: parse immediate once and not every time
    auto pc = ctx->get_pc() + 1;
    uint256_t imm = 0;
    for (int i = 0; i < bytes; i++) {
      imm = (imm << 8) | ctx->prog.code[pc++];
    }

    ctx->s.push(imm);
    ctx->set_pc(pc);
  }

  void Processor::dup()
  {
    ctx->s.dup(get_op() - DUP1);
  }

  void Processor::swap()
  {
    ctx->s.swap(get_op() - SWAP1 + 1);
  }

  void Processor::log()
  {
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    const auto offset = ctx->s.popu64();
    const auto size = ctx->s.popu64();

    // Get topic data from stack
    const uint8_t number_of_logs = get_op() - LOG0;
    std::vector<uint256_t> topics(number_of_logs);
    for (int i = 0; i < number_of_logs; i++) {
      topics[i] = ctx->s.pop();
    }

    // Gas
    bool gas_error = use_gas((size * GP_LOG_DATA) + (number_of_logs * GP_EXTRA_PER_LOG));
    if (gas_error) return;

    // TODO implement printing log table in transaction receipt
    bool memory_error = prepare_mem_access(offset, size);
    if (memory_error) return;
    std::vector<uint8_t> outputs = {ctx->mem.begin() + offset, ctx->mem.begin() + offset + size};

    transaction.log_handler.add({ ctx->callee.get_address(), outputs, topics });
    transaction.add_modification({ SMT::LOG, 0, 0, 0, 0, 0 });
  }

  void Processor::invalid() {
    throw_error(Exception(ET::illegalInstruction, "Invalid Instruction"), {});
    return;
  }

  void Processor::illegal() {
    throw_error(Exception(ET::illegalInstruction, "Illegal Instruction"), {});
    return;
  }

  // Common for both CREATE and CREATE2
  void Processor::_create(const uint64_t& contract_value, const uint64_t& offset, const int64_t& size, const Address& new_address) {
    // Clear return data
    last_return_data.clear();

    bool memory_error = prepare_mem_access(offset, size);
    if (memory_error) return;
    std::vector<uint8_t> init_code = {ctx->mem.begin() + offset, ctx->mem.begin() + offset + size};

    // Depth and balance Validation
    bool max_call_depth = get_call_depth() >= ProcessorConsts::MAX_CALL_DEPTH;
    bool insufficient_balance = ctx->callee.get_balance() < contract_value;
    if (max_call_depth || insufficient_balance) {
      ctx->s.push(0);
      return;
    }

    // Gas limit (63/64)
    const auto gas_limit = ctx->gas_left - (ctx->gas_left / 64);

    // For contract accounts, the nonce counts the number of contract-creations by this account
    increment_nonce(ctx->callee.get_address());

    // Create account using new address
    auto [new_account, error] = create_account(new_address, 0, true);
    if (error) {
      ctx->s.push(0);
      use_gas(gas_limit);
      return;
    }

    // In contract creation, the transaction value is an endowment for the newly created account
    bool transfer_error = transfer_internal(ctx->callee.get_address(), new_account.get_address(), contract_value);
    if (transfer_error) return;

    // Execute new account's code
    auto parent_context = ctx;
    auto new_account_address = new_account.get_address();
    auto success_cb = [&, parent_context, new_account_address](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      // Set return data
      last_return_data = output;

      // Sub execution gas
      bool sub_gas_error = use_gas(sub_gas_used);
      if (sub_gas_error) return;

      // Create data gas
      bool create_gas_error = use_gas(output.size() * GP_CREATE_DATA);
      if (create_gas_error) return;

      // Set code
      set_code(new_account_address, move(output));

      // Push created address on stack
      parent_context->s.push(new_account_address);
    };
    auto error_cb = [&, parent_context](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      parent_context->s.push(0);
      bool gas_error = use_gas(sub_gas_used);
      if (gas_error) return;
    };
    push_context(
      transaction.state_modifications.size(),
      ctx->callee.get_address(),
      new_account,
      gas_limit,
      false,
      0, // Value
      std::move(std::vector<uint8_t>{}), // Data is empty
      std::move(init_code),
      std::move(success_cb),
      std::move(error_cb)
    );
  }

  void Processor::create()
  {
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    const auto contract_value = ctx->s.popAmount();
    const auto offset         = ctx->s.popu64();
    const auto size           = ctx->s.popu64();
    const auto nonce          = ctx->callee.get_nonce();

    auto new_address = generate_address(ctx->callee.get_address(), nonce);
    _create(contract_value, offset, size, new_address);
  }

  void Processor::create2()
  {
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    const auto contract_value = ctx->s.popAmount();
    const auto offset         = ctx->s.popu64();
    const auto size           = ctx->s.popu64();
    const auto arbitrary      = ctx->s.pop();

    // Gas cost for hashing new address
    const auto arbitrary_size = static_cast<int>(intx::count_significant_words<uint8_t>(arbitrary));
    bool error = use_gas(GP_SHA3_WORD * ((arbitrary_size + 31) / 32));
    if (error) return;

    auto new_address = generate_address(ctx->callee.get_address(), arbitrary);
    _create(contract_value, offset, size, new_address);
  }

  void Processor::call()
  {
    // Clear return data
    last_return_data.clear();

    const auto op = get_op();

    // Pop 6 (DELEGATECALL) or 7 (OTHER CALLS) from stack
    const auto _gas_limit = ctx->s.pop();
    const auto toAddress  = ctx->s.pop_addr();
    const auto value      = op == Opcode::DELEGATECALL ? ctx->call_value : ctx->s.popAmount();
    const auto offIn      = ctx->s.popu64();
    const auto sizeIn     = ctx->s.popu64();
    const auto offOut     = ctx->s.popu64();
    const auto sizeOut    = ctx->s.popu64();

    // Check call depth
    if (get_call_depth() >= ProcessorConsts::MAX_CALL_DEPTH) {
      ctx->s.push(0);
      return;
    }

    // TODO: implement precompiled contracts
    if (toAddress >= 1 && toAddress <= 8) {
      return (void) throw_error(Exception(ET::notImplemented, "Precompiled contracts/native extensions are not implemented."), {});
    }

    // Get new to account and check not empty
    const Account& to_account = get_account(toAddress);
    std::vector<uint8_t> new_code = to_account.get_code();

    // Validate new code
    if (new_code.empty()) {
      ctx->s.push(1);
      return;
    }

    // Dynamically determine parameters
    const bool     is_static  = op == Opcode::STATICCALL;
    const int64_t& new_value  = op == Opcode::STATICCALL ? 0 : value;
    const Account& new_caller = op == Opcode::DELEGATECALL ? ctx->caller : ctx->callee;
    const Account& new_callee = op == Opcode::DELEGATECALL || op == Opcode::CALLCODE
                                  ? ctx->callee
                                  : to_account;

    // 63/64 gas
    const auto gas_allowed = ctx->gas_left - (ctx->gas_left / 64);
    auto gas_limit = (_gas_limit > gas_allowed) ? gas_allowed : _gas_limit;

    // Max gas for calls
    if (value != 0) {
      if (op == Opcode::CALL || op == Opcode::CALLCODE) {
        // Check not static
        if (ctx->is_static) {
          return (void) throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
        }

        // callValueTransfer (9000) - callStipend (2300)
        gas_limit += GP_CALL_STIPEND;
        bool error = use_gas(GP_CALL_VALUE_TRANSFER - GP_CALL_STIPEND);
        if (error) return;
      }

      // Cost for creating new account
      if (new_callee.is_empty()) {
        bool error = use_gas(GP_NEW_ACCOUNT);
        if (error) return;
      }

      // Transfer value
      bool error = transfer_internal(ctx->callee.get_address(), new_callee.get_address(), value);
      if (error) return;
    }

    // Fetch input if available
    std::vector<uint8_t> input = {};
    if (sizeIn > 0) {
      bool error = prepare_mem_access(offIn, sizeIn);
      if (error) return;

      input = {ctx->mem.begin() + offIn, ctx->mem.begin() + offIn + sizeIn};
    }

    // Prepare memory for output
    bool error = prepare_mem_access(offOut, sizeOut);
    if (error) return;

    // Push call context with callbacks
    auto parent_context = ctx;
    auto success_cb = [&, parent_context](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      if (!output.empty()) {
        // Set return data
        last_return_data = output;

        // Memory
        bool error = copy_mem_raw(offOut, 0, sizeOut, parent_context->mem, output);
        if (error) return;
      }

      parent_context->s.push(1);
      bool error = use_gas(sub_gas_used);
      if (error) return;
    };
    auto error_cb = [&, parent_context](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      parent_context->s.push(0);

      bool error = use_gas(sub_gas_used);
      if (error) return;
    };
    push_context(
      transaction.state_modifications.size(),
      new_caller,
      new_callee,
      gas_limit,
      is_static,
      new_value,
      std::move(input),
      std::move(new_code),
      std::move(success_cb),
      std::move(error_cb)
    );
  }

  void Processor::return_()
  {
    const auto offset = ctx->s.popu64();
    const auto size = ctx->s.popu64();

    // Output
    bool error = prepare_mem_access(offset, size);
    if (error) return;
    std::vector<uint8_t> output = {ctx->mem.begin() + offset, ctx->mem.begin() + offset + size};

    // invoke caller's return handler
    auto success_cb = ctx->success_cb;
    auto gas_used = ctx->gas_used();
    pop_context();
    success_cb(output, gas_used);
  }

  void Processor::revert()
  {
    const auto offset = ctx->s.popu64();
    const auto size = ctx->s.popu64();

    // Revert state
    revert_state(ctx->sm_checkpoint);

    // invoke caller's return handler
    bool error = prepare_mem_access(offset, size);
    if (error) return;
    std::vector<uint8_t> output = {ctx->mem.begin() + offset, ctx->mem.begin() + offset + size};

    // Set return data
    last_return_data = output;

    // Error
    throw_error(
      Exception(ET::revert, "The transaction has been reverted to it's initial state. Likely issue: A non-payable function was called with value, or your balance is too low."),
      output
    );
  }

  void Processor::selfdestruct()
  {
    // Check not static
    if (ctx->is_static) {
      return (void) throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
    }

    // Pop Stack
    auto recipient_address = ctx->s.pop_addr();

    // Find recipient
    auto recipient = get_account(recipient_address);

    // Contract addres
    auto contract_address = ctx->callee.get_address();

    // Gas refund if not already scheduled for deletion
    auto existing = std::find(transaction.selfdestruct_list.begin(), transaction.selfdestruct_list.end(), contract_address);
    if (existing != transaction.selfdestruct_list.end()) {
      refund_gas(GP_SELFDESTRUCT_REFUND);
    }

    // Check contract balance
    auto balance = ctx->callee.get_balance();
    if (balance > 0) {
      // New Account gas fee
      if (recipient.is_empty()) {
        bool gas_error = use_gas(GP_NEW_ACCOUNT);
        if (gas_error) return;
      }

      // Transfer all balance
      bool transfer_error = transfer_internal(contract_address, recipient_address, balance);
      if (transfer_error) return;
    }

    // Add to list for later
    transaction.selfdestruct_list.push_back(contract_address);
    transaction.add_modification({ SMT::SELF_DESTRUCT, 0, contract_address, 0, 0, 0 });

    // Stop execution
    stop();
  }
} // namespace eosio_evm
