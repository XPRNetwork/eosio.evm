// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "eosio.evm.hpp"
#include "tables.hpp"

namespace eosio_evm {
  // Forward Declaration
  class evm;

  struct EVMAccount {
    Account& account = {};
    evm* contract;

    EVMAccount (evm* contract) : contract(contract)  {}
    ~EVMAccount () {}

    Account& find_account (uint256_t address) {
      auto accounts_byaddress = contract->_accounts.get_index<eosio::name("byaddress")>();
      auto existing_address = accounts_byaddress.find(toChecksum256(address));
      eos_account = &*existing_address;
    }
  };
} // namespace eosio_evm
