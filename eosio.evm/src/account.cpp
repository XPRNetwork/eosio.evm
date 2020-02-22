// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  // Internal transfers only
  void Processor::transfer_internal(const Address& from, const Address& to, const int64_t amount) {
    if (amount == 0) return;

    // Index
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();

    // Addresses and accounts
    auto from_256     = toChecksum256(from);
    auto to_256       = toChecksum256(to);
    auto from_account = accounts_byaddress.find(from_256);
    auto to_account   = accounts_byaddress.find(to_256);

    // Amount verification
    if (amount < 0) return throw_error(Exception(ET::outOfFunds, "transfer amount must not be negative"), {});

    // From balance verification
    if (from_account == accounts_byaddress.end()) return throw_error(Exception(ET::outOfFunds, "account does not have a balance"), {});
    if (from_account->get_balance() < amount) return throw_error(Exception(ET::outOfFunds, "account balance too low"), {});

    // Create To account if it does not exist
    if (to_account == accounts_byaddress.end()) {
      auto [new_account, error] = create_account(to, 0);
      if (error) {
        return throw_error(Exception(ET::outOfBounds, "Error creating new address to transfer value"), {});
      } else {
        to_account = accounts_byaddress.iterator_to(new_account);
      }
    }

    // Reflect state
    accounts_byaddress.modify(from_account, eosio::same_payer, [&](auto& a) {
      a.balance.amount -= amount;
    });
    accounts_byaddress.modify(to_account, eosio::same_payer, [&](auto& a) {
      a.balance.amount += amount;
    });

    // Modification record
    transaction.add_modification({ SMT::TRANSFER, 0, from, to, amount, 0 });
  }

  // Only used by CREATE instruction to increment nonce of contract
  void Processor::increment_nonce(const Address& address) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, contract->get_self(), [&](auto& a) {
        a.nonce += 1;
      });

      transaction.add_modification({ SMT::INCREMENT_NONCE, 0, address, existing_address->get_nonce() - 1, 0, existing_address->get_nonce()});
    }
  }

  // Only used for reverting
  void Processor::decrement_nonce(const Address& address) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, contract->get_self(), [&](auto& a) {
        a.nonce -= 1;
      });
    }
  }

  // Used to push result of top level create and instruction CREATE
  void Processor::set_code(const Address& address, const std::vector<uint8_t>& code) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, contract->get_self(), [&](auto& a) {
        a.code = code;
      });
    }
  }

  // Only used while reverting
  void Processor::remove_code(const Address& address) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, contract->get_self(), [&](auto& a) {
        a.code = {};
      });
    }
  }

  // Returns an empty account if not found
  const Account& Processor::get_account(const Address& address)
  {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    if (existing_address == accounts_byaddress.end())
    {
      static const auto empty_account = Account(address);
      return empty_account;
    }
    else
    {
      return *existing_address;
    }
  }

  // Returns [account, error], error is true if account already exists
  Processor::AccountResult Processor::create_account(
    const Address& address,
    const int64_t& balance,
    const bool& is_contract // default false
  )
  {
    // Find address
    auto address_160        = addressToChecksum160(address);
    auto address_256        = pad160(address_160);
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);

    // ERROR if nonce > 0
    if (existing_address != accounts_byaddress.end() && (existing_address->get_nonce() > 0 || !existing_address->get_code().empty()))
    {
      static const auto empty_account = Account(address);
      return { empty_account, true };
    }

    // Initial Nonce is 1 for contracts and 0 for accounts (no code)
    uint64_t nonce = is_contract ? 1 : 0;

    // Create address if it does not exists
    auto new_address = contract->_accounts.emplace(contract->get_self(), [&](auto& a) {
      a.index   = contract->_accounts.available_primary_key();
      a.nonce   = nonce;
      a.address = address_160;
      a.balance = eosio::asset(balance, TOKEN_SYMBOL);
    });

    // Add modification record
    transaction.add_modification({ SMT::CREATE_ACCOUNT, 0, address, 0, 0, 0 });

    return { *new_address, false };
  }

  // Only used when reverting
  void Processor::remove_account(const Address& address)
  {
    // Find address
    auto address_160        = addressToChecksum160(address);
    auto address_256        = pad160(address_160);
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);

    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.erase(existing_address);
    }
  }

  /**
   * Used for self-destruct
   */
  void Processor::selfdestruct(const Address& address)
  {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
        a.nonce = 0;
        a.balance.amount = 0;
        a.code = {};
      });
    }
  }

  // Returns original state
  void Processor::storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value) {
    // Get scoped state table for account state
    account_state_table accounts_states(contract->get_self(), address_index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    auto checksum_key          = toChecksum256(key);
    auto account_state         = accounts_states_bykey.find(checksum_key);

    // eosio::print("\n\nStore KV for address ", intx::hex(address),
    //              "\nKey: ", to_string(key, 10),
    //              "\nValue ", to_string(value, 10),
    //              "\nAddress Index: ", to_string(address_index),
    //              "\nFound: ", account_state != accounts_states_byaddress.end(), "\n");

    // Key found
    if (account_state != accounts_states_bykey.end())
    {
      transaction.add_modification({ SMT::STORE_KV, address_index, key, account_state->value, 0, value });

      if (value == 0)
      {
        accounts_states_bykey.erase(account_state);
      }
      else
      {
        accounts_states_bykey.modify(account_state, eosio::same_payer, [&](auto& a) {
          a.value = value;
        });
      }
    }
    // Key not found and new value exists
    else if (value != 0)
    {
      transaction.add_modification({ SMT::STORE_KV, address_index, key, 0, 0, value });

      accounts_states.emplace(contract->get_self(), [&](auto& a) {
        a.index   = accounts_states.available_primary_key();
        a.key     = checksum_key;
        a.value   = value;
      });
    }
  }

  uint256_t Processor::loadkv(const uint64_t& address_index, const uint256_t& key) {
    // Get scoped state table for account
            eosio::print("\n---A4---\n");

    account_state_table accounts_states(contract->get_self(), address_index);
            eosio::print("\n---A41---\n");

    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
            eosio::print("\n---A42---\n");

    auto checksum_key          = toChecksum256(key);
            eosio::print("\n---A43---\n");

    auto account_state         = accounts_states_bykey.find(checksum_key);
        eosio::print("\n---A3---\n");

    // eosio::print("\n\nLoad KV for address ", intx::hex(address),
    //              "\nKey: ", key,
    //              "\nAddress Index: ", to_string(address_index),
    //              "\nFound: ", account_state != accounts_states_bykey.end(), "\n");

    // Key found
    if (account_state != accounts_states_bykey.end())
    {
              eosio::print("\n---A31---\n");

      // eosio::print("\nFound KV Value ", to_string(account_state->value, 10));
      return account_state->value;
    }
    // Key not found
    else {
              eosio::print("\n---A32---\n");

      return 0;
    }
  }
} // namespace eosio_evm
