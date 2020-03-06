// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "exception.hpp"

namespace eosio_evm {
  // Forward declaration
  class Context;

  // Stack used by Processor
  class Stack
  {
  public:
    std::deque<uint256_t> st;
    Context* ctx;
    std::optional<std::string> stack_error;

    Stack(Context* _ctx): ctx(_ctx) {};

    uint256_t pop();
    uint256_t pop_addr();
    void push(const uint256_t& val);
    uint64_t size() const;
    void swap(uint64_t i);
    void dup(uint64_t a);
    void print();
    std::string as_array();
  };
}