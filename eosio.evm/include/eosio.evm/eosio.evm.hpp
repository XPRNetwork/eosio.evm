#pragma once

// Standard
#include <optional>
#include <vector>

// External
#include <intx/base.hpp>
#include <ecc/uECC.c>
#include <keccak256/k.c>
#include <rlp/rlp.hpp>

// EOS
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

// Local
#include "constants.hpp"
#include "util.hpp"
#include "exception.hpp"
#include "datastream.hpp"
#include "block.hpp"
#include "transaction.hpp"
#include "context.hpp"
#include "processor.hpp"
#include "program.hpp"
#include "tables.hpp"
// #include "account.hpp"

namespace eosio_evm {
  class [[eosio::contract("eosio.evm")]] evm : public eosio::contract {
  public:
    using contract::contract;

    evm( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
      : contract(receiver, code, ds),
        _accounts(receiver, receiver.value) {}

    ACTION raw      ( const std::vector<int8_t>& tx,
                      const std::optional<eosio::checksum160>& sender);
    ACTION create   ( const eosio::name& account,
                      const std::string& data);
    ACTION withdraw ( const eosio::name& to,
                      const eosio::asset& quantity);

    [[eosio::on_notify("eosio.token::transfer")]]
    void transfer( const eosio::name& from,
                   const eosio::name& to,
                   const eosio::asset& quantity,
                   const std::string& memo );

    // Action wrappers
    using withdraw_action = eosio::action_wrapper<"withdraw"_n, &evm::withdraw>;
    using transfer_action = eosio::action_wrapper<"transfer"_n, &evm::transfer>;

    // Define account table
    account_table _accounts;

    #ifdef TESTING
    ACTION teststatetx(const std::vector<int8_t>& tx, const Env& env);
    ACTION devnewstore(const eosio::checksum160& address, const std::string& key, const std::string value);
    ACTION devnewacct(const eosio::checksum160& address, const uint64_t balance, const std::vector<uint8_t> code, const uint64_t nonce, const eosio::name& account);
    ACTION printstate(const eosio::checksum160& address);
    ACTION testtx(const std::vector<int8_t>& tx);
    ACTION printtx(const std::vector<int8_t>& tx);
    ACTION clearall () {
      require_auth(get_self());

      account_table db(get_self(), get_self().value);
      auto itr = db.end();
      while(db.begin() != itr){
        itr = db.erase(--itr);
      }

      for(int i = 0; i < 10; i++) {
        account_state_table db2(get_self(), i);
        auto itr = db2.end();
        while(db2.begin() != itr){
          itr = db2.erase(--itr);
        }
      }
    }
    #endif

    // Transfer
    void sub_balance (const eosio::name& user, const eosio::asset& quantity);
    void sub_balance (const Address& address, const int64_t& amount);
    void add_balance (const eosio::name& user, const eosio::asset& quantity);
    void add_balance (const Address& address, const int64_t& amount);
    void transfer_internal(const Address& from, const Address& to, const int64_t amount);

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

    // Storage
    void storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value);
    uint256_t loadkv(const uint64_t& address_index, const uint256_t& key);
    void removekv(const uint64_t& address_index, const uint256_t& key);
  };
}