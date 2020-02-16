// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..

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
      eosio::print("\nProcessor:\n");
      callee.print();
      eosio::print("input: " + bin2hex(input) + "\n");
      eosio::print("code: " + bin2hex(code) + "\n");
      eosio::print("ISCREATE?: ", transaction.is_create(), "\n");
      eosio::print("call_value: ", call_value, "\n");

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

      eosio::print("\nPC: ", (uint8_t) ctxt->get_pc());
      eosio::print("\nProg Size: ", (uint8_t) ctxt->prog.code.size());

      // run
      while (ctxt->get_pc() < ctxt->prog.code.size())
      {
        // TODO handle exception
        dispatch();

        ctxt->s.print();

        if (!ctxt)
          break;

        ctxt->step();
      }

      // Result of run
      eosio::print("\nRun result:\n");
      result.print();

      // halt outer context if it did not do so itself
      if (ctxt)
        stop();

      // clean-up
      for (const auto& addr : transaction.selfdestruct_list) {
        contract->remove_code(addr);
      }
      // TODO add touched account cleanup, look at JS

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
      // Check max depth
      if (get_call_depth() >= ProcessorConsts::MAX_CALL_DEPTH) {
        eosio::check(false, "Reached max call depth (" + to_string(ProcessorConsts::MAX_CALL_DEPTH) + ") (outOfBounds)");
      }

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
      eosio::print(" OP Gas: ", to_string(amount), ", Used gas: ", to_string(transaction.gas_used), ", Gas Left: ", to_string(ctxt->gas_left));
      eosio::check(transaction.gas_used <= transaction.gas_limit && ctxt->gas_left >= 0, "Out of Gas");
    }

    void refund_gas(uint256_t amount) {
      transaction.gas_used -= amount;
      ctxt->gas_left += amount;
      eosio::print(" OP Gas: ", to_string(amount), ", Refunded gas: ", to_string(transaction.gas_used), " Gas Left: ", to_string(ctxt->gas_left));
      eosio::check(transaction.gas_used <= transaction.gas_limit && ctxt->gas_left >= 0, "Out of Gas");
    }

    // Complex calculation from EIP 2200
    // Original value is the first value at sart
    // Current value is what is currently stored in EOSIO
    // New value is the value to be stored
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
      const auto end = offset + size;
      eosio::check(end > offset, "Integer overflow in memory access (" + to_string(end) + " < " + to_string(offset) + ") (outOfBounds)");
      eosio::check(end < ProcessorConsts::MAX_MEM_SIZE, "Memory limit exceeded (" + to_string(end) + " > " + to_string(ProcessorConsts::MAX_MEM_SIZE) + ") (outOfBounds)");

      if (end > ctxt->mem.size()) {
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

    template <
      typename X,
      typename Y,
      typename = enable_if_t<is_unsigned<X>::value && is_unsigned<Y>::value>
    >
    static auto safeAdd(const X x, const Y y)
    {
      const auto r = x + y;
      if (r < x) {
        eosio::check(false, "integer overflow");
      }
      return r;
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
      eosio::print("\nOperation: ", opcodeToString(op));

      // Add gas
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
          create(false);
          break;
        case Opcode::CREATE2:
          create(true);
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
          // TODO fix
          // std::string err;
          // err += "Unknown/unsupported Opcode: 0x" + std::string(get_op());
          // err += " in contract " + intx::hex(ctxt->callee.get_address().extract_as_byte_array());
          // err += " called by " + intx::hex(ctxt->caller);
          // err += " at position " + ctxt->get_pc();
          // err += ", call-depth " + get_call_depth();
          // eosio::check(false, err + " (illegalInstruction)");
          eosio::check(false, "(illegalInstruction)");
      };
    }

    /**
     * OP Code Implementations
     */
    void swap()
    {
      ctxt->s.swap(get_op() - SWAP1 + 1);
    }

    void dup()
    {
      ctxt->s.dup(get_op() - DUP1);
    }

    void add()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x + y);
    }

    void sub()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x - y);
    }

    void mul()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push(x * y);
    }

    void div()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (!y)
      {
        ctxt->s.push(0);
      }
      else
      {
        ctxt->s.push(x / y);
      }
    }

    void sdiv()
    {
      auto x = ctxt->s.pop();
      auto y = ctxt->s.pop();
      const auto min = (numeric_limits<uint256_t>::max() / 2) + 1;

      if (y == 0)
        ctxt->s.push(0);
      // special "overflow case" from the yellow paper
      else if (x == min && y == -1)
        ctxt->s.push(x);
      else
      {
        const auto signX = get_sign(x);
        const auto signY = get_sign(y);
        if (signX == -1)
          x = 0 - x;
        if (signY == -1)
          y = 0 - y;

        auto z = (x / y);
        if (signX != signY)
          z = 0 - z;
        ctxt->s.push(z);
      }
    }

    void mod()
    {
      const auto x = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
        ctxt->s.push(0);
      else
        ctxt->s.push(x % m);
    }

    void smod()
    {
      auto x = ctxt->s.pop();
      auto m = ctxt->s.pop();
      if (m == 0)
        ctxt->s.push(0);
      else
      {
        const auto signX = get_sign(x);
        const auto signM = get_sign(m);
        if (signX == -1)
          x = 0 - x;
        if (signM == -1)
          m = 0 - m;

        auto z = (x % m);
        if (signX == -1)
          z = 0 - z;
        ctxt->s.push(z);
      }
    }

    void addmod()
    {
      const uint512_t x = ctxt->s.pop();
      const uint512_t y = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
      {
        ctxt->s.push(0);
      }
      else
      {
        const uint512_t n = (x + y) % m;
        ctxt->s.push(n.lo);
      }
    }

    void mulmod()
    {
      const uint512_t x = ctxt->s.pop();
      const uint512_t y = ctxt->s.pop();
      const auto m = ctxt->s.pop();
      if (!m)
      {
        ctxt->s.push(m);
      }
      else
      {
        const uint512_t n = (x * y) % m;
        ctxt->s.push(n.lo);
      }
    }

    void exp()
    {
      const auto b = ctxt->s.pop();
      const auto e = ctxt->s.popu64();

      // Optimize: X^0 = 1
      if (e == 0) {
        ctxt->s.push(1);
        return;
      }

      // Gas
      const auto sig_bytes = static_cast<int>(intx::count_significant_words<uint8_t>(e));
      use_gas(sig_bytes * GP_EXP_BYTE);

      // Push result
      auto res = intx::exp(b, uint256_t(e));
      ctxt->s.push(res);
    }

    void signextend()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (x >= 32)
      {
        ctxt->s.push(y);
        return;
      }
      const auto idx = (8 * shrink<uint8_t>(x)) + 7;
      const auto sign = static_cast<uint8_t>((y >> idx) & 1);
      constexpr auto zero = uint256_t(0);
      const auto mask = ~zero >> (256 - idx);
      const auto yex = ((sign ? ~zero : zero) << idx) | (y & mask);
      ctxt->s.push(yex);
    }

    void lt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push((x < y) ? 1 : 0);
    }

    void gt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      ctxt->s.push((x > y) ? 1 : 0);
    }

    void slt()
    {
      const auto x = ctxt->s.pop();
      const auto y = ctxt->s.pop();
      if (x == y)
      {
        ctxt->s.push(0);
        return;
      }

      const auto signX = get_sign(x);
      const auto signY = get_sign(y);
      if (signX != signY)
      {
        if (signX == -1)
          ctxt->s.push(1);
        else
          ctxt->s.push(0);
      }
      else
      {
        ctxt->s.push((x < y) ? 1 : 0);
      }
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
      ctxt->s.push((x == y) ? 1 : 0);
    }

    void isZero()
    {
      const auto x = ctxt->s.pop();
      ctxt->s.push((x == 0) ? 1 : 0);
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
      const auto idx = ctxt->s.pop();
      if (idx >= 32) {
        ctxt->s.push(0);
        return;
      }
      const auto shift = 256 - 8 - 8 * shrink<uint8_t>(idx);
      const auto mask = uint256_t(255) << shift;
      const auto val = ctxt->s.pop();
      ctxt->s.push((val & mask) >> shift);
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
      eosio::print("JUMPI to ", newPc);
      if (cond)
        jump_to(newPc);
    }

    void jumpdest() {}

    void pc()
    {
      ctxt->s.push(ctxt->get_pc());
    }

    void msize()
    {
      ctxt->s.push(ctxt->get_used_mem() * 32);
    }

    void mload()
    {
      const auto offset = ctxt->s.popu64();
      prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
      const auto start = ctxt->mem.data() + offset;
      auto res = intx::from_big_endian(start, ProcessorConsts::WORD_SIZE);
      eosio::print("\nMLOAD: ", to_string(res));
      ctxt->s.push(res);
    }

    void mstore()
    {
      const auto offset = ctxt->s.popu64();
      const auto word = ctxt->s.pop();
      prepare_mem_access(offset, ProcessorConsts::WORD_SIZE);
      eosio::print("\nMSTORE: ", to_string(word));
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
      eosio::print("\nSSTORE Key ", intx::hex(k), " Value: ", intx::hex(v));

      if (!v) {
        contract->removekv(ctxt->callee.get_address(), k);
      } else {
        contract->storekv(ctxt->callee.get_address(), k, v);
      }
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

    void codecopy()
    {
      auto size = (ctxt->prog.code.size());
      copy_mem(ctxt->mem, ctxt->prog.code, Opcode::STOP);
    }

    void codesize()
    {
      ctxt->s.push(ctxt->prog.code.size());
    }

    void calldataload()
    {
      const auto offset = ctxt->s.popu64();
      safeAdd(offset, ProcessorConsts::WORD_SIZE);
      const auto sizeInput = ctxt->input.size();

      uint256_t v = 0;
      for (uint8_t i = 0; i < ProcessorConsts::WORD_SIZE; i++)
      {
        const auto j = offset + i;
        if (j < sizeInput)
        {
          v = (v << 8) + ctxt->input[j];
        }
        else
        {
          v <<= 8 * (ProcessorConsts::WORD_SIZE - i);
          break;
        }
      }
      ctxt->s.push(v);
    }

    void calldatasize()
    {
      ctxt->s.push(ctxt->input.size());
    }

    void calldatacopy()
    {
      copy_mem(ctxt->mem, ctxt->input, 0);
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

    void address()
    {
      ctxt->s.push(ctxt->callee.get_address());
    }

    void balance()
    {
      auto address = pop_addr(ctxt->s);
      const Account& given_account = contract->get_account(address);
      ctxt->s.push(given_account.get_balance_u64());
    }

    void origin()
    {
      ctxt->s.push(*transaction.to_address);
    }

    void caller()
    {
      ctxt->s.push(ctxt->caller);
    }

    void callvalue()
    {
      ctxt->s.push(ctxt->call_value);
    }

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

    void pop()
    {
      ctxt->s.pop();
    }

    void log()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

      const uint8_t n = get_op() - LOG0;
      const auto offset = ctxt->s.popu64();
      const auto size = ctxt->s.popu64();

      vector<uint256_t> topics(n);
      for (int i = 0; i < n; i++) {
        topics[i] = ctxt->s.pop();
      }

      // TODO implement log table as optional feature
      transaction.log_handler.add({
        ctxt->callee.get_address(),
        copy_from_mem(offset, size),
        topics
      });
    }

    void blockhash()
    {
      const auto i = ctxt->s.popu64();
      if (i >= 256)
        ctxt->s.push(0);
      else
        ctxt->s.push(get_block_hash(i % 256));
    }

    void number()
    {
      ctxt->s.push(eosio::tapos_block_num());
    }

    void gasprice()
    {
      ctxt->s.push(GAS_PRICE);
    }

    void coinbase()
    {
      ctxt->s.push(get_current_block().coinbase);
    }

    void timestamp()
    {
      ctxt->s.push(get_current_block().timestamp);
    }

    void difficulty()
    {
      ctxt->s.push(get_current_block().difficulty);
    }

    void gas()
    {
      // TODO fix
      ctxt->s.push(ctxt->gas_left - 2);
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

    void stop()
    {
      // (1) save halt handler
      auto halt_cb = ctxt->halt_cb;

      // (2) pop current context
      pop_context();

      // (3) invoke halt handler
      halt_cb();
    }

    void selfdestruct()
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

      // Find recipient
      auto recipient_address = pop_addr(ctxt->s);
      auto recipient = contract->get_account(recipient_address);

      // Check contract balance
      auto balance = ctxt->callee.get_balance();
      if (balance > 0) {
        // New Account gas fee
        if (recipient.is_empty()) {
          use_gas(GP_NEW_ACCOUNT);
        }

        // Transfer all balance
        contract->transfer_internal(ctxt->callee.get_address(), recipient.get_address(), balance);
      }

      // Add to list for later
      transaction.selfdestruct_list.push_back(ctxt->callee.get_address());
      stop();
    }

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

    // Starting point for both CREATE and CREATE2
    void create(bool create2 = false)
    {
      eosio::check(!ctxt->is_static, "Invalid static state change");

      const auto contract_value = ctxt->s.popAmount();
      const auto offset         = ctxt->s.popu64();
      const auto size           = ctxt->s.popu64();
      const auto arbitrary      = create2
                                    ? ctxt->s.pop()
                                    : ctxt->callee.get_nonce();

      auto new_address = generate_address(ctxt->callee.get_address(), arbitrary);
      _create(contract_value, offset, size, new_address);
    }

    void call()
    {
      // Get current OP
      const auto op = get_op();

      // Check not static
      if (op == Opcode::CALL || op == Opcode::CALLCODE) {
        eosio::check(!ctxt->is_static, "Invalid static state change");
      }

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
