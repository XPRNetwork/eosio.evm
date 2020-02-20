// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "tables.hpp"

namespace eosio_evm
{
  /**
   * EVM Logs
   */
  struct LogEntry
  {
    Address address;
    std::vector<uint8_t> data;
    std::vector<uint256_t> topics;
  };

  struct LogHandler
  {
    std::vector<LogEntry> logs = {};
    inline void add(LogEntry&& e) { logs.emplace_back(e); }
  };

  /**
   * ChangeLog for REVERTs
   */
  struct ChangeLog {
    std::map<uint256_t, const Account&> accounts = {};
    std::map<uint256_t, const AccountState&> states = {};

    void add_account (Address& address, const Account& account) {
      if (accounts.count(address) == 0) {
        accounts.emplace(address, account);
      }
    }

    void add_state (Address& address, const AccountState& state) {
      if (states.count(address) == 0) {
        states.emplace(address, state);
      }
    }
  };
} // namespace eosio_evm
