// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "tables.hpp"
#include "context.hpp"
#include "stack.hpp"
#include "eosio.evm.hpp"

namespace eosio_evm {
  // Forward Declarations
  class evm;

  enum class ExitReason : uint8_t
  {
    empty = 0,
    returned,
    threw
  };

  struct ExecResult
  {
    ExitReason er = {};
    Exception::Type ex = {};
    std::string exmsg = {};
    std::vector<uint8_t> output = {};

    void print () {
      eosio::print("\n-------------Exec Result------------\n");
      eosio::print("\nExitReason: ", (uint8_t) er);
      eosio::print("\nException Type:", (uint8_t) ex);
      eosio::print("\nException message:" + exmsg);
      eosio::print("\nOutput:", bin2hex(output));
      eosio::print("\n-------------End Exec Result------------\n");
    }
  };

  class Processor
  {
  private:
    EthereumTransaction& transaction;  // the transaction object
    evm* contract;               // pointer to parent contract (to call EOSIO actions)
    Context* ctxt;                     // pointer to the current context
    std::vector<std::unique_ptr<Context>> ctxts; // the stack of contexts (one per nested call)
    std::vector<uint8_t> last_return_data;  // last returned data

  public:
    using ET = Exception::Type;

    Processor(EthereumTransaction& transaction, evm* contract)
      : transaction(transaction),
        contract(contract)
    {}

    void initialize_create (const Account& caller);
    void initialize_call (const Account& caller);

    ExecResult run(
      const Account& caller,
      const Account& callee,
      uint256_t gas_limit,
      const bool& is_static,
      const std::vector<uint8_t>& data,
      const std::vector<uint8_t>& code,
      const int64_t& call_value
    );

    inline constexpr int64_t num_words(uint64_t size_in_bytes) noexcept
    {
      return (static_cast<int64_t>(size_in_bytes) + (ProcessorConsts::WORD_SIZE - 1)) / ProcessorConsts::WORD_SIZE;
    }

    inline uint256_t pop_addr(Stack& stack)
    {
      static const uint256_t MASK_160 = (uint256_t(1) << 160) - 1;
      return stack.pop() & MASK_160;
    }

    uint16_t get_call_depth() const;
    Opcode get_op() const;
    void pop_context();

    template <typename T>
    static T shrink(uint256_t i)
    {
      return static_cast<T>(i & std::numeric_limits<T>::max());
    }

    void throw_error(const Exception& exception, const std::vector<uint8_t>& output);
    void use_gas(uint256_t amount);
    void refund_gas(uint256_t amount);
    void process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value);
    void copy_mem_raw(
      const uint64_t offDst,
      const uint64_t offSrc,
      const uint64_t size,
      std::vector<uint8_t>& dst,
      const std::vector<uint8_t>& src,
      const uint8_t pad = 0
    );
    void copy_mem(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src, const uint8_t pad);
    void prepare_mem_access(const uint64_t offset, const uint64_t size);
    std::vector<uint8_t> copy_from_mem(const uint64_t offset, const uint64_t size);
    void jump_to(const uint64_t newPc);
    void dispatch();

    // Instructions
    void stop();
    void add();
    void mul();
    void sub();
    void div();
    void sdiv();
    void mod();
    void addmod();
    void smod();
    void mulmod();
    void exp();
    void signextend();
    void lt();
    void gt();
    void slt();
    void sgt();
    void eq();
    void isZero();
    void and_();
    void or_();
    void xor_();
    void not_();
    void byte();
    void shl();
    void shr();
    void sar();
    void sha3();
    void address();
    void balance();
    void origin();
    void caller();
    void callvalue();
    void calldataload();
    void calldatasize();
    void calldatacopy();
    void codecopy();
    void gasprice();
    void extcodesize();
    void extcodecopy();
    void returndatasize();
    void returndatacopy();
    void extcodehash();
    void blockhash();
    void coinbase();
    void timestamp();
    void number();
    void difficulty();
    void gaslimit();
    void chainid();
    void selfbalance();
    void pop();
    void mload();
    void mstore();
    void mstore8();
    void codesize();
    void sload();
    void sstore();
    void jump();
    void jumpi();
    void pc();
    void msize();
    void gas();
    void jumpdest();
    void push();
    void dup();
    void swap();
    void log();
    void _create(const uint64_t& contract_value, const uint64_t& offset, const int64_t& size, const Address& new_address);
    void create();
    void create2();
    void call();
    void return_();
    void revert();
    void selfdestruct();
    void invalid();
    void illegal();
  };
}