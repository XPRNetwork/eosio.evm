// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace eosio_evm {
  // Forward declaration
  class Context;

  // Stack used by Processor
  class Stack
  {
  public:
    std::deque<uint256_t> st;
    Context* ctxt;

    Stack(Context* _ctxt): ctxt(_ctxt) {};

    uint256_t pop();
    uint64_t popu64();
    int64_t popAmount();
    void push(const uint256_t& val);
    uint64_t size() const;
    void swap(uint64_t i);
    void dup(uint64_t a);
    void print();
    std::string asArray();
  };
}