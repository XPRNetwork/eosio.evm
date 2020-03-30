// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.
//  - mods, signextend, byte, shl, shr, sar, calldataload, codecopy, mstores, log

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
      "\"stack\":",   ctx->s.as_array(), ",",
      "\"depth\":",   std::to_string(get_call_depth() - 1), ",",
      "\"opName\": \"",  opcodeToString(op), "\",",
      "\"sm\": ",  transaction.state_modifications.size(),
      "}",
      ","
    );
    // eosio::print("\nMem Pages: ", __builtin_wasm_current_memory());
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
      case Opcode::CREATE2:
        create();
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
    const auto success_cb = ctx->success_cb;
    const auto gas_used = ctx->gas_used();
    pop_context();
    success_cb({}, gas_used);
  }

  void Processor::add()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x + y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::mul()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x * y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::sub()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x - y);
    if (ctx->s.stack_error) return throw_stack();
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
    if (ctx->s.stack_error) return throw_stack();
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

    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::mod()
  {
    const auto x = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(x % m);

    if (ctx->s.stack_error) return throw_stack();
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

    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::smod()
  {
    const auto x = ctx->s.pop();
    const auto m = ctx->s.pop();

    if (m == 0)
      ctx->s.push(0);
    else
      ctx->s.push(intx::sdivrem(x, m).rem);

    if (ctx->s.stack_error) return throw_stack();
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

    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::exp()
  {
    const auto b = ctx->s.pop();
    const auto e = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Optimize: X^0 = 1
    if (e == 0) {
      ctx->s.push(1);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    // Gas
    const auto sig_bytes = static_cast<int>(intx::count_significant_words<uint8_t>(e));
    bool error = use_gas(sig_bytes * GP_EXP_BYTE);
    if (error) return;

    // Push result
    const auto res = intx::exp(b, uint256_t(e));
    ctx->s.push(res);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::signextend()
  {
    const auto ext = ctx->s.pop();
    const auto x = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    if (ext >= 32) {
      ctx->s.push(x);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    const auto sign_bit = static_cast<int>(ext) * 8 + 7;
    const auto sign_mask = uint256_t{1} << sign_bit;
    const auto value_mask = sign_mask - 1;
    const auto is_neg = (x & sign_mask) != 0;
    ctx->s.push(is_neg ? x | ~value_mask : x & value_mask);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::lt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x < y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::gt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x > y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::slt()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();

    auto x_neg = static_cast<bool>(x >> 255);
    auto y_neg = static_cast<bool>(y >> 255);
    ctx->s.push((x_neg ^ y_neg) ? x_neg : x < y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::sgt()
  {
    ctx->s.swap(1);
    if (ctx->s.stack_error) return throw_stack();
    slt();
  }

  void Processor::eq()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x == y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::isZero()
  {
    const auto x = ctx->s.pop();
    ctx->s.push(x == 0);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::and_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x & y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::or_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x | y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::xor_()
  {
    const auto x = ctx->s.pop();
    const auto y = ctx->s.pop();
    ctx->s.push(x ^ y);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::not_()
  {
    const auto x = ctx->s.pop();
    ctx->s.push(~x);
    if (ctx->s.stack_error) return throw_stack();
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
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::shl()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();
    ctx->s.push(value << shift);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::shr()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();
    ctx->s.push(value >> shift);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::sar()
  {
    const auto shift = ctx->s.pop();
    const auto value = ctx->s.pop();

    if ((value & (uint256_t{1} << 255)) == 0) {
      ctx->s.push(value >> shift);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    constexpr auto ones = ~uint256_t{};
    if (shift >= 256) {
      ctx->s.push(ones);
    } else {
      ctx->s.push((value >> shift) | (ones << (256 - shift)));
    }
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::sha3()
  {
    const auto offset = ctx->s.pop();
    const auto size = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Memory acess + gas
    bool memory_error = access_mem(offset, size);
    if (memory_error) return;

    // Update gas (ceiling)
    bool gas_error = use_gas(num_words(static_cast<uint64_t>(size)) * GP_SHA3_WORD);
    if (gas_error) return;

    // Find keccak 256 hash
    uint8_t h[32];
    keccak_256(ctx->mem.data() + static_cast<uint64_t>(offset), static_cast<unsigned int>(size), h);

    ctx->s.push(intx::be::load<uint256_t>(h));
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::address()
  {
    ctx->s.push(ctx->callee.get_address());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::balance()
  {
    const auto address = ctx->s.pop_addr();

    const Account& given_account = get_account(address);
    ctx->s.push(given_account.get_balance());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::origin()
  {
    const auto address = checksum160ToAddress(*transaction.sender);
    ctx->s.push(address);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::caller()
  {
    ctx->s.push(ctx->caller.get_address());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::callvalue()
  {
    ctx->s.push(ctx->call_value);
    if (ctx->s.stack_error) return throw_stack();
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
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::calldatasize()
  {
    ctx->s.push(ctx->input.size());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::codesize()
  {
    ctx->s.push(ctx->prog.code.size());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::codecopy()
  {
    const auto mem_index = ctx->s.pop();
    const auto input_index = ctx->s.pop();
    const auto size = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    bool memory_error = access_mem(mem_index, size);
    if (memory_error) return;

    const auto code_size = ctx->prog.code.size();
    auto dst = static_cast<size_t>(mem_index);
    auto src = (code_size < input_index) ? code_size : static_cast<size_t>(input_index);
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
    ctx->s.push(transaction.gas_price);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::extcodesize()
  {
    auto address = ctx->s.pop_addr();

    auto code = get_account(address).get_code();
    ctx->s.push(code.size());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::returndatasize()
  {
    ctx->s.push(ctx->last_return_data.size());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::extcodehash()
  {
    auto address = ctx->s.pop_addr();
    if (ctx->s.stack_error) return throw_stack();

    // Fetch code account
    const Account& code_account = get_account(address);

    // If account is empty, return 0
    if (code_account.is_empty()) {
      ctx->s.push(0);
      if (ctx->s.stack_error) return throw_stack();
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
    const auto i = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    if (i >= 256)
      ctx->s.push(0);
    else
      ctx->s.push(get_block_hash(static_cast<uint8_t>(i % 256)));
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::coinbase()
  {
    ctx->s.push(get_current_block().coinbase);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::timestamp()
  {
    ctx->s.push(get_current_block().timestamp);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::number()
  {
    ctx->s.push(get_current_block().number);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::difficulty()
  {
    ctx->s.push(get_current_block().difficulty);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::gaslimit()
  {
    ctx->s.push(get_current_block().gas_limit);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::chainid()
  {
    ctx->s.push(CURRENT_CHAIN_ID);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::selfbalance()
  {
    ctx->s.push(ctx->callee.get_balance());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::pop()
  {
    ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::mload()
  {
    const auto offset = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    bool error = access_mem(offset, WORD_SIZE);
    if (error) return;

    auto res = intx::be::unsafe::load<uint256_t>(&ctx->mem[static_cast<uint64_t>(offset)]);

    ctx->s.push(res);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::mstore()
  {
    const auto offset = ctx->s.pop();
    const auto word = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    bool error = access_mem(offset, WORD_SIZE);
    if (error) return;

    intx::be::unsafe::store(&ctx->mem[static_cast<size_t>(offset)], word);
  }

  void Processor::mstore8()
  {
    const auto offset = ctx->s.pop();
    const auto byte = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    bool error = access_mem(offset, 1);
    if (error) return;

    ctx->mem[static_cast<size_t>(offset)] = static_cast<uint8_t>(byte);
  }

  void Processor::sload()
  {
    const auto k = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();
    uint256_t loaded = loadkv(ctx->callee.primary_key(), k);

    ctx->s.push(loaded);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::sstore()
  {
    #if (PRINT_STATE == true)
    eosio::print("\nSSTORE for contract ", ctx->callee.address);
    #endif

    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    // Get items from stack
    const auto k = ctx->s.pop();
    const auto v = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

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
    const auto newPc = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    bool error = jump_to(newPc);
    if (error) return;
  }

  void Processor::jumpi()
  {
    const auto newPc = ctx->s.pop();
    const auto cond = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    if (cond) {
      bool error = jump_to(newPc);
      if (error) return;
    }
  }

  void Processor::pc()
  {
    ctx->s.push(ctx->get_pc());
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::msize()
  {
    ctx->s.push(ctx->get_used_mem() * 32);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::gas()
  {
    ctx->s.push(ctx->gas_left);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::jumpdest() {}

  void Processor::push()
  {
    const uint8_t bytes = get_op() - PUSH1 + 1;
    const auto end = ctx->get_pc() + bytes;

    if (end < ctx->get_pc()) {
      throw_error(Exception(ET::OOB, "Integer overflow in push"), {});
      return;
    }
    if (end >= ctx->prog.code.size()) {
      stop();
      return;
    }

    // TODO: parse immediate once and not every time
    auto pc = ctx->get_pc() + 1;
    uint256_t imm = 0;
    for (int i = 0; i < bytes; i++) {
      imm = (imm << 8) | ctx->prog.code[pc++];
    }

    ctx->s.push(imm);
    if (ctx->s.stack_error) return throw_stack();
    ctx->set_pc(pc);
  }

  void Processor::dup()
  {
    ctx->s.dup(get_op() - DUP1);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::swap()
  {
    ctx->s.swap(get_op() - SWAP1 + 1);
    if (ctx->s.stack_error) return throw_stack();
  }

  void Processor::log()
  {
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    // Pop initial
    const auto offset = ctx->s.pop();
    const auto size = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Memory access
    bool memory_error = access_mem(offset, size);
    if (memory_error) return;

    // Bound size
    const auto o = static_cast<size_t>(offset);
    const auto s = static_cast<size_t>(size);

    // Number of logs
    const uint8_t num_topics = get_op() - LOG0;

    // Gas
    bool gas_error = use_gas((size * GP_LOG_DATA) + (num_topics * GP_EXTRA_PER_LOG));
    if (gas_error) return;

    // POP logs
    std::vector<uint256_t> topics (num_topics);
    for (size_t i = 0; i < num_topics; ++i) {
      auto log = ctx->s.pop();
      if (ctx->s.stack_error) return throw_stack();

      topics[i] = log;
    }

    #if (PRINT_LOGS == true)
      if (size != 0) {
        const auto& data = std::vector<uint8_t>(ctx->mem.begin() + o, ctx->mem.begin() + o + s);
        LogEntry log ({ ctx->callee.get_address(), std::move(data), topics });
        transaction.logs.logs.emplace_back(std::move(log));
        transaction.add_modification({ SMT::LOG, 0, 0, 0, 0, 0 });
      }
    #endif
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
  void Processor::create() {
    const auto op = get_op();

    // Check if static
    if (ctx->is_static) {
      throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
      return;
    }

    // Pop stack
    const auto contract_value = ctx->s.pop();
    const auto offset         = ctx->s.pop();
    const auto size           = ctx->s.pop();
    const auto arbitrary      = op == CREATE2 ? ctx->s.pop() : ctx->callee.get_nonce();
    if (ctx->s.stack_error) return throw_stack();

    // Find init code
    bool memory_error = access_mem(offset, size);
    if (memory_error) return;

    // Check max
    const std::vector<uint8_t> init_code = { &ctx->mem[size_t(offset)], &ctx->mem[size_t(offset)] + size_t(size) };

    // Extra gas cast for CREATE2
    if (op == CREATE2) {
      bool error = use_gas(num_words(size_t(size)) * GP_SHA3_WORD);
      if (error) return;
    }

    // Generate address
    auto new_address = op == CREATE2
      ? generate_address2(ctx->callee.get_address(), arbitrary, init_code)
      : generate_address(ctx->callee.get_address(), arbitrary);

    // Gas limit (63/64)
    const auto gas_limit = ctx->gas_left - (ctx->gas_left / 64);

    // Clear return data
    ctx->last_return_data.clear();

    // Depth and balance Validation
    bool max_call_depth = get_call_depth() > MAX_CALL_DEPTH;
    bool insufficient_balance = ctx->callee.get_balance() < contract_value;
    if (max_call_depth || insufficient_balance) {
      ctx->s.push(0);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    // For contract accounts, the nonce counts the number of contract-creations by this account
    increment_nonce(ctx->callee.get_address());

    /**
     *
     *
     * Reversible from this point on
     *
     *
     */
    const auto sm_checkpoint = transaction.state_modifications.size();

    // Create account using new address
    auto [new_account, error] = create_account(new_address, true);
    if (error) {
      ctx->s.push(0);
      if (ctx->s.stack_error) return throw_stack();
      use_gas(gas_limit); // Collisions are full exceptions
      return;
    }

    // In contract creation, the transaction value is an endowment for the newly created account
    bool transfer_error = transfer_internal(ctx->callee.get_address(), new_account.get_address(), contract_value);
    if (transfer_error) {
      ctx->s.push(0);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    // Initial refunds
    const auto old_refunds = transaction.gas_refunds;

    // Execute new account's code
    auto p_ctx = ctx;
    auto new_account_address = new_account.get_address();

    auto success_cb = [&, p_ctx, new_account_address, gas_limit](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      auto create_data_gas = output.size() * GP_CREATE_DATA;
      auto total_sub_gas = sub_gas_used + create_data_gas;

      // Error
      if (total_sub_gas > gas_limit) {
        bool gas_error = use_gas(gas_limit);
        if (gas_error) return;

        p_ctx->s.push(0);
        if (ctx->s.stack_error) return throw_stack();
        return;
      }

      // Charge gas
      bool gas_error = use_gas(total_sub_gas);
      if (gas_error) return;

      // Set code
      set_code(new_account_address, move(output));

      // Push created address on stack
      p_ctx->s.push(new_account_address);
      if (p_ctx->s.stack_error) return throw_stack();
    };
    auto error_cb = [&, p_ctx, old_refunds](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      // Set return data if revert
      if (ex_.type == ET::revert) {
        p_ctx->last_return_data = output;
      }

      p_ctx->s.push(0);
      if (p_ctx->s.stack_error) return throw_stack();

      // Reset refunds
      transaction.gas_refunds = old_refunds;

      bool gas_error = use_gas(sub_gas_used);
      if (gas_error) return;
    };

    push_context(
      sm_checkpoint,
      ctx->callee,
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

  uint256_t Processor::value_by_call_type(const unsigned char call_type) {
    if (call_type == Opcode::DELEGATECALL) {
      return ctx->call_value;
    } else if (call_type == Opcode::STATICCALL) {
      return 0;
    } else {
      return ctx->s.pop();
    }
  }

  void Processor::call()
  {
    const auto op = get_op();

    // Pop 6 (DELEGATECALL) or 7 (OTHER CALLS) from stack
    const auto _gas_limit = ctx->s.pop();
    const auto toAddress  = ctx->s.pop_addr();
    const auto value      = value_by_call_type(op);
    const auto offIn      = ctx->s.pop();
    const auto sizeIn     = ctx->s.pop();
    const auto off_out    = ctx->s.pop();
    const auto size_out   = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Fetch "to" account and code
    const Account& to_account = get_account(toAddress);
    std::vector<uint8_t> new_code = to_account.get_code();

    // Dynamically determine parameters
    const bool     is_static  = op == Opcode::STATICCALL || ctx->is_static;
    const Account& new_caller = op == Opcode::DELEGATECALL ? ctx->caller : ctx->callee;
    const Account& new_callee = op == Opcode::DELEGATECALL || op == Opcode::CALLCODE
                                  ? ctx->callee
                                  : to_account;

    // Fetch input if available (+ charge for gas in mem access prepare)
    std::vector<uint8_t> input = {};
    if (sizeIn > 0) {
      bool error = access_mem(offIn, sizeIn);
      if (error) return;

      input = {
        ctx->mem.begin() + static_cast<uint64_t>(offIn),
        ctx->mem.begin() + static_cast<uint64_t>(offIn) + static_cast<uint64_t>(sizeIn)
      };
    }

    // Prepare memory for output and pay gas for memory
    bool error = access_mem(off_out, size_out);
    if (error) return;

    // callValueTransfer (9000) (PRE 63/64)
    if (value > 0 && (op == Opcode::CALL || op == Opcode::CALLCODE)) {
      bool error = use_gas(GP_CALL_VALUE_TRANSFER);
      if (error) return;
    }

    // 63/64 gas
    const auto gas_allowed = ctx->gas_left - (ctx->gas_left / 64);
    auto gas_limit = (_gas_limit > gas_allowed) ? gas_allowed : _gas_limit;

    // Max gas for calls
    if (value > 0) {
      // Check not static
      if (op == Opcode::CALL && ctx->is_static) {
        return (void) throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
      }

      // callStipend (2300)
      if (op == Opcode::CALL || op == Opcode::CALLCODE) {
        gas_limit += GP_CALL_STIPEND;
        refund_gas(GP_CALL_STIPEND);
      }

      // Cost for creating new account
      if (new_callee.is_empty()) {
        bool error = use_gas(GP_NEW_ACCOUNT);
        if (error) return;
      }
    }

    // Clear return data
    ctx->last_return_data.clear();

    // Depth and balance Validation
    bool max_call_depth = get_call_depth() > MAX_CALL_DEPTH;
    bool insufficient_balance = ctx->callee.get_balance() < value;
    if (max_call_depth || insufficient_balance) {
      ctx->s.push(0);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    /**
     *
     *
     * Reversible from this point on
     *
     *
     */
    const auto sm_checkpoint = transaction.state_modifications.size();

    // Transfer value
    if (value > 0) {
      bool transfer_error = transfer_internal(ctx->callee.get_address(), new_callee.get_address(), value);
      if (transfer_error) {
        ctx->s.push(0);
        if (ctx->s.stack_error) return throw_stack();
        return;
      }
    }

    // Skip execution if code is empty and not a precompile
    if (!is_precompile(toAddress) && new_code.empty()) {
      ctx->s.push(1);
      if (ctx->s.stack_error) return throw_stack();
      return;
    }

    // Initial refunds
    const auto old_refunds = transaction.gas_refunds;

    // Push call context with callbacks
    auto p_ctx = ctx;

    auto success_cb = [&, p_ctx, size_out, off_out](const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      p_ctx->last_return_data = output;

      p_ctx->s.push(1);
      if (p_ctx->s.stack_error) return throw_stack();

      // Copy results to memory
      auto bytes_to_copy = std::min(static_cast<unsigned int>(size_out), output.size());
      if (bytes_to_copy > 0) {
        std::memcpy(&p_ctx->mem[static_cast<uint64_t>(off_out)], output.data(), bytes_to_copy);
      }

      // Charge sub-execution gas
      bool error = use_gas(sub_gas_used);
      if (error) return;
    };

    auto error_cb = [&, p_ctx, size_out, off_out, old_refunds](const Exception& ex_, const std::vector<uint8_t>& output, const uint256_t& sub_gas_used) {
      if (ex_.type == ET::revert) {
        p_ctx->last_return_data = output;
      }

      p_ctx->s.push(0);
      if (p_ctx->s.stack_error) return throw_stack();

      // Only copy output to memory if REVERT
      if (ex_.type == ET::revert) {
        auto bytes_to_copy = std::min(static_cast<unsigned int>(size_out), output.size());
        if (bytes_to_copy > 0) {
          std::memcpy(&p_ctx->mem[static_cast<uint64_t>(off_out)], output.data(), bytes_to_copy);
        }
      }

      // Reset refunds
      transaction.gas_refunds = old_refunds;

      // Charge sub-execution gas
      bool error = use_gas(sub_gas_used);
      if (error) return;
    };

    push_context(
      sm_checkpoint,
      new_caller,
      new_callee,
      gas_limit,
      is_static,
      value,
      std::move(input),
      std::move(new_code),
      std::move(success_cb),
      std::move(error_cb)
    );

    // Executes precompile
    if (is_precompile(toAddress)) {
      precompile_execute(toAddress);
    }
  }

  void Processor::return_()
  {
    const auto offset = ctx->s.pop();
    const auto size = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Prepare memory access
    bool error = access_mem(offset, size);
    if (error) return;

    // Fetch output
    std::vector<uint8_t> output = {
      ctx->mem.begin() + static_cast<uint64_t>(offset),
      ctx->mem.begin() + static_cast<uint64_t>(offset) + static_cast<uint64_t>(size)
    };

    // invoke caller's return handler
    auto success_cb = ctx->success_cb;
    auto gas_used = ctx->gas_used();

    pop_context();
    success_cb(output, gas_used);
  }

  void Processor::revert()
  {
    const auto offset = ctx->s.pop();
    const auto size   = ctx->s.pop();
    if (ctx->s.stack_error) return throw_stack();

    // Prepare Memory
    bool error = access_mem(offset, size);
    if (error) return;

    // Fetch output
    std::vector<uint8_t> output = {
      ctx->mem.begin() + static_cast<uint64_t>(offset),
      ctx->mem.begin() + static_cast<uint64_t>(offset) + static_cast<uint64_t>(size)
    };

    // Set return data
    ctx->last_return_data = output;

    // Error
    throw_error(Exception(ET::revert, "One of the actions in this transaction was REVERTed."), output);
  }

  void Processor::selfdestruct()
  {
    // Check not static
    if (ctx->is_static) {
      return (void) throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
    }

    // Pop Stack
    auto recipient_address = ctx->s.pop_addr();
    if (ctx->s.stack_error) return throw_stack();

    // Find recipient
    auto recipient = get_account(recipient_address);

    // Contract addres
    auto contract_address = ctx->callee.get_address();

    // Gas refund if not already scheduled for deletion
    auto existing = std::find(transaction.selfdestruct_list.begin(), transaction.selfdestruct_list.end(), contract_address);
    if (existing == transaction.selfdestruct_list.end()) {
      transaction.gas_refunds += GP_SELFDESTRUCT_REFUND;
    }

    // Check contract balance
    auto balance = ctx->callee.get_balance();
    if (balance > 0) {
      // New Account gas fee
      if (recipient.is_empty()) {
        bool gas_error = use_gas(GP_NEW_ACCOUNT);
        if (gas_error) return;
      }

      // Special case of BURN when sender == recipient
      if (contract_address == recipient_address)
      {
        auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
        accounts_byaddress.modify(accounts_byaddress.iterator_to(ctx->callee), eosio::same_payer, [&](auto& a) {
          a.balance = 0;
        });
      }
      // Transfer all balance
      else
      {
        bool transfer_error = transfer_internal(contract_address, recipient_address, balance);
        if (transfer_error) return;
      }
    }

    // Add to list for later removal of account data and storage keys
    transaction.selfdestruct_list.push_back(contract_address);
    transaction.add_modification({ SMT::SELF_DESTRUCT, 0, contract_address, 0, 0, 0 });

    // Stop execution
    stop();
  }
} // namespace eosio_evm
