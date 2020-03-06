// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  uint256_t Stack::pop()
  {
    if (st.empty()) {
      stack_error = "Stack out of range";
      return 0;
    }

    uint256_t val = st.front();
    st.pop_front();
    return val;
  }

  uint256_t Stack::pop_addr()
  {
    static const uint256_t MASK_160 = (uint256_t(1) << 160) - 1;
    return pop() & MASK_160;
  }

  void Stack::push(const uint256_t& val)
  {
    if (size() == MAX_STACK_SIZE) {
      stack_error = "Stack memory exceeded";
      return;
    }

    st.push_front(val);
  }

  uint64_t Stack::size() const
  {
    return st.size();
  }

  void Stack::swap(uint64_t i)
  {
    if (i >= size()) {
      stack_error = "Swap out of range";
      return;
    }

    std::swap(st[0], st[i]);
  }

  void Stack::dup(uint64_t a)
  {
    if (a >= size()) {
      stack_error = "Swap out of range";
      return;
    }

    push(st[a]);
  }

  void Stack::print() {
    eosio::print("\n");

    for (const auto& elem : st) {
      eosio::print(intx::hex(elem), "\n");
    }
  }

  std::string Stack::as_array() {
    std::string base = "[";
    for (int i = st.size() - 1; i >= 0; i--) {
      base += "\"0x" + intx::hex(st[i]) + "\",";
    }
    if (!st.empty()) {
      base.pop_back();
    }
    base += "]";
    return base;
  }
} // Namespace eosio_evm