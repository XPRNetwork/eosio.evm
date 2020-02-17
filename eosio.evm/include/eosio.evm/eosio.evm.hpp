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
#include "util.hpp"
#include "exception.hpp"
#include "datastream.hpp"
#include "block.hpp"
#include "transaction.hpp"
#include "context.hpp"
#include "processor.hpp"
#include "stack.hpp"
#include "tables.hpp"
// #include "account.hpp"

namespace evm4eos {
  class [[eosio::contract("eosio.evm")]] evm : public eosio::contract {
  public:
    using contract::contract;

    evm( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
      : contract(receiver, code, ds),
        _accounts(receiver, receiver.value),
        _accounts_states(receiver, receiver.value) {}

    ACTION raw      ( const std::vector<int8_t>& tx,
                      const std::optional<eosio::checksum160>& sender);
    ACTION create   ( const eosio::name& account,
                      const std::string& data);
    ACTION withdraw ( const eosio::name& to,
                      const eosio::asset& quantity);

    [[eosio::on_notify(INCOMING_TRANSFER_NOTIFY)]]
    void transfer( const eosio::name& from,
                   const eosio::name& to,
                   const eosio::asset& quantity,
                   const std::string& memo );

    // Action wrappers
    using withdraw_action = eosio::action_wrapper<"withdraw"_n, &evm::withdraw>;
    using transfer_action = eosio::action_wrapper<"transfer"_n, &evm::transfer>;

    // Define tables
    account_table _accounts;
    account_state_table _accounts_states;

    // TODO remove in prod
    ACTION devcreate(const eosio::checksum160& address, const eosio::name& account);
    ACTION testtx(const std::vector<int8_t>& tx);
    ACTION printtx(const std::vector<int8_t>& tx);
    template <typename T>
    void cleanTable(){
      T db(get_self(), get_self().value);
      auto itr = db.end();
      while(db.begin() != itr){
        itr = db.erase(--itr);
      }
    }
    ACTION clearall () {
      require_auth(get_self());
      cleanTable<account_table>();
      cleanTable<account_state_table>();
    }

    // Transfer
    void sub_balance (const eosio::name& user, const eosio::asset& quantity);
    void sub_balance (const Address& address, const int64_t& amount);
    void add_balance (const eosio::name& user, const eosio::asset& quantity);
    void add_balance (const Address& address, const int64_t& amount);

    inline void transfer_internal(const Address& from, const Address& to, const int64_t amount) {
      if (amount == 0) return;
      sub_balance(from, amount);
      add_balance(to, amount);
    }

    // State
    const Account& get_account(const Address& address);
    const Account& create_account(
      const Address& address,
      const int64_t& balance = 0,
      const std::vector<uint8_t>& code = {},
      const eosio::name& account = {}
    );
    const Account& set_code(const Address& address, const std::vector<uint8_t>& code);
    void remove_code(const Address& addr);
    void increment_nonce (const Address& address);

    // Storage
    void storekv(const Address& address, const uint256_t& key, const uint256_t& value);
    uint256_t loadkv(const Address& address, const uint256_t& key);
    void removekv(const Address& address, const uint256_t& key);
  };
}