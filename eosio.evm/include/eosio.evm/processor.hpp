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

  class Processor
  {
  private:
    EthereumTransaction& transaction;           // the transaction object
    evm* contract;                              // pointer to parent contract (to call EOSIO actions)
    Context* ctx;                               // pointer to the current context
    std::vector<std::unique_ptr<Context>> ctxs; // the stack of contexts (one per nested call)
    std::vector<uint8_t> last_return_data;      // last returned data

  public:
    Processor(EthereumTransaction& transaction, evm* contract)
      : transaction(transaction),
        contract(contract)
    {}

    void process_transaction(const Account& caller);
    ExecResult initialize_create(const Account& caller);
    ExecResult initialize_call(const Account& caller);
    void run();

    uint16_t get_call_depth() const;
    const uint8_t get_op() const;
    void revert_state(const size_t& revert_to);
    void pop_context();
    void refund_gas(uint256_t amount);
    void push_context(
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
    );
    void dispatch();

    // Can throw errors
    bool copy_mem_raw(
      const uint64_t offDst,
      const uint64_t offSrc,
      const uint64_t size,
      std::vector<uint8_t>& dst,
      const std::vector<uint8_t>& src,
      const uint8_t pad = 0
    );
    bool copy_to_mem(const std::vector<uint8_t>& src, const uint8_t pad);
    bool prepare_mem_access(const uint64_t offset, const uint64_t size);
    bool jump_to(const uint64_t newPc);
    bool use_gas(uint256_t amount);
    bool process_sstore_gas(uint256_t original_value, uint256_t current_value, uint256_t new_value);
    bool throw_error(const Exception& exception, const std::vector<uint8_t>& output);

    // State
    struct AccountResult {
      const Account& account;
      const bool error;
    };
    const Account& get_account(const Address& address);
    AccountResult create_account(
      const Address& address,
      const int64_t& balance = 0,
      const bool& is_contract = false
    );
    void increment_nonce(const Address& address);
    void set_code(const Address& address, const std::vector<uint8_t>& code);
    void selfdestruct(const Address& addr);
    bool transfer_internal(const Address& from, const Address& to, const int64_t amount);

    // Reverting
    void remove_code(const Address& address);
    void remove_account(const Address& address);
    void decrement_nonce(const Address& address);

    // Storage
    void storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value);
    uint256_t loadkv(const uint64_t& address_index, const uint256_t& key);

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

    template <typename T>
    static T shrink(uint256_t i)
    {
      return static_cast<T>(i & std::numeric_limits<T>::max());
    }

    inline constexpr int64_t num_words(uint64_t size_in_bytes) noexcept
    {
      return (static_cast<int64_t>(size_in_bytes) + (ProcessorConsts::WORD_SIZE - 1)) / ProcessorConsts::WORD_SIZE;
    }

  };
}