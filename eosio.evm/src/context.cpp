// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License..

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm
{
  void store_account(const uint256_t& address, const Account& account)
  {
    local_changes[address] = account;
  }

  uint256_t load_account(evm* contract, const uint256_t& address)
  {
    const auto local_accounts_itr = local_accounts.find(address);
    if (local_accounts_itr != local_accounts.end())
    {
      return local_accounts_itr->second;
    }
    else
    {
      // If not found, return from permanent store
      return contract->get_account(address);
    }
  }
}
