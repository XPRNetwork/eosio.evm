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
  void Processor::dispatch()
  {
    const auto op = get_op();

    // Clear return data and clear memory.
    last_return_data.clear();
    last_return_data.shrink_to_fit();

    // Debug
    eosio::print(
      "\n",
      "{",
      "\"pc\":",      ctxt->get_pc(), ",",
      "\"gasLeft\":", to_string(ctxt->gas_left), ",",
      "\"gasCost\":", to_string(OpFees::by_code[op]), ",",
      "\"stack\":",   ctxt->s.asArray(), ",",
      "\"depth\":",   to_string(get_call_depth()), ",",
      "\"opName\": \"",  opcodeToString(op), "\"",
      "}",
      ","
    );

    // Charge gas
    use_gas(OpFees::by_code[op]);

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
   * OP Code Implementations
   */
  void Processor::stop()
  {
    ctxt->result_cb({});
  }

  void Processor::add()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x + y);
  }

  void Processor::mul()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x * y);
  }

  void Processor::sub()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x - y);
  }

  void Processor::div()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();

    if (y == 0) {
      ctxt->s.push(0);
    } else {
      ctxt->s.push(x / y);
    }
  }

  void Processor::sdiv()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    const auto min = (numeric_limits<uint256_t>::max() / 2) + 1;

    if (y == 0)
      ctxt->s.push(0);
    // special "overflow case" from the yellow paper
    else if (x == min && y == -1)
      ctxt->s.push(x);
    else
      ctxt->s.push(intx::sdivrem(x, y).quot);
  }

  void Processor::mod()
  {
    const auto x = ctxt->s.pop();
    const auto m = ctxt->s.pop();

    if (m == 0)
      ctxt->s.push(0);
    else
      ctxt->s.push(x % m);
  }

  void Processor::addmod()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    const auto m = ctxt->s.pop();

    if (m == 0)
      ctxt->s.push(0);
    else
      ctxt->s.push(intx::addmod(x, y, m));
  }

  void Processor::smod()
  {
    const auto x = ctxt->s.pop();
    const auto m = ctxt->s.pop();

    if (m == 0)
      ctxt->s.push(0);
    else
      ctxt->s.push(intx::sdivrem(x, m).rem);
  }

  void Processor::mulmod()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    const auto m = ctxt->s.pop();

    if (m == 0)
      ctxt->s.push(0);
    else
      ctxt->s.push(intx::mulmod(x, y, m));
  }

  void Processor::exp()
  {
    const auto b = ctxt->s.pop();
    const auto e = ctxt->s.pop();

    // Optimize: X^0 = 1
    if (e == 0) {
      ctxt->s.push(1);
      return;
    }

    // Gas
    const auto sig_bytes = static_cast<int>(intx::count_significant_words<uint8_t>(e));
    use_gas(sig_bytes * GP_EXP_BYTE);

    // Push result
    const auto res = intx::exp(b, uint256_t(e));
    ctxt->s.push(res);
  }

  void Processor::signextend()
  {
    const auto ext = ctxt->s.pop();
    const auto x = ctxt->s.pop();

    if (ext >= 32) {
      ctxt->s.push(x);
      return;
    }

    const auto sign_bit = static_cast<int>(ext) * 8 + 7;
    const auto sign_mask = uint256_t{1} << sign_bit;
    const auto value_mask = sign_mask - 1;
    const auto is_neg = (x & sign_mask) != 0;
    ctxt->s.push(is_neg ? x | ~value_mask : x & value_mask);
  }

  void Processor::lt()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x < y);
  }

  void Processor::gt()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x > y);
  }

  void Processor::slt()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();

    auto x_neg = static_cast<bool>(x >> 255);
    auto y_neg = static_cast<bool>(y >> 255);
    ctxt->s.push((x_neg ^ y_neg) ? x_neg : x < y);
  }

  void Processor::sgt()
  {
    ctxt->s.swap(1);
    slt();
  }

  void Processor::eq()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x == y);
  }

  void Processor::isZero()
  {
    const auto x = ctxt->s.pop();
    ctxt->s.push(x == 0);
  }

  void Processor::and_()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x & y);
  }

  void Processor::or_()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x | y);
  }

  void Processor::xor_()
  {
    const auto x = ctxt->s.pop();
    const auto y = ctxt->s.pop();
    ctxt->s.push(x ^ y);
  }

  void Processor::not_()
  {
    const auto x = ctxt->s.pop();
    ctxt->s.push(~x);
  }

  void Processor::byte()
  {
    const auto n = ctxt->s.pop();
    const auto x = ctxt->s.pop();

    if (n > 31)
      ctxt->s.push(0);
    else
    {
      auto sh = (31 - static_cast<unsigned>(n)) * 8;
      auto y = x >> sh;
      ctxt->s.push(y & 0xff);
    }
  }

  void Processor::shl()
  {
    const auto shift = ctxt->s.pop();
    const auto value = ctxt->s.pop();
    ctxt->s.push(value << shift);
  }

  void Processor::shr()
  {
    const auto shift = ctxt->s.pop();
    const auto value = ctxt->s.pop();
    ctxt->s.push(value >> shift);
  }

  void Processor::sar()
  {
    const auto shift = ctxt->s.pop();
    const auto value = ctxt->s.pop();

    if ((value & (uint256_t{1} << 255)) == 0) {
      ctxt->s.push(value >> shift);
      return;
    }

    constexpr auto ones = ~uint256_t{};
    if (shift >= 256) {
      ctxt->s.push(ones);
    } else {
      ctxt->s.push((value >> shift) | (ones << (256 - shift)));
    }
  }

  void Processor::sha3()
  {
    const auto offset = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();
    prepare_mem_access(offset, size);

    // Update gas (ceiling)
    use_gas(((size + 31) / 32) * GP_SHA3_WORD);

    uint8_t h[32];
    keccak_256(ctxt->mem.data() + offset, static_cast<unsigned int>(size), h);
    ctxt->s.push(intx::from_big_endian(h, sizeof(h)));
  }

  void Processor::address()
  {
    ctxt->s.push(ctxt->callee.get_address());
  }

  void Processor::balance()
  {
    const auto address = pop_addr(ctxt->s);
    const Account& given_account = contract->get_account(address);
    ctxt->s.push(given_account.get_balance_u64());
  }

  void Processor::origin()
  {
    const auto address = checksum160ToAddress(*transaction.sender);
    ctxt->s.push(address);
  }

  void Processor::caller()
  {
    ctxt->s.push(ctxt->caller);
  }

  void Processor::callvalue()
  {
    ctxt->s.push(ctxt->call_value);
  }

  void Processor::calldataload()
  {
    const auto index = ctxt->s.pop();
    const auto input_size = ctxt->input.size();

    if (input_size < index)
      ctxt->s.push(0);
    else
    {
      const auto begin = static_cast<size_t>(index);
      const auto end = std::min(begin + 32, input_size);

      uint8_t data[32] = {};
      for (size_t i = 0; i < (end - begin); ++i)
          data[i] = ctxt->input[begin + i];

      ctxt->s.push(intx::be::load<uint256_t>(data));
    }
  }

  void Processor::calldatasize()
  {
    ctxt->s.push(ctxt->input.size());
  }

  void Processor::calldatacopy()
  {
    copy_mem(ctxt->mem, ctxt->input, 0);
  }

  void Processor::codesize()
  {
    ctxt->s.push(ctxt->prog.code.size());
  }

  void Processor::codecopy()
  {
    const auto mem_index = ctxt->s.popu64();
    const auto input_index = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();

    prepare_mem_access(mem_index, size);

    const auto code_size = ctxt->prog.code.size();
    auto dst = static_cast<size_t>(mem_index);
    auto src = code_size < input_index ? code_size : static_cast<size_t>(input_index);
    auto s = static_cast<size_t>(size);
    auto copy_size = std::min(s, code_size - src);

    // Gas cost
    use_gas(num_words(s) * GP_COPY);

    if (copy_size > 0)
        std::memcpy(&ctxt->mem[dst], &ctxt->prog.code[src], copy_size);

    if (s - copy_size > 0)
        std::memset(&ctxt->mem[dst + copy_size], 0, s - copy_size);

  }

  void Processor::gasprice()
  {
    ctxt->s.push(GAS_PRICE);
  }

  void Processor::extcodesize()
  {
    auto address = pop_addr(ctxt->s);
    auto code = contract->get_account(address).get_code();
    ctxt->s.push(code.size());
  }

  void Processor::extcodecopy()
  {
    auto address = pop_addr(ctxt->s);
    auto code = contract->get_account(address).get_code();
    copy_mem(ctxt->mem, code, Opcode::STOP);
  }

  void Processor::returndatasize()
  {
    ctxt->s.push(last_return_data.size());
  }

  void Processor::returndatacopy()
  {
    auto size = (last_return_data.size());
    copy_mem(ctxt->mem, last_return_data, 0);
  }

  void Processor::extcodehash()
  {
    auto address = pop_addr(ctxt->s);

    // Fetch code account
    const Account& code_account = contract->get_account(address);

    // If account is empty, return 0
    if (code_account.is_empty()) {
      ctxt->s.push(0);
      return;
    }

    // Get account code
    auto code = code_account.get_code();

    // Hash and return result
    uint8_t h[32];
    keccak_256(code.data(), code.size(), h);
    ctxt->s.push(intx::from_big_endian(h, sizeof(h)));
  }

  void Processor::blockhash()
  {
    const auto i = ctxt->s.popu64();
    if (i >= 256)
      ctxt->s.push(0);
    else
      ctxt->s.push(get_block_hash(i % 256));
  }

  void Processor::coinbase()
  {
    ctxt->s.push(get_current_block().coinbase);
  }

  void Processor::timestamp()
  {
    ctxt->s.push(get_current_block().timestamp);
  }

  void Processor::number()
  {
    ctxt->s.push(eosio::tapos_block_num());
  }

  void Processor::difficulty()
  {
    ctxt->s.push(get_current_block().difficulty);
  }

  void Processor::gaslimit()
  {
    ctxt->s.push(get_current_block().gas_limit);
  }

  void Processor::chainid()
  {
    ctxt->s.push(CURRENT_CHAIN_ID);
  }

  // TODO check if balance of contract is correct if it changes mid operation from start
  void Processor::selfbalance()
  {
    ctxt->s.push(ctxt->callee.get_balance_u64());
  }

  void Processor::pop()
  {
    ctxt->s.pop();
  }

  void Processor::mload()
  {
    const auto offset = ctxt->s.popu64();
    prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
    const auto start = ctxt->mem.data() + offset;
    auto res = intx::from_big_endian(start, ProcessorConsts::WORD_SIZE);
    ctxt->s.push(res);
  }

  void Processor::mstore()
  {
    const auto offset = ctxt->s.popu64();
    const auto word = ctxt->s.pop();
    prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
    intx::to_big_endian(word, ctxt->mem.data() + offset);
  }

  void Processor::mstore8()
  {
    const auto offset = ctxt->s.popu64();
    const auto b = shrink<uint8_t>(ctxt->s.pop());
    prepare_mem_access(offset, sizeof(b));
    ctxt->mem[offset] = b;
  }

  void Processor::sload()
  {
    const auto k = ctxt->s.pop();
    uint256_t loaded = contract->loadkv(ctxt->callee.primary_key(), k);
    ctxt->s.push(loaded);
  }

  void Processor::sstore()
  {
    if (ctxt->is_static) {
      return throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
    }

    // Get items from stack
    const auto k = ctxt->s.pop();
    const auto v = ctxt->s.pop();

    // Store as original if first time seeing it
    uint256_t current_value = contract->loadkv(ctxt->callee.primary_key(), k);
    if (transaction.original_storage.count(k) == 0) {
      transaction.original_storage[k] = current_value;
    }

    // Charge gas
    process_sstore_gas(transaction.original_storage[k], current_value, v);

    if (!v) {
      contract->removekv(ctxt->callee.primary_key(), k);
    } else {
      contract->storekv(ctxt->callee.primary_key(), k, v);
    }
  }

  void Processor::jump()
  {
    const auto newPc = ctxt->s.popu64();
    jump_to(newPc);
  }

  void Processor::jumpi()
  {
    const auto newPc = ctxt->s.popu64();
    const auto cond = ctxt->s.pop();
    if (cond)
      jump_to(newPc);
  }

  void Processor::pc()
  {
    ctxt->s.push(ctxt->get_pc());
  }

  void Processor::msize()
  {
    ctxt->s.push(ctxt->get_used_mem() * 32);
  }

  void Processor::gas()
  {
    ctxt->s.push(ctxt->gas_left);
  }

  void Processor::jumpdest() {}

  void Processor::push()
  {
    const uint8_t bytes = get_op() - PUSH1 + 1;
    const auto end = ctxt->get_pc() + bytes;

    if (end < ctxt->get_pc()) {
      return throw_error(Exception(ET::outOfBounds, "Integer overflow in push"), {});
    }
    if (end >= ctxt->prog.code.size()) {
      return throw_error(Exception(ET::outOfBounds, "Push immediate exceeds size of program "), {});
    }

    // TODO: parse immediate once and not every time
    auto pc = ctxt->get_pc() + 1;
    uint256_t imm = 0;
    for (int i = 0; i < bytes; i++) {
      imm = (imm << 8) | ctxt->prog.code[pc++];
    }

    ctxt->s.push(imm);
    ctxt->set_pc(pc);
  }

  void Processor::dup()
  {
    ctxt->s.dup(get_op() - DUP1);
  }

  void Processor::swap()
  {
    ctxt->s.swap(get_op() - SWAP1 + 1);
  }

  void Processor::log()
  {
    if (ctxt->is_static) {
      return throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
    }

    const auto offset = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();

    // Get topic data from stack
    const uint8_t number_of_logs = get_op() - LOG0;
    vector<uint256_t> topics(number_of_logs);
    for (int i = 0; i < number_of_logs; i++) {
      topics[i] = ctxt->s.pop();
    }

    // Gas
    use_gas((size * GP_LOG_DATA) + (number_of_logs * GP_EXTRA_PER_LOG));

    // TODO implement log table as optional feature
    transaction.log_handler.add({
      ctxt->callee.get_address(),
      copy_from_mem(offset, size),
      topics
    });
  }

  void Processor::invalid() {
    throw_error(Exception(ET::illegalInstruction, "Invalid Instruction"), {});
  }

  void Processor::illegal() {
    throw_error(Exception(ET::illegalInstruction, "Illegal Instruction"), {});
  }

  // Common for both CREATE and CREATE2
  void Processor::_create(const uint64_t& contract_value, const uint64_t& offset, const int64_t& size, const Address& new_address) {
    auto init_code = copy_from_mem(offset, size);

    // For contract accounts, the nonce counts the number of contract-creations by this account
    contract->increment_nonce(ctxt->callee.get_address());

    // Create account using new address and value
    const Account* new_account = contract->create_account(new_address, contract_value, {}, {});
    if (new_account == NULL) {
      return throw_error(Exception(ET::outOfBounds, "New account already exists"), {});
    }

    // In contract creation, the transaction value is an endowment for the newly created account
    contract->transfer_internal(ctxt->callee.get_address(), new_account->get_address(), contract_value);

    // Execute new account's code
    auto result = run(
      ctxt->callee.get_address(),
      *new_account,
      transaction.gas_left(),
      false, // CREATE and CREATE2 cannot be called statically
      {}, // Data
      std::move(init_code),
      0
    );

    // Success
    if (result.er == ExitReason::returned)
    {
      // Set code from output
      auto new_account_address = new_account->get_address();
      if (!result.output.empty()) {
        contract->set_code(new_account_address, move(result.output));
      }

      ctxt->s.push(new_account_address);

      // TODO should we push 0 on stack here in case of halt?
    }
    else
    {
      ctxt->s.push(0);

      if (result.ex == ET::revert) {
        last_return_data = move(result.output);
      }
    }
  }

  void Processor::create()
  {
    if (ctxt->is_static) {
      return throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
    }

    const auto contract_value = ctxt->s.popAmount();
    const auto offset         = ctxt->s.popu64();
    const auto size           = ctxt->s.popu64();
    const auto arbitrary      = ctxt->callee.get_nonce();

    auto new_address = generate_address(ctxt->callee.get_address(), arbitrary);
    _create(contract_value, offset, size, new_address);
  }

  void Processor::create2()
  {
    if (ctxt->is_static) {
      return throw_error(Exception(ET::staticStateChange, "Invalid static state change"), {});
    }

    const auto contract_value = ctxt->s.popAmount();
    const auto offset         = ctxt->s.popu64();
    const auto size           = ctxt->s.popu64();
    const auto arbitrary      = ctxt->s.pop();

    // Gas cost for hashing new address
    const auto arbitrary_size = static_cast<int>(intx::count_significant_words<uint8_t>(arbitrary));
    use_gas(GP_SHA3_WORD * ((arbitrary_size + 31) / 32));

    auto new_address = generate_address(ctxt->callee.get_address(), arbitrary);
    _create(contract_value, offset, size, new_address);
  }

  void Processor::call()
  {
    const auto op = get_op();

    // Pop 6 (DELEGATECALL) or 7 (REST) from stack
    const auto _gas_limit = ctxt->s.pop();
    const auto toAddress  = pop_addr(ctxt->s);
    const auto value      = op == Opcode::DELEGATECALL ? 0 : ctxt->s.popAmount();
    const auto offIn      = ctxt->s.popu64();
    const auto sizeIn     = ctxt->s.popu64();
    const auto offOut     = ctxt->s.popu64();
    const auto sizeOut    = ctxt->s.popu64();

    // TODO: implement precompiled contracts
    if (toAddress >= 1 && toAddress <= 8) {
      throw_error(Exception(ET::notImplemented, "Precompiled contracts/native extensions are not implemented."), {});
      return;
    }

    // Get new account and check not empty
    decltype(auto) new_callee = contract->get_account(toAddress);
    if (new_callee.get_code().empty()) {
      ctxt->s.push(1);
      return;
    }

    // Max gas for calls
    if (value != 0) {
      // callValueTransfer (9000) - callStipend (2300)
      if (op == Opcode::CALL || op == Opcode::CALLCODE) {
        // Check not static
        if (ctxt->is_static) {
          return throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
        }

        // Gas change
        use_gas(GP_CALL_VALUE_TRANSFER - GP_CALL_STIPEND);
      }

      // callnew_accountount
      if (!new_callee.is_empty()) {
        use_gas(GP_NEW_ACCOUNT);
      }

      // Transfer value
      contract->transfer_internal(ctxt->callee.get_address(), new_callee.get_address(), value);
    }

    // 63/64 gas
    const auto gas_allowed = ctxt->gas_left - (ctxt->gas_left / 64);
    const auto gas_limit = (_gas_limit > gas_allowed) ? gas_allowed : _gas_limit;

    // Check max depth
    if (get_call_depth() >= ProcessorConsts::MAX_CALL_DEPTH) {
      return throw_error(Exception(ET::outOfBounds, "Reached max call depth."), {});
    }

    // Fetch input if available
    std::vector<uint8_t> input = {};
    if (sizeIn > 0) {
      input = copy_from_mem(offIn, sizeIn);
    }

    // Prepare memory for output and handlers
    prepare_mem_access(offOut, sizeOut);

    auto parentContext = ctxt;
    auto result_cb = [offOut, sizeOut, parentContext, this](const vector<uint8_t>& output) {
      if (!output.empty()) {
        // Memory
        copy_mem_raw(offOut, 0, sizeOut, parentContext->mem, output);

        // Return data
        copy_mem_raw(offOut, 0, sizeOut, last_return_data, output);
      }

      parentContext->s.push(1);
    };
    auto error_cb = [parentContext, this](const Exception& ex, const vector<uint8_t>& output) {
      parentContext->s.push(0);

      if (ex.type == ET::revert) {
        last_return_data = move(output);
      }
    };

    // Address, callee and value are variable amongst call ops
    auto context_address = ctxt->callee.get_address();
    auto context_callee  = new_callee;
    auto context_value   = value;
    auto is_static       = ctxt->is_static;

    if (op == Opcode::STATICCALL) {
      is_static = true;
    }

    if (op == Opcode::CALLCODE) {
      context_callee = ctxt->callee;
    }

    if (op == Opcode::DELEGATECALL) {
      context_callee  = ctxt->callee;
      context_address = ctxt->caller;
      context_value   = ctxt->call_value;
    }

    // Push call context
    auto result = run(
      context_address,
      context_callee,
      gas_limit,
      is_static,
      move(input),
      new_callee.get_code(),
      context_value
    );
  }

  void Processor::return_()
  {
    const auto offset = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();

    // invoke caller's return handler
    ctxt->result_cb(copy_from_mem(offset, size));
  }

  void Processor::revert()
  {
    const auto offset = ctxt->s.popu64();
    const auto size = ctxt->s.popu64();

    // invoke caller's return handler
    throw_error(
      Exception(ET::revert, "The transaction has been reverted to it's initial state. Likely issue: A non-payable function was called with value, or your balance is too low."),
      copy_from_mem(offset, size)
    );
  }

  void Processor::selfdestruct()
  {
    // Check not static
    if (ctxt->is_static) {
      return throw_error(Exception(ET::staticStateChange, "Invalid static state change."), {});
    }

    // Find recipient
    auto recipient_address = pop_addr(ctxt->s);
    auto recipient = contract->get_account(recipient_address);

    // Contract addres
    auto contract_address = ctxt->callee.get_address();

    // Gas refund if not already scheduled for deletion
    auto existing = std::find(transaction.selfdestruct_list.begin(), transaction.selfdestruct_list.end(), contract_address);
    if (existing != transaction.selfdestruct_list.end()) {
      refund_gas(GP_SELFDESTRUCT_REFUND);
    }

    // Check contract balance
    auto balance = ctxt->callee.get_balance();
    if (balance > 0) {
      // New Account gas fee
      if (recipient.is_empty()) {
        use_gas(GP_NEW_ACCOUNT);
      }

      // Transfer all balance
      contract->transfer_internal(contract_address, recipient_address, balance);
    }

    // Add to list for later
    transaction.selfdestruct_list.push_back(contract_address);
    stop();
  }
} // namespace eosio_evm
