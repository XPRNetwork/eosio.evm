// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <deque>
#include "exception.hpp"

namespace evm4eos
{
  // Stack used by Processor
  class Stack
  {
  public:
    std::deque<uint256_t> st;
    static constexpr std::size_t MAX_SIZE = 1024;
    Stack() = default;

    inline uint256_t pop()
    {
      // TODO: don't check size for every single pop, but rather once at the
      // beginning of each op handler in vm.cpp
      if (st.empty()) {
        eosio::check(false, "Stack out of range (outOfBounds)");
      }

      uint256_t val = st.front();
      st.pop_front();
      return val;
    }

    inline uint64_t popu64()
    {
      const auto val = pop();
      if (val > std::numeric_limits<uint64_t>::max()) {
        eosio::check(false, "Value on stack (" + intx::hex(val) + ") is larger than 2^64 (outOfBounds)");
      }

      return static_cast<uint64_t>(val);
    }

    inline int64_t popAmount()
    {
      const auto val = pop();
      if (val > eosio::asset::max_amount) {
        eosio::check(false, "Value on stack (" + intx::hex(val) + ") is larger than 2^62 - 1 (outOfBounds)");
      }

      return static_cast<int64_t>(val);
    }

    inline void push(const uint256_t& val)
    {
      if (size() == MAX_SIZE) {
        if (val > std::numeric_limits<uint64_t>::max()) {
          eosio::check(false, "Stack mem exceeded (" + std::to_string(size()) + " == " + std::to_string(MAX_SIZE) + ") (outOfBounds");
        }
      }

      // TODO Could throw bad_alloc
      st.push_front(val);
    }

    uint64_t size() const
    {
      return st.size();
    }

    void swap(uint64_t i)
    {
      if (i >= size()) {
        eosio::check(false, "Swap out of range (" + std::to_string(i) + " >= " + std::to_string(size()) + ") (outOfBounds)");
      }

      std::swap(st[0], st[i]);
    }

    void dup(uint64_t a)
    {
      if (a >= size()) {
        eosio::check(false, "Dup out of range (" + std::to_string(a) + " >= " + std::to_string(size()) + ") (outOfBounds)");
      }

      st.push_front(st[a]);
    }

    inline void print() {
      eosio::print("\n");

      for (const auto& elem : st) {
        eosio::print(intx::hex(elem), "\n");
      }
    }
  };
} // namespace evm4eos
