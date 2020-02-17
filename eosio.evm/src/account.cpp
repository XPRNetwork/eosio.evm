// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace evm4eos {
  void evm::increment_nonce(const Address& address) {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    eosio::check(existing_address != accounts_byaddress.end(), "cannot increment nonce, account does not exist.");
    accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
      a.nonce += 1;
    });
  }

  const Account& evm::set_code(const Address& address, const std::vector<uint8_t>& code) {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    // Address not found, create it
    if (existing_address == accounts_byaddress.end()) {
      return create_account(address, 0, code, {});
    // Address found, set code
    } else {
      // This happens if account was pre-created e.g. https://github.com/ConsenSys/smart-contract-best-practices/issues/61
      eosio::check(existing_address->get_code().empty() && !code.empty(), "cannot set code, code already exists at account");

      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
        a.code = code;
      });
    }

    return *existing_address;
  }

  const Account& evm::get_account(const Address& address)
  {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    // Address does not exist, return empty account
    if (existing_address == accounts_byaddress.end())
    {
      static const Account& empty_account = {};
      return empty_account;
    }
    // Address does exist, return it
    else
    {
      return *existing_address;
    }
  }

  const Account& evm::create_account(
    const Address& address,
    const int64_t& balance,
    const std::vector<uint8_t>& code,
    const eosio::name& account
  )
  {
    // Find address
    auto address_bin        = toBin(address);
    auto address_160        = toChecksum160(address_bin);
    auto address_256        = pad160(address_160);
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);

    // Nonce: 3 states
    // 1. EOS account -> 1
    // 2. Ethereum contract -> 1
    // 3. Ethereum account (no code) -> 0
    uint64_t nonce = account || !code.empty() ? 1 : 0;

    // Debug
    eosio::print("\n--------------");
    eosio::print("\nCreate Account");
    eosio::print("\nAddress 160: ", address_160);
    eosio::print("\nAddress 256: ", address_256);
    eosio::print("\n--------------");

    // Address not found, create it
    if (existing_address == accounts_byaddress.end())
    {
      auto new_address = _accounts.emplace(get_self(), [&](auto& a) {
        a.index   = _accounts.available_primary_key();
        a.address = address_160;
        a.balance = eosio::asset(balance, TOKEN_SYMBOL);
        a.nonce   = nonce;
        a.code    = code;
        a.account = account;
      });
      return *new_address;
    }
    // Address found
    else
    {
      return set_code(address, code);
    }
  }

  /**
   * Used for self-destruct
   */
  void evm::remove_code(const Address& address)
  {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    eosio::check(existing_address != accounts_byaddress.end(), "cannot remove code, account does not exist.");

    accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
      a.nonce = 0;
      a.balance.amount = 0;
      a.code = std::vector<uint8_t>{};
    });
  }

  // Storage
  void evm::storekv(const Address& address, const uint256_t& key, const uint256_t& value) {
    auto address_160               = addressToChecksum160(address);
    auto accounts_states_byaddress = _accounts_states.get_index<eosio::name("bykey")>();
    auto addresskey                = generate_key(address_160, key);
    auto account_state             = accounts_states_byaddress.find(addresskey);

    // eosio::print("\n\nStore KV for address ", intx::hex(address),
    //              "\nKey: ", to_string(key, 10),
    //              "\nValue ", to_string(value, 10),
    //              "\nAddressKey: ", addresskey,
    //              "\nFound: ", account_state != accounts_states_byaddress.end(), "\n");

    // Key found, set value
    if (account_state != accounts_states_byaddress.end())
    {
      accounts_states_byaddress.modify(account_state, eosio::same_payer, [&](auto& a) {
        a.value = value;
      });
    }
    // Key not found, create key and value
    else
    {
      _accounts_states.emplace(get_self(), [&](auto& a) {
        a.index   = _accounts_states.available_primary_key();
        a.key     = addresskey;
        a.value   = value;
      });
    }
  }

  uint256_t evm::loadkv(const Address& address, const uint256_t& key) {
    auto address_160               = addressToChecksum160(address);
    auto accounts_states_byaddress = _accounts_states.get_index<eosio::name("bykey")>();
    auto addresskey                = generate_key(address_160, key);
    auto account_state             = accounts_states_byaddress.find(addresskey);

    // eosio::print("\n\nLoad KV for address ", intx::hex(address),
    //              "\nKey: ", to_string(key, 10),
    //              "\nAddressKey: ", addresskey,
    //              "\nFound: ", account_state != accounts_states_byaddress.end(), "\n");

    // Key found
    if (account_state != accounts_states_byaddress.end())
    {
      eosio::print("\nFound KV Value ", to_string(account_state->value, 10));
      return account_state->value;
    }
    // Key not found
    else {
      return 0;
    }
  }

  void evm::removekv(const Address& address, const uint256_t& key) {
    auto address_160               = addressToChecksum160(address);
    auto accounts_states_byaddress = _accounts_states.get_index<eosio::name("bykey")>();
    auto addresskey                = generate_key(address_160, key);
    auto account_state             = accounts_states_byaddress.find(addresskey);

    eosio::print("\n\nRemove KV for address ", intx::hex(address),
                 "\nKey: ", to_string(key, 10),
                 "\nAddressKey: ", addresskey,
                 "\nFound: ", account_state != accounts_states_byaddress.end(), "\n");

    // Remove key value
    if (account_state != accounts_states_byaddress.end()){
      accounts_states_byaddress.erase(account_state);
    }
  }
} // namespace evm4eos
