// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..

// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <eosio.evm/eosio.evm.hpp>

using namespace std;

namespace evm4eos
{
  class _Processor
  {
  private:
    EthereumTransaction& transaction;  // the transaction object
    evm* const contract;               // pointer to parent contract (to call EOSIO actions)
    Context* ctxt;                     // pointer to the current context
    vector<unique_ptr<Context>> ctxts; // the stack of contexts (one per nested call)
    vector<uint8_t> last_return_data;  // last returned data

  public:
    using ET = Exception::Type;

    _Processor(evm4eos::EthereumTransaction& transaction, evm* contract)
      : transaction(transaction),
        contract(contract) {}

    ExecResult run(
      const Address& caller,
      const Account& callee
    )
    {
      // Initialize
      vector<uint8_t> input = transaction.data;
      const int64_t& call_value = transaction.get_value();
      auto code = transaction.is_create()
                    ? transaction.data
                    : callee.get_code();

      // Debug
      // eosio::print("\nProcessor:\n");
      // callee.print();
      // eosio::print("input: " + bin2hex(input) + "\n");
      // eosio::print("code: " + bin2hex(code) + "\n");
      // eosio::print("ISCREATE?: ", transaction.is_create(), "\n");
      // eosio::print("call_value: ", call_value, "\n");

      // create the first context
      ExecResult result;
      auto result_cb = [&result, this](vector<uint8_t> output) {
        result.er = ExitReason::returned;
        result.output = move(output);
      };
      auto halt_cb = [&result]() { result.er = ExitReason::halted; };
      auto error_cb = [&result](const Exception& ex_, vector<uint8_t> output) {
        result.er = ExitReason::threw;
        result.ex = ex_.type;
        result.exmsg = ex_.what();
      };

      // TODO check what happens when caller/calle data is updated in a transaction
      push_context(
        caller,
        callee,
        transaction.gas_left(),
        move(input),
        move(code),
        call_value,
        result_cb,
        halt_cb,
        error_cb
      );

      // eosio::print("\nPC: ", (uint8_t) ctxt->get_pc());
      // eosio::print("\nProg Size: ", (uint8_t) ctxt->prog.code.size());

      // run
      while (ctxt->get_pc() < ctxt->prog.code.size())
      {
        // TODO handle exception
        dispatch();

        if (!ctxt)
          break;

        ctxt->step();
      }

      // Result of run
      // eosio::print("\nRun result:\n");
      // result.print();

      // halt outer context if it did not do so itself
      if (ctxt)
        stop();

      // clean-up
      for (const auto& addr : transaction.selfdestruct_list) {
        contract->remove_code(addr);
      }

      return result;
    }

  private:
    void push_context(
      const Address& caller,
      const Account& callee,
      const uint256_t& gas_limit,
      vector<uint8_t>&& input,
      Program&& prog,
      const int64_t& call_value,
      ReturnHandler&& result_cb,
      HaltHandler&& halt_cb,
      ExceptionHandler&& error_cb
    )
    {
      // TODO Static if static opcode or context is already static
      // const auto is_static = get_op() == Opcode::STATICCALL || ctxt->is_static;
      const auto is_static = false;

      auto c = make_unique<Context>(
        caller,
        callee,
        gas_limit,
        is_static,
        move(input),
        call_value,
        move(prog),
        move(result_cb),
        move(halt_cb),
        move(error_cb)
      );
      ctxts.emplace_back(move(c));
      ctxt = ctxts.back().get();
    }

    constexpr int64_t num_words(uint64_t size_in_bytes) noexcept
    {
      return (static_cast<int64_t>(size_in_bytes) + (ProcessorConsts::WORD_SIZE - 1)) / ProcessorConsts::WORD_SIZE;
    }

    uint16_t get_call_depth() const
    {
      return static_cast<uint16_t>(ctxts.size());
    }

    Opcode get_op() const
    {
      return static_cast<Opcode>(ctxt->prog.code[ctxt->get_pc()]);
    }

    uint256_t pop_addr(Stack& stack)
    {
      static const uint256_t MASK_160 = (uint256_t(1) << 160) - 1;
      return stack.pop() & MASK_160;
    }

    void pop_context()
    {
      ctxts.pop_back();
      if (!ctxts.empty())
        ctxt = ctxts.back().get();
      else
        ctxt = nullptr;
    }

    void use_gas(uint256_t amount) {
      transaction.gas_used += amount;
      ctxt->gas_left -= amount;
      eosio::check(transaction.gas_used <= transaction.gas_limit && ctxt->gas_left >= 0, "Out of Gas");
    }

    // TODO check gas refunds
    void refund_gas(uint256_t amount) {
      transaction.gas_used -= amount;
      ctxt->gas_left += amount;
      eosio::check(transaction.gas_used <= transaction.gas_limit && ctxt->gas_left >= 0, "Out of Gas");
    }

    // Complex calculation from EIP 2200
    // - Original value is the first value at start of TX
    // - Current value is what is currently stored in EOSIO
    // - New value is the value to be stored
    void process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value) {
      eosio::check(ctxt->gas_left > 2300, "out of gas");

      if (current_value == new_value) {
        return use_gas(800);
      }

      if (original_value == current_value) {
        if (!original_value) {
          return use_gas(20000);
        }
        if (!new_value) {
          refund_gas(15000);
        }
        return use_gas(5000);
      }

      if (original_value) {
        if (!current_value) {
          // sub_refund(15000);
        } else if (!new_value) {
          refund_gas(15000);
        }
      }

      if (original_value == new_value) {
        if (!original_value) {
          refund_gas(19200);
        } else {
          refund_gas(4200);
        }
      }

      return use_gas(800);
    }

    // TODO fix memory gas usage (evmone check_memory)
    static void copy_mem_raw(
      const uint64_t offDst,
      const uint64_t offSrc,
      const uint64_t size,
      vector<uint8_t>& dst,
      const vector<uint8_t>& src,
      const uint8_t pad = 0
    )
    {
      if (!size)
        return;

      const auto lastDst = offDst + size;
      eosio::check(lastDst > offDst, "Integer overflow in copy_mem_raw (" + to_string(lastDst) + " < " + to_string(offDst) + ") (outOfBounds)");
      eosio::check(lastDst < ProcessorConsts::MAX_MEM_SIZE, "Memory limit exceeded (" + to_string(lastDst) + " > " + to_string(ProcessorConsts::MAX_MEM_SIZE) + ") (outOfBounds)");

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

    void copy_mem(
      vector<uint8_t>& dst, const vector<uint8_t>& src, const uint8_t pad)
    {
      const auto offDst = ctxt->s.popu64();
      const auto offSrc = ctxt->s.popu64();
      const auto size = ctxt->s.popu64();

      // Gas calculation (copy cost is 3)
      use_gas(GP_COPY * ((size + 31) / 32));

      copy_mem_raw(offDst, offSrc, size, dst, src, pad);
    }

    void prepare_mem_access(const uint64_t offset, const uint64_t size)
    {
      eosio::check(offset < ProcessorConsts::MAX_BUFFER_SIZE, "Out of Gas");

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
        eosio::check(end < ProcessorConsts::MAX_MEM_SIZE, "Memory limit exceeded (" + to_string(end) + " > " + to_string(ProcessorConsts::MAX_MEM_SIZE) + ") (outOfBounds)");
        ctxt->mem.resize(end);
      }
    }

    vector<uint8_t> copy_from_mem(const uint64_t offset, const uint64_t size)
    {
      prepare_mem_access(offset, size);
      return {ctxt->mem.begin() + offset, ctxt->mem.begin() + offset + size};
    }

    void jump_to(const uint64_t newPc)
    {
      eosio::check(ctxt->prog.jump_dests.find(newPc) != ctxt->prog.jump_dests.end(), to_string(newPc) + " is not a valid jump destination (illegalInstruction)");
      ctxt->set_pc(newPc);
    }

    template <typename T>
    static T shrink(uint256_t i)
    {
      return static_cast<T>(i & numeric_limits<T>::max());
    }

    void dispatch()
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
          eosio::check(false, "Invalid Instruction");
          break;
        case Opcode::REVERT:
          revert();
          break;
        default:
          eosio::check(false, "(illegalInstruction)");
      };
    }

    /**
     * OP Code Implementations
     */
    void stop()
    {
      // (1) save halt handler
      auto halt_cb = ctxt->halt_cb;

      // (2) pop current context
      pop_context();

      // (3) invoke halt handler
      halt_cb();
    }

    void add()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x + y);
    }

    void mul()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x * y);
    }

    void sub()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x - y);
    }

    void div()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();

      if (y == 0) {
        ctxt->s.push(0);
      } else {
        ctxt->s.push(x / y);
      }
    }

    void sdiv()
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

    void mod()
    {
      const auto x = ctxt->s.pop();
      const auto m = ctxt->s.pop();

      if (m == 0)
        ctxt->s.push(0);
      else
        ctxt->s.push(x % m);
    }

    void addmod()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      const auto m = ctxt->s.pop();

      if (m == 0)
        ctxt->s.push(0);
      else
        ctxt->s.push(intx::addmod(x, y, m));
    }

    void smod()
    {
      const auto x = ctxt->s.pop();
      const auto m = ctxt->s.pop();

      if (m == 0)
        ctxt->s.push(0);
      else
        ctxt->s.push(intx::sdivrem(x, m).rem);
    }

    void mulmod()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      const auto m = ctxt->s.pop();

      if (m == 0)
        ctxt->s.push(0);
      else
        ctxt->s.push(intx::mulmod(x, y, m));
    }

    void exp()
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

    void signextend()
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

    void lt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x < y);
    }

    void gt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x > y);
    }

    void slt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();

      auto x_neg = static_cast<bool>(x >> 255);
      auto y_neg = static_cast<bool>(y >> 255);
      ctxt->s.push((x_neg ^ y_neg) ? x_neg : x < y);
    }

    void sgt()
    {
      ctxt->s.swap(1);
      slt();
    }

    void eq()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x == y);
    }

    void isZero()
    {
      const auto x = ctxt->s.pop();
      ctxt->s.push(x == 0);
    }

    void and_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x & y);
    }

    void or_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x | y);
    }

    void xor_()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x ^ y);
    }

    void not_()
    {
      const auto x = ctxt->s.pop();
      ctxt->s.push(~x);
    }

    void byte()
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

    void shl()
    {
      const auto shift = ctxt->s.pop();
      const auto value = ctxt->s.pop();
      ctxt->s.push(value << shift);
    }

    void shr()
    {
      const auto shift = ctxt->s.pop();
      const auto value = ctxt->s.pop();
      ctxt->s.push(value >> shift);
    }

    void sar()
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

    void sha3()
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

    void address()
    {
      ctxt->s.push(ctxt->callee.get_address());
    }

    void balance()
    {
      const auto address = pop_addr(ctxt->s);
      const Account& given_account = contract->get_account(address);
      ctxt->s.push(given_account.get_balance_u64());
    }

    void origin()
    {
      const auto address = checksum160ToAddress(*transaction.sender);
      ctxt->s.push(address);
    }

    void caller()
    {
      ctxt->s.push(ctxt->caller);
    }

    void callvalue()
    {
      ctxt->s.push(ctxt->call_value);
    }

    void calldataload()
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

    void calldatasize()
    {
      ctxt->s.push(ctxt->input.size());
    }

    void calldatacopy()
    {
      copy_mem(ctxt->mem, ctxt->input, 0);
    }

    void codesize()
    {
      ctxt->s.push(ctxt->prog.code.size());
    }

    void codecopy()
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

    void gasprice()
    {
      ctxt->s.push(GAS_PRICE);
    }

    void extcodesize()
    {
      auto address = pop_addr(ctxt->s);
      auto code = contract->get_account(address).get_code();
      ctxt->s.push(code.size());
    }

    void extcodecopy()
    {
      auto address = pop_addr(ctxt->s);
      auto code = contract->get_account(address).get_code();
      copy_mem(ctxt->mem, code, Opcode::STOP);
    }

    void returndatasize()
    {
      ctxt->s.push(last_return_data.size());
    }

    void returndatacopy()
    {
      auto size = (last_return_data.size());
      copy_mem(ctxt->mem, last_return_data, 0);
    }

    void extcodehash()
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

    void blockhash()
    {
      const auto i = ctxt->s.popu64();
      if (i >= 256)
        ctxt->s.push(0);
      else
        ctxt->s.push(get_block_hash(i % 256));
    }

    void coinbase()
    {
      ctxt->s.push(get_current_block().coinbase);
    }

    void timestamp()
    {
      ctxt->s.push(get_current_block().timestamp);
    }

    void number()
    {
      ctxt->s.push(eosio::tapos_block_num());
    }

    void difficulty()
    {
      ctxt->s.push(get_current_block().difficulty);
    }

    void gaslimit()
    {
      ctxt->s.push(get_current_block().gas_limit);
    }

    void chainid()
    {
      ctxt->s.push(CURRENT_CHAIN_ID);
    }

    // TODO check if balance of contract is correct if it changes mid operation from start
    void selfbalance()
    {
      ctxt->s.push(ctxt->callee.get_balance_u64());
    }

    void pop()
    {
      ctxt->s.pop();
    }

    void mload()
    {
      const auto offset = ctxt->s.popu64();
      prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
      const auto start = ctxt->mem.data() + offset;
      auto res = intx::from_big_endian(start, ProcessorConsts::WORD_SIZE);
      ctxt->s.push(res);
    }

    void mstore()
    {
      const auto offset = ctxt->s.popu64();
      const auto word = ctxt->s.pop();
      prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
      intx::to_big_endian(word, ctxt->mem.data() + offset);
    }

    void mstore8()
    {
      const auto offset = ctxt->s.popu64();
      const auto b = shrink<uint8_t>(ctxt->s.pop());
      prepare_mem_access(offset, sizeof(b));
      ctxt->mem[offset] = b;
    }

    void sload()
    {
      const auto k = ctxt->s.pop();
      auto loaded = contract->loadkv(ctxt->callee.get_address(), k);
      ctxt->s.push(loaded);
    }

    void sstore()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");
      const auto k = ctxt->s.pop();
      const auto v = ctxt->s.pop();
      // eosio::print("\nSSTORE Key ", intx::hex(k), " Value: ", intx::hex(v));

      if (!v) {
        contract->removekv(ctxt->callee.get_address(), k);
      } else {
        contract->storekv(ctxt->callee.get_address(), k, v);
      }
    }

    void jump()
    {
      const auto newPc = ctxt->s.popu64();
      jump_to(newPc);
    }

    void jumpi()
    {
      const auto newPc = ctxt->s.popu64();
      const auto cond = ctxt->s.pop();
      if (cond)
        jump_to(newPc);
    }

    void pc()
    {
      ctxt->s.push(ctxt->get_pc());
    }

    void msize()
    {
      ctxt->s.push(ctxt->get_used_mem() * 32);
    }

    void gas()
    {
      ctxt->s.push(ctxt->gas_left);
    }

    void jumpdest() {}

    void push()
    {
      const uint8_t bytes = get_op() - PUSH1 + 1;
      const auto end = ctxt->get_pc() + bytes;
      if (end < ctxt->get_pc()) {
        eosio::check(false, "Integer overflow in push (" + to_string(end) + " < " + to_string(ctxt->get_pc()) + ") (outOfBounds)");
      }

      if (end >= ctxt->prog.code.size()) {
        eosio::check(false, "Push immediate exceeds size of program (" + to_string(end) + " >= " + to_string(ctxt->prog.code.size()) + ") (outOfBounds)");
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

    void dup()
    {
      ctxt->s.dup(get_op() - DUP1);
    }

    void swap()
    {
      ctxt->s.swap(get_op() - SWAP1 + 1);
    }

    void log()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

      const auto offset = ctxt->s.popu64();
      const auto size = ctxt->s.popu64();

      // Gas
      use_gas(size * GP_LOG_DATA);

      // Get topic data from stack
      const uint8_t number_of_logs = get_op() - LOG0;
      vector<uint256_t> topics(number_of_logs);
      for (int i = 0; i < number_of_logs; i++) {
        topics[i] = ctxt->s.pop();
      }

      // TODO implement log table as optional feature
      transaction.log_handler.add({
        ctxt->callee.get_address(),
        copy_from_mem(offset, size),
        topics
      });
    }

    // Common for both CREATE and CREATE2
    void _create (const uint64_t& contract_value, const uint64_t& offset, const int64_t& size, const Address& new_address) {
      auto initCode = copy_from_mem(offset, size);

      // For contract accounts, the nonce counts the number of contract-creations by this account
      contract->increment_nonce(ctxt->callee.get_address());

      // Create account using new address and value
      decltype(auto) newAcc = contract->create_account(new_address, contract_value, {}, {});

      // In contract creation, the transaction value is an endowment for the newly created account
      contract->transfer_internal(ctxt->callee.get_address(), newAcc.get_address(), contract_value);

      // Return handlers
      auto parentContext = ctxt;
      auto result_cb = [&newAcc, parentContext, this](std::vector<uint8_t> output) {
        // Set code from output
        contract->set_code(newAcc.get_address(), move(output));
        parentContext->s.push(newAcc.get_address());
      };
      auto halt_cb = [parentContext]() { parentContext->s.push(0); };
      auto error_cb = [parentContext, this](const Exception& ex_, std::vector<uint8_t> output) {
        parentContext->s.push(0);

        if (ex_.type == Exception::Type::revert) {
          last_return_data = move(output);
        }
      };

      // create new context for init code execution
      push_context(
        ctxt->callee.get_address(),
        newAcc,
        transaction.gas_left(),
        {},
        std::move(initCode),
        0,
        result_cb,
        halt_cb,
        error_cb
      );
    }

    void create()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

      const auto contract_value = ctxt->s.popAmount();
      const auto offset         = ctxt->s.popu64();
      const auto size           = ctxt->s.popu64();
      const auto arbitrary      = ctxt->callee.get_nonce();

      auto new_address = generate_address(ctxt->callee.get_address(), arbitrary);
      _create(contract_value, offset, size, new_address);
    }

    void create2()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

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

    // TODO fix gas pricing
    void call()
    {
      // Get current OP
      const auto op = get_op();

      // Pop 6 (DELEGATECALL) or 7 (REST) from stack
      const auto _gas_limit = ctxt->s.pop();
      const auto addr       = pop_addr(ctxt->s);
      const auto value      = op == Opcode::DELEGATECALL ? 0 : ctxt->s.popAmount();
      const auto offIn      = ctxt->s.popu64();
      const auto sizeIn     = ctxt->s.popu64();
      const auto offOut     = ctxt->s.popu64();
      const auto sizeOut    = ctxt->s.popu64();

      // TODO: implement precompiled contracts
      if (addr >= 1 && addr <= 8) {
        eosio::check(false, "Precompiled contracts/native extensions are not implemented. (notImplemented)");
      }

      // Get new account and check not empty
      decltype(auto) new_callee = contract->get_account(addr);
      if (new_callee.get_code().empty()) {
        ctxt->s.push(1);
        return;
      }

      // Max gas for calls
      if (value != 0) {
        // callValueTransfer (9000) - callStipend (2300)
        if (op == Opcode::CALL || op == Opcode::CALLCODE) {
          // Check not static
          eosio::check(!ctxt->is_static, "Invalid static state change");

          // Gas change
          use_gas(GP_CALL_VALUE_TRANSFER - GP_CALL_STIPEND);
        }

        // callNewAccount
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
        eosio::check(false, "Reached max call depth (" + to_string(ProcessorConsts::MAX_CALL_DEPTH) + ") (outOfBounds)");
      }

      // Fetch input if available
      std::vector<uint8_t> input = {};
      if (sizeIn > 0) {
        input = copy_from_mem(offIn, sizeIn);
      }

      // Prepare memory for output and handlers
      prepare_mem_access(offOut, sizeOut);

      auto parentContext = ctxt;
      auto result_cb =
        [offOut, sizeOut, parentContext, this](const vector<uint8_t>& output) {
          copy_mem_raw(offOut, 0, sizeOut, parentContext->mem, output);
          parentContext->s.push(1);
          copy_mem_raw(offOut, 0, sizeOut, last_return_data, output);
        };
      auto halt_cb = [parentContext]() { parentContext->s.push(1); };
      auto he = [parentContext, this](const Exception& ex, vector<uint8_t> output) {
        parentContext->s.push(0);

        if (ex.type == Exception::Type::revert) {
          last_return_data = move(output);
        }
      };

      // Address, callee and value are variable amongst call ops
      auto context_address = ctxt->callee.get_address();
      auto context_callee  = new_callee;
      auto context_value   = value;

      if (op == Opcode::CALLCODE ) {
        context_callee  = ctxt->callee;
      }

      if (op == Opcode::DELEGATECALL) {
        context_callee  = ctxt->callee;
        context_address = ctxt->caller;
        context_value   = ctxt->call_value;
      }

      // Push call context
      push_context(
        context_address,
        context_callee,
        gas_limit,
        move(input),
        new_callee.get_code(),
        context_value,
        result_cb,
        halt_cb,
        he
      );
    }

    void return_()
    {
      const auto offset = ctxt->s.popu64();
      const auto size = ctxt->s.popu64();

      // invoke caller's return handler
      ctxt->result_cb(copy_from_mem(offset, size));
      pop_context();
    }

    void revert()
    {
      const auto offset = ctxt->s.popu64();
      const auto size = ctxt->s.popu64();

      // invoke caller's return handler
      ctxt->error_cb(
        Exception(Exception::Type::revert, "The transaction has been reverted to it's initial state. Likely issue: A non-payable function was called with value, or your balance is too low."),
        copy_from_mem(offset, size)
      );
      pop_context();
    }

    void selfdestruct()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

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
  };

  ExecResult Processor::run(
    EthereumTransaction& transaction,
    const Address& caller,
    const Account& callee,
    evm* contract
  )
  {
    return _Processor(transaction, contract).run(caller, callee);
  }
} // namespace evm4eos
