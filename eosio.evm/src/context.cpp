// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void store_state(const uint64_t& index, const uint256_t& key, const uint256_t& value)
  {
    local_account_states[index] = {key, value};
  }
  void store_account(const uint256_t& address, const Account& account)
  {
    local_accounts[address] = account;
  }

  uint256_t load_key(const uint64_t& address_index, const uint256_t& key)
  {
    const auto lstates_itr = local_account_states.find(address_index);
    if (lstates_itr != local_account_states.end()) {
      const auto local_states_kv_itr = lstates_itr->second.find(key);

      if (local_states_kv_itr != lstates_itr->second.end()) {
        return local_states_kv_itr->second;
      }
    }

    // If not found, return from permanent store
    return processor->contract->loadkv(address_index, key)
  }

  uint256_t load_account(const uint256_t& address)
  {
    const auto local_accounts_itr = local_accounts.find(address);
    if (local_accounts_itr != local_accounts.end()) {
      return local_accounts_itr->second();
    } else {
      // If not found, return from permanent store
      return processor->contract->get_account(address)
    }
  }

}
