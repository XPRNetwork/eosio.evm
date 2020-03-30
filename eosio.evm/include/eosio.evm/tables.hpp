// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace eosio_evm {
  struct [[eosio::table, eosio::contract("eosio.evm")]] Account {
    uint64_t index;
    eosio::checksum160 address;
    eosio::name account;
    uint64_t nonce;
    std::vector<uint8_t> code;
    bigint::checksum256 balance;

    Account () = default;
    Account (uint256_t _address): address(addressToChecksum160(_address)) {}
    uint64_t primary_key() const { return index; };

    uint64_t get_account_value() const { return account.value; };
    uint256_t get_address() const { return checksum160ToAddress(address); };
    uint256_t get_balance() const { return balance; };
    uint64_t get_nonce() const { return nonce; };
    std::vector<uint8_t> get_code() const { return code; };
    bool is_empty() const { return nonce == 0 && balance == 0 && code.size() == 0; };

    eosio::checksum256 by_address() const { return pad160(address); };


    #if (TESTING == true)
    void print() const {
      eosio::print("\n---Acc Info Start-----");
      eosio::print("\nAddress ", address);
      eosio::print("\nIndex ", index);
      eosio::print("\nEOS Account " + account.to_string());
      eosio::print("\nBalance ", intx::to_string(balance));
      eosio::print("\nCode ", bin2hex(code));
      eosio::print("\nNonce ", nonce);
      eosio::print("\n---Acc Info End---\n");
    }
    #endif /* TESTING */

    EOSLIB_SERIALIZE(Account, (index)(address)(account)(nonce)(code)(balance));
  };

  struct [[eosio::table, eosio::contract("eosio.evm")]] AccountState {
    uint64_t index;
    eosio::checksum256 key;
    bigint::checksum256 value;

    uint64_t primary_key() const { return index; };
    eosio::checksum256 by_key() const { return key; };

    EOSLIB_SERIALIZE(AccountState, (index)(key)(value));
  };

  typedef eosio::multi_index<"account"_n, Account,
    eosio::indexed_by<eosio::name("byaddress"), eosio::const_mem_fun<Account, eosio::checksum256, &Account::by_address>>,
    eosio::indexed_by<eosio::name("byaccount"), eosio::const_mem_fun<Account, uint64_t, &Account::get_account_value>>
  > account_table;
  typedef eosio::multi_index<"accountstate"_n, AccountState,
    eosio::indexed_by<eosio::name("bykey"), eosio::const_mem_fun<AccountState, eosio::checksum256, &AccountState::by_key>>
  > account_state_table;
}