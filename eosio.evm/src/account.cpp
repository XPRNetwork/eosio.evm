// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  // Internal transfers only, returns true if error
  bool Processor::transfer_internal(const Address& from, const Address& to, const uint256_t& amount) {
    if (amount == 0) {
      return false;
    }

    // Index
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();

    // Addresses and accounts
    auto from_256     = toChecksum256(from);
    auto to_256       = toChecksum256(to);
    auto from_account = accounts_byaddress.find(from_256);
    auto to_account   = accounts_byaddress.find(to_256);

    // Amount verification
    if (amount < 0) {
      transaction.errors.push_back("transfer amount must not be negative");
      return true;
    }

    // From balance verification
    if (from_account == accounts_byaddress.end()) {
      transaction.errors.push_back("account does not have a balance");
      return true;
    }
    if (amount > from_account->get_balance()) {
      transaction.errors.push_back("account balance too low");
      return true;
    }

    // Create To account if it does not exist
    if (to_account == accounts_byaddress.end()) {
      auto [new_account, error] = create_account(to);
      if (error) {
        transaction.errors.push_back("Error creating new address to transfer value");
        return true;
      } else {
        to_account = accounts_byaddress.iterator_to(new_account);
      }
    }

    // Reflect state
    accounts_byaddress.modify(from_account, eosio::same_payer, [&](auto& a) {
      a.balance -= amount;
    });
    accounts_byaddress.modify(to_account, eosio::same_payer, [&](auto& a) {
      a.balance += amount;
    });

    // Modification record
    transaction.add_modification({ SMT::TRANSFER, 0, from, to, amount, 0 });

    // Return false for no error
    return false;
  }

  // Only used by CREATE instruction to increment nonce of contract
  void Processor::increment_nonce(const Address& address) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
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
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
        a.nonce -= 1;
      });
    }
  }

  // Used to push result of top level create and instruction CREATE
  void Processor::set_code(const Address& address, const std::vector<uint8_t>& code) {
    if (code.empty()) return;

    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, transaction.ram_payer, [&](auto& a) {
        a.code = code;
      });
    }
  }

  // Only used while reverting
  void Processor::remove_code(const Address& address) {
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
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
    const bool& is_contract // default false
  )
  {
    // Find address
    auto address_160        = addressToChecksum160(address);
    auto address_256        = pad160(address_160);
    auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);

    // If account already exists
    if (existing_address != accounts_byaddress.end()) {
      // If it has nonce or non-empty code -> ERROR
      if (existing_address->get_nonce() > 0 || !existing_address->get_code().empty())
      {
        static const auto empty_account = Account(address);
        return { empty_account, true };
      }
      // If it doesn't have nonce or non-empty code -> increment nonce if contract and return
      // We also kill all storage, as it may be remnant of CREATE2
      else
      {
        if (is_contract) {
          kill_storage(existing_address->primary_key());
          accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
            a.nonce += 1;
          });
          transaction.add_modification({ SMT::INCREMENT_NONCE, 0, address, existing_address->get_nonce() - 1, 0, existing_address->get_nonce()});
        }
        return { *existing_address, false };
      }
    }

    // Initial Nonce is 1 for contracts and 0 for accounts (no code)
    uint64_t nonce = is_contract ? 1 : 0;

    // Create address if it does not exists
    auto new_address = contract->_accounts.emplace(transaction.ram_payer, [&](auto& a) {
      a.index   = contract->_accounts.available_primary_key();
      a.nonce   = nonce;
      a.address = address_160;
      a.balance = 0;
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
      // Kill storage first
      kill_storage(existing_address->primary_key());

      // Make account empty
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
        a.nonce = 0;
        a.balance = 0;
        a.code = {};
      });
    }
  }
  // TODO need to use table indirection instead to save for future processing.
  void Processor::kill_storage(const uint64_t& address_index)
  {
    account_state_table accounts_states(contract->get_self(), address_index);
    auto itr = accounts_states.end();
    while (accounts_states.begin() != itr) {
      --itr;
      transaction.add_modification({ SMT::STORE_KV, address_index, checksum256ToValue(itr->by_key()), itr->value, 0, 0 });
      itr = accounts_states.erase(itr);
    }
  }

  // Returns original state
  void Processor::storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value) {
    // Get scoped state table for account state
    account_state_table accounts_states(contract->get_self(), address_index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    auto checksum_key          = toChecksum256(key);
    auto account_state         = accounts_states_bykey.find(checksum_key);

    #if (PRINT_STATE == true)
    eosio::print(
      "\n\nStore KV for address index ", address_index,
      "\nKey: ", intx::hex(key),
      "\nValue: ", intx::hex(value),
      "\nFound: ", account_state != accounts_states_bykey.end(),
      "\nValue is not 0: ", value != 0, "\n"
    );
    if (account_state != accounts_states_bykey.end()) {
      eosio::print("\nOld Value:", intx::hex(account_state->value));
    }
    #endif

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
      accounts_states.emplace(transaction.ram_payer, [&](auto& a) {
        a.index = accounts_states.available_primary_key();
        a.key   = checksum_key;
        a.value = value;
      });
    }
  }

  uint256_t Processor::loadkv(const uint64_t& address_index, const uint256_t& key) {
    // Get scoped state table for account
    account_state_table accounts_states(contract->get_self(), address_index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    const auto checksum_key    = toChecksum256(key);
    auto account_state         = accounts_states_bykey.find(checksum_key);

    #if (PRINT_STATE == true)
    eosio::print("\n\nLoad KV for address index ", address_index,
                 "\nKey: ", intx::hex(key),
                 "\nFound: ", account_state != accounts_states_bykey.end(), "\n");
    if (account_state != accounts_states_bykey.end()) {
      eosio::print("\nValue: ", intx::hex(account_state->value), "\n");
    }
    #endif

    // Value
    auto current_value = account_state != accounts_states_bykey.end() ? account_state->value : 0;

    // Place into original storage
    transaction.emplace_original(address_index, key, current_value);

    return current_value;
  }
} // namespace eosio_evm
