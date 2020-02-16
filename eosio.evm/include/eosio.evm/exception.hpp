// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "opcode.hpp"

namespace evm4eos
{
  class Exception
  {
  public:
    enum class Type
    {
      outOfBounds,
      outOfGas,
      outOfFunds,
      overflow,
      staticStateChange,
      illegalInstruction,
      notImplemented,
      revert
    };
    const Type type;

  private:
    const std::string msg;

  public:
    Exception(Type t, const std::string& m)
      : type(t),
        msg(m) {}

    const char* what() const noexcept { return msg.c_str(); }
  };
} // namespace evm4eos
