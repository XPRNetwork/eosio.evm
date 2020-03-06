// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
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
      const uint256_t call_value,
      std::vector<uint8_t>&& input,
      Program&& prog,
      SuccessHandler&& success_cb,
      ErrorHandler&& error_cb
    );
    void dispatch();

    // Can return/throw errors
    bool access_mem(const uint256_t& offset, const uint256_t& size);
    bool prepare_mem_access(const uint256_t& offset, const uint64_t& size);
    bool jump_to(const uint256_t& newPc);
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
      const bool& is_contract = false
    );
    void increment_nonce(const Address& address);
    void set_code(const Address& address, const std::vector<uint8_t>& code);
    void selfdestruct(const Address& addr);
    void kill_storage(const uint64_t& address_index);
    bool transfer_internal(const Address& from, const Address& to, const uint256_t& amount);
    void throw_stack();

    // Reverting
    void remove_code(const Address& address);
    void remove_account(const Address& address);
    void decrement_nonce(const Address& address);

    // Storage
    void storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value);
    uint256_t loadkv(const uint64_t& address_index, const uint256_t& key);

    // Precompile
    void precompile_execute(uint256_t address);
    void precompile_return(const std::vector<uint8_t>& output);
    void precompile_not_implemented();
    void precompile_ecrecover();
    void precompile_sha256();
    void precompile_ripemd160();
    void precompile_identity();

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
    void create();
    uint256_t value_by_call_type(const unsigned char call_type);
    void call();
    void return_();
    void revert();
    void selfdestruct();
    void invalid();
    void illegal();
  };
}